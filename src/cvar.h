#pragma once

#include "string_utils.h"
#include <cstddef>
#include <string>
enum class CVarFlags : uint32_t
{
    None = 0,
	Noedit = 1 << 1,
	EditReadOnly = 1 << 2,
	Advanced = 1 << 3,

	EditCheckbox = 1 << 8,
	EditFloatDrag = 1 << 9,
};

enum class CVarType : char
{
    INT,
    FLAOT,
    STRING,
};

class CVarParameter
{
public:
    friend class CVarSystemImpl;

    int32_t arrayIndex;

    CVarType type;
    CVarFlags flags;
    std::string name;
    std::string descriptor;
};

class CVarSystem
{
public:
    static CVarSystem* Get();

	virtual CVarParameter* GetCVar(StringUtils::StringHash hash) = 0;
	virtual CVarParameter* CreateFloatCVar(const char* name, const char* description, float defaultValue, float currentValue) = 0;
	virtual CVarParameter* CreateIntCVar(const char* name, const char* description, int defaultValue, int currentValue) = 0;
	virtual CVarParameter* CreateStringCVar(const char* name, const char* description, std::string defaultValue, std::string currentValue) = 0;

	virtual float* GetFloatCVar(StringUtils::StringHash hash) = 0;
	virtual void SetFloatCVar(StringUtils::StringHash hash, float value) = 0;
	virtual int* GetIntCVar(StringUtils::StringHash hash) = 0;
	virtual void SetIntCVar(StringUtils::StringHash hash, int value) = 0;
	virtual std::string* GetStringCVar(StringUtils::StringHash hash) = 0;
	virtual void SetStringCVar(StringUtils::StringHash hash, std::string value) = 0;
};

template<typename T>
struct AutoCVar
{
protected:
	int index;
	using CVarType = T;
};

// AutoCVar_T 宏定义
#define DEFINE_AUTO_CVAR_STRUCT(TypeName, ValueType) \
struct AutoCVar_##TypeName : AutoCVar<ValueType> \
{ \
    AutoCVar_##TypeName(const char* name, const char* description, ValueType defaultValue, CVarFlags flags = CVarFlags::None); \
    \
    ValueType Get(); \
    void Set(ValueType val); \
};

DEFINE_AUTO_CVAR_STRUCT(Float, float)
DEFINE_AUTO_CVAR_STRUCT(Int, int)
DEFINE_AUTO_CVAR_STRUCT(String, std::string)