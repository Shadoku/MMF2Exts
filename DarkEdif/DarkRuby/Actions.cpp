#include "Common.hpp"

void Extension::RunRubyScript(const TCHAR* ScriptText)
{
    std::string utf8 = DarkEdif::TStringToUTF8(ScriptText);
    std::string result;
    if (!engine->Execute(utf8.c_str(), result))
        DarkEdif::MsgBox::Error(_T("Ruby Error"), DarkEdif::UTF8ToTString(result).c_str());
}

void Extension::SetGlobalNumber(const TCHAR* Name, float Value)
{
    std::string n = DarkEdif::TStringToUTF8(Name);
    engine->SetGlobalNumber(n.c_str(), Value);
}

void Extension::SetGlobalString(const TCHAR* Name, const TCHAR* Value)
{
    std::string n = DarkEdif::TStringToUTF8(Name);
    std::string v = DarkEdif::TStringToUTF8(Value);
    engine->SetGlobalString(n.c_str(), v.c_str());
}
