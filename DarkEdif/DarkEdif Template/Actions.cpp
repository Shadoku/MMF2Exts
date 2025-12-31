#include "Common.hpp"

void Extension::ResetContext()
{
	dukCtx.reset();
	InitialiseContext();
}

void Extension::RunJavaScript(const TCHAR * code)
{
	if (!code)
		return;
	EvalJavaScript(code);
}

void Extension::CallFunctionExpression(const TCHAR * callText)
{
	if (!callText)
		return;
	std::tstring wrapped = _T("(function(){ return ");
	wrapped += callText;
	wrapped += _T("; })();");
	EvalJavaScript(wrapped);
}

void Extension::RebuildMMFI()
{
        if (!dukCtx)
                InitialiseContext();
        else
                RegisterMMFIHelpers();
}

void Extension::ProbeClasses()
{
        if (!dukCtx)
                InitialiseContext();

        if (!dukCtx)
                return;

        ProbeClassSupport();
}
