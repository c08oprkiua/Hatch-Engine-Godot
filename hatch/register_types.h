#ifndef GODOT_HATCH_REGISTER_TYPES_H
#define GODOT_HATCH_REGISTER_TYPES_H

#include "modules/register_module_types.h"

void register_hatch_types();
void unregister_hatch_types();

void initialize_hatch_module(ModuleInitializationLevel p_level);
void uninitialize_hatch_module(ModuleInitializationLevel p_level);

#endif // GODOT_PHYSICS_2D_REGISTER_TYPES_H
