#include "EmbersWrapper.hpp"

// Managed Embers runtime only available when building with MSVC and /clr.
#if defined(_MSC_VER) && defined(__CLR_VER)
#using <mscorlib.dll>
#using "Embers.dll"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace Embers;

EmbersEngine::EmbersEngine()
{
    engine = gcnew Embers::Runtime();
}

EmbersEngine::~EmbersEngine()
{
    delete safe_cast<Embers::Runtime^>(engine);
    engine = nullptr;
}

bool EmbersEngine::Execute(const char* code, std::string& result)
{
    try {
        String^ managedCode = gcnew String(code);
        auto rt = safe_cast<Embers::Runtime^>(engine);
        rt->Execute(managedCode);
        result.clear();
        return true;
    }
    catch (Exception^ ex) {
        result = msclr::interop::marshal_as<std::string>(ex->Message);
        return false;
    }
}

bool EmbersEngine::Eval(const char* expression, std::string& result)
{
    try {
        String^ managedExpr = gcnew String(expression);
        auto rt = safe_cast<Embers::Runtime^>(engine);
        auto value = rt->Evaluate(managedExpr);
        result = msclr::interop::marshal_as<std::string>(value->ToString());
        return true;
    }
    catch (Exception^ ex) {
        result = msclr::interop::marshal_as<std::string>(ex->Message);
        return false;
    }
}

void EmbersEngine::SetGlobalNumber(const char* name, double value)
{
    auto rt = safe_cast<Embers::Runtime^>(engine);
    String^ n = gcnew String(name);
    rt->Globals[n] = value;
}

void EmbersEngine::SetGlobalString(const char* name, const char* value)
{
    auto rt = safe_cast<Embers::Runtime^>(engine);
    String^ n = gcnew String(name);
    String^ v = gcnew String(value);
    rt->Globals[n] = v;
}

double EmbersEngine::GetGlobalNumber(const char* name)
{
    auto rt = safe_cast<Embers::Runtime^>(engine);
    String^ n = gcnew String(name);
    auto val = rt->Globals[n];
    return Convert::ToDouble(val);
}

bool EmbersEngine::GetGlobalString(const char* name, std::string& value)
{
    try {
        auto rt = safe_cast<Embers::Runtime^>(engine);
        String^ n = gcnew String(name);
        auto val = rt->Globals[n];
        if (val == nullptr) {
            value.clear();
            return false;
        }
        value = msclr::interop::marshal_as<std::string>(val->ToString());
        return true;
    }
    catch (Exception^) {
        value.clear();
        return false;
    }
}

#else

EmbersEngine::EmbersEngine() : engine(nullptr) {}
EmbersEngine::~EmbersEngine() {}

bool EmbersEngine::Execute(const char*, std::string&) { return false; }
bool EmbersEngine::Eval(const char*, std::string&) { return false; }
void EmbersEngine::SetGlobalNumber(const char*, double) {}
void EmbersEngine::SetGlobalString(const char*, const char*) {}
double EmbersEngine::GetGlobalNumber(const char*) { return 0.0; }
bool EmbersEngine::GetGlobalString(const char*, std::string&) { return false; }

#endif
