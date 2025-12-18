#include "Common.hpp"

namespace
{
	std::string ToUTF8(const TCHAR* text)
	{
		return DarkEdif::TStringToUTF8(text ? text : _T(""));
	}
}

double Extension::WorldToScreenX(double worldX, double worldY, double worldZ)
{
	const auto screen = map.WorldToScreen(worldX, worldY, worldZ, map.layers.empty() ? nullptr : &map.layers.front());
	return screen.x;
}

double Extension::WorldToScreenY(double worldX, double worldY, double worldZ)
{
	const auto screen = map.WorldToScreen(worldX, worldY, worldZ, map.layers.empty() ? nullptr : &map.layers.front());
	return screen.y;
}

double Extension::ScreenToTileQ(double screenX, double screenY)
{
	const auto tile = map.ScreenToTile(screenX, screenY, map.layers.empty() ? nullptr : &map.layers.front());
	return tile.x;
}

double Extension::ScreenToTileR(double screenX, double screenY)
{
	const auto tile = map.ScreenToTile(screenX, screenY, map.layers.empty() ? nullptr : &map.layers.front());
	return tile.y;
}

double Extension::TileHeightAt(const TCHAR* layerName, int x, int y)
{
	return map.TileHeightAt(ToUTF8(layerName), x, y);
}

double Extension::SurfaceNormalX(const TCHAR* layerName, int x, int y)
{
	return map.SurfaceNormalAt(ToUTF8(layerName), x, y).x;
}

double Extension::SurfaceNormalY(const TCHAR* layerName, int x, int y)
{
	return map.SurfaceNormalAt(ToUTF8(layerName), x, y).y;
}

double Extension::SurfaceNormalZ(const TCHAR* layerName, int x, int y)
{
	return map.SurfaceNormalAt(ToUTF8(layerName), x, y).z;
}

double Extension::RaycastDistance(double x1, double y1, double z1, double x2, double y2, double z2)
{
	double distance = -1.0;
	if (map.Raycast({ x1, y1, z1 }, { x2, y2, z2 }, distance))
		return distance;
	return -1.0;
}

double Extension::DepthSortKey(double screenX, double screenY, double z)
{
	return map.DepthSortKey(screenX, screenY, z);
}

double Extension::BodyContactFlags(const TCHAR* name)
{
	const auto id = ToUTF8(name);
	const auto it = map.bodies.find(id);
	if (it == map.bodies.end())
		return 0.0;
	int flags = 0;
	if (it->second.onGround)
		flags |= 1;
	if (it->second.ccdEnabled)
		flags |= 2;
	return static_cast<double>(flags);
}

const TCHAR* Extension::TileProperty(const TCHAR* layerName, int x, int y, const TCHAR* key)
{
	auto lookup = map.TileAt(ToUTF8(layerName), x, y);
	if (lookup.tile)
	{
		const auto propKey = ToUTF8(key);
		const auto it = lookup.tile->properties.find(propKey);
		if (it != lookup.tile->properties.end())
		{
			textScratch = DarkEdif::UTF8ToTString(it->second);
			return textScratch.c_str();
		}
	}
	textScratch.clear();
	return textScratch.c_str();
}

const TCHAR* Extension::TilesetProperty(std::uint32_t gid, const TCHAR* key)
{
	const auto searchKey = ToUTF8(key);
	for (auto it = map.tilesets.rbegin(); it != map.tilesets.rend(); ++it)
	{
		if (gid < it->firstGid)
			continue;
		const auto localId = gid - it->firstGid;
		const auto tileIt = it->tiles.find(localId);
		if (tileIt == it->tiles.end())
			continue;
		const auto propIt = tileIt->second.properties.find(searchKey);
		if (propIt != tileIt->second.properties.end())
		{
			textScratch = DarkEdif::UTF8ToTString(propIt->second);
			return textScratch.c_str();
		}
	}
	textScratch.clear();
	return textScratch.c_str();
}

double Extension::LayerParallaxX(const TCHAR* layerName)
{
	const auto name = ToUTF8(layerName);
	for (const auto& layer : map.layers)
	{
		if (layer.name == name)
			return layer.parallaxX;
	}
	return 1.0;
}

double Extension::LayerParallaxY(const TCHAR* layerName)
{
	const auto name = ToUTF8(layerName);
	for (const auto& layer : map.layers)
	{
		if (layer.name == name)
			return layer.parallaxY;
	}
	return 1.0;
}

double Extension::BodyX(const TCHAR* name)
{
	const auto id = ToUTF8(name);
	const auto it = map.bodies.find(id);
	if (it == map.bodies.end())
		return 0.0;
	return it->second.position.x;
}

double Extension::BodyY(const TCHAR* name)
{
	const auto id = ToUTF8(name);
	const auto it = map.bodies.find(id);
	if (it == map.bodies.end())
		return 0.0;
	return it->second.position.y;
}

double Extension::BodyZ(const TCHAR* name)
{
	const auto id = ToUTF8(name);
	const auto it = map.bodies.find(id);
	if (it == map.bodies.end())
		return 0.0;
	return it->second.position.z;
}

