#include "Common.hpp"

void Extension::RunJSScript(const TCHAR* ScriptText)
{
    if (currentCtx < 0 || currentCtx >= (int)contexts.size())
        return;
    std::string utf8 = DarkEdif::TStringToUTF8(ScriptText);
    if (duk_peval_string(contexts[currentCtx], utf8.c_str()) != 0) {
        const char* err = duk_safe_to_string(contexts[currentCtx], -1);
        DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
    }
    duk_pop(contexts[currentCtx]);
}

void Extension::SetContext(int ctxId)
{
    if(ctxId>=0 && ctxId<(int)contexts.size()) currentCtx = ctxId;
}

void Extension::RegisterObjectScript(int fixedValue, int altStringIndex, int runEveryFrame)
{
    RunObjectMultiPlatPtr obj = Runtime.RunObjPtrFromFixed(fixedValue);
    if (!obj)
    {
        DarkEdif::MsgBox::Error(_T("JS Error"), _T("Could not find object to register."));
        return;
    }

    if (CacheObjectScript(fixedValue, obj, altStringIndex, runEveryFrame != 0) && runEveryFrame)
    {
        Runtime.Rehandle();
    }
}

void Extension::UnregisterObjectScript(int fixedValue)
{
    auto it = registeredScripts.find(fixedValue);
    if (it == registeredScripts.end())
        return;

    ClearScriptReference(it->second);
    registeredScripts.erase(it);
}

void Extension::InvokeObjectScript(int fixedValue)
{
    auto it = registeredScripts.find(fixedValue);
    if (it == registeredScripts.end())
    {
        DarkEdif::MsgBox::Error(_T("JS Error"), _T("Object has no registered script."));
        return;
    }

    RunObjectMultiPlatPtr obj = Runtime.RunObjPtrFromFixed(fixedValue);
    if (!obj)
    {
        DarkEdif::MsgBox::Error(_T("JS Error"), _T("Target object could not be found."));
        return;
    }

    ExecuteObjectScript(fixedValue, it->second, obj);
}
