#include "hsl_script.h"

void HatchScript::reload_from_file(){

};

bool HatchScript::can_instantiate() const {
	return false;
};

Ref<Script> HatchScript::get_base_script() const {
	return nullptr;
}; //for script inheritance

StringName HatchScript::get_global_name() const {
	return "";
};
bool HatchScript::inherits_script(const Ref<Script> &p_script) const {
	return false;
};
