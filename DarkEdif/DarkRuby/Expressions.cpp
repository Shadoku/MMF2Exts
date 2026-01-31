#include "Common.hpp"

const TCHAR* Extension::EvalRuby(const TCHAR* ExpressionText)
{
    std::string utf8 = DarkEdif::TStringToUTF8(ExpressionText);
    std::string result;
    if (!engine->Eval(utf8.c_str(), result))
        return Runtime.CopyString(_T(""));
    return Runtime.CopyString(DarkEdif::UTF8ToTString(result).c_str());
}

float Extension::GetGlobalNumber(const TCHAR* Name)
{
    std::string n = DarkEdif::TStringToUTF8(Name);
    return engine->GetGlobalNumber(n.c_str());
}

const TCHAR* Extension::GetGlobalString(const TCHAR* Name)
{
    std::string n = DarkEdif::TStringToUTF8(Name);
    std::string result;
    if (!engine->GetGlobalString(n.c_str(), result))
        return Runtime.CopyString(_T(""));
    return Runtime.CopyString(DarkEdif::UTF8ToTString(result).c_str());
}
