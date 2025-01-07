#include "register_types.h"
#include "core/object/class_db.h"

#include "fileaccess_hatch.h"

void register_hatch_types(){
	ClassDB::register_class<HatchArchiveReader>();
}

void unregister_hatch_types(){}

void initialize_hatch_module(ModuleInitializationLevel p_level){
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(HatchArchiveReader);
	}
}

void uninitialize_hatch_module(ModuleInitializationLevel p_level){}
