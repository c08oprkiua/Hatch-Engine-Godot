#include "fileaccess_hatch.h"


//yes I just copy pasted this function. No I don't care that I didn't make a function from
//scratch that would do the same exact thing just so I would not copy the function.
uint32_t crc_32_encrypt_data(const void* data, size_t size, uint32_t crc = 0xFFFFFFFFU) {
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

uint32_t FileAccessHatch::crc32_string(String path){
	const char *raw_path = path.ascii().ptr();

	return crc_32_encrypt_data((const void *) raw_path, strlen(raw_path), 0xFFFFFFFFU);
}

void FileAccessHatch::open(String path){

	if (path.is_empty()){
		path = "Data.hatch";
	}

	Error open_err;

	file = FileAccess::open(path, FileAccess::ModeFlags::READ, &open_err);


	if (file->file_exists(path)){
		WARN_PRINT("Hatch file found!");
		load(path);
	}
}

void FileAccessHatch::load(String path){
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
		uint32_t crc32 = file->get_32();
		item.offset = file->get_64();
		item.size = file->get_64();
		item.data_flag = file->get_32();
		item.compressed_size = file->get_64();

		crc_array.set(cur_file, crc32);

		resource_registry.insert(crc32, item);
	}

}

Dictionary FileAccessHatch::get_file_information(int index){
	ERR_FAIL_INDEX_V(index, crc_array.size(), Dictionary());

	uint32_t crc32 = crc_array.get(index);

	return get_file_information_hash(crc32);
};

Dictionary FileAccessHatch::get_file_information_hash(uint32_t hash){
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

PackedByteArray FileAccessHatch::load_resource(String filename){

	uint32_t path_hash = crc32_string(filename.validate_filename());

	return load_resource_hash(path_hash);
}

PackedByteArray FileAccessHatch::load_resource_hash(uint32_t hash){
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

void FileAccessHatch::_bind_methods(){
    ClassDB::bind_method(D_METHOD("get_file_count"), &FileAccessHatch::get_file_count);
    ClassDB::bind_method(D_METHOD("set_file_count", "file_count"), &FileAccessHatch::set_file_count);

    ClassDB::bind_method(D_METHOD("crc32_string", "str"), &FileAccessHatch::crc32_string);

	ClassDB::bind_method(D_METHOD("open", "file_path"), &FileAccessHatch::open);
    ClassDB::bind_method(D_METHOD("load_resource_from_name", "file_name"), &FileAccessHatch::load_resource);
    ClassDB::bind_method(D_METHOD("load_resource_from_hash", "file_name"), &FileAccessHatch::load_resource_hash);
	ClassDB::bind_method(D_METHOD("get_file_information_from_index", "file_index"), &FileAccessHatch::get_file_information);
	ClassDB::bind_method(D_METHOD("get_file_information_from_hash", "name_hash"), &FileAccessHatch::get_file_information_hash);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "file_count"), "set_file_count", "get_file_count");
}

uint16_t FileAccessHatch::get_file_count(){
    return file_count;
}

void FileAccessHatch::set_file_count(uint16_t new_file_count){
    file_count = new_file_count;
}
