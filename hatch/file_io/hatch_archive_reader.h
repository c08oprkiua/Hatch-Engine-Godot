#ifndef HATCH_ARCHIVE_READER_H
#define HATCH_ARCHIVE_READER_H

#include "core/io/file_access.h"

#define HATCH_CRC_MAGIC_VALUE 0xFFFFFFFFU


typedef struct ResourceRegistryItem {
	PackedByteArray table;
	uint64_t offset;
	uint64_t size;
	uint32_t data_flag;
	uint64_t compressed_size;
} ResourceRegistryItem;

class HatchArchiveReader : public RefCounted {
	GDCLASS(HatchArchiveReader, RefCounted);

	HashMap<uint32_t, ResourceRegistryItem> resource_registry;
	PackedInt32Array crc_array;

	uint16_t file_count;

	Ref<FileAccess> file;

protected:
	static void _bind_methods();

public:
    uint16_t get_file_count();

	static uint32_t p_crc_32_encrypt_data(PackedByteArray data, int size, uint32_t crc); //script API friendly
	static uint32_t crc_32_encrypt_data(const void* data, size_t size, uint32_t crc = HATCH_CRC_MAGIC_VALUE);
 	static uint32_t crc32_string(String string);

	void open(String file_path);
	//void create_archive(String base_path, String out_path);

	void load(String path);

	PackedByteArray load_resource(String filename);
	PackedByteArray load_resource_hash(uint32_t hash);

	bool has_resource(String filename);
	bool has_resource_hash(uint32_t hash);

	Dictionary get_file_information(int index);
	Dictionary get_file_information_hash(uint32_t hash);
};

#endif
