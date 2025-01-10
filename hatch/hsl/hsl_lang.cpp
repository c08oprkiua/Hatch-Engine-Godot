#include "hsl_lang.h"

void HatchScriptLanguage::load_objects_hcm(){
	ERR_FAIL_COND_MSG(objects_hcm_path.is_empty(), "The path to Objects.hcm is empty, so HSL cannot be loaded");
	ERR_FAIL_COND_MSG(not objects_hcm_path.is_valid_filename(), "The path to Objects.hcm is invalid, so HSL cannot be loaded");
}

String HatchScriptLanguage::get_name() const {
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
	static const char *_reserved_words[] = {
		"and",
		"as",
		"break",
		"case",
		"class",
		"continue",
		"default",
		"do",
		"else",
		"enum",
		"event",
		"false",
		"foreach",
		"from",
		"has",
		"if",
		"in",
		"import",
		"local",
		"namespace",
		"null",
		"new",
		"or",
		"print",
		"return",
		"repeat",
		"static",
		"super",
		"switch",
		"this",
		"true",
		"typeof",
		"using",
		"var",
		"with",
		"while",
		nullptr,
	};

	const char **w = _reserved_words;

	while (*w) {
		p_words->push_back(*w);
		w++;
	}
}

bool HatchScriptLanguage::is_control_flow_keyword(const String &p_keyword) const {
	return p_keyword == "break" or
			p_keyword == "case" or
			p_keyword == "continue" or
			p_keyword == "do" or
			p_keyword == "else" or
			p_keyword == "foreach" or
			p_keyword == "if" or
			p_keyword == "switch" or
			p_keyword == "pass" or
			p_keyword == "repeat" or //I think
			p_keyword == "return" or
			p_keyword == "when" or
			p_keyword == "while";
}

void HatchScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("//");
}

void HatchScriptLanguage::get_doc_comment_delimiters(List<String> *p_delimiters) const {
	return get_comment_delimiters(p_delimiters);
};

void HatchScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("\" \"");
	//p_delimiters->push_back("' '");
	//p_delimiters->push_back("\"\"\" \"\"\"");
	//p_delimiters->push_back("''' '''");
}



