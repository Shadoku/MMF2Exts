#include "Common.hpp"

namespace
{
	std::string ToUTF8(const TCHAR* text)
	{
		return DarkEdif::TStringToUTF8(text ? text : _T(""));
	}
}

bool Extension::OnTMXLoadSuccess()
{
	const bool result = lastLoadSuccess;
	lastLoadSuccess = false;
	return result;
}

bool Extension::OnTMXLoadFailure()
{
	const bool result = lastLoadFailure;
	lastLoadFailure = false;
	return result;
}

bool Extension::OnCollisionStart(const TCHAR* bodyName)
{
	if (collisionStartBody.empty())
		return false;
	const auto match = collisionStartBody == ToUTF8(bodyName) || ToUTF8(bodyName).empty();
	if (match)
		collisionStartBody.clear();
	return match;
}

bool Extension::OnCollisionEnd(const TCHAR* bodyName)
{
	if (collisionEndBody.empty())
		return false;
	const auto match = collisionEndBody == ToUTF8(bodyName) || ToUTF8(bodyName).empty();
	if (match)
		collisionEndBody.clear();
	return match;
}

bool Extension::OnClearPath(const TCHAR* bodyName)
{
	if (clearPathBody.empty())
		return false;
	const bool match = clearPathBody == ToUTF8(bodyName) || ToUTF8(bodyName).empty();
	if (match)
		clearPathBody.clear();
	return match;
}

bool Extension::OnLanding(const TCHAR* bodyName)
{
	const auto name = ToUTF8(bodyName);
	if (name.empty())
	{
		for (auto& [id, state] : bodyContacts)
		{
			if (state.landingEvent)
			{
				state.landingEvent = false;
				return true;
			}
		}
		return false;
	}
	auto it = bodyContacts.find(name);
	if (it == bodyContacts.end() || !it->second.landingEvent)
		return false;
	it->second.landingEvent = false;
	return true;
}

bool Extension::OnLeavingGround(const TCHAR* bodyName)
{
	const auto name = ToUTF8(bodyName);
	if (name.empty())
	{
		for (auto& [id, state] : bodyContacts)
		{
			if (state.leavingEvent)
			{
				state.leavingEvent = false;
				return true;
			}
		}
		return false;
	}
	auto it = bodyContacts.find(name);
	if (it == bodyContacts.end() || !it->second.leavingEvent)
		return false;
	it->second.leavingEvent = false;
	return true;
}

bool Extension::OnTileTypeInRegion(const TCHAR* layerName, const TCHAR* terrain, int x1, int y1, int x2, int y2)
{
	const auto layer = ToUTF8(layerName);
	const auto terrainStr = ToUTF8(terrain);
	for (int y = y1; y <= y2; ++y)
	{
		for (int x = x1; x <= x2; ++x)
		{
			auto lookup = map.TileAt(layer, x, y);
			if (lookup.tile && lookup.tile->terrain == terrainStr)
				return true;
		}
	}
	return false;
}
