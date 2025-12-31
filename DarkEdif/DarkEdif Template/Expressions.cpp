#include "Common.hpp"

const TCHAR * Extension::LastResult()
{
	return Runtime.CopyString(lastResultText.c_str());
}

double Extension::LastNumber()
{
	return hasNumericResult ? lastResultNumber : 0.0;
}

const TCHAR * Extension::LastError()
{
	return Runtime.CopyString(lastError.c_str());
}

int Extension::ClassSupport()
{
	return classSyntaxSupported ? 1 : 0;
}

int Extension::CurrentFrameIndex()
{
	return Runtime.GetCurrentFusionFrameNumber();
}

int Extension::ObjectCount()
{
        return rhPtr ? static_cast<int>(rhPtr->get_NObjects()) : 0;
}

int Extension::LastResultType()
{
        return static_cast<int>(lastResultKind);
}

int Extension::LastBoolean()
{
        return (lastResultKind == ResultKind::Boolean && lastResultBoolean) ? 1 : 0;
}
