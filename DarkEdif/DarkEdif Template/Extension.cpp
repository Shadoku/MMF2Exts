#include "Common.hpp"
#include <cmath>

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
        LinkAction(0, ResetContext);
        LinkAction(1, RunJavaScript);
        LinkAction(2, CallFunctionExpression);
        LinkAction(3, RebuildMMFI);
        LinkAction(4, ProbeClasses);

	LinkCondition(0, OnJavaScriptError);

	LinkExpression(0, LastResult);
	LinkExpression(1, LastNumber);
	LinkExpression(2, LastError);
        LinkExpression(3, ClassSupport);
        LinkExpression(4, CurrentFrameIndex);
        LinkExpression(5, ObjectCount);
        LinkExpression(6, LastResultType);
        LinkExpression(7, LastBoolean);

	exposeMMFIOnStart = edPtr->Props.IsPropChecked(_T("Expose MMFI on start"));
	probeClassesOnStart = edPtr->Props.IsPropChecked(_T("Probe class support on start"));
	bootstrapCode = edPtr->Props.GetPropertyStr(_T("Bootstrap code"));

	FusionDebugger.AddItemToDebugger(
		[](Extension* ext, std::tstring& writeTo) {
			writeTo = _T("Last JS result: ") + ext->lastResultText;
		},
		nullptr, 500, nullptr);

	FusionDebugger.AddItemToDebugger(
		[](Extension* ext, std::tstring& writeTo) {
			writeTo = ext->lastError.empty() ? _T("No errors") : (_T("Error: ") + ext->lastError);
		},
		nullptr, 500, nullptr);

	InitialiseContext();
}

Extension::~Extension()
{
}

REFLAG Extension::Handle()
{
	return REFLAG::ONE_SHOT;
}

void Extension::UnlinkedAction(int ID)
{
	DarkEdif::MsgBox::Error(_T("Action %i not linked!"), ID);
}

long Extension::UnlinkedCondition(int ID)
{
	DarkEdif::MsgBox::Error(_T("Condition %i not linked!"), ID);
	return false;
}

long Extension::UnlinkedExpression(int ID)
{
	DarkEdif::MsgBox::Error(_T("Expression %i not linked!"), ID);
	return 0;
}

void Extension::InitialiseContext()
{
        lastError.clear();
        lastResultText.clear();
        hasNumericResult = false;
        lastResultBoolean = false;
        lastResultKind = ResultKind::None;
        pendingErrorEvent = false;
        classSyntaxSupported = false;

	dukCtx.reset(duk_create_heap_default());
	if (!dukCtx)
	{
		RecordError(_T("Failed to create Duktape heap."), true);
		return;
	}

	duk_push_heap_stash(dukCtx.get());
	duk_push_pointer(dukCtx.get(), this);
	duk_put_prop_string(dukCtx.get(), -2, "ext_ptr");
	duk_pop(dukCtx.get());

	if (exposeMMFIOnStart)
		RegisterMMFIHelpers();

	if (probeClassesOnStart)
		ProbeClassSupport();

	if (!bootstrapCode.empty())
		EvalJavaScript(bootstrapCode, false);
}

void Extension::RegisterMMFIHelpers()
{
        if (!dukCtx)
                return;

        duk_push_global_object(dukCtx.get());
        duk_push_object(dukCtx.get());

        auto defineGetter = [](duk_context* ctx, const char* name, duk_c_function getter)
        {
                duk_push_string(ctx, name);
                duk_push_c_function(ctx, getter, 0);
                duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE);
        };

        duk_push_object(dukCtx.get());
        duk_push_c_function(dukCtx.get(), DukRuntimeCurrentFrame, 0);
        duk_put_prop_string(dukCtx.get(), -2, "currentFrame");
        duk_push_c_function(dukCtx.get(), DukRuntimeObjectCount, 0);
        duk_put_prop_string(dukCtx.get(), -2, "objectCount");
        duk_push_c_function(dukCtx.get(), DukRuntimeTriggerEvent, 1);
        duk_put_prop_string(dukCtx.get(), -2, "triggerEvent");
        duk_put_prop_string(dukCtx.get(), -2, "runtime");

        duk_push_object(dukCtx.get());
        duk_push_boolean(dukCtx.get(), 1);
        duk_put_prop_string(dukCtx.get(), -2, "available");
        defineGetter(dukCtx.get(), "xLeft", DukFrameXLeft);
        defineGetter(dukCtx.get(), "xRight", DukFrameXRight);
        defineGetter(dukCtx.get(), "yTop", DukFrameYTop);
        defineGetter(dukCtx.get(), "yBottom", DukFrameYBottom);
        defineGetter(dukCtx.get(), "width", DukFrameWidth);
        defineGetter(dukCtx.get(), "height", DukFrameHeight);
        defineGetter(dukCtx.get(), "virtualWidth", DukFrameVirtualWidth);
        defineGetter(dukCtx.get(), "virtualHeight", DukFrameVirtualHeight);
        duk_push_c_function(dukCtx.get(), DukFrameTestPoint, DUK_VARARGS);
        duk_put_prop_string(dukCtx.get(), -2, "testPoint");
        duk_push_c_function(dukCtx.get(), DukFrameTestRect, DUK_VARARGS);
        duk_put_prop_string(dukCtx.get(), -2, "testRect");
        duk_put_prop_string(dukCtx.get(), -2, "frame");

        duk_push_object(dukCtx.get());
        duk_push_boolean(dukCtx.get(), 1);
        duk_put_prop_string(dukCtx.get(), -2, "available");
        duk_push_c_function(dukCtx.get(), DukKeyboardKeyDown, 1);
        duk_put_prop_string(dukCtx.get(), -2, "keyDown");
        duk_push_c_function(dukCtx.get(), DukKeyboardKeyUp, 1);
        duk_put_prop_string(dukCtx.get(), -2, "keyUp");
        duk_put_prop_string(dukCtx.get(), -2, "keyboard");

        duk_push_object(dukCtx.get());
        duk_push_boolean(dukCtx.get(), 1);
        duk_put_prop_string(dukCtx.get(), -2, "available");
        defineGetter(dukCtx.get(), "x", DukMouseX);
        defineGetter(dukCtx.get(), "y", DukMouseY);
        defineGetter(dukCtx.get(), "clientX", DukMouseClientX);
        defineGetter(dukCtx.get(), "clientY", DukMouseClientY);
        defineGetter(dukCtx.get(), "wheelDelta", DukMouseWheelDelta);
        duk_push_c_function(dukCtx.get(), DukMouseButtonDown, 1);
        duk_put_prop_string(dukCtx.get(), -2, "buttonDown");
        duk_push_c_function(dukCtx.get(), DukMouseButtonUp, 1);
        duk_put_prop_string(dukCtx.get(), -2, "buttonUp");
        duk_put_prop_string(dukCtx.get(), -2, "mouse");

        duk_push_object(dukCtx.get());
        duk_push_boolean(dukCtx.get(), 1);
        duk_put_prop_string(dukCtx.get(), -2, "available");
        defineGetter(dukCtx.get(), "width", DukWindowWidth);
        defineGetter(dukCtx.get(), "height", DukWindowHeight);
        defineGetter(dukCtx.get(), "clientWidth", DukWindowClientWidth);
        defineGetter(dukCtx.get(), "clientHeight", DukWindowClientHeight);
        defineGetter(dukCtx.get(), "frameWidth", DukWindowFrameWidth);
        defineGetter(dukCtx.get(), "frameHeight", DukWindowFrameHeight);
        duk_put_prop_string(dukCtx.get(), -2, "window");

        duk_push_c_function(dukCtx.get(), DukMMFIUnsupported, DUK_VARARGS);
        duk_set_magic(dukCtx.get(), -1, 4);
        duk_put_prop_string(dukCtx.get(), -2, "newObject");

        duk_push_c_function(dukCtx.get(), DukMMFIUnsupported, DUK_VARARGS);
        duk_set_magic(dukCtx.get(), -1, 5);
        duk_put_prop_string(dukCtx.get(), -2, "newObjectClass");

#ifdef _WIN32
        duk_push_int(dukCtx.get(), VK_LEFT);
        duk_put_prop_string(dukCtx.get(), -2, "VK_LEFT");
        duk_push_int(dukCtx.get(), VK_RIGHT);
        duk_put_prop_string(dukCtx.get(), -2, "VK_RIGHT");
        duk_push_int(dukCtx.get(), VK_UP);
        duk_put_prop_string(dukCtx.get(), -2, "VK_UP");
        duk_push_int(dukCtx.get(), VK_DOWN);
        duk_put_prop_string(dukCtx.get(), -2, "VK_DOWN");
        duk_push_int(dukCtx.get(), VK_SPACE);
        duk_put_prop_string(dukCtx.get(), -2, "VK_SPACE");
        duk_push_int(dukCtx.get(), VK_RETURN);
        duk_put_prop_string(dukCtx.get(), -2, "VK_RETURN");
        duk_push_int(dukCtx.get(), VK_ESCAPE);
        duk_put_prop_string(dukCtx.get(), -2, "VK_ESCAPE");
        duk_push_int(dukCtx.get(), VK_SHIFT);
        duk_put_prop_string(dukCtx.get(), -2, "VK_SHIFT");
        duk_push_int(dukCtx.get(), VK_CONTROL);
        duk_put_prop_string(dukCtx.get(), -2, "VK_CONTROL");
        duk_push_int(dukCtx.get(), VK_MENU);
        duk_put_prop_string(dukCtx.get(), -2, "VK_ALT");
#endif

        duk_push_string(dukCtx.get(), "Duktape 2.7.0");
        duk_put_prop_string(dukCtx.get(), -2, "interpreter");

        duk_push_string(dukCtx.get(), "MMFI runtime subset");
        duk_put_prop_string(dukCtx.get(), -2, "api");

        duk_put_prop_string(dukCtx.get(), -2, "mmf");
        duk_pop(dukCtx.get());
}

bool Extension::EvalJavaScript(const std::tstring_view code, bool triggerEvents)
{
	if (!dukCtx)
		InitialiseContext();
	if (!dukCtx)
		return false;

        lastError.clear();
        lastResultText.clear();
        hasNumericResult = false;
        lastResultBoolean = false;
        lastResultKind = ResultKind::None;

        const std::string utf8 = DarkEdif::TStringToUTF8(code);
        duk_int_t evalResult = duk_peval_lstring(dukCtx.get(), utf8.c_str(), utf8.size());
        if (evalResult != 0)
        {
                std::tstring err = DarkEdif::UTF8ToTString(duk_safe_to_string(dukCtx.get(), -1));

                if (duk_is_object(dukCtx.get(), -1))
                {
                        if (duk_get_prop_string(dukCtx.get(), -1, "stack"))
                        {
                                std::tstring stack = DarkEdif::UTF8ToTString(duk_safe_to_string(dukCtx.get(), -1));
                                if (!stack.empty())
                                        err += _T("\n") + stack;
                        }
                        duk_pop(dukCtx.get());
                }
                duk_pop(dukCtx.get());
                RecordError(err, triggerEvents);
                return false;
        }

        if (duk_is_number(dukCtx.get(), -1))
        {
                lastResultNumber = duk_get_number(dukCtx.get(), -1);
                hasNumericResult = true;
                lastResultKind = ResultKind::Number;
        }
        else if (duk_is_boolean(dukCtx.get(), -1))
        {
                lastResultBoolean = duk_get_boolean(dukCtx.get(), -1) != 0;
                lastResultKind = ResultKind::Boolean;
        }
        else if (duk_is_string(dukCtx.get(), -1))
        {
                lastResultKind = ResultKind::String;
        }
        else
        {
                lastResultKind = ResultKind::None;
        }
        lastResultText = DarkEdif::UTF8ToTString(duk_safe_to_string(dukCtx.get(), -1));
        duk_pop(dukCtx.get());
        return true;
}

void Extension::RecordError(const std::tstring& message, bool triggerEvents)
{
	lastError = message;
	pendingErrorEvent = true;
	if (triggerEvents)
		Runtime.GenerateEvent(0);
}

void Extension::ProbeClassSupport()
{
        if (!dukCtx)
                return;
	const char* probeScript =
		"class __DarkDuktapeProbe { constructor(v){this.v=v;} value(){return this.v;} }"
		"; (new __DarkDuktapeProbe(1234)).value();";
	if (duk_peval_string(dukCtx.get(), probeScript) == 0)
	{
		if (duk_is_number(dukCtx.get(), -1) && std::fabs(duk_get_number(dukCtx.get(), -1) - 1234.0) < 0.001)
			classSyntaxSupported = true;
	}
        duk_pop(dukCtx.get());
}

void Extension::ProbeClasses()
{
        if (!dukCtx)
                InitialiseContext();

        if (!dukCtx)
                return;

        ProbeClassSupport();
}

Extension* Extension::FromCtx(duk_context* ctx)
{
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, "ext_ptr");
	auto* ext = static_cast<Extension*>(duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);
	return ext;
}

duk_ret_t Extension::DukRuntimeCurrentFrame(duk_context* ctx)
{
	auto* ext = FromCtx(ctx);
	duk_push_int(ctx, ext ? ext->Runtime.GetCurrentFusionFrameNumber() : 0);
	return 1;
}

duk_ret_t Extension::DukRuntimeObjectCount(duk_context* ctx)
{
	auto* ext = FromCtx(ctx);
	duk_push_int(ctx, (ext && ext->rhPtr) ? static_cast<int>(ext->rhPtr->get_NObjects()) : 0);
	return 1;
}

duk_ret_t Extension::DukRuntimeTriggerEvent(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext)
                return 0;
        duk_int_t eventId = duk_is_number(ctx, 0) ? duk_get_int(ctx, 0) : 0;
        ext->Runtime.GenerateEvent(static_cast<int>(eventId));
        return 0;
}

duk_ret_t Extension::DukFrameXLeft(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh3.rh3DisplayX);
        return 1;
}

duk_ret_t Extension::DukFrameXRight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh3.rh3DisplayX + ext->rhPtr->rh3.rh3WindowSx);
        return 1;
}

duk_ret_t Extension::DukFrameYTop(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh3.rh3DisplayY);
        return 1;
}

duk_ret_t Extension::DukFrameYBottom(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh3.rh3DisplayY + ext->rhPtr->rh3.rh3WindowSy);
        return 1;
}

duk_ret_t Extension::DukFrameWidth(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr || !ext->rhPtr->rhFrame)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rhFrame->m_hdr.leWidth);
        return 1;
}

duk_ret_t Extension::DukFrameHeight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr || !ext->rhPtr->rhFrame)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rhFrame->m_hdr.leHeight);
        return 1;
}

duk_ret_t Extension::DukFrameVirtualWidth(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rhLevelSx);
        return 1;
}

duk_ret_t Extension::DukFrameVirtualHeight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rhLevelSy);
        return 1;
}

duk_ret_t Extension::DukFrameTestPoint(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        const int nargs = duk_get_top(ctx);
        if (nargs < 2 || !duk_is_number(ctx, 0) || !duk_is_number(ctx, 1))
                return 0;
        const int x = duk_get_int(ctx, 0);
        const int y = duk_get_int(ctx, 1);
        const bool insideX = x >= ext->rhPtr->rh3.rh3DisplayX && x <= (ext->rhPtr->rh3.rh3DisplayX + ext->rhPtr->rh3.rh3WindowSx);
        const bool insideY = y >= ext->rhPtr->rh3.rh3DisplayY && y <= (ext->rhPtr->rh3.rh3DisplayY + ext->rhPtr->rh3.rh3WindowSy);
        duk_push_boolean(ctx, insideX && insideY);
        return 1;
}

duk_ret_t Extension::DukFrameTestRect(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        const int nargs = duk_get_top(ctx);
        if (nargs < 4 || !duk_is_number(ctx, 0) || !duk_is_number(ctx, 1) || !duk_is_number(ctx, 2) || !duk_is_number(ctx, 3))
                return 0;
        const int x = duk_get_int(ctx, 0);
        const int y = duk_get_int(ctx, 1);
        const int w = duk_get_int(ctx, 2);
        const int h = duk_get_int(ctx, 3);
        const int left = ext->rhPtr->rh3.rh3DisplayX;
        const int top = ext->rhPtr->rh3.rh3DisplayY;
        const int right = left + ext->rhPtr->rh3.rh3WindowSx;
        const int bottom = top + ext->rhPtr->rh3.rh3WindowSy;
        const bool intersects = (x < right && (x + w) > left && y < bottom && (y + h) > top);
        duk_push_boolean(ctx, intersects);
        return 1;
}

duk_ret_t Extension::DukKeyboardKeyDown(duk_context* ctx)
{
        const duk_int_t key = duk_is_number(ctx, 0) ? duk_get_int(ctx, 0) : 0;
#ifdef _WIN32
        const short state = GetAsyncKeyState(static_cast<int>(key));
        duk_push_boolean(ctx, (state & 0x8000) != 0);
#else
        (void)key;
        duk_push_boolean(ctx, 0);
#endif
        return 1;
}

duk_ret_t Extension::DukKeyboardKeyUp(duk_context* ctx)
{
        const duk_int_t key = duk_is_number(ctx, 0) ? duk_get_int(ctx, 0) : 0;
#ifdef _WIN32
        const short state = GetAsyncKeyState(static_cast<int>(key));
        duk_push_boolean(ctx, (state & 0x8000) == 0);
#else
        (void)key;
        duk_push_boolean(ctx, 1);
#endif
        return 1;
}

duk_ret_t Extension::DukMouseX(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh2.rh2Mouse.x);
        return 1;
}

duk_ret_t Extension::DukMouseY(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh2.rh2Mouse.y);
        return 1;
}

duk_ret_t Extension::DukMouseClientX(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh2.rh2MouseClient.x);
        return 1;
}

duk_ret_t Extension::DukMouseClientY(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh2.rh2MouseClient.y);
        return 1;
}

duk_ret_t Extension::DukMouseWheelDelta(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
        if (!ext || !ext->rhPtr)
                return 0;
        duk_push_int(ctx, ext->rhPtr->rh4.rh4MouseWheelDelta);
        return 1;
}

duk_ret_t Extension::DukMouseButtonDown(duk_context* ctx)
{
        const duk_int_t button = duk_is_number(ctx, 0) ? duk_get_int(ctx, 0) : 0;
#ifdef _WIN32
        short state = 0;
        switch (button)
        {
        case 1: state = GetAsyncKeyState(VK_LBUTTON); break;
        case 2: state = GetAsyncKeyState(VK_MBUTTON); break;
        case 3: state = GetAsyncKeyState(VK_RBUTTON); break;
        case 4: state = GetAsyncKeyState(VK_XBUTTON1); break;
        case 5: state = GetAsyncKeyState(VK_XBUTTON2); break;
        default: state = 0; break;
        }
        duk_push_boolean(ctx, (state & 0x8000) != 0);
#else
        (void)button;
        duk_push_boolean(ctx, 0);
#endif
        return 1;
}

duk_ret_t Extension::DukMouseButtonUp(duk_context* ctx)
{
        const duk_int_t button = duk_is_number(ctx, 0) ? duk_get_int(ctx, 0) : 0;
#ifdef _WIN32
        short state = 0;
        switch (button)
        {
        case 1: state = GetAsyncKeyState(VK_LBUTTON); break;
        case 2: state = GetAsyncKeyState(VK_MBUTTON); break;
        case 3: state = GetAsyncKeyState(VK_RBUTTON); break;
        case 4: state = GetAsyncKeyState(VK_XBUTTON1); break;
        case 5: state = GetAsyncKeyState(VK_XBUTTON2); break;
        default: state = 0; break;
        }
        duk_push_boolean(ctx, (state & 0x8000) == 0);
#else
        (void)button;
        duk_push_boolean(ctx, 1);
#endif
        return 1;
}

duk_ret_t Extension::DukWindowWidth(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetWindowRect(ext->rhPtr->rhHMainWin, &rc);
        duk_push_int(ctx, rc.right - rc.left);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukWindowHeight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetWindowRect(ext->rhPtr->rhHMainWin, &rc);
        duk_push_int(ctx, rc.bottom - rc.top);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukWindowClientWidth(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetClientRect(ext->rhPtr->rhHMainWin, &rc);
        duk_push_int(ctx, rc.right - rc.left);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukWindowClientHeight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetClientRect(ext->rhPtr->rhHMainWin, &rc);
        duk_push_int(ctx, rc.bottom - rc.top);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukWindowFrameWidth(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetWindowRect(ext->rhPtr->rhHEditWin, &rc);
        duk_push_int(ctx, rc.right - rc.left);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukWindowFrameHeight(duk_context* ctx)
{
        auto* ext = FromCtx(ctx);
#ifdef _WIN32
        if (!ext || !ext->rhPtr)
                return 0;
        RECT rc{};
        GetWindowRect(ext->rhPtr->rhHEditWin, &rc);
        duk_push_int(ctx, rc.bottom - rc.top);
        return 1;
#else
        (void)ext;
        return 0;
#endif
}

duk_ret_t Extension::DukMMFIUnsupported(duk_context* ctx)
{
        const duk_int_t magic = duk_get_current_magic(ctx);
        const char* feature = "mmf feature";

        switch (magic)
        {
        case 0: feature = "mmf.frame"; break;
        case 1: feature = "mmf.keyboard"; break;
        case 2: feature = "mmf.mouse"; break;
        case 3: feature = "mmf.window"; break;
        case 4: feature = "mmf.newObject"; break;
        case 5: feature = "mmf.newObjectClass"; break;
        default: break;
        }

        duk_error(ctx, DUK_ERR_ERROR, "%s is not implemented in the Duktape port of MMFI yet.", feature);
        return DUK_RET_ERROR;
}
