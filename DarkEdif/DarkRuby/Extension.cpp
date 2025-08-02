#include "Common.hpp"

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
    LinkAction(0, RunRubyScript);
    LinkAction(1, SetGlobalNumber);
    LinkAction(2, SetGlobalString);

    LinkCondition(0, DummyCondition);

    LinkExpression(0, EvalRuby);
    LinkExpression(1, GetGlobalNumber);
    LinkExpression(2, GetGlobalString);

    FusionDebugger.AddItemToDebugger(
        [](Extension *ext, std::tstring &writeTo) {
            writeTo = _T("My text is: ") + ext->exampleDebuggerTextItem;
        },
        [](Extension *ext, std::tstring &newText)
        {
            ext->exampleDebuggerTextItem = newText;
            return true;
        }, 500, NULL
    );

    engine = std::make_unique<EmbersEngine>();

    bool checkboxWithinFolder = edPtr->Props.IsPropChecked("Checkbox within folder"sv);
    std::tstring editable6Text = edPtr->Props.GetPropertyStr("Editable 6"sv);
    (void)checkboxWithinFolder;
    (void)editable6Text;
}

Extension::~Extension()
{
    engine.reset();
}

REFLAG Extension::Handle()
{
    return REFLAG::ONE_SHOT;
}

REFLAG Extension::Display()
{
    return REFLAG::DISPLAY;
}

short Extension::FusionRuntimePaused()
{
    return 0;
}

short Extension::FusionRuntimeContinued()
{
    return 0;
}

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
    if ((size_t)ID < Edif::SDK->ExpressionInfos.size() && Edif::SDK->ExpressionInfos[ID]->Flags.ef == ExpReturnType::String)
        return (long)Runtime.CopyString(_T(""));
    return 0;
}
