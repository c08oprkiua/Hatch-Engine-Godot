#include "fileaccess_hatch.h"

//#include "thirdparty/basis_universal/encoder/basisu_miniz.h"

uint32_t HatchArchiveReader::p_crc_32_encrypt_data(PackedByteArray data, int size, uint32_t crc){
	return crc_32_encrypt_data((const void *)data.ptr(), (size_t) size, crc);
}

//yes I just copy pasted this function. No I don't care that I didn't make a function from
//scratch that would do the same exact thing just so I would not copy the function.
uint32_t HatchArchiveReader::crc_32_encrypt_data(const void* data, size_t size, uint32_t crc) {
    int j;
    uint32_t byte, mask;
    uint8_t* message = (uint8_t*)data;

    while (size) {
        byte = *message;
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        message++;
		size--;
    }
    return ~crc;
}

uint32_t HatchArchiveReader::crc32_string(String path){
	const char *raw_path = path.ascii().ptr();

	return crc_32_encrypt_data((const void *) raw_path, strlen(raw_path), HATCH_CRC_MAGIC_VALUE);
}

void HatchArchiveReader::open(String path){
	if (path.is_empty()){
		path = "Data.hatch";
	}

	Error open_err;

	file = FileAccess::open(path, FileAccess::ModeFlags::READ, &open_err);

	if (file->file_exists(path)){
		load(path);
	}
}

void HatchArchiveReader::load(String path){
	file = FileAccess::open(path, FileAccess::ModeFlags::READ);

	uint8_t hatch_magic[5];
	file->get_buffer(hatch_magic, 5);

	if (memcmp(hatch_magic, "HATCH", 5)) {
		//TODO: Error; this is not the file we want
	}

	file->get_8();
	file->get_8();
	file->get_8();

	file_count = file->get_16();

	crc_array.resize(file_count + 1);

	for (int cur_file = 0; cur_file < file_count; cur_file++){
		ResourceRegistryItem item;
		uint32_t crc_32 = file->get_32();
		item.offset = file->get_64();
		item.size = file->get_64();
		item.data_flag = file->get_32();
		item.compressed_size = file->get_64();

		crc_array.set(cur_file, crc_32);

		resource_registry.insert(crc_32, item);
	}

}

Dictionary HatchArchiveReader::get_file_information(int index){
	ERR_FAIL_INDEX_V(index, crc_array.size(), Dictionary());

	uint32_t crc_32 = crc_array.get(index);

	return get_file_information_hash(crc_32);
};

Dictionary HatchArchiveReader::get_file_information_hash(uint32_t hash){
	Dictionary out;

	if (not resource_registry.has(hash)){
		ERR_FAIL_V(out);
	}

	ResourceRegistryItem item = resource_registry.get(hash);

	out["crc32"] = hash;
	out["offset"] = item.offset;
	out["size"] = item.size;
	out["data_flag"] = item.data_flag;
	out["compressed_size"] = item.compressed_size;

	return out;
}

PackedByteArray HatchArchiveReader::load_resource(String filename){

	uint32_t path_hash = crc32_string(filename.validate_filename());

	return load_resource_hash(path_hash);
}

PackedByteArray HatchArchiveReader::load_resource_hash(uint32_t hash){
	PackedByteArray memory;

	ResourceRegistryItem item;

	if (not resource_registry.has(hash)){
		WARN_PRINT("Invalid hash for file in hatch archive!");
		return memory;
	}

	item = resource_registry.get(hash);

	memory.resize(item.size + 1);

	memory.set(item.size, 0);

	file->seek(item.offset);

	if (item.size != item.compressed_size){
		PackedByteArray compressed_mem;
		compressed_mem.resize(item.size);

		compressed_mem = file->get_buffer(item.compressed_size);

		/*
		buminiz::mz_stream infstream;
		infstream.zalloc = Z_NULL;
		infstream.zfree = Z_NULL;
		infstream.opaque = Z_NULL;

		//no idea if this would even work (the ptrw usage)
		infstream.next_in = compressed_mem.ptrw();
		infstream.next_out = memory.ptrw();
		infstream.avail_in = (int) item.compressed_size;
		infstream.avail_out = (int) item.size;

		mz_inflateInit(&infstream);
		mz_inflate(&infstream, buminiz::MZ_NO_FLUSH);
		mz_inflateEnd(&infstream);
		*/

		compressed_mem.clear();

	} else {
		memory = file->get_buffer(item.size);
	}

	//yes, also copy pasted. Don't care.
    if (item.data_flag == 2) {
        uint8_t keyA[16];
        uint8_t keyB[16];
        uint32_t sizeHash = crc_32_encrypt_data(&item.size, sizeof(item.size));

        // Populate Key A
        uint32_t* keyA32 = (uint32_t*)&keyA[0];
        keyA32[0] = hash;
        keyA32[1] = hash;
        keyA32[2] = hash;
        keyA32[3] = hash;

        // Populate Key B
        uint32_t* keyB32 = (uint32_t*)&keyB[0];
        keyB32[0] = sizeHash;
        keyB32[1] = sizeHash;
        keyB32[2] = sizeHash;
        keyB32[3] = sizeHash;

        int swapNibbles = 0;
        int indexKeyA = 0;
        int indexKeyB = 8;
        int xorValue = (item.size >> 2) & 0x7F;
        for (uint32_t x = 0; x < item.size; x++) {
            uint8_t temp = memory[x];

            temp ^= xorValue ^ keyB[indexKeyB++];

            if (swapNibbles)
                temp = (((temp & 0x0F) << 4) | ((temp & 0xF0) >> 4));

            temp ^= keyA[indexKeyA++];

			memory.set(x, temp);

            if (indexKeyA <= 15) {
                if (indexKeyB > 12) {
                    indexKeyB = 0;
                    swapNibbles ^= 1;
                }
            }
            else if (indexKeyB <= 8) {
                indexKeyA = 0;
                swapNibbles ^= 1;
            }
            else {
                xorValue = (xorValue + 2) & 0x7F;
                if (swapNibbles) {
                    swapNibbles = false;
                    indexKeyA = xorValue % 7;
                    indexKeyB = (xorValue % 12) + 2;
                }
                else {
                    swapNibbles = true;
                    indexKeyA = (xorValue % 12) + 3;
                    indexKeyB = xorValue % 7;
                }
            }
        }
    }

	return memory;
}

uint16_t HatchArchiveReader::get_file_count(){
    return file_count;
}

void HatchArchiveReader::_bind_methods(){
	ClassDB::bind_static_method("HatchArchiveReader", D_METHOD("crc_32_encrypt_data", "data", "size", "crc"), &HatchArchiveReader::p_crc_32_encrypt_data);
	ClassDB::bind_static_method("HatchArchiveReader", D_METHOD("crc32_string", "str"), &HatchArchiveReader::crc32_string);

    ClassDB::bind_method(D_METHOD("get_file_count"), &HatchArchiveReader::get_file_count);

	ClassDB::bind_method(D_METHOD("open", "file_path"), &HatchArchiveReader::open);
    ClassDB::bind_method(D_METHOD("load_resource_from_name", "file_name"), &HatchArchiveReader::load_resource);
    ClassDB::bind_method(D_METHOD("load_resource_from_hash", "file_name"), &HatchArchiveReader::load_resource_hash);
	ClassDB::bind_method(D_METHOD("get_file_information_from_index", "file_index"), &HatchArchiveReader::get_file_information);
	ClassDB::bind_method(D_METHOD("get_file_information_from_hash", "name_hash"), &HatchArchiveReader::get_file_information_hash);

}

Error FileAccessHatch::open_internal(const String &p_path, int p_mode_flags) {
	ERR_PRINT("Can't open pack-referenced file.");
	return ERR_UNAVAILABLE;
}

bool FileAccessHatch::is_open() const {
	if (file.is_valid()) {
		return file->is_open();
	} else {
		return false;
	}
}

void FileAccessHatch::seek(uint64_t p_position){
	ERR_FAIL_COND_MSG(file.is_null(), "File must be opened before use.");

	file->seek(file_info.offset + p_position);
	position = p_position;

	swapNibbles = 0;
	indexKeyA = 0;
	indexKeyB = 8;
	xorValue = (file_info.size >> 2) & 0x7F;

	while (p_position) {
		indexKeyB++;
		indexKeyA++;

		if (indexKeyA <= 15) {
			if (indexKeyB > 12) {
				indexKeyB = 0;
				swapNibbles ^= 1;
			}
		}
		else if (indexKeyB <= 8) {
			indexKeyA = 0;
			swapNibbles ^= 1;
		}
		else {
			xorValue = (xorValue + 2) & 0x7F;
			if (swapNibbles) {
				swapNibbles = false;
				indexKeyA = xorValue % 7;
				indexKeyB = (xorValue % 12) + 2;
			}
			else {
				swapNibbles = true;
				indexKeyA = (xorValue % 12) + 3;
				indexKeyB = xorValue % 7;
			}
		}
		p_position--;
	}
}

void FileAccessHatch::seek_end(int64_t p_position){
	seek(file_info.size + p_position);
}

uint64_t FileAccessHatch::get_position() const {
	return position;
}

uint64_t FileAccessHatch::get_length() const {
	return 0;
}

bool FileAccessHatch::eof_reached() const {
	return position > file_info.size;
}

uint64_t FileAccessHatch::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V_MSG(file.is_null(), -1, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	if (eof_reached()) {
		return 0;
	}

	int64_t to_read = p_length;

	if (to_read + position > file_info.size) {
		to_read = (int64_t)file_info.size - (int64_t)position;
	}

	position += to_read;

	if (to_read <= 0) {
		return 0;
	}

	//This might not work
    if (file_info.encrypted) {
        uint8_t keyA[16];
        uint8_t keyB[16];
        uint32_t sizeHash = HatchArchiveReader::crc_32_encrypt_data(&file_info.size, sizeof(file_info.size));

        // Populate Key A
        uint32_t* keyA32 = (uint32_t*)&keyA[0];
        keyA32[0] = path_hash;
        keyA32[1] = path_hash;
        keyA32[2] = path_hash;
        keyA32[3] = path_hash;

        // Populate Key B
        uint32_t* keyB32 = (uint32_t*)&keyB[0];
        keyB32[0] = sizeHash;
        keyB32[1] = sizeHash;
        keyB32[2] = sizeHash;
        keyB32[3] = sizeHash;

        for (uint32_t x = 0; x < p_length; x++) {
            uint8_t temp = p_dst[x];

            temp ^= xorValue ^ keyB[indexKeyB++];

            if (swapNibbles)
                temp = (((temp & 0x0F) << 4) | ((temp & 0xF0) >> 4));

            temp ^= keyA[indexKeyA++];

			p_dst[x] = temp;

            if (indexKeyA <= 15) {
                if (indexKeyB > 12) {
                    indexKeyB = 0;
                    swapNibbles ^= 1;
                }
            }
            else if (indexKeyB <= 8) {
                indexKeyA = 0;
                swapNibbles ^= 1;
            }
            else {
                xorValue = (xorValue + 2) & 0x7F;
                if (swapNibbles) {
                    swapNibbles = false;
                    indexKeyA = xorValue % 7;
                    indexKeyB = (xorValue % 12) + 2;
                }
                else {
                    swapNibbles = true;
                    indexKeyA = (xorValue % 12) + 3;
                    indexKeyB = xorValue % 7;
                }
            }
        }
    }

	file->get_buffer(p_dst, to_read);

	return to_read;
}

void FileAccessHatch::set_big_endian(bool p_big_endian) {
	ERR_FAIL_COND_MSG(file.is_null(), "File must be opened before use.");

	FileAccess::set_big_endian(p_big_endian);
	file->set_big_endian(p_big_endian);
}

Error FileAccessHatch::get_error() const {
	if (eof_reached()) {
		return ERR_FILE_EOF;
	}
	return OK;
}

void FileAccessHatch::flush() {
	ERR_FAIL();
}

bool FileAccessHatch::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_V(false);
}


bool FileAccessHatch::file_exists(const String &p_name) {
	return false;
}

void FileAccessHatch::close() {
	file = Ref<FileAccess>();
}

FileAccessHatch::FileAccessHatch(const String &p_path, const PackedData::PackedFile &p_file){
	file_info = p_file;
	file = FileAccess::open(p_path, ModeFlags::READ);

	file->seek(file_info.offset);
	position = 0;
}

bool PackSourceHatch::try_open_pack(const String &p_path, bool p_replace_files, uint64_t p_offset){
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);

	if (file.is_null()){
		return false;
	}

	file->seek(p_offset);

	uint8_t hatch_magic[5];
	file->get_buffer(hatch_magic, 5);

	if (memcmp(hatch_magic, "HATCH", 5)) {
		return false;
	}

	file->get_8(); //major version
	file->get_8(); //minor version
	file->get_8(); //patch version?

	uint16_t file_count = file->get_16();

	for (int cur_file = 0; cur_file < file_count; cur_file++){

		uint32_t crc_32 = file->get_32();
		uint64_t file_offset = file->get_64();
		uint64_t file_size = file->get_64();
		uint32_t file_data_flag = file->get_32();
		/*uint64_t file_compressed_size =*/ file->get_64();



		uint8_t crc_32_b[16];

		crc_32_b[0] = crc_32 & 0xFF;
		crc_32_b[1] = (crc_32 >> 8) & 0xFF;
		crc_32_b[2] = (crc_32 >> 16) & 0xFF;
		crc_32_b[3] = (crc_32 >> 24) & 0xFF;

		String path = String("hatch://") + String::num_uint64(crc_32);

		//The code would suggest that "crc_32_b" will cause issues because it is a crc_32 and not a md5.
		//But in reality, this passed in value is just stored and not used for important retrieval or
		//anything, so it's totally fine :thumbsup:
		PackedData::get_singleton()->add_path(p_path, path, file_offset + p_offset, file_size, crc_32_b, this, p_replace_files, file_data_flag == 2);
	}

	return true;
};

Ref<FileAccess> PackSourceHatch::get_file(const String &p_path, PackedData::PackedFile *p_file){
	return memnew(FileAccessHatch(p_path, *p_file));

};
