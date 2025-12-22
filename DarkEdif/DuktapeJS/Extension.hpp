#pragma once
#include "DarkEdif.hpp"
#include "../../duktape-2.7.0/src/duktape.h"

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

	struct SharedContextState
	{
		duk_context* ctx = nullptr;
		int refCount = 0;
		bool helpersInstalled = false;
	};

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

	// To add items to the Fusion Debugger, just uncomment this line.
	DarkEdif::FusionDebugger FusionDebugger;
	// After enabling it, you run FusionDebugger.AddItemToDebugger() inside Extension's constructor

	// Context handling
	static SharedContextState sharedContext;
	duk_context* ctx = nullptr;
	bool usingSharedContext = true;
	bool useIsolatedContext = false;

	std::tstring scriptBaseDirectory;
	std::tstring lastError;
	std::tstring lastResult;
	bool lastCallSucceeded = true;

	duk_context* CreateContext();
	void InstallHelpers(duk_context* context);
	void AcquireContext();
	void ReleaseContext();
	void ResetContext();
	bool EnsureContext();

	bool EvaluateBuffer(const std::string& buffer, const std::tstring& debugName);
	bool EvaluateScriptString(const std::tstring& code);
	bool EvaluateScriptFile(const std::tstring& path);
	std::tstring ResolveScriptPath(const std::tstring& rawPath) const;
	void ClearResults();
	void SetLastError(const std::tstring& message);

	// Actions

	void LoadScriptFile(const TCHAR* path);
	void EvaluateString(const TCHAR* code);
	void ResetDuktapeContext();

	// Conditions

	bool LastCallWasSuccess();
	bool LastCallWasFailure();

	// Expressions

	const TCHAR* LastResultText();
	const TCHAR* LastErrorText();

	// Runs every tick of Fusion's runtime, can be toggled off and back on
	REFLAG Handle();

#if TEXT_OEFLAG_EXTENSION
	// Extension text struct. Required for text exts.
	DarkEdif::FontInfoMultiPlat font;
	void OnFontChanged(bool colorEdit, DarkEdif::Rect* rc);
#endif
#if DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_SIMPLE
	// Extension display surface ptr. Required for simple display exts.
	DarkEdif::Surface * surf = nullptr;
#elif DARKEDIF_DISPLAY_TYPE == DARKEDIF_DISPLAY_MANUAL
	void Display();
	void GetZoneInfos();
	DarkEdif::Surface * GetDisplaySurface();
	DarkEdif::CollisionMask * GetCollisionMask(std::uint32_t flags);
#endif

	// These are called if there's no function linked to an ID
	void UnlinkedAction(int ID);
	long UnlinkedCondition(int ID);
	long UnlinkedExpression(int ID);

#if PAUSABLE_EXTENSION
	// Called when Fusion runtime is pausing - not just the F3 pause dialog
	void FusionRuntimePaused();
	// Called when Fusion runtime is resuming after a pause
	void FusionRuntimeContinued();
#endif // PAUSABLE_EXTENSION
};
