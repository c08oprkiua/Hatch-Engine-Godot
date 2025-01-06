#include "hsl_lang.h"

void HatchScriptLanguage::load_objects_hcm(){
	ERR_FAIL_COND_MSG(objects_hcm_path.is_empty(), "The path to Objects.hcm is empty, so HSL cannot be loaded");
	ERR_FAIL_COND_MSG(not objects_hcm_path.is_valid_filename(), "The path to Objects.hcm is invalid, so HSL cannot be loaded");

}

String HatchScriptLanguage::get_name() const{
	return "HatchScriptLanguage";
}

void HatchScriptLanguage::init(){

}

String HatchScriptLanguage::get_type() const {
	return "HatchScript";
}

String HatchScriptLanguage::get_extension() const {
	return "hsl";
}

void HatchScriptLanguage::finish(){

}

void HatchScriptLanguage::get_reserved_words(List<String> *p_words) const {

}
