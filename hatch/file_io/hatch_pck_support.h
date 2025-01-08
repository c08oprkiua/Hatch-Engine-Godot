#ifndef HATCH_PACK_SUPPORT_H
#define HATCH_PACK_SUPPORT_H

/*
 The code contained in this and its paired cpp file was an attempt to make .hatch archives natively
 supported by Godot engine so that the engine can treat a Hatch archive like a Godot archive, ie.
 with direct FileAccess access and all the other benefits. Plans for this fell through due to a
 limitation in how Godot handles pack sources. This code is kept for future reference, and possibly
 in case this limitation is ever removed in Godot.
 */

#include "hatch_archive_reader.h"
#include "core/io/file_access_pack.h"

class FileAccessHatch : public FileAccess {
	uint32_t path_hash;
	Ref<FileAccess> file;
	PackedData::PackedFile file_info;
	mutable uint64_t position;
	mutable int swapNibbles = 0;
	mutable int indexKeyA = 0;
	mutable int indexKeyB = 8;
	mutable int xorValue;

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

	FileAccessHatch(const String &p_path, const PackedData::PackedFile &p_file);
};

class PackSourceHatch : public PackSource {
	static PackSourceHatch *singleton;

	struct MD5Path { //just a copy of PathMD5 that isn't private to this
		uint64_t a;
		uint64_t b;
		explicit MD5Path(const Vector<uint8_t> &p_buf) {
			a = *((uint64_t *)&p_buf[0]);
			b = *((uint64_t *)&p_buf[8]);
		}
	};

	HashMap<MD5Path, uint32_t> md5_crc32_map;

public:
	static PackSourceHatch *get_singleton();

	virtual bool try_open_pack(const String &p_path, bool p_replace_files, uint64_t p_offset) override;
	virtual Ref<FileAccess> get_file(const String &p_path, PackedData::PackedFile *p_file) override;
};


#endif
