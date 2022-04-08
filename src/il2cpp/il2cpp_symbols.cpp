#include <stdinclude.hpp>

il2cpp_string_new_utf16_t il2cpp_string_new_utf16;
il2cpp_string_new_t il2cpp_string_new;
il2cpp_domain_get_t il2cpp_domain_get;
il2cpp_domain_assembly_open_t il2cpp_domain_assembly_open;
il2cpp_assembly_get_image_t il2cpp_assembly_get_image;
il2cpp_class_from_name_t il2cpp_class_from_name;
il2cpp_class_get_methods_t il2cpp_class_get_methods;
il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name;
il2cpp_method_get_param_t il2cpp_method_get_param;
il2cpp_object_new_t il2cpp_object_new;
il2cpp_resolve_icall_t il2cpp_resolve_icall;
il2cpp_array_new_t il2cpp_array_new;
il2cpp_thread_attach_t il2cpp_thread_attach;
il2cpp_thread_detach_t il2cpp_thread_detach;
il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name;
il2cpp_class_is_assignable_from_t il2cpp_class_is_assignable_from;
il2cpp_class_for_each_t il2cpp_class_for_each;
il2cpp_class_get_nested_types_t il2cpp_class_get_nested_types;
il2cpp_class_get_type_t il2cpp_class_get_type;
il2cpp_type_get_object_t il2cpp_type_get_object;
il2cpp_gchandle_new_t il2cpp_gchandle_new;
il2cpp_gchandle_free_t il2cpp_gchandle_free;
il2cpp_gchandle_get_target_t il2cpp_gchandle_get_target;
il2cpp_class_from_type_t il2cpp_class_from_type;

char* il2cpp_array_addr_with_size(void* array, int32_t size, uintptr_t idx)
{
	return ((char*)array) + kIl2CppSizeOfArray + size * idx;
}

namespace il2cpp_symbols
{
#define RESOLVE_IMPORT(name) name = reinterpret_cast<name##_t>(GetProcAddress(game_module, #name))

	void* il2cpp_domain = nullptr;

	void init(HMODULE game_module)
	{
		RESOLVE_IMPORT(il2cpp_string_new_utf16);
		RESOLVE_IMPORT(il2cpp_string_new);
		RESOLVE_IMPORT(il2cpp_domain_get);
		RESOLVE_IMPORT(il2cpp_domain_assembly_open);
		RESOLVE_IMPORT(il2cpp_assembly_get_image);
		RESOLVE_IMPORT(il2cpp_class_from_name);
		RESOLVE_IMPORT(il2cpp_class_get_methods);
		RESOLVE_IMPORT(il2cpp_class_get_method_from_name);
		RESOLVE_IMPORT(il2cpp_method_get_param);
		RESOLVE_IMPORT(il2cpp_object_new);
		RESOLVE_IMPORT(il2cpp_resolve_icall);
		RESOLVE_IMPORT(il2cpp_array_new);
		RESOLVE_IMPORT(il2cpp_thread_attach);
		RESOLVE_IMPORT(il2cpp_thread_detach);
		RESOLVE_IMPORT(il2cpp_class_get_field_from_name);
		RESOLVE_IMPORT(il2cpp_class_is_assignable_from);
		RESOLVE_IMPORT(il2cpp_class_for_each);
		RESOLVE_IMPORT(il2cpp_class_get_nested_types);
		RESOLVE_IMPORT(il2cpp_class_get_type);
		RESOLVE_IMPORT(il2cpp_type_get_object);
		RESOLVE_IMPORT(il2cpp_gchandle_new);
		RESOLVE_IMPORT(il2cpp_gchandle_free);
		RESOLVE_IMPORT(il2cpp_gchandle_get_target);
		RESOLVE_IMPORT(il2cpp_class_from_type);

		il2cpp_domain = il2cpp_domain_get();
	}

	void* get_class(const char* assemblyName, const char* namespaze, const char* klassName)
	{
		auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
		auto image = il2cpp_assembly_get_image(assembly);
		return il2cpp_class_from_name(image, namespaze, klassName);
	}

	uintptr_t get_method_pointer(const char* assemblyName, const char* namespaze,
								 const char* klassName, const char* name, int argsCount)
	{
		auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
		auto image = il2cpp_assembly_get_image(assembly);
		auto klass = il2cpp_class_from_name(image, namespaze, klassName);

		return il2cpp_class_get_method_from_name(klass, name, argsCount)->methodPointer;
	}

	void* find_nested_class_from_name(void* klass, const char* name)
	{
		return find_nested_class(klass, [name = std::string_view(name)](void* nestedClass) {
			return static_cast<Il2CppClassHead*>(nestedClass)->name == name;
		});
	}

	MethodInfo* get_method(const char* assemblyName, const char* namespaze,
						   const char* klassName, const char* name, int argsCount)
	{
		auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
		auto image = il2cpp_assembly_get_image(assembly);
		auto klass = il2cpp_class_from_name(image, namespaze, klassName);

		return il2cpp_class_get_method_from_name(klass, name, argsCount);
	}

	uintptr_t find_method(const char* assemblyName, const char* namespaze,
						  const char* klassName, std::function<bool(const MethodInfo*)> predict)
	{
		auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
		auto image = il2cpp_assembly_get_image(assembly);
		auto klass = il2cpp_class_from_name(image, namespaze, klassName);

		void* iter = nullptr;
		while (const MethodInfo* method = il2cpp_class_get_methods(klass, &iter))
		{
			if (predict(method))
				return method->methodPointer;
		}

		return 0;
	}

	FieldInfo* get_field(const char* assemblyName, const char* namespaze,
						 const char* klassName, const char* name)
	{
		const auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
		const auto image = il2cpp_assembly_get_image(assembly);
		const auto klass = il2cpp_class_from_name(image, namespaze, klassName);

		return il2cpp_class_get_field_from_name(klass, name);
	}

	void* get_class_from_instance(const void* instance)
	{
		return *static_cast<void* const*>(std::assume_aligned<alignof(void*)>(instance));
	}
}
