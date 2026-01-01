#include "Common.hpp"

namespace
{
	std::string ToUTF8(const TCHAR* text)
	{
		if (text == nullptr)
			return {};
		return DarkEdif::TStringToUTF8(text);
	}
}

void Extension::LoadTMX(const TCHAR* path)
{
	std::string error;
	const auto fsPath = std::filesystem::path(ToUTF8(path));

	lastLoadSuccess = map.LoadTMX(fsPath, error);
	lastLoadFailure = !lastLoadSuccess;
	if (lastLoadSuccess)
	{
		bodyContacts.clear();
		Runtime.GenerateEvent(0);
	}
	else
	{
		textScratch = DarkEdif::UTF8ToTString(error);
		Runtime.GenerateEvent(1);
	}
}

void Extension::LoadJSONCache(const TCHAR* path)
{
	std::string error;
	const auto fsPath = std::filesystem::path(ToUTF8(path));
	lastLoadSuccess = map.LoadJSON(fsPath, error);
	lastLoadFailure = !lastLoadSuccess;
	if (lastLoadSuccess)
	{
		bodyContacts.clear();
		Runtime.GenerateEvent(0);
	}
	else
	{
		textScratch = DarkEdif::UTF8ToTString(error);
		Runtime.GenerateEvent(1);
	}
}

void Extension::SaveJSONCache(const TCHAR* path)
{
	std::string error;
	const auto fsPath = std::filesystem::path(ToUTF8(path));
	if (!map.SaveJSON(fsPath, error))
		textScratch = DarkEdif::UTF8ToTString(error);
}

void Extension::ExportTMXLayer(const TCHAR* path, const TCHAR* layerName)
{
	std::string error;
	const auto fsPath = std::filesystem::path(ToUTF8(path));
	const auto layer = ToUTF8(layerName);
	if (!map.ExportTMXLayer(fsPath, layer, error))
		textScratch = DarkEdif::UTF8ToTString(error);
}

void Extension::SetTileHeights(const TCHAR* layerName, int x, int y, int tl, int tr, int bl, int br)
{
	auto lookup = map.TileAt(ToUTF8(layerName), x, y);
	if (lookup.tile != nullptr)
	{
		lookup.tile->heights = { static_cast<float>(tl), static_cast<float>(tr), static_cast<float>(bl), static_cast<float>(br) };
	}
}

void Extension::SetTileMask(const TCHAR* layerName, int x, int y, int maskId)
{
	auto lookup = map.TileAt(ToUTF8(layerName), x, y);
	if (lookup.tile != nullptr)
		lookup.tile->collisionMaskId = static_cast<std::uint32_t>(maskId);
}

void Extension::MoveBody(const TCHAR* name, float dx, float dy, float dz)
{
	const auto bodyName = ToUTF8(name);
	const Iso::Vec3 delta { dx, dy, dz };
	const bool clear = map.MoveBody(bodyName, delta);

	auto& state = bodyContacts[bodyName];
	const bool nowGrounded = map.bodies.count(bodyName) ? map.bodies[bodyName].onGround : false;
	if (!state.onGround && nowGrounded)
	{
		state.landingEvent = true;
		Runtime.GenerateEvent(5);
	}
	else if (state.onGround && !nowGrounded)
	{
		state.leavingEvent = true;
		Runtime.GenerateEvent(6);
		collisionEndBody = bodyName;
		Runtime.GenerateEvent(3);
	}

	state.onGround = nowGrounded;

	if (!clear)
	{
		collisionStartBody = bodyName;
		Runtime.GenerateEvent(2);
	}
	else
	{
		clearPathBody = bodyName;
		Runtime.GenerateEvent(4);
	}
}

void Extension::TeleportIfClear(const TCHAR* name, float x, float y, float z)
{
	const auto bodyName = ToUTF8(name);
	if (map.TeleportIfClear(bodyName, Iso::Vec3 { x, y, z }))
	{
		clearPathBody = bodyName;
		Runtime.GenerateEvent(4);
	}
	else
	{
		collisionStartBody = bodyName;
		Runtime.GenerateEvent(2);
	}
}

void Extension::ClearChunkCache()
{
	map.layers.clear();
	map.bodies.clear();
	map.collisionMasks.clear();
	bodyContacts.clear();
	collisionStartBody.clear();
	collisionEndBody.clear();
	clearPathBody.clear();
	landingBody.clear();
	leavingBody.clear();
}

void Extension::SpawnBodiesFromLayer(const TCHAR* layerName)
{
	map.SpawnBodiesFromLayer(ToUTF8(layerName));
}

void Extension::EnableCCD(const TCHAR* name, bool enabled)
{
	const auto bodyName = ToUTF8(name);
	const auto it = map.bodies.find(bodyName);
	if (it != map.bodies.end())
		it->second.ccdEnabled = enabled;
}

void Extension::SetBodyPhysics(const TCHAR* name, float gravity, float friction, float bounce)
{
	const auto bodyName = ToUTF8(name);
	if (!bodyName.empty())
	{
		auto it = map.bodies.find(bodyName);
		if (it != map.bodies.end())
		{
			it->second.properties["gravity"] = std::to_string(gravity);
			it->second.properties["friction"] = std::to_string(friction);
			it->second.properties["bounce"] = std::to_string(bounce);
		}
	}
	else
	{
		map.gravity = gravity;
		map.friction = friction;
		map.bounce = bounce;
	}
}

void Extension::SetDebugOverlay(bool enabled)
{
	debugOverlay = enabled;
}
