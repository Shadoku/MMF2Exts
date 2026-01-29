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
