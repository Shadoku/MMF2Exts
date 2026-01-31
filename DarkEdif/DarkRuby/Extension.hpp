#pragma once
#include <memory>
#include "DarkEdif.hpp"
#include "EmbersWrapper.hpp"

class Extension
{
public:

    RunHeader* rhPtr;
    RunObjectMultiPlatPtr rdPtr;
#ifdef __ANDROID__
    global<jobject> javaExtPtr;
#elif defined(__APPLE__)
    void* const objCExtPtr;
#endif

    Edif::Runtime Runtime;
    std::unique_ptr<EmbersEngine> engine;

    static const int MinimumBuild = 254;
    static const int Version = 1;

    static const OEFLAGS OEFLAGS = OEFLAGS::NONE;
    static const OEPREFS OEPREFS = OEPREFS::NONE;

    static const int WindowProcPriority = 100;

#ifdef _WIN32
    Extension(RunObject* const rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr);
#elif defined(__ANDROID__)
    Extension(const EDITDATA* const edPtr, const jobject javaExtPtr);
#else
    Extension(const EDITDATA* const edPtr, void* const objCExtPtr);
#endif
    ~Extension();

    DarkEdif::FusionDebugger FusionDebugger;
    std::tstring exampleDebuggerTextItem;

    /// Actions
    void RunRubyScript(const TCHAR* ScriptText);
    void SetGlobalNumber(const TCHAR* Name, float Value);
    void SetGlobalString(const TCHAR* Name, const TCHAR* Value);

    /// Conditions
    bool DummyCondition();

    /// Expressions
    const TCHAR* EvalRuby(const TCHAR* ExpressionText);
    float GetGlobalNumber(const TCHAR* Name);
    const TCHAR* GetGlobalString(const TCHAR* Name);

    void UnlinkedAction(int ID);
    long UnlinkedCondition(int ID);
    long UnlinkedExpression(int ID);

    REFLAG Handle();
    REFLAG Display();

    short FusionRuntimePaused();
    short FusionRuntimeContinued();
};
