#include <cstdint>
#include <cvar.h>
#include <string>
#include <string_utils.h>
#include <unordered_map>

template<typename T>
struct CVarStorage
{
    T initial;
    T current;
    CVarParameter* cVarParameter;
};

template<typename T>
struct CVarArray
{
    std::vector<CVarStorage<T>> cvars;

    CVarArray(int size)
    {
        cvars.resize(size);
    }
    
    int Size() const { return cvars.size(); }
    T* GetCurrentPtr(int index)
    {
        return &cvars[index].current;
    }
    T GetCurrent(int index)
    {
        return cvars[index].current;
    }

    void SetCurrent(T val, int index)
    {
        cvars[index].current = val;
    }

    void Add(T current, T initial, CVarParameter* cVarParameter)
    {
        CVarStorage<T> storage;
        storage.initial = initial;
        storage.current = current;
        cVarParameter->arrayIndex = cvars.size();
        storage.cVarParameter = cVarParameter;
        cvars.push_back(storage);
    }
};

class CVarSystemImpl : public CVarSystem
{
public:
    constexpr static int INIT_CVARS = 100;
    CVarArray<int> intCVars{ INIT_CVARS };
    CVarArray<float> floatCVars { INIT_CVARS };
    CVarArray<std::string> stringCVars { INIT_CVARS };

    template<typename T>
    CVarArray<T>& GetCVarArray();

	virtual CVarParameter* GetCVar(StringUtils::StringHash hash) override;
	virtual CVarParameter* CreateFloatCVar(const char* name, const char* description, float defaultValue, float currentValue) override;
	virtual CVarParameter* CreateIntCVar(const char* name, const char* description, int defaultValue, int currentValue) override;
	virtual CVarParameter* CreateStringCVar(const char* name, const char* description, std::string defaultValue, std::string currentValue) override;

	virtual float* GetFloatCVar(StringUtils::StringHash hash) override;
	virtual void SetFloatCVar(StringUtils::StringHash hash, float value) override;
	virtual int* GetIntCVar(StringUtils::StringHash hash) override;
	virtual void SetIntCVar(StringUtils::StringHash hash, int value) override;
	virtual std::string* GetStringCVar(StringUtils::StringHash hash) override;
	virtual void SetStringCVar(StringUtils::StringHash hash, std::string value) override;

    static CVarSystemImpl* Get()
	{
		return static_cast<CVarSystemImpl*>(CVarSystem::Get());
	}
private:
    CVarParameter* InitCVar(const char *name, const char* description);
    std::unordered_map<uint32_t, CVarParameter> CVars;

    //templated get-set cvar versions for syntax sugar
	template<typename T>
	T* GetCVarCurrent(uint32_t namehash) {
		CVarParameter* par = GetCVar(namehash);
		if (!par) {
			return nullptr;
		}
		else {
			return GetCVarArray<T>().GetCurrentPtr(par->arrayIndex);
		}
	}

	template<typename T>
	void SetCVarCurrent(uint32_t namehash, const T& value)
	{
		CVarParameter* cvar = GetCVar(namehash);
		if (cvar)
		{
			GetCVarArray<T>().SetCurrent(value, cvar->arrayIndex);
		}
	}
};

template<>
CVarArray<int>& CVarSystemImpl::GetCVarArray<int>()
{
    return intCVars;
}

template<>
CVarArray<float>& CVarSystemImpl::GetCVarArray<float>()
{
    return floatCVars;
}

template<>
CVarArray<std::string>& CVarSystemImpl::GetCVarArray<std::string>()
{
    return stringCVars;
}

CVarSystem* CVarSystem::Get()
{
    static CVarSystemImpl cvarSys{};
    return &cvarSys;
}

CVarParameter* CVarSystemImpl::GetCVar(StringUtils::StringHash hash)
{
    auto it = CVars.find(hash);
    if (it == CVars.end()) return nullptr;

    return &it->second;
}

CVarParameter* CVarSystemImpl::InitCVar(const char *name, const char* description)
{
    if (GetCVar(name)) return nullptr;

    uint32_t hash = StringUtils::StringHash{name};
    CVars[hash] = CVarParameter();
    CVarParameter& param = CVars[hash];
    param.name = name;
    param.descriptor = description;

    return &param;
}

CVarParameter* CVarSystemImpl::CreateFloatCVar(const char* name, const char* description, float defaultValue, float currentValue)
{
    CVarParameter* param = InitCVar(name, description);
    if (param) { return nullptr; }

    param->type = CVarType::FLAOT;
    GetCVarArray<float>().Add(currentValue, defaultValue, param);

    return param;
}

CVarParameter* CVarSystemImpl::CreateIntCVar(const char* name, const char* description, int defaultValue, int currentValue)
{
    CVarParameter* param = InitCVar(name, description);
    if (param) { return nullptr; }

    param->type = CVarType::INT;
    GetCVarArray<int>().Add(currentValue, defaultValue, param);

    return param;
}

CVarParameter* CVarSystemImpl::CreateStringCVar(const char* name, const char* description, std::string defaultValue, std::string currentValue)
{
    CVarParameter* param = InitCVar(name, description);
    if (param) { return nullptr; }

    param->type = CVarType::STRING;
    GetCVarArray<std::string>().Add(currentValue, defaultValue, param);

    return param;
}

float* CVarSystemImpl::GetFloatCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<float>(hash);
}
void CVarSystemImpl::SetFloatCVar(StringUtils::StringHash hash, float value)
{
	SetCVarCurrent<float>(hash, value);
}

int* CVarSystemImpl::GetIntCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<int>(hash);
}
void CVarSystemImpl::SetIntCVar(StringUtils::StringHash hash, int value)
{
	SetCVarCurrent<int>(hash, value);
}

std::string* CVarSystemImpl::GetStringCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<std::string>(hash);
}
void CVarSystemImpl::SetStringCVar(StringUtils::StringHash hash, std::string value)
{
	SetCVarCurrent<std::string>(hash, value);
}


#define DEFINE_AUTO_CVAR_IMPL(TypeName, ValueType) \
AutoCVar_##TypeName::AutoCVar_##TypeName(const char* name, const char* description, ValueType defaultValue, CVarFlags flags)\
{\
	CVarParameter* cvar = CVarSystemImpl::Get()->Create##TypeName##CVar(name, description, defaultValue, defaultValue);\
	cvar->flags = flags;\
	index = cvar->arrayIndex;\
}\
ValueType AutoCVar_##TypeName::Get()\
{\
    return CVarSystemImpl::Get()->GetCVarArray<ValueType>().GetCurrent(index);\
}\
void AutoCVar_##TypeName::Set(ValueType f)\
{\
    CVarSystemImpl::Get()->GetCVarArray<ValueType>().SetCurrent(f, index);\
}\

DEFINE_AUTO_CVAR_IMPL(Float, float)
DEFINE_AUTO_CVAR_IMPL(Int, int)
DEFINE_AUTO_CVAR_IMPL(String, std::string)