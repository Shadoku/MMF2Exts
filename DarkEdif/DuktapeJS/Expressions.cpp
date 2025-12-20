#include "Common.hpp"

const TCHAR* Extension::LastResultText()
{
	return Runtime.CopyString(lastResult.c_str());
}

const TCHAR* Extension::LastErrorText()
{
	return Runtime.CopyString(lastError.c_str());
}
