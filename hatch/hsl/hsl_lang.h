#ifndef HATCH_SCRIPT_LANG_H
#define HATCH_SCRIPT_LANG_H

#include "core/object/script_language.h"

class HatchScriptLanguage : public ScriptLanguage {
	GDCLASS(HatchScriptLanguage, ScriptLanguage);

	HatchScriptLanguage *singleton;

	String objects_hcm_path = "Objects/Objects.hcm"; //bind


protected:
	static void _bind_methods();

public:
	void load_objects_hcm(PackedByteArray buffer);

	virtual String get_name() const override;

	/* LANGUAGE FUNCTIONS */
	virtual void init() override;
	virtual String get_type() const override;
	virtual String get_extension() const override;
	virtual void finish() override;

	virtual void get_reserved_words(List<String> *p_words) const override;
	virtual bool is_control_flow_keyword(const String &p_string) const override;
	virtual void get_comment_delimiters(List<String> *p_delimiters) const override;
	virtual void get_doc_comment_delimiters(List<String> *p_delimiters) const override;
	virtual void get_string_delimiters(List<String> *p_delimiters) const override;
	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override { return Ref<Script>(); }
	virtual Vector<ScriptTemplate> get_built_in_templates(const StringName &p_object) override { return Vector<ScriptTemplate>(); }
	virtual bool is_using_templates() override { return false; }
	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const = 0;
	virtual String validate_path(const String &p_path) const override { return ""; }
	virtual Script *create_script() const = 0;
#ifndef DISABLE_DEPRECATED
	virtual bool has_named_classes() const = 0;
#endif
	virtual bool supports_builtin_mode() const = 0;
	virtual bool supports_documentation() const override { return false; }
	virtual bool can_inherit_from_file() const override { return false; }
	virtual int find_function(const String &p_function, const String &p_code) const override;
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const = 0;
	virtual bool can_make_function() const override { return true; }
	virtual Error open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) override { return ERR_UNAVAILABLE; }
	virtual bool overrides_external_editor() override { return false; }
	virtual ScriptNameCasing preferred_file_name_casing() const override { return SCRIPT_NAME_CASING_SNAKE_CASE; }

	virtual Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) override { return ERR_UNAVAILABLE; }

	virtual Error lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, LookupResult &r_result) override { return ERR_UNAVAILABLE; }

	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const = 0;
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) = 0;
	virtual void add_named_global_constant(const StringName &p_name, const Variant &p_value) override {}
	virtual void remove_named_global_constant(const StringName &p_name) override {}

	/* MULTITHREAD FUNCTIONS */

	//some VMs need to be notified of thread creation/exiting to allocate a stack
	virtual void thread_enter() {}
	virtual void thread_exit() {}

	virtual String debug_get_error() const = 0;
	virtual int debug_get_stack_level_count() const = 0;
	virtual int debug_get_stack_level_line(int p_level) const = 0;
	virtual String debug_get_stack_level_function(int p_level) const = 0;
	virtual String debug_get_stack_level_source(int p_level) const = 0;
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual ScriptInstance *debug_get_stack_level_instance(int p_level) override { return nullptr; }
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) = 0;

	virtual Vector<StackInfo> debug_get_current_stack_info() override { return Vector<StackInfo>(); }

	virtual void reload_all_scripts() = 0;
	virtual void reload_scripts(const Array &p_scripts, bool p_soft_reload) = 0;
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) = 0;
	/* LOADER FUNCTIONS */

	virtual void get_recognized_extensions(List<String> *p_extensions) const = 0;
	virtual void get_public_functions(List<MethodInfo> *p_functions) const = 0;
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const = 0;
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const = 0;

	virtual void profiling_start() = 0;
	virtual void profiling_stop() = 0;
	virtual void profiling_set_save_native_calls(bool p_enable) = 0;

	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) = 0;
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) = 0;

	virtual void frame();

	virtual bool handles_global_class_type(const String &p_type) const override { return false; }
	virtual String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr) const override { return String(); }

	virtual ~HatchScriptLanguage() {}
};


#endif
