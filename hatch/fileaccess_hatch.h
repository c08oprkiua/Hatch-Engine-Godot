#ifndef FILEACCESS_HATCH_H
#define FILEACCESS_HATCH_H

#include "core/io/file_access.h"
#include "core/io/file_access_pack.h"

#include "core/templates/hash_map.h"

typedef struct ResourceRegistryItem {
	PackedByteArray table;
	uint64_t offset;
	uint64_t size;
	uint32_t data_flag;
	uint64_t compressed_size;
} ResourceRegistryItem;

class FileAccessHatch : public RefCounted {
	GDCLASS(FileAccessHatch, RefCounted);

	HashMap<uint32_t, ResourceRegistryItem> resource_registry;
	PackedInt32Array crc_array;

	Ref<FileAccess> file;

	uint16_t file_count;

protected:
	static void _bind_methods();

public:
    uint16_t get_file_count();
    void set_file_count(uint16_t new_file_count);

	uint32_t crc32_string(String string);

	void open(String file_path);

	void load(String path);

	PackedByteArray load_resource(String filename);
	PackedByteArray load_resource_hash(uint32_t hash);

	Dictionary get_file_information(int index);
	Dictionary get_file_information_hash(uint32_t hash);
};

//new implementation based on how Godot manages its own loading of pck archives.
//The goal here is to make it as seamless as doing this with Godot's own systems.

class NewFileAccessHatch : public FileAccess {
	virtual Error open_internal(const String &p_path, int p_mode_flags) override;
	virtual uint64_t _get_modified_time(const String &p_file) override { return 0; }
	virtual BitField<FileAccess::UnixPermissionFlags> _get_unix_permissions(const String &p_file) override { return 0; }
	virtual Error _set_unix_permissions(const String &p_file, BitField<FileAccess::UnixPermissionFlags> p_permissions) override { return FAILED; }

	virtual bool _get_hidden_attribute(const String &p_file) override { return false; }
	virtual Error _set_hidden_attribute(const String &p_file, bool p_hidden) override { return ERR_UNAVAILABLE; }
	virtual bool _get_read_only_attribute(const String &p_file) override { return false; }
	virtual Error _set_read_only_attribute(const String &p_file, bool p_ro) override { return ERR_UNAVAILABLE; }

public:
	virtual bool is_open() const override;

	virtual void seek(uint64_t p_position) override;
	virtual void seek_end(int64_t p_position = 0) override;
	virtual uint64_t get_position() const override;
	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override;

	virtual uint64_t get_buffer(uint8_t *p_dst, uint64_t p_length) const override;

	virtual void set_big_endian(bool p_big_endian) override;

	virtual Error get_error() const override;

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }
	virtual void flush() override;
	virtual bool store_buffer(const uint8_t *p_src, uint64_t p_length) override;

	virtual bool file_exists(const String &p_name) override;

	virtual void close() override;

	NewFileAccessHatch(const String &p_path, const PackedData::PackedFile &p_file);
};

class PackSourceHatch : public PackSource {
public:
	virtual bool try_open_pack(const String &p_path, bool p_replace_files, uint64_t p_offset) override;
	virtual Ref<FileAccess> get_file(const String &p_path, PackedData::PackedFile *p_file) override;
};

#endif
