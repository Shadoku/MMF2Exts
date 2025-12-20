#include "Common.hpp"

bool Extension::LastCallWasSuccess()
{
	return lastCallSucceeded;
}

bool Extension::LastCallWasFailure()
{
	return !lastCallSucceeded;
}
