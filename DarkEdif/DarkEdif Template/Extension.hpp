#pragma once
#include "DarkEdif.hpp"
#include "duktape.h"
#include <memory>

class Extension final
{
public:
	// ======================================
	// Required variables + functions
	// Variables here must not be moved or swapped around or it can cause future issues
	// ======================================
	RunHeader* rhPtr;
	RunObjectMultiPlatPtr rdPtr;
#ifdef __ANDROID__
	global<jobject> javaExtPtr;
#elif defined(__APPLE__)
	void* const objCExtPtr;
#endif

	Edif::Runtime Runtime;

	static const int MinimumBuild = 254;
	static const int Version = 1;

	// Warning: OEFLAGS/OEPREFS cannot be freely modified when you have used them in MFAs.
	static constexpr OEFLAGS OEFLAGS = OEFLAGS::NONE;
	static constexpr OEPREFS OEPREFS = OEPREFS::NONE;
	// If OEFLAGS::WINDOW_PROC (otherwise you can delete)
	// static constexpr int WindowProcPriority = 100;
	// If OEFLAGS::TEXT (otherwise you can delete)
	// static constexpr TextCapacity TextCapacity = TextCapacity::None;

#ifdef _WIN32
	Extension(RunObject* const rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr);
#elif defined(__ANDROID__)
	Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr);
#else
	Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr);
#endif
	~Extension();

	// ======================================
	// Extension data
	// ======================================

	DarkEdif::FusionDebugger FusionDebugger;

	// Persistent scripting state
	using DukContextPtr = std::unique_ptr<duk_context, decltype(&duk_destroy_heap)>;
	DukContextPtr dukCtx { nullptr, &duk_destroy_heap };
	bool pendingErrorEvent = false;
	bool classSyntaxSupported = false;
	bool exposeMMFIOnStart = true;
	bool probeClassesOnStart = true;
	std::tstring bootstrapCode;
        std::tstring lastError;
        std::tstring lastResultText;
        double lastResultNumber = 0.0;
        bool hasNumericResult = false;
        bool lastResultBoolean = false;
        enum class ResultKind
        {
                None = 0,
                Number,
                Boolean,
                String
        };
        ResultKind lastResultKind = ResultKind::None;

	// Actions
	void ResetContext();
	void RunJavaScript(const TCHAR * code);
	void CallFunctionExpression(const TCHAR * callText);
        void RebuildMMFI();
        void ProbeClasses();

	// Conditions
	bool OnJavaScriptError();

	// Expressions
	const TCHAR * LastResult();
	double LastNumber();
        const TCHAR * LastError();
        int ClassSupport();
        int CurrentFrameIndex();
        int ObjectCount();
        int LastResultType();
        int LastBoolean();

	REFLAG Handle();

	void UnlinkedAction(int ID);
	long UnlinkedCondition(int ID);
	long UnlinkedExpression(int ID);

	void InitialiseContext();
        void RegisterMMFIHelpers();
        bool EvalJavaScript(const std::tstring_view code, bool triggerEvents = true);
        void RecordError(const std::tstring & message, bool triggerEvents);
        void ProbeClassSupport();
        static Extension * FromCtx(duk_context * ctx);
        static duk_ret_t DukRuntimeCurrentFrame(duk_context * ctx);
        static duk_ret_t DukRuntimeObjectCount(duk_context * ctx);
        static duk_ret_t DukRuntimeTriggerEvent(duk_context * ctx);
        static duk_ret_t DukFrameXLeft(duk_context * ctx);
        static duk_ret_t DukFrameXRight(duk_context * ctx);
        static duk_ret_t DukFrameYTop(duk_context * ctx);
        static duk_ret_t DukFrameYBottom(duk_context * ctx);
        static duk_ret_t DukFrameWidth(duk_context * ctx);
        static duk_ret_t DukFrameHeight(duk_context * ctx);
        static duk_ret_t DukFrameVirtualWidth(duk_context * ctx);
        static duk_ret_t DukFrameVirtualHeight(duk_context * ctx);
        static duk_ret_t DukFrameTestPoint(duk_context * ctx);
        static duk_ret_t DukFrameTestRect(duk_context * ctx);
        static duk_ret_t DukKeyboardKeyDown(duk_context * ctx);
        static duk_ret_t DukKeyboardKeyUp(duk_context * ctx);
        static duk_ret_t DukMouseX(duk_context * ctx);
        static duk_ret_t DukMouseY(duk_context * ctx);
        static duk_ret_t DukMouseClientX(duk_context * ctx);
        static duk_ret_t DukMouseClientY(duk_context * ctx);
        static duk_ret_t DukMouseWheelDelta(duk_context * ctx);
        static duk_ret_t DukMouseButtonDown(duk_context * ctx);
        static duk_ret_t DukMouseButtonUp(duk_context * ctx);
        static duk_ret_t DukWindowWidth(duk_context * ctx);
        static duk_ret_t DukWindowHeight(duk_context * ctx);
        static duk_ret_t DukWindowClientWidth(duk_context * ctx);
        static duk_ret_t DukWindowClientHeight(duk_context * ctx);
        static duk_ret_t DukWindowFrameWidth(duk_context * ctx);
        static duk_ret_t DukWindowFrameHeight(duk_context * ctx);
        static duk_ret_t DukMMFIUnsupported(duk_context * ctx);
};
