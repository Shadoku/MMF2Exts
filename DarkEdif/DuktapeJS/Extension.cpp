#include "Common.hpp"
#include <fstream>
#include <sstream>
#include <string_view>
#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
	duk_ret_t DukPrintBridge(duk_context* ctx)
	{
		std::ostringstream stream;
		const duk_idx_t argCount = duk_get_top(ctx);
		for (duk_idx_t i = 0; i < argCount; ++i)
		{
			if (i != 0)
				stream << ' ';
			const char* piece = duk_safe_to_string(ctx, i);
			if (piece)
				stream << piece;
		}

#ifdef _WIN32
		const std::string message = stream.str();
		OutputDebugStringA((message + "\n").c_str());
#else
		(void)stream;
#endif
		return 0;
	}
}

Extension::SharedContextState Extension::sharedContext {};

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
	rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this), FusionDebugger(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr) :
	javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
	Runtime(this, this->javaExtPtr), FusionDebugger(this)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr) :
	objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr), FusionDebugger(this)
#endif
{
	using namespace std::string_view_literals;

	LinkAction(0, LoadScriptFile);
	LinkAction(1, EvaluateString);
	LinkAction(2, ResetDuktapeContext);

	LinkCondition(0, LastCallWasSuccess);
	LinkCondition(1, LastCallWasFailure);

	LinkExpression(0, LastResultText);
	LinkExpression(1, LastErrorText);

	useIsolatedContext = edPtr->Props.IsPropChecked("Use isolated context"sv);
	scriptBaseDirectory = edPtr->Props.GetPropertyStr("Default script directory"sv);

	FusionDebugger.AddItemToDebugger(
		[](Extension* ext, std::tstring& writeTo) {
			if (!ext)
				return;
			if (!ext->lastError.empty())
				writeTo = _T("Error: ") + ext->lastError;
			else if (!ext->lastResult.empty())
				writeTo = _T("Result: ") + ext->lastResult;
			else
				writeTo = _T("Idle");
		},
		nullptr,
		500,
		nullptr);

	AcquireContext();
}

Extension::~Extension()
{
	ReleaseContext();
}

duk_context* Extension::CreateContext()
{
	return duk_create_heap(nullptr, nullptr, nullptr, nullptr, nullptr);
}

void Extension::InstallHelpers(duk_context* context)
{
	if (!context)
		return;

	duk_push_global_object(context);
	duk_push_c_function(context, DukPrintBridge, DUK_VARARGS);
	duk_put_prop_string(context, -2, "print");

	if (!duk_get_prop_string(context, -1, "console"))
	{
		duk_pop(context);
		duk_push_object(context);
	}

	duk_push_c_function(context, DukPrintBridge, DUK_VARARGS);
	duk_put_prop_string(context, -2, "log");
	duk_put_prop_string(context, -2, "console");
	duk_pop(context);
}

void Extension::AcquireContext()
{
	ReleaseContext();
	usingSharedContext = !useIsolatedContext;

	if (usingSharedContext)
	{
		if (!sharedContext.ctx)
		{
			sharedContext.ctx = CreateContext();
			sharedContext.helpersInstalled = false;
		}

		if (sharedContext.ctx)
		{
			ctx = sharedContext.ctx;
			++sharedContext.refCount;
			if (!sharedContext.helpersInstalled)
			{
				InstallHelpers(ctx);
				sharedContext.helpersInstalled = true;
			}
		}
	}
	else
	{
		ctx = CreateContext();
		if (ctx)
			InstallHelpers(ctx);
	}

	if (!ctx)
		SetLastError(_T("Failed to create Duktape context."));
}

void Extension::ReleaseContext()
{
	if (usingSharedContext)
	{
		if (ctx != nullptr && ctx == sharedContext.ctx && sharedContext.refCount > 0)
		{
			--sharedContext.refCount;
			if (sharedContext.refCount == 0)
			{
				duk_destroy_heap(sharedContext.ctx);
				sharedContext.ctx = nullptr;
				sharedContext.helpersInstalled = false;
			}
		}
		ctx = nullptr;
		return;
	}

	if (ctx)
	{
		duk_destroy_heap(ctx);
		ctx = nullptr;
	}
}

void Extension::ResetContext()
{
	if (usingSharedContext)
	{
		if (sharedContext.refCount > 1)
		{
			SetLastError(_T("Cannot reset shared context while other objects are using it."));
			return;
		}

		if (sharedContext.ctx)
			duk_destroy_heap(sharedContext.ctx);

		sharedContext.ctx = CreateContext();
		sharedContext.helpersInstalled = false;
		sharedContext.refCount = 0;
	}
	else if (ctx)
	{
		duk_destroy_heap(ctx);
		ctx = nullptr;
	}

	AcquireContext();
	ClearResults();
}

bool Extension::EnsureContext()
{
	if (ctx)
		return true;

	AcquireContext();
	return ctx != nullptr;
}

void Extension::ClearResults()
{
	lastError.clear();
	lastResult.clear();
	lastCallSucceeded = true;
}

void Extension::SetLastError(const std::tstring& message)
{
	lastCallSucceeded = false;
	lastError = message;
	lastResult.clear();
}

bool Extension::EvaluateBuffer(const std::string& buffer, const std::tstring& debugName)
{
	if (!EnsureContext())
		return false;

	ClearResults();
	if (buffer.empty())
	{
		SetLastError(_T("Script buffer is empty."));
		return false;
	}

	const int evalResult = duk_peval_lstring(ctx, buffer.data(), buffer.size());
	if (evalResult != 0)
	{
		const char* errorText = duk_safe_to_string(ctx, -1);
		lastError = DarkEdif::UTF8ToTString(errorText ? std::string_view(errorText) : std::string_view());
		lastCallSucceeded = false;
		duk_pop(ctx);
		return false;
	}

	size_t resultLength = 0;
	const char* resultText = duk_safe_to_lstring(ctx, -1, &resultLength);
	if (resultText && resultLength > 0)
		lastResult = DarkEdif::UTF8ToTString(std::string_view(resultText, resultLength));
	else
		lastResult.clear();

	lastCallSucceeded = true;
	duk_pop(ctx);
	(void)debugName;
	return true;
}

bool Extension::EvaluateScriptString(const std::tstring& code)
{
	const std::string utf8 = DarkEdif::TStringToUTF8(code);
	return EvaluateBuffer(utf8, _T("inline code"));
}

std::tstring Extension::ResolveScriptPath(const std::tstring& rawPath) const
{
	if (rawPath.empty())
		return rawPath;

	const bool hasDrive = rawPath.size() > 1 && rawPath[1] == _T(':');
	const bool isUNC = !rawPath.empty() && (rawPath[0] == _T('\\') || rawPath[0] == _T('/'));
	if (hasDrive || isUNC || scriptBaseDirectory.empty())
		return rawPath;

	std::tstring combined = scriptBaseDirectory;
	if (!combined.empty() && combined.back() != _T('\\') && combined.back() != _T('/'))
		combined += _T("\\");
	combined += rawPath;
	return combined;
}

bool Extension::EvaluateScriptFile(const std::tstring& path)
{
	const std::tstring resolvedPath = ResolveScriptPath(path);
	if (resolvedPath.empty())
	{
		SetLastError(_T("No script path provided."));
		return false;
	}

	std::ifstream scriptFile(DarkEdif::TStringToWide(resolvedPath).c_str(), std::ios::binary);
	if (!scriptFile)
	{
		SetLastError(_T("Unable to open script file."));
		return false;
	}

	std::string buffer((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());
	return EvaluateBuffer(buffer, resolvedPath);
}

// Runs every tick of Fusion's runtime, can be toggled off and back on
REFLAG Extension::Handle()
{
	return REFLAG::ONE_SHOT;
}
