#include "Common.hpp"

void Extension::RunJSScript(const TCHAR* ScriptText)
{
    if (!EnsureCurrentContext())
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
    if(ctxId>=0 && ctxId<(int)contexts.size()) {
        currentCtx = ctxId;
    }
    else if (!contexts.empty()) {
        currentCtx = 0;
    }

    EnsureCurrentContext();
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

void Extension::RunScriptFile(const TCHAR* filePath)
{
    if (!EnsureCurrentContext())
        return;

    std::string spec = DarkEdif::TStringToUTF8(filePath ? filePath : _T(""));
    std::string baseDir = DarkEdif::TStringToUTF8(moduleRootPath);
    std::string resolved = ResolveModulePath(spec, baseDir);
    if (resolved.empty())
    {
        DarkEdif::MsgBox::Error(_T("JS Error"), _T("Could not resolve script file path."));
        return;
    }

    if (LoadModule(contexts[currentCtx], resolved, DirName(resolved)))
    {
        duk_pop(contexts[currentCtx]); // discard exports after top-level load
    }
}

void Extension::SetModuleRoot(const TCHAR* rootPath)
{
    moduleRootPath = (rootPath && rootPath[0]) ? rootPath : _T(".");

    if (!EnsureCurrentContext())
        return;

    std::string baseDir = DarkEdif::TStringToUTF8(moduleRootPath);
    for (auto* ctx : contexts)
    {
        if (!ctx)
                continue;
        duk_push_heap_stash(ctx);
        duk_del_prop_string(ctx, -1, "moduleCache");
        duk_pop(ctx);

        PushRequireFunction(ctx, baseDir);
        duk_put_global_string(ctx, "require");
    }
}
