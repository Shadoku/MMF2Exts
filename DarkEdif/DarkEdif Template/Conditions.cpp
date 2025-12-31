#include "Common.hpp"

bool Extension::OnJavaScriptError()
{
	if (!pendingErrorEvent)
		return false;
	pendingErrorEvent = false;
	return true;
}
