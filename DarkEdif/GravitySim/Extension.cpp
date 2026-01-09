#include "Common.hpp"
#include <cmath>

///
/// EXTENSION CONSTRUCTOR/DESTRUCTOR
///

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
        rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr) :
        javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
        Runtime(this, this->javaExtPtr)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr) :
        objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr)
#endif
{
        LinkAction(0, AddBody);
        LinkAction(1, SetMass);
        LinkAction(2, SetVelocity);
        LinkAction(3, StepSimulation);
        LinkAction(4, ClearBodies);
        LinkAction(5, SetGravitationalConstant);
        LinkAction(6, TrackActive);
        LinkAction(7, UntrackActive);

        LinkCondition(0, BodyExists);

        LinkExpression(0, BodyX);
        LinkExpression(1, BodyY);
        LinkExpression(2, BodyVX);
        LinkExpression(3, BodyVY);
        LinkExpression(4, BodyCount);
        LinkExpression(5, LastAddedBody);
        LinkExpression(6, GravitationalConstant);

        gravitationalConstant = edPtr->Props.GetPropertyNum(_T("Gravitational Constant"));
        if (std::isnan(gravitationalConstant) || gravitationalConstant == 0.0)
                gravitationalConstant = 1.0;

        lastAddedId = 0;
}

Extension::~Extension()
{

}

bool Extension::IsValidId(int id) const
{
        return id > 0 && static_cast<size_t>(id) <= bodies.size();
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

	// Will not be called next loop
	return REFLAG::ONE_SHOT;
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
