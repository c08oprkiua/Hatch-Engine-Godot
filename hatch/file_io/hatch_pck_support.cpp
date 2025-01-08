#include "hatch_pck_support.h"

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
	file = FileAccess::open(String::num_uint64(HatchArchiveReader::crc32_string(p_path)), ModeFlags::READ);

	file->seek(file_info.offset);
	position = 0;
}

PackSourceHatch *PackSourceHatch::get_singleton(){
	if (singleton == nullptr){
		singleton = memnew(PackSourceHatch);
	}

	return singleton;
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

		String path = String(HATCH_FILE_PREFIX) + String::num_uint64(crc_32);

		//The code would suggest that "crc_32_b" will cause issues because it is a crc_32 and not a md5.
		//But in reality, this passed in value is just stored and not used for important retrieval or
		//anything, so it's totally fine :thumbsup:

		PackedData::get_singleton()->add_path(p_path, path, file_offset + p_offset, file_size, crc_32_b, this, p_replace_files, file_data_flag == 2);


	}

	return true;
};

Ref<FileAccess> PackSourceHatch::get_file(const String &p_path, PackedData::PackedFile *p_file){
	String adjusted_path = p_path.trim_prefix(HATCH_FILE_PREFIX);

	return memnew(FileAccessHatch(p_path, *p_file));

};
