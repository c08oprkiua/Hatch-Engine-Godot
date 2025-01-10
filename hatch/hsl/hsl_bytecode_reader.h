#ifndef HATCH_BYTECODE_READER_H
#define HATCH_BYTECODE_READER_H

#include "core/object/ref_counted.h"


class HSLBytecodeReader : public RefCounted {
	GDCLASS(HSLBytecodeReader, RefCounted);

public:
	enum MetaInfo {
		HAS_DEBUG_INFO = 1 << 0,
		HAS_SOURCE_FILENAME = 1 << 1,
	};


	enum BytecodeOpCodes {
		OP_ERROR = 0,
		OP_CONSTANT,
		// Classes and Instances
		OP_DEFINE_GLOBAL,
		OP_GET_PROPERTY,
		OP_SET_PROPERTY,
		OP_GET_GLOBAL,
		OP_SET_GLOBAL,
		OP_GET_LOCAL,
		OP_SET_LOCAL,
		//
		OP_PRINT_STACK,
		//
		OP_INHERIT,
		OP_RETURN,
		OP_METHOD,
		OP_CLASS,
		// Function Operations
		OP_CALL,
		OP_SUPER,
		OP_INVOKE,
		// Jumping
		OP_JUMP,
		OP_JUMP_IF_FALSE,
		OP_JUMP_BACK,
		// Stack Operation
		OP_POP,
		OP_COPY,
		// Numeric Operations
		OP_ADD,
		OP_SUBTRACT,
		OP_MULTIPLY,
		OP_DIVIDE,
		OP_MODULO,
		OP_NEGATE,
		OP_INCREMENT,
		OP_DECREMENT,
		// Bit Operations
		OP_BITSHIFT_LEFT,
		OP_BITSHIFT_RIGHT,
		// Constants
		OP_NULL,
		OP_TRUE,
		OP_FALSE,
		// Bitwise Operations
		OP_BW_NOT,
		OP_BW_AND,
		OP_BW_OR,
		OP_BW_XOR,
		// Logical Operations
		OP_LG_NOT,
		OP_LG_AND,
		OP_LG_OR,
		// Equality and Comparison Operators
		OP_EQUAL,
		OP_EQUAL_NOT,
		OP_GREATER,
		OP_GREATER_EQUAL,
		OP_LESS,
		OP_LESS_EQUAL,
		//
		OP_PRINT,
		OP_ENUM_NEXT,
		OP_SAVE_VALUE,
		OP_LOAD_VALUE,
		OP_WITH,
		OP_GET_ELEMENT,
		OP_SET_ELEMENT,
		OP_NEW_ARRAY,
		OP_NEW_MAP,
		//
		OP_SWITCH_TABLE,
		OP_FAILSAFE,
		OP_EVENT,
		OP_TYPEOF,
		OP_NEW,
		OP_IMPORT,
		OP_SWITCH,
		OP_POPN,
		OP_HAS_PROPERTY,
		OP_IMPORT_MODULE,
		OP_ADD_ENUM,
		OP_NEW_ENUM,
		OP_GET_SUPERCLASS,
		OP_GET_MODULE_LOCAL,
		OP_SET_MODULE_LOCAL,
		OP_DEFINE_MODULE_LOCAL,
		OP_USE_NAMESPACE,

		OP_LAST
	};


	struct HSLFunction {
		//Obj object;
		int arg_count;
		int min_arg_count;
		int up_value_count;
		PackedByteArray bytecode;
		PackedInt32Array lines;

		String name;
		String class_name;

		uint32_t hash;
	};

	struct HSLVariable {
		uint32_t hash;
		String name;
	};

	struct HSLClassInfo {
		uint32_t class_hash;
		String class_name;
		HashMap<uint32_t, HSLFunction> functions;

	};

private:
	static const char *HSL_BYTECODE_MAGIC;
	static const char *HSL_CLASSMAP_MAGIC;

	HashMap<uint32_t, HSLClassInfo> class_map_info;
	HashMap<uint32_t, HSLVariable> game_globals;

	HashMap<uint32_t, HSLFunction> bytecode_function_list;
	PackedInt32Array bytecode_hash_list; //for looking up by index cause that's broken in HashMap itself
	Array bytecode_constants;

	String source_file_path;

	uint8_t bytecode_version;
	uint8_t bytecode_options;

	Dictionary _get_dict_info(HSLFunction *func);

	void _clear_data(); //clears bytecode data specifically, not classmap data

	String _get_global(uint32_t hash);
	String _get_class(uint32_t hash);
	String _get_namespace(uint32_t hash);

protected:
	static void _bind_methods();
public:
	static uint32_t murmur_encrypt_data(const void* key, size_t size, uint32_t hash);
	static uint32_t murmur_encrypt_string(String str);


	void load_bytecode(PackedByteArray buffer);
	void load_class_map(PackedByteArray buffer);

	bool has_debug_info();
	bool has_source_path();

	Dictionary get_function_by_name(String func_name);
	Dictionary get_function_by_index(uint32_t index);
	Dictionary get_function_by_hash(uint32_t hash);

	uint32_t get_function_count();
	String get_source_path();

	String decompile_function(uint32_t func_hash);


};

VARIANT_ENUM_CAST(HSLBytecodeReader::BytecodeOpCodes);

#endif
