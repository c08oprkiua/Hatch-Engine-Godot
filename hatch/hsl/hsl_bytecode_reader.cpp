#include "hsl_bytecode_reader.h"
#include "core/io/marshalls.h"
#include "core/io/stream_peer.h"

const char* HSLBytecodeReader::HSL_BYTECODE_MAGIC = "HTVM";

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

uint32_t murmer_encrypt_data(const void* key, size_t size, uint32_t hash) {
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

uint32_t murmur_encrypt_string(String str){
	PackedByteArray buf_str = str.to_ascii_buffer();
	buf_str.append('\0');

	const char *char_buf = (char *) buf_str.ptr();

	return murmer_encrypt_data(char_buf, strlen(char_buf), 0xDEADBEEF);
}

void HSLBytecodeReader::load_bytecode(PackedByteArray p_buffer){
	StreamPeerBuffer buffer;
	buffer.set_data_array(p_buffer);

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

	if (memcmp(magic, "HVTM", 4)){
		WARN_PRINT("File magic is wrong for Hatch bytecode!");
		//return;
	}



	version = buffer.get_8();
	options = buffer.get_8();


	bool has_debug_info = options & HAS_DEBUG_INFO;

	buffer.get_16();
	//there are two bytes at 6 and 7 that currently do nothing

	int chunk_count = buffer.get_32();

	if (not chunk_count){
		return;
	}

	function_list.reserve(chunk_count);
	hash_list.resize(chunk_count);

    for (int i = 0; i < chunk_count; i++) {
		HSLFunction function;
        int length = buffer.get_32();

        if (version < 0x0001) {
            function.arity = buffer.get_32();
			function.min_arity = function.arity;
        }
        else {
			function.arity = buffer.get_8();
            function.min_arity = buffer.get_8();
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
		constants.resize(const_count);

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
            constants.append(var);
        }

		function_list.insert(hash, function);
		hash_list.set(i, hash);
    }

    if (has_debug_info) {
        int tokenCount = buffer.get_32();
        for (int t = 0; t < tokenCount; t++) {
			String str = read_null_term_str(&buffer);
			uint32_t hash = murmur_encrypt_string(str);

			if (function_list.has(hash)){
				HSLFunction *func = function_list.getptr(hash);
				func->name = str;
			}
        }
    }
    if (options & HAS_SOURCE_FILENAME){
		source_file_path = read_null_term_str(&buffer);
	}
}

bool HSLBytecodeReader::has_debug_info(){
	return options & HAS_DEBUG_INFO;
};

bool HSLBytecodeReader::has_source_path(){
	return options & HAS_SOURCE_FILENAME;
}

uint32_t HSLBytecodeReader::get_function_count(){
	return function_list.size();
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
	int arity = func->arity;
	out.set("arity", arity);
	int min_arity = func->min_arity;
	out.set("min_arity", min_arity);
	PackedInt32Array lines = func->lines.duplicate();
	out.set("lines", lines);

	return out;
}

Dictionary HSLBytecodeReader::get_function_by_name(String func_name){
	uint32_t hash = murmur_encrypt_string(func_name);

	ERR_FAIL_COND_V(not function_list.has(hash), Dictionary());

	return _get_dict_info(function_list.getptr(hash));
}
Dictionary HSLBytecodeReader::get_function_by_index(uint32_t index){
	ERR_FAIL_INDEX_V(index, hash_list.size(), Dictionary());

	HSLFunction func = function_list.get(hash_list[index]);

	return _get_dict_info(&func);
}

Dictionary HSLBytecodeReader::get_function_by_hash(uint32_t hash){
	ERR_FAIL_COND_V(not function_list.has(hash), Dictionary());

	return _get_dict_info(function_list.getptr(hash));
}

void HSLBytecodeReader::_bind_methods(){
	ClassDB::bind_method(D_METHOD("load_bytecode", "buffer"), &HSLBytecodeReader::load_bytecode);

	ClassDB::bind_method(D_METHOD("has_debug_info"), &HSLBytecodeReader::has_debug_info);
	ClassDB::bind_method(D_METHOD("has_source_path"), &HSLBytecodeReader::has_source_path);

	ClassDB::bind_method(D_METHOD("get_function_count"), &HSLBytecodeReader::get_function_count);
	ClassDB::bind_method(D_METHOD("get_source_path"), &HSLBytecodeReader::get_source_path);

	ClassDB::bind_method(D_METHOD("get_function_by_index", "index"), &HSLBytecodeReader::get_function_by_index);
	ClassDB::bind_method(D_METHOD("get_function_by_name", "function_name"), &HSLBytecodeReader::get_function_by_name);
	ClassDB::bind_method(D_METHOD("get_function_by_hash", "name_hash"), &HSLBytecodeReader::get_function_by_hash);
}
