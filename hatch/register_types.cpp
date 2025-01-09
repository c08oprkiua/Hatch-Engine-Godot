#include "register_types.h"
#include "core/object/class_db.h"

#include "file_io/hatch_archive_reader.h"
#include "hsl/hsl_bytecode_reader.h"

void register_hatch_types(){
	ClassDB::register_class<HatchArchiveReader>();
}

void unregister_hatch_types(){}

void initialize_hatch_module(ModuleInitializationLevel p_level){
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE){ //This level *should* be fine?
		//PackedData::get_singleton()->add_pack_source(PackSourceHatch::get_singleton());
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(HatchArchiveReader);
		GDREGISTER_CLASS(HSLBytecodeReader);
	}
}

void uninitialize_hatch_module(ModuleInitializationLevel p_level){}
