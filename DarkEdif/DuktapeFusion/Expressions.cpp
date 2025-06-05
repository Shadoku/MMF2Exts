#include "Common.hpp"

const TCHAR* Extension::EvalJS(const TCHAR* ExpressionText)
{
    if (currentCtx < 0 || currentCtx >= (int)contexts.size())
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
    duk_context* ctx = duk_create_heap_default();
    if(!ctx) return -1;
    contexts.push_back(ctx);
    // inject helper object
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_push_c_function(ctx, JS_ListObjects, 1);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "\xff\xffext");
    duk_put_prop_string(ctx, -2, "listObjects");
    duk_push_c_function(ctx, JS_GetPosition, 1);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "\xff\xffext");
    duk_put_prop_string(ctx, -2, "getPos");
    duk_push_c_function(ctx, JS_SetPosition, 3);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "\xff\xffext");
    duk_put_prop_string(ctx, -2, "setPos");
    duk_push_c_function(ctx, JS_GetAltValue, 2);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "\xff\xffext");
    duk_put_prop_string(ctx, -2, "getAltValue");
    duk_push_c_function(ctx, JS_SetAltValue, 3);
    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, "\xff\xffext");
    duk_put_prop_string(ctx, -2, "setAltValue");
    duk_put_prop_string(ctx, -2, "fusion");
    duk_pop(ctx);
    currentCtx = (int)contexts.size()-1;
    return currentCtx;
}
