#include "Common.hpp"

const TCHAR* Extension::EvalJS(const TCHAR* ExpressionText)
{
    if (!EnsureCurrentContext())
        return Runtime.CopyString(_T(""));
    std::string utf8 = DarkEdif::TStringToUTF8(ExpressionText);
    if (duk_peval_string(contexts[currentCtx], utf8.c_str()) != 0) {
        const char* err = duk_safe_to_string(contexts[currentCtx], -1);
        DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
        duk_pop(contexts[currentCtx]);
        return Runtime.CopyString(_T(""));
    }
    const char* res = duk_safe_to_string(contexts[currentCtx], -1);
    std::tstring tres = DarkEdif::UTF8ToTString(res ? res : "");
    duk_pop(contexts[currentCtx]);
    return Runtime.CopyString(tres.c_str());
}

int Extension::NewContext()
{
    if (contexts.empty())
    {
        duk_context* ctx = CreateContextWithHelpers();
        if (!ctx)
                return -1;

        contexts.push_back(ctx);
        currentCtx = 0;
        return currentCtx;
    }

    int targetCtx = (currentCtx >= 0 && currentCtx < (int)contexts.size()) ? currentCtx : 0;
    if (!RebuildContext(targetCtx))
            return -1;

    currentCtx = targetCtx;
    return currentCtx;
}
