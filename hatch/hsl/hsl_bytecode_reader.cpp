#include "hsl_bytecode_reader.h"
#include "core/io/marshalls.h"
#include "core/io/stream_peer.h"

const char* HSLBytecodeReader::HSL_BYTECODE_MAGIC = "HTVM";
const char* HSLBytecodeReader::HSL_CLASSMAP_MAGIC = "HMAP";

String read_null_term_str(StreamPeerBuffer *peer){
	String ret;
	uint8_t byte = peer->get_8();

	while (byte != '\0'){
		ret += String::chr(byte);

		if (peer->get_available_bytes() <= 0){
			break;
		}

		byte = peer->get_8();
	}

	return ret;
}

uint32_t HSLBytecodeReader::murmur_encrypt_data(const void* key, size_t size, uint32_t hash) {
    const unsigned int m = 0x5bd1e995;
    const int r = 24;
	unsigned int h = hash ^ (uint32_t)size;

	const unsigned char* data = (const unsigned char*)key;

	while (size >= 4) {
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		size -= 4;
	}

	// Handle the last few bytes of the input array
	switch (size) {
    	case 3: h ^= data[2] << 16;
    	case 2: h ^= data[1] << 8;
    	case 1: h ^= data[0];
    	        h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}

uint32_t HSLBytecodeReader::murmur_encrypt_string(String str){
	PackedByteArray buf_str = str.to_ascii_buffer();
	buf_str.append('\0'); //Hatch uses null termination strings

	const char *char_buf = (char *) buf_str.ptr();

	return murmur_encrypt_data(char_buf, strlen(char_buf), 0xDEADBEEF);
}

void HSLBytecodeReader::_clear_data(){
	bytecode_options = 0;
	bytecode_version = 0;
	source_file_path.clear();
	bytecode_function_list.clear();
	bytecode_hash_list.clear();
	bytecode_constants.clear();
}

String HSLBytecodeReader::_get_global(uint32_t hash){
	return String("GLOBAL_") + String::num_uint64(hash);
}

String HSLBytecodeReader::_get_class(uint32_t hash){
	if (class_map_info.has(hash)){
		HSLClassInfo this_class = class_map_info.get(hash);

		if (not this_class.class_name.is_empty()){
			return this_class.class_name;
		} else {
			return String("CLASS_") + String::num_uint64(hash);
		}
	} else {
		return String("CLASS_") + String::num_uint64(hash);
	}
}

String HSLBytecodeReader::_get_namespace(uint32_t hash){
	return String("NAMESPACE_") + String::num_uint64(hash);
}

void HSLBytecodeReader::load_bytecode(PackedByteArray p_buffer){
	StreamPeerBuffer buffer;
	buffer.set_data_array(p_buffer);

	_clear_data();

	uint8_t magic[4];
	magic[0] = buffer.get_8();
	magic[1] = buffer.get_8();
	magic[2] = buffer.get_8();
	magic[3] = buffer.get_8();

	if (buffer.get_size() == 0){
		WARN_PRINT("Buffer for Hatch bytecode is empty!");
		return;
	} else if (buffer.get_size() < 7){
		WARN_PRINT("Buffer for Hatch bytecode is too small!");
		return;
	}

	if (memcmp(magic, HSL_BYTECODE_MAGIC, 4)){
		WARN_PRINT("File magic is wrong for Hatch bytecode!");
		//return;
	}

	bytecode_version = buffer.get_8();
	bytecode_options = buffer.get_8();

	bool has_debug_info = bytecode_options & HAS_DEBUG_INFO;

	buffer.get_16();
	//there are two bytes at 6 and 7 that currently do nothing

	int chunk_count = buffer.get_32();

	if (not chunk_count){
		return;
	}

	bytecode_function_list.reserve(chunk_count);
	bytecode_hash_list.resize(chunk_count);

    for (int i = 0; i < chunk_count; i++) {
		HSLFunction function;
        int length = buffer.get_32();

        if (bytecode_version < 0x0001) {
            function.arg_count = buffer.get_32();
			function.min_arg_count = function.arg_count;
        }
        else {
			function.arg_count = buffer.get_8();
            function.min_arg_count = buffer.get_8();
        }

        uint32_t hash = buffer.get_u32();
		function.hash = hash;

		function.bytecode.resize(length);

		uint8_t *raw_bytecode = function.bytecode.ptrw();

		buffer.get_data(&raw_bytecode[0], length);

        if (has_debug_info and buffer.get_available_bytes() > length * sizeof(int)) {
			function.lines.resize(length);
			for (int line = 0; line < length; line++){
				function.lines.set(line, buffer.get_32());
			}
        } else {
			WARN_PRINT("Size error for reading back bytecode lines!");
		}

        int const_count = buffer.get_32();
		bytecode_constants.resize(const_count);

        for (int c = 0; c < const_count; c++) {
            uint8_t type = buffer.get_8();
			Variant var;
            switch (type) {
                case 1: //int
					var = Variant((int32_t) buffer.get_32());
                    break;
                case 2: //float
					var = Variant((float) buffer.get_32());
                    break;
                case 3: //object
					var = Variant(read_null_term_str(&buffer));
                    break;
            }
            bytecode_constants.append(var);
        }

		bytecode_function_list.insert(hash, function);
		bytecode_hash_list.set(i, hash);
    }

    if (has_debug_info) {
        int tokenCount = buffer.get_32();
        for (int t = 0; t < tokenCount; t++) {
			String str = read_null_term_str(&buffer);
			uint32_t hash = murmur_encrypt_string(str);

			if (bytecode_function_list.has(hash)){
				HSLFunction *func = bytecode_function_list.getptr(hash);
				func->name = str;
			}
        }
    }
    if (bytecode_options & HAS_SOURCE_FILENAME){
		source_file_path = read_null_term_str(&buffer);
	}
}

void HSLBytecodeReader::load_class_map(PackedByteArray byte_buffer){
	StreamPeerBuffer buffer;
	buffer.set_data_array(byte_buffer);

	if (buffer.get_size() == 0){
		WARN_PRINT("Buffer for Class Map is empty!");
		return;
	}

	uint8_t magic[4];

	buffer.get_data(magic, 4);

	if (memcmp(magic, HSL_CLASSMAP_MAGIC, 4)){
		WARN_PRINT("Invalid file magic for Class Map!");
	}

	//version data
	buffer.get_8();
	buffer.get_8();
	buffer.get_8();
	buffer.get_8();

	uint32_t class_count = buffer.get_u32();
	class_map_info.reserve(class_count);

	//For this, we do checks if it already exists because the user is able to
	//fill this information out independently

	for (int cur_class = 0; cur_class < class_count; cur_class++){
		HSLClassInfo class_info;
		uint32_t class_hash = buffer.get_u32();

		if (class_map_info.has(class_hash)){
			class_info = class_map_info.get(class_hash);
		} else {
			class_info.class_hash = class_hash;
		}

		uint32_t func_count = buffer.get_u32();

		class_info.functions.reserve(func_count);

		for (int cur_func = 0; cur_func < func_count; cur_func++){
			uint32_t func_hash = buffer.get_u32();

			if (not class_info.functions.has(func_hash)){
				HSLFunction new_func;
				new_func.hash = func_hash;
				class_info.functions.insert(func_hash, new_func);
			}
		}
		class_map_info.insert(class_hash, class_info);
	}
}


bool HSLBytecodeReader::has_debug_info(){
	return bytecode_options & HAS_DEBUG_INFO;
};

bool HSLBytecodeReader::has_source_path(){
	return bytecode_options & HAS_SOURCE_FILENAME;
}

uint32_t HSLBytecodeReader::get_function_count(){
	return bytecode_function_list.size();
}

String HSLBytecodeReader::get_source_path(){
	return source_file_path;
}

Dictionary HSLBytecodeReader::_get_dict_info(HSLFunction *func){
	Dictionary out;

	String name = func->name;
	out.set("name", name);
	uint32_t hash = func->hash;
	out.set("hash", hash);
	PackedByteArray bytecode = func->bytecode.duplicate();
	out.set("bytecode", bytecode);
	int arg_count = func->arg_count;
	out.set("arg_count", arg_count);
	int min_arg_count = func->min_arg_count;
	out.set("min_arg_count", min_arg_count);
	PackedInt32Array lines = func->lines.duplicate();
	out.set("lines", lines);

	return out;
}

Dictionary HSLBytecodeReader::get_function_by_name(String func_name){
	uint32_t hash = murmur_encrypt_string(func_name);

	ERR_FAIL_COND_V(not bytecode_function_list.has(hash), Dictionary());

	return _get_dict_info(bytecode_function_list.getptr(hash));
}
Dictionary HSLBytecodeReader::get_function_by_index(uint32_t index){
	ERR_FAIL_INDEX_V(index, bytecode_hash_list.size(), Dictionary());

	HSLFunction func = bytecode_function_list.get(bytecode_hash_list[index]);

	return _get_dict_info(&func);
}

Dictionary HSLBytecodeReader::get_function_by_hash(uint32_t hash){
	ERR_FAIL_COND_V(not bytecode_function_list.has(hash), Dictionary());

	return _get_dict_info(bytecode_function_list.getptr(hash));
}

String HSLBytecodeReader::decompile_function(uint32_t func_hash){
	ERR_FAIL_COND_V_MSG(class_map_info.is_empty(), "", "There is no Class Map loaded to reference in decompiling the function!");

	ERR_FAIL_COND_V(not bytecode_function_list.has(func_hash), "");

	HSLFunction func = bytecode_function_list.get(func_hash);

	StreamPeerBuffer buffer;
	buffer.set_data_array(func.bytecode);

	String code;

	while (buffer.get_available_bytes() > 0){
		BytecodeOpCodes op = (BytecodeOpCodes) buffer.get_8();

		uint32_t u_int_a;

		uint8_t byte_a;
		uint8_t byte_b;

		switch (op){
			case OP_ERROR:
				break;
			case OP_CONSTANT:

				break;
			case OP_DEFINE_GLOBAL:
				u_int_a = buffer.get_u32(); //global name hash

				code += _get_global(u_int_a);


				break;
			case OP_GET_PROPERTY:

				break;
			case OP_SET_PROPERTY:

				break;
			case OP_GET_GLOBAL:
				u_int_a = buffer.get_32(); //hash

				break;
			case OP_SET_GLOBAL:
				u_int_a = buffer.get_32(); //hash

				break;
			case OP_GET_LOCAL:
				byte_a = buffer.get_u8(); //slot

				break;
			case OP_SET_LOCAL:

				break;
			case OP_PRINT_STACK:
				code += "PrintStack();"; //idk

				break;
			case OP_INHERIT:

				break;
			case OP_RETURN:
				code += "return;";

				break;
			case OP_METHOD:
				byte_a = buffer.get_u8(); //index
				u_int_a = buffer.get_u32(); //hash


				break;
			case OP_CLASS:
				u_int_a = buffer.get_32(); //class hash
				byte_a = buffer.get_u8(); //class type


				break;
			case OP_CALL:

				break;
			case OP_SUPER:
				u_int_a = buffer.get_u32(); //super class


				break;
			case OP_INVOKE:

				break;
			case OP_JUMP:

				break;
			case OP_JUMP_IF_FALSE:

				break;
			case OP_JUMP_BACK:

				break;
			case OP_POP:

				break;
			case OP_COPY:

				break;
			case OP_ADD:
				byte_b = buffer.get_u8();
				byte_a = buffer.get_u8();


				break;
			case OP_SUBTRACT:
				byte_b = buffer.get_u8();
				byte_a = buffer.get_u8();

				break;
			case OP_MULTIPLY: break;
			case OP_DIVIDE: break;
			case OP_MODULO: break;
			case OP_NEGATE: break;
			case OP_INCREMENT: break;
			case OP_DECREMENT: break;
			case OP_BITSHIFT_LEFT: break;
			case OP_BITSHIFT_RIGHT: break;
			case OP_NULL:
				code += "null";

				break;
			case OP_TRUE:
				code += "true";
				break;
			case OP_FALSE:
				code += "false";

				break;
			case OP_BW_NOT: break;
			case OP_BW_AND: break;
			case OP_BW_OR: break;
			case OP_BW_XOR: break;
			case OP_LG_NOT: break;
			case OP_LG_AND: break;
			case OP_LG_OR: break;
			case OP_EQUAL: break;
			case OP_EQUAL_NOT: break;
			case OP_GREATER: break;
			case OP_GREATER_EQUAL: break;
			case OP_LESS: break;
			case OP_LESS_EQUAL: break;
			case OP_PRINT: break;
			case OP_ENUM_NEXT: break;
			case OP_SAVE_VALUE: break;
			case OP_LOAD_VALUE: break;
			case OP_WITH: break;
			case OP_GET_ELEMENT: break;
			case OP_SET_ELEMENT: break;
			case OP_NEW_ARRAY:
				u_int_a = buffer.get_u32(); //array size

				break;
			case OP_NEW_MAP:
				u_int_a = buffer.get_u32(); //map size

				break;
			case OP_SWITCH_TABLE: break;
			case OP_FAILSAFE: break;
			case OP_EVENT: break;
			case OP_TYPEOF: break;
			case OP_NEW: break;
			case OP_IMPORT:

				break;
			case OP_SWITCH: break;
			case OP_POPN: break;
			case OP_HAS_PROPERTY: break;
			case OP_IMPORT_MODULE: break;
			case OP_ADD_ENUM: break;
			case OP_NEW_ENUM: break;
			case OP_GET_SUPERCLASS: break;
			case OP_GET_MODULE_LOCAL: break;
			case OP_SET_MODULE_LOCAL: break;
			case OP_DEFINE_MODULE_LOCAL: break;
			case OP_USE_NAMESPACE: break;

			case OP_LAST: break;
		}

	}

	return code;
}


void HSLBytecodeReader::_bind_methods(){
	ClassDB::bind_static_method("HSLBytecodeReader", D_METHOD("murmur_encrypt_data", "data", "size", "hash"), &HSLBytecodeReader::murmur_encrypt_data);
	ClassDB::bind_static_method("HSLBytecodeReader", D_METHOD("murmur_encrypt_string", "str"), &HSLBytecodeReader::murmur_encrypt_string);

	ClassDB::bind_method(D_METHOD("load_bytecode", "buffer"), &HSLBytecodeReader::load_bytecode);
	ClassDB::bind_method(D_METHOD("load_class_map", "buffer"), &HSLBytecodeReader::load_class_map);

	ClassDB::bind_method(D_METHOD("has_debug_info"), &HSLBytecodeReader::has_debug_info);
	ClassDB::bind_method(D_METHOD("has_source_path"), &HSLBytecodeReader::has_source_path);

	ClassDB::bind_method(D_METHOD("get_function_count"), &HSLBytecodeReader::get_function_count);
	ClassDB::bind_method(D_METHOD("get_source_path"), &HSLBytecodeReader::get_source_path);

	ClassDB::bind_method(D_METHOD("get_function_by_index", "index"), &HSLBytecodeReader::get_function_by_index);
	ClassDB::bind_method(D_METHOD("get_function_by_name", "function_name"), &HSLBytecodeReader::get_function_by_name);
	ClassDB::bind_method(D_METHOD("get_function_by_hash", "name_hash"), &HSLBytecodeReader::get_function_by_hash);

	ClassDB::bind_method(D_METHOD("decompile_function", "func_hash"), &HSLBytecodeReader::decompile_function);
}
