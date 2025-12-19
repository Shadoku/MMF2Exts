#include "Common.hpp"
#include <sstream>

const TCHAR* Extension::EvalJS(const TCHAR* ExpressionText)
{
    if (!EnsureCurrentContext())
        return Runtime.CopyString(_T(""));

    duk_context* ctx = (currentCtx >= 0 && currentCtx < (int)contexts.size()) ? contexts[currentCtx] : nullptr;
    if (!ctx)
        return Runtime.CopyString(_T(""));
    std::string utf8 = DarkEdif::TStringToUTF8(ExpressionText);
    if (duk_peval_string(ctx, utf8.c_str()) != 0) {
        const char* err = duk_safe_to_string(ctx, -1);
        DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
        duk_pop(ctx);
        return Runtime.CopyString(_T(""));
    }
    const char* res = duk_safe_to_string(ctx, -1);
    std::tstring tres = DarkEdif::UTF8ToTString(res ? res : "");
    duk_pop(ctx);
    return Runtime.CopyString(tres.c_str());
}

int Extension::NewContext()
{
    std::ostringstream ss;
    ss << "NewContext: entry currentCtx=" << currentCtx << " contexts=" << contexts.size();
    DebugTrace(ss.str());
    if (contexts.empty())
    {
        duk_context* ctx = CreateContextWithHelpers();
        if (!ctx)
        {
            DebugTrace("NewContext: CreateContextWithHelpers failed on empty init");
            return -1;
        }

        contexts.push_back(ctx);
        currentCtx = 0;
        DebugTraceContextState("NewContext: initial context created");
        return currentCtx;
    }

    int targetCtx = (currentCtx >= 0 && currentCtx < (int)contexts.size()) ? currentCtx : 0;
    if (!RebuildContext(targetCtx))
    {
        DebugTrace("NewContext: RebuildContext failed");
        return -1;
    }

    currentCtx = targetCtx;
    DebugTraceContextState("NewContext: rebuilt context");
    return currentCtx;
}
