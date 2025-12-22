#include "Common.hpp"

void Extension::LoadScriptFile(const TCHAR* path)
{
	EvaluateScriptFile(path ? path : _T(""));
}

void Extension::EvaluateString(const TCHAR* code)
{
	EvaluateScriptString(code ? code : _T(""));
}

void Extension::ResetDuktapeContext()
{
	ResetContext();
}
