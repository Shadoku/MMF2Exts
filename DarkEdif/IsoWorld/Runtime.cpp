#include "Common.hpp"

REFLAG Extension::Handle()
{
	for (auto& [name, body] : map.bodies)
	{
		auto& state = bodyContacts[name];
		if (!state.onGround && body.onGround)
		{
			state.landingEvent = true;
			landingBody = name;
			Runtime.GenerateEvent(5);
			collisionStartBody = name;
			Runtime.GenerateEvent(2);
		}
		else if (state.onGround && !body.onGround)
		{
			state.leavingEvent = true;
			leavingBody = name;
			Runtime.GenerateEvent(6);
			collisionEndBody = name;
			Runtime.GenerateEvent(3);
		}
		state.onGround = body.onGround;
	}

	return REFLAG::ONE_SHOT;
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
	return 0;
}

