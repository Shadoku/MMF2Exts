#include "Common.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>

///
/// EXTENSION CONSTRUCTOR/DESTRUCTOR
///

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
	rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this), FusionDebugger(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr) :
	javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
	Runtime(this, this->javaExtPtr), FusionDebugger(this)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr) :
	objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr), FusionDebugger(this)
#endif
{
        DebugTrace("Extension ctor: begin");
	/*
		Link all your action/condition/expression functions to their IDs to match the
		IDs in the JSON here
	*/

        LinkAction(0, RunJSScript);
        LinkAction(1, SetContext);
        LinkAction(2, RegisterObjectScript);
        LinkAction(3, UnregisterObjectScript);
        LinkAction(4, InvokeObjectScript);
        LinkAction(5, RunScriptFile);
        LinkAction(6, SetModuleRoot);

        LinkCondition(0, DummyCondition);
        LinkCondition(1, IsObjectRegistered);

        LinkExpression(0, EvalJS);
        LinkExpression(1, NewContext);

	/*
		This is where you'd do anything you'd do in CreateRunObject in the original SDK

		It's the only place you'll get access to edPtr at runtime, so you should transfer
		anything from edPtr to the extension class here.

	*/

	// Don't use "this" inside these lambda functions, always ext.
	// There can be nothing in the [] section of the lambda.
	// If you're not sure about lambdas, you can remove this debugger stuff without any side effects;
	// it's just an example of how to use the debugger. You can view it in Fusion itself to see.
        FusionDebugger.AddItemToDebugger(
                // reader function for your debug item
                [](Extension *ext, std::tstring &writeTo) {
                        writeTo = _T("My text is: ") + ext->exampleDebuggerTextItem;
                },
		// writer function (can be null if you don't want user to be able to edit it in debugger)
		[](Extension *ext, std::tstring &newText)
		{
			ext->exampleDebuggerTextItem = newText;
			return true; // accept the changes
		}, 500, NULL
        );

        // create first context
        DebugTraceContextState("Extension ctor: before NewContext");
        NewContext();
        DebugTraceContextState("Extension ctor: after NewContext");



	// Read object DarkEdif properties; you can pass property name, or property index
	// This will work on all platforms the same way.
	bool checkboxWithinFolder = edPtr->Props.IsPropChecked("Checkbox within folder"sv);
	std::tstring editable6Text = edPtr->Props.GetPropertyStr("Editable 6"sv);

	// These lines do nothing, but prevent the compiler warning the variables are unused
	(void)checkboxWithinFolder;
	(void)editable6Text;
}

Extension::~Extension()
{
        DebugTrace("Extension dtor: begin");
        for (auto& entry : registeredScripts)
                ClearScriptReference(entry.second);
        registeredScripts.clear();
        for (auto ctx : contexts)
                duk_destroy_heap(ctx);
        DebugTrace("Extension dtor: finished");
}

bool Extension::EnsureCurrentContext()
{
        DebugTraceContextState("EnsureCurrentContext: entry");
        auto hasCtx = [this](int idx) -> bool {
                return idx >= 0 && idx < (int)contexts.size() && contexts[idx] != nullptr;
        };

        if (currentCtx < 0 || currentCtx >= (int)contexts.size())
        {
                DebugTrace("EnsureCurrentContext: clamping currentCtx to 0");
                currentCtx = 0;
        }

        if (hasCtx(currentCtx))
        {
                DebugTraceContextState("EnsureCurrentContext: current valid");
                return true;
        }

        if (hasCtx(0))
        {
                currentCtx = 0;
                DebugTrace("EnsureCurrentContext: fell back to context 0");
                return true;
        }

        if (!contexts.empty())
        {
                // slot exists but the heap was destroyed; rebuild the shared context
                DebugTrace("EnsureCurrentContext: attempting rebuild of context 0");
                if (RebuildContext(0))
                {
                        currentCtx = 0;
                        DebugTraceContextState("EnsureCurrentContext: rebuild succeeded");
                        return true;
                }
                DebugTrace("EnsureCurrentContext: rebuild failed");
        }

        int newCtx = NewContext();
        if (newCtx < 0)
        {
                DebugTrace("EnsureCurrentContext: NewContext failed, clearing currentCtx");
                currentCtx = -1;
                return false;
        }

        currentCtx = newCtx;
        DebugTraceContextState("EnsureCurrentContext: created new context");
        return true;
}

duk_context* Extension::CreateContextWithHelpers()
{
        DebugTrace("CreateContextWithHelpers: creating heap");
        duk_context* ctx = duk_create_heap_default();
        if (!ctx)
        {
                DebugTrace("CreateContextWithHelpers: heap creation failed");
                return nullptr;
        }

        static const char* kExtPtrKey = "__fusion_ext";

        duk_push_global_object(ctx);
        duk_push_object(ctx);
        duk_push_c_function(ctx, JS_ListObjects, 1);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, kExtPtrKey);
        duk_put_prop_string(ctx, -2, "listObjects");
        duk_push_c_function(ctx, JS_GetPosition, 1);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, kExtPtrKey);
        duk_put_prop_string(ctx, -2, "getPos");
        duk_push_c_function(ctx, JS_SetPosition, 3);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, kExtPtrKey);
        duk_put_prop_string(ctx, -2, "setPos");
        duk_push_c_function(ctx, JS_GetAltValue, 2);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, kExtPtrKey);
        duk_put_prop_string(ctx, -2, "getAltValue");
        duk_push_c_function(ctx, JS_SetAltValue, 3);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, kExtPtrKey);
        duk_put_prop_string(ctx, -2, "setAltValue");
        duk_put_prop_string(ctx, -2, "fusion");
        // module cache placeholder
        duk_push_heap_stash(ctx);
        duk_push_object(ctx);
        duk_put_prop_string(ctx, -2, "moduleCache");
        duk_pop(ctx);
        // global require bound to module root
        PushRequireFunction(ctx, DarkEdif::TStringToUTF8(moduleRootPath));
        duk_put_global_string(ctx, "require");
        duk_pop(ctx);

        std::ostringstream ss;
        ss << "CreateContextWithHelpers: heap=" << ctx;
        DebugTrace(ss.str());
        return ctx;
}

bool Extension::RebuildContext(int ctxId)
{
        std::ostringstream ss;
        ss << "RebuildContext: ctxId=" << ctxId << " size=" << contexts.size();
        DebugTrace(ss.str());
        if (ctxId < 0)
                return false;

        std::vector<std::tuple<int, int, bool>> toRestore;
        toRestore.reserve(registeredScripts.size());
        for (auto& entry : registeredScripts)
        {
                toRestore.emplace_back(entry.first, entry.second.altStringIndex, entry.second.runEveryFrame);
        }

        duk_context* newCtx = CreateContextWithHelpers();
        if (!newCtx)
        {
                DebugTrace("RebuildContext: CreateContextWithHelpers failed");
                return false;
        }

        // Destroy any extra contexts so we keep one shared heap around.
        for (size_t i = 0; i < contexts.size(); ++i)
        {
                if ((int)i == ctxId)
                        continue;
                if (contexts[i])
                {
                        duk_destroy_heap(contexts[i]);
                        contexts[i] = nullptr;
                }
        }
        contexts.resize((size_t)ctxId + 1, nullptr);

        if (contexts[ctxId])
        {
                DebugTrace("RebuildContext: destroying existing heap in target slot");
                duk_destroy_heap(contexts[ctxId]);
        }

        contexts[ctxId] = newCtx;
        registeredScripts.clear();

        int previousCtx = currentCtx;
        currentCtx = ctxId;
        bool needsHandle = false;
        for (auto& entry : toRestore)
        {
                int fixedValue = std::get<0>(entry);
                RunObjectMultiPlatPtr obj = Runtime.RunObjPtrFromFixed(fixedValue);
                if (!obj)
                        continue;

                needsHandle = needsHandle || std::get<2>(entry);
                CacheObjectScript(fixedValue, obj, std::get<1>(entry), std::get<2>(entry));
        }
        currentCtx = previousCtx;

        if (needsHandle)
                Runtime.Rehandle();

        DebugTraceContextState("RebuildContext: complete");
        return true;
}

void Extension::DebugTrace(const std::string& message) const
{
        std::string line = "[DuktapeFusion] " + message;
#ifdef _WIN32
        line.push_back('\n');
        OutputDebugStringA(line.c_str());
#else
        std::fprintf(stderr, "%s\n", line.c_str());
#endif
}

void Extension::DebugTraceContextState(const char* stage) const
{
        std::ostringstream ss;
        ss << "ContextState";
        if (stage && stage[0])
                ss << " (" << stage << ")";
        ss << ": currentCtx=" << currentCtx << " total=" << contexts.size();
        for (size_t i = 0; i < contexts.size(); ++i)
        {
                ss << " [" << i << ":" << (contexts[i] ? "ok" : "null") << "]";
        }
        DebugTrace(ss.str());
}


REFLAG Extension::Handle()
{
	/*
		If your extension will draw to the MMF window you should first
		check if anything about its display has changed :

			if (rdPtr->roc.rcChanged)
			  return REFLAG::DISPLAY;
			else
			  return REFLAG::NONE;

		You will also need to make sure you change this flag yourself
		to 1 whenever you want to redraw your object

		If your extension won't draw to the window, but it still needs
		to do something every MMF loop use :

			return REFLAG::NONE;

		If you don't need to do something every loop, use :

			return REFLAG::ONE_SHOT;

		This doesn't mean this function can never run again. If you want MMF
		to handle your object again (causing this code to run) use this function:

			Runtime.Rehandle();

		At the end of the loop this code will run

	*/

    for (auto it = registeredScripts.begin(); it != registeredScripts.end(); )
    {
        auto& info = it->second;
        if (!info.runEveryFrame)
        {
            ++it;
            continue;
        }

        if (info.contextId < 0 || info.contextId >= (int)contexts.size())
        {
            ClearScriptReference(info);
            it = registeredScripts.erase(it);
            continue;
        }

        if (contexts[info.contextId] == nullptr)
        {
            ClearScriptReference(info);
            it = registeredScripts.erase(it);
            continue;
        }

        RunObjectMultiPlatPtr obj = Runtime.RunObjPtrFromFixed(it->first);
        if (!obj)
        {
            ClearScriptReference(info);
            it = registeredScripts.erase(it);
            continue;
        }

        ExecuteObjectScript(it->first, info, obj);
        ++it;
    }

    return REFLAG::NONE;
}


REFLAG Extension::Display()
{
	/*
		If you return REFLAG_DISPLAY in Handle() this routine will run.
	*/

	// Ok
	return REFLAG::DISPLAY;
}

short Extension::FusionRuntimePaused()
{

	// Ok
	return 0;
}

short Extension::FusionRuntimeContinued()
{

	// Ok
	return 0;
}


// These are called if there's no function linked to an ID

void Extension::UnlinkedAction(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedAction() called"), _T("Running a fallback for action ID %d. Make sure you ran LinkAction()."), ID);
}

long Extension::UnlinkedCondition(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedCondition() called"), _T("Running a fallback for condition ID %d. Make sure you ran LinkCondition()."), ID);
	return 0;
}

long Extension::UnlinkedExpression(int ID)
{

        DarkEdif::MsgBox::Error(_T("Extension::UnlinkedExpression() called"), _T("Running a fallback for expression ID %d. Make sure you ran LinkExpression()."), ID);
        // Unlinked A/C/E is fatal error, but try not to return null string and definitely crash it
        if ((size_t)ID < Edif::SDK->ExpressionInfos.size() && Edif::SDK->ExpressionInfos[ID]->Flags.ef == ExpReturnType::String)
                return (long)Runtime.CopyString(_T(""));
        return 0;
}

duk_ret_t Extension::JS_ListObjects(duk_context* ctx)
{
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);
        int oi = (int)duk_require_int(ctx, 0);
        duk_idx_t arr = duk_push_array(ctx);
        int idx = 0;
        for (auto ro : DarkEdif::ObjectIterator(ext->rhPtr, oi, DarkEdif::Selection::Explicit))
        {
                duk_push_int(ctx, ro->get_rHo()->GetFixedValue());
                duk_put_prop_index(ctx, arr, idx++);
        }
        return 1;
}

duk_ret_t Extension::JS_GetPosition(duk_context* ctx)
{
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);
        int fv = (int)duk_require_int(ctx, 0);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro)
        {
                return 0;
        }
        HeaderObject* ho = ro->get_rHo();
        duk_push_int(ctx, ho->X);
        duk_push_int(ctx, ho->Y);
        return 2;
}

duk_ret_t Extension::JS_SetPosition(duk_context* ctx)
{
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);
        int fv = (int)duk_require_int(ctx, 0);
        int x = (int)duk_require_int(ctx, 1);
        int y = (int)duk_require_int(ctx, 2);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro)
                return 0;
        HeaderObject* ho = ro->get_rHo();
        ho->X = x;
        ho->Y = y;
        return 0;
}

duk_ret_t Extension::JS_GetAltValue(duk_context* ctx)
{
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);
        int fv = (int)duk_require_int(ctx, 0);
        int index = (int)duk_require_int(ctx, 1);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro || !ro->get_rov())
        {
                return 0;
        }
        const CValueMultiPlat* val = ro->get_rov()->GetAltValueAtIndex(index);
        if (!val)
        {
                return 0;
        }
        if (val->m_type == TYPE_STRING)
        {
                duk_push_string(ctx, DarkEdif::TStringToUTF8(val->m_pString ? val->m_pString : _T("")).c_str());
        }
        else if (val->m_type == TYPE_FLOAT)
        {
                duk_push_number(ctx, val->m_double);
        }
        else
        {
                duk_push_int(ctx, val->m_long);
        }
        return 1;
}

duk_ret_t Extension::JS_SetAltValue(duk_context* ctx)
{
        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);
        int fv = (int)duk_require_int(ctx, 0);
        int index = (int)duk_require_int(ctx, 1);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro || !ro->get_rov())
                return 0;

        if (duk_is_string(ctx, 2))
        {
                const char* s = duk_get_string(ctx, 2);
                ro->get_rov()->SetAltStringAtIndex(index, DarkEdif::UTF8ToTString(s));
        }
        else if (duk_is_number(ctx, 2) && duk_get_int(ctx, 2) == duk_get_number(ctx,2))
        {
                ro->get_rov()->SetAltValueAtIndex(index, (int)duk_get_int(ctx, 2));
        }
        else if (duk_is_number(ctx, 2))
        {
                ro->get_rov()->SetAltValueAtIndex(index, duk_get_number(ctx, 2));
        }
        return 0;
}

std::string Extension::NormalizePath(const std::string& path) const
{
        std::string norm = path;
        for (auto& c : norm)
        {
                if (c == '\\')
                        c = '/';
        }
        while (norm.size() > 1 && norm.back() == '/')
        {
                norm.pop_back();
        }
        return norm;
}

std::string Extension::JoinPath(const std::string& base, const std::string& relative) const
{
        if (relative.empty())
                return NormalizePath(base);

        if (relative.front() == '/' || relative.front() == '\\')
                return NormalizePath(relative);

        std::string joined = base;
        if (!joined.empty() && joined.back() != '/' && joined.back() != '\\')
                joined.push_back('/');
        joined += relative;
        return NormalizePath(joined);
}

std::string Extension::DirName(const std::string& path) const
{
        std::string norm = NormalizePath(path);
        size_t pos = norm.find_last_of('/');
        if (pos == std::string::npos)
                return "";
        return norm.substr(0, pos);
}

bool Extension::FileExists(const std::string& path) const
{
        std::ifstream f(path, std::ios::binary);
        return f.good();
}

bool Extension::ReadFileUTF8(const std::string& path, std::string& out) const
{
        std::ifstream file(path, std::ios::binary);
        if (!file)
                return false;

        std::ostringstream ss;
        ss << file.rdbuf();
        out = ss.str();
        return true;
}

std::string Extension::ExtractPackageMain(const std::string& json) const
{
        const std::string key = "\"main\"";
        size_t pos = json.find(key);
        if (pos == std::string::npos)
                return "";

        pos = json.find(':', pos + key.size());
        if (pos == std::string::npos)
                return "";

        pos = json.find('"', pos);
        if (pos == std::string::npos)
                return "";

        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos || end <= pos + 1)
                return "";

        return json.substr(pos + 1, end - pos - 1);
}

std::string Extension::ResolveAsFileOrDirectory(const std::string& path)
{
        std::string normalized = NormalizePath(path);
        if (FileExists(normalized))
                return normalized;

        if (FileExists(normalized + ".js"))
                return normalized + ".js";

        std::string packageJson = JoinPath(normalized, "package.json");
        if (FileExists(packageJson))
        {
                std::string json;
                if (ReadFileUTF8(packageJson, json))
                {
                        std::string mainEntry = ExtractPackageMain(json);
                        if (!mainEntry.empty())
                        {
                                std::string mainPath = ResolveAsFileOrDirectory(JoinPath(normalized, mainEntry));
                                if (!mainPath.empty())
                                        return mainPath;
                        }
                }
        }

        std::string indexPath = JoinPath(normalized, "index.js");
        if (FileExists(indexPath))
                return indexPath;

        return "";
}

std::string Extension::ResolveModulePath(const std::string& specifier, const std::string& parentDir)
{
        if (specifier.empty())
                return "";

        std::string baseDir = parentDir.empty() ? DarkEdif::TStringToUTF8(moduleRootPath) : parentDir;
        baseDir = NormalizePath(baseDir);

        bool isAbsolute = (specifier.size() > 1 && specifier[1] == ':') || specifier.front() == '/' || specifier.front() == '\\';
        bool isRelative = specifier.front() == '.';
        if (isAbsolute)
        {
                return ResolveAsFileOrDirectory(NormalizePath(specifier));
        }
        else if (isRelative)
        {
                return ResolveAsFileOrDirectory(JoinPath(baseDir, specifier));
        }

        std::string searchDir = baseDir;
        while (!searchDir.empty())
        {
                std::string candidate = ResolveAsFileOrDirectory(JoinPath(JoinPath(searchDir, "node_modules"), specifier));
                if (!candidate.empty())
                        return candidate;

                size_t slashPos = searchDir.find_last_of('/');
                if (slashPos == std::string::npos)
                        break;
                searchDir = searchDir.substr(0, slashPos);
        }

        return ResolveAsFileOrDirectory(JoinPath(DarkEdif::TStringToUTF8(moduleRootPath), specifier));
}

bool Extension::EnsureModuleCache(duk_context* ctx)
{
        duk_push_heap_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, "moduleCache"))
        {
                duk_pop(ctx);
                duk_push_object(ctx);
                duk_dup(ctx, -1);
                duk_put_prop_string(ctx, -3, "moduleCache");
        }
        duk_remove(ctx, -2);
        return true;
}

void Extension::PushRequireFunction(duk_context* ctx, const std::string& baseDir)
{
        duk_push_c_function(ctx, JS_Require, 1);
        duk_push_pointer(ctx, this);
        duk_put_prop_string(ctx, -2, "__fusion_ext");
        duk_push_string(ctx, baseDir.c_str());
        duk_put_prop_string(ctx, -2, "__fusion_basedir");
}

bool Extension::LoadModule(duk_context* ctx, const std::string& resolvedPath, const std::string& baseDir)
{
        if (!ctx)
                return false;

        duk_idx_t baseTop = duk_get_top(ctx);
        EnsureModuleCache(ctx);
        duk_idx_t cacheIdx = duk_get_top_index(ctx);

        duk_push_string(ctx, resolvedPath.c_str());
        if (duk_get_prop(ctx, cacheIdx))
        {
                duk_remove(ctx, cacheIdx);
                if (baseTop > 0)
                        duk_remove(ctx, 0);
                return true;
        }
        duk_pop(ctx);

        std::string source;
        if (!ReadFileUTF8(resolvedPath, source))
        {
                DarkEdif::MsgBox::Error(_T("JS Error"), (DarkEdif::UTF8ToTString("Failed to read file: ") + DarkEdif::UTF8ToTString(resolvedPath)).c_str());
                duk_set_top(ctx, baseTop);
                return false;
        }

        std::string wrapped = "(function(exports, require, module, __filename, __dirname){\n" + source + "\n})";
        if (duk_pcompile_lstring(ctx, DUK_COMPILE_FUNCTION, wrapped.c_str(), wrapped.size()) != 0)
        {
                const char* err = duk_safe_to_string(ctx, -1);
                DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
                duk_set_top(ctx, baseTop);
                return false;
        }

        duk_push_object(ctx); // exports
        PushRequireFunction(ctx, baseDir); // require
        duk_push_object(ctx); // module
        duk_dup(ctx, -3);     // exports -> module.exports
        duk_put_prop_string(ctx, -2, "exports");
        duk_push_string(ctx, resolvedPath.c_str());
        duk_put_prop_string(ctx, -2, "filename");
        duk_push_string(ctx, baseDir.c_str());
        duk_put_prop_string(ctx, -2, "dirname");
        duk_push_string(ctx, resolvedPath.c_str());
        duk_push_string(ctx, baseDir.c_str());

        // preserve exports for caching after call
        duk_dup(ctx, 2);
        duk_insert(ctx, 0);

        // stack: exportsCopy, cache, func, exports, require, module, filename, dirname
        if (duk_pcall(ctx, 5) != 0)
        {
                const char* err = duk_safe_to_string(ctx, -1);
                DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
                duk_pop_3(ctx); // error, cache, exportsCopy
                duk_set_top(ctx, baseTop);
                return false;
        }

        duk_pop(ctx); // pop return value
        duk_dup(ctx, 0); // exportsCopy
        duk_put_prop_string(ctx, 2, resolvedPath.c_str()); // cache at index 2 after pop?
        duk_remove(ctx, 2); // remove cache
        duk_set_top(ctx, 1); // keep exports only
        return true;
}

duk_ret_t Extension::JS_Require(duk_context* ctx)
{
        const char* spec = duk_require_string(ctx, 0);

        duk_push_current_function(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_ext");
        Extension* ext = (Extension*)duk_get_pointer(ctx, -1);
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "__fusion_basedir");
        std::string baseDir = duk_is_string(ctx, -1) ? duk_get_string(ctx, -1) : "";
        duk_pop_2(ctx);

        if (!ext)
                return duk_error(ctx, DUK_ERR_ERROR, "No extension bound to require().");

        std::string resolved = ext->ResolveModulePath(spec ? spec : "", baseDir);
        if (resolved.empty())
        {
                return duk_error(ctx, DUK_ERR_ERROR, "Cannot find module: %s", spec ? spec : "(null)");
        }

        if (!ext->LoadModule(ctx, resolved, ext->DirName(resolved)))
        {
                return duk_error(ctx, DUK_ERR_ERROR, "Failed to load module: %s", spec ? spec : "(null)");
        }

        return 1;
}

bool Extension::CacheObjectScript(int fixedValue, RunObjectMultiPlatPtr obj, int altStringIndex, bool runEveryFrame)
{
        if (!EnsureCurrentContext())
        {
                DarkEdif::MsgBox::Error(_T("JS Error"), _T("No active context to register script."));
                return false;
        }

        if (!obj || !obj->get_rov())
        {
                DarkEdif::MsgBox::Error(_T("JS Error"), _T("Could not resolve object for registration."));
                return false;
        }

        std::size_t stringCount = obj->get_rov()->GetAltStringCount();
        if (altStringIndex < 0 || (std::size_t)altStringIndex >= stringCount)
        {
                DarkEdif::MsgBox::Error(_T("JS Error"), _T("Alterable string index is out of range."));
                return false;
        }

        const TCHAR* rawScript = obj->get_rov()->GetAltStringAtIndex((std::size_t)altStringIndex);
        if (!rawScript || rawScript[0] == 0)
        {
                DarkEdif::MsgBox::Error(_T("JS Error"), _T("Alterable string is empty; nothing to register."));
                return false;
        }

        std::string scriptUTF8 = DarkEdif::TStringToUTF8(rawScript);
        std::string wrapper = "(function(meta){\n" + scriptUTF8 + "\n})";

        duk_context* ctx = (currentCtx >= 0 && currentCtx < (int)contexts.size()) ? contexts[currentCtx] : nullptr;
        if (!ctx)
                return false;
        if (duk_pcompile_lstring(ctx, DUK_COMPILE_FUNCTION, wrapper.c_str(), wrapper.size()) != 0)
        {
                const char* err = duk_safe_to_string(ctx, -1);
                DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
                duk_pop(ctx);
                return false;
        }

        std::string stashKey = "fusion_script_" + std::to_string(currentCtx) + "_" + std::to_string(fixedValue);

        duk_push_heap_stash(ctx);
        duk_dup(ctx, -2);
        duk_put_prop_string(ctx, -2, stashKey.c_str());
        duk_pop_2(ctx);

        auto existing = registeredScripts.find(fixedValue);
        if (existing != registeredScripts.end())
        {
                ClearScriptReference(existing->second);
        }

        ScriptInfo info;
        info.contextId = currentCtx;
        info.source = scriptUTF8;
        info.stashKey = stashKey;
        info.altStringIndex = altStringIndex;
        info.runEveryFrame = runEveryFrame;

        registeredScripts[fixedValue] = std::move(info);
        return true;
}

bool Extension::ExecuteObjectScript(int fixedValue, ScriptInfo& info, RunObjectMultiPlatPtr obj)
{
        if (info.contextId < 0 || info.contextId >= (int)contexts.size())
        {
            return false;
        }

        duk_context* ctx = contexts[info.contextId];
        if (!ctx)
                return false;
        duk_push_heap_stash(ctx);
        if (!duk_get_prop_string(ctx, -1, info.stashKey.c_str()))
        {
                duk_pop(ctx);
                return false;
        }

        duk_require_function(ctx, -1);
        duk_remove(ctx, -2); // remove stash, leave function

        HeaderObject* ho = obj ? obj->get_rHo() : nullptr;
        duk_push_object(ctx);
        duk_push_int(ctx, fixedValue);
        duk_put_prop_string(ctx, -2, "fixedValue");
        duk_push_int(ctx, info.altStringIndex);
        duk_put_prop_string(ctx, -2, "altStringIndex");
        duk_push_boolean(ctx, info.runEveryFrame);
        duk_put_prop_string(ctx, -2, "runEveryFrame");
        duk_push_int(ctx, info.contextId);
        duk_put_prop_string(ctx, -2, "contextId");
        if (ho)
        {
                duk_push_int(ctx, ho->X);
                duk_put_prop_string(ctx, -2, "x");
                duk_push_int(ctx, ho->Y);
                duk_put_prop_string(ctx, -2, "y");
        }

        if (duk_pcall(ctx, 1) != 0)
        {
                const char* err = duk_safe_to_string(ctx, -1);
                DarkEdif::MsgBox::Error(_T("JS Error"), DarkEdif::UTF8ToTString(err).c_str());
                duk_pop(ctx);
                return false;
        }

        duk_pop(ctx);
        return true;
}

void Extension::ClearScriptReference(const ScriptInfo& info)
{
        if (info.contextId < 0 || info.contextId >= (int)contexts.size() || info.stashKey.empty())
                return;

        duk_context* ctx = contexts[info.contextId];
        if (!ctx)
                return;
        duk_push_heap_stash(ctx);
        duk_del_prop_string(ctx, -1, info.stashKey.c_str());
        duk_pop(ctx);
}
