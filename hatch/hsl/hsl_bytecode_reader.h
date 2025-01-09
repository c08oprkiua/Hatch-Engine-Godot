#ifndef HATCH_BYTECODE_READER_H
#define HATCH_BYTECODE_READER_H

#include "core/object/ref_counted.h"


class HSLBytecodeReader : public RefCounted {
	GDCLASS(HSLBytecodeReader, RefCounted);

	enum MetaInfo {
		HAS_DEBUG_INFO = 1 << 0,
		HAS_SOURCE_FILENAME = 1 << 1,
	};

	struct HSLFunction {
		//Obj object;
		int arity;
		int min_arity;
		int up_value_count;
		PackedByteArray bytecode;
		PackedInt32Array lines;


		String name;
		String class_name;

		uint32_t hash;
	};

	static const char *HSL_BYTECODE_MAGIC;

	HashMap<uint32_t, HSLFunction> function_list;
	PackedInt32Array hash_list;
	Array constants;

	String source_file_path;

	uint8_t version;
	uint8_t options;

	Dictionary _get_dict_info(HSLFunction *func);

protected:
	static void _bind_methods();
public:

	void load_bytecode(PackedByteArray buffer);

	bool has_debug_info();
	bool has_source_path();

	Dictionary get_function_by_name(String func_name);
	Dictionary get_function_by_index(uint32_t index);
	Dictionary get_function_by_hash(uint32_t hash);

	uint32_t get_function_count();
	String get_source_path();
};


#endif
