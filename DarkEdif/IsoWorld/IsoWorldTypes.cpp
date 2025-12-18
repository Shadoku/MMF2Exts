#include "IsoWorldTypes.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>

using nlohmann::json;

namespace
{
	std::string ToLower(std::string_view text)
	{
		std::string out(text);
		std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return out;
	}

	std::string ProjectionToString(Iso::Projection projection)
	{
		switch (projection)
		{
			case Iso::Projection::Orthogonal: return "orthogonal";
			case Iso::Projection::Isometric: return "isometric";
			case Iso::Projection::Staggered: return "staggered";
			case Iso::Projection::Hexagonal: return "hexagonal";
		}
		return "orthogonal";
	}

	Iso::Projection ParseProjection(std::string_view value)
	{
		const auto val = ToLower(value);
		if (val == "isometric")
			return Iso::Projection::Isometric;
		if (val == "staggered")
			return Iso::Projection::Staggered;
		if (val == "hexagonal")
			return Iso::Projection::Hexagonal;
		return Iso::Projection::Orthogonal;
	}

	double ParseDoubleAttr(const pugi::xml_node& node, const char* name, double fallback = 0.0)
	{
		if (auto attr = node.attribute(name); attr)
			return attr.as_double(fallback);
		return fallback;
	}

	int ParseIntAttr(const pugi::xml_node& node, const char* name, int fallback = 0)
	{
		if (auto attr = node.attribute(name); attr)
			return attr.as_int(fallback);
		return fallback;
	}
}

namespace Iso
{
	bool IsoMap::LoadTMX(const std::filesystem::path& filePath, std::string& errorMsg)
	{
		pugi::xml_document doc;
		const auto result = doc.load_file(filePath.c_str());
		if (!result)
		{
			errorMsg = "Failed to parse TMX: " + std::string(result.description());
			return false;
		}

		const auto mapNode = doc.child("map");
		if (!mapNode)
		{
			errorMsg = "TMX missing map node.";
			return false;
		}

		mapPath = filePath;
		projection = ParseProjection(mapNode.attribute("orientation").as_string("orthogonal"));
		tileWidth = ParseIntAttr(mapNode, "tilewidth");
		tileHeight = ParseIntAttr(mapNode, "tileheight");
		mapWidth = ParseIntAttr(mapNode, "width");
		mapHeight = ParseIntAttr(mapNode, "height");
		hexSideLength = ParseIntAttr(mapNode, "hexsidelength");
		chunkWidth = ParseIntAttr(mapNode, "chunkwidth", chunkWidth);
		chunkHeight = ParseIntAttr(mapNode, "chunkheight", chunkHeight);
		staggerAxis = mapNode.attribute("staggeraxis").as_string();
		staggerIndex = mapNode.attribute("staggerindex").as_string();

		layers.clear();
		bodies.clear();
		collisionMasks.clear();
		tilesets.clear();

		if (!ParseTilesets(mapNode, errorMsg))
			return false;
		if (!ParseTileLayers(mapNode, errorMsg))
			return false;
		if (!ParseObjectLayers(mapNode, errorMsg))
			return false;

		return true;
	}

	bool IsoMap::LoadJSON(const std::filesystem::path& filePath, std::string& errorMsg)
	{
		std::ifstream in(filePath);
		if (!in)
		{
			errorMsg = "Failed to open JSON cache.";
			return false;
		}

		json j;
		try
		{
			in >> j;
		}
		catch (const std::exception& e)
		{
			errorMsg = std::string("Invalid cache JSON: ") + e.what();
			return false;
		}

		projection = ParseProjection(j.value("projection", "orthogonal"));
		tileWidth = j.value("tileWidth", tileWidth);
		tileHeight = j.value("tileHeight", tileHeight);
		mapWidth = j.value("mapWidth", mapWidth);
		mapHeight = j.value("mapHeight", mapHeight);
		chunkWidth = j.value("chunkWidth", chunkWidth);
		chunkHeight = j.value("chunkHeight", chunkHeight);
		originX = j.value("originX", originX);
		originY = j.value("originY", originY);
		gravity = j.value("gravity", gravity);
		friction = j.value("friction", friction);
		bounce = j.value("bounce", bounce);
		hexSideLength = j.value("hexSideLength", hexSideLength);
		staggerAxis = j.value("staggerAxis", staggerAxis);
		staggerIndex = j.value("staggerIndex", staggerIndex);

		layers.clear();
		collisionMasks.clear();
		bodies.clear();
		tilesets.clear();

		if (j.contains("layers"))
		{
			for (const auto& layerJson : j["layers"])
			{
				IsoLayer layer;
				layer.name = layerJson.value("name", "");
				layer.id = layerJson.value("id", -1);
				layer.visible = layerJson.value("visible", true);
				layer.isObjectLayer = layerJson.value("isObjectLayer", false);
				layer.offsetX = layerJson.value("offsetX", 0.0);
				layer.offsetY = layerJson.value("offsetY", 0.0);
				layer.parallaxX = layerJson.value("parallaxX", 1.0);
				layer.parallaxY = layerJson.value("parallaxY", 1.0);

				if (!layer.isObjectLayer && layerJson.contains("chunks"))
				{
					for (const auto& chunkJson : layerJson["chunks"])
					{
						IsoChunk chunk;
						chunk.originX = chunkJson.value("originX", 0);
						chunk.originY = chunkJson.value("originY", 0);
						chunk.width = chunkJson.value("width", 0);
						chunk.height = chunkJson.value("height", 0);
						const auto tilesJson = chunkJson["tiles"];
						chunk.tiles.reserve(tilesJson.size());
						for (const auto& tileJson : tilesJson)
						{
							IsoTile tile;
							tile.gid = tileJson.value("gid", 0);
							const auto heightsJson = tileJson["heights"];
							for (std::size_t i = 0; i < tile.heights.size() && i < heightsJson.size(); ++i)
								tile.heights[i] = heightsJson[i].get<float>();
							if (tileJson.contains("collisionMaskId"))
								tile.collisionMaskId = tileJson["collisionMaskId"].get<std::uint32_t>();
							if (tileJson.contains("properties"))
								tile.properties = tileJson["properties"].get<std::unordered_map<std::string, std::string>>();
							tile.terrain = tileJson.value("terrain", "");
							chunk.tiles.emplace_back(std::move(tile));
						}
						layer.chunks.emplace_back(std::move(chunk));
					}
				}
				else if (layer.isObjectLayer && layerJson.contains("objects"))
				{
					for (const auto& objJson : layerJson["objects"])
					{
						IsoBody body;
						body.name = objJson.value("name", "");
						body.position.x = objJson.value("x", 0.0);
						body.position.y = objJson.value("y", 0.0);
						body.position.z = objJson.value("z", 0.0);
						body.size.x = objJson.value("w", 0.0);
						body.size.y = objJson.value("h", 0.0);
						body.size.z = objJson.value("d", 0.0);
						body.triggerOnly = objJson.value("triggerOnly", false);
						body.ccdEnabled = objJson.value("ccd", false);
						body.collisionGroup = objJson.value("group", 0);
						if (objJson.contains("properties"))
							body.properties = objJson["properties"].get<std::unordered_map<std::string, std::string>>();
						layer.objects.emplace_back(std::move(body));
					}
				}

				layers.emplace_back(std::move(layer));
			}
		}

		if (j.contains("collisionMasks"))
		{
			for (const auto& [key, maskJson] : j["collisionMasks"].items())
			{
				IsoCollisionMask mask;
				const auto type = maskJson.value("type", "none");
				if (type == "rectangle")
					mask.type = CollisionMaskType::Rectangle;
				else if (type == "ellipse")
					mask.type = CollisionMaskType::Ellipse;
				else if (type == "polygon")
					mask.type = CollisionMaskType::Polygon;
				mask.width = maskJson.value("width", 0.0);
				mask.height = maskJson.value("height", 0.0);
				if (maskJson.contains("polygon"))
				{
					for (const auto& ptJson : maskJson["polygon"])
					{
						Vec2 pt;
						pt.x = ptJson.value("x", 0.0);
						pt.y = ptJson.value("y", 0.0);
						mask.polygon.emplace_back(pt);
					}
				}
				collisionMasks[static_cast<std::uint32_t>(std::stoul(key))] = std::move(mask);
			}
		}

		if (j.contains("bodies"))
		{
			for (const auto& [name, bodyJson] : j["bodies"].items())
			{
				IsoBody body;
				body.name = name;
				body.position.x = bodyJson.value("x", 0.0);
				body.position.y = bodyJson.value("y", 0.0);
				body.position.z = bodyJson.value("z", 0.0);
				body.size.x = bodyJson.value("w", 0.0);
				body.size.y = bodyJson.value("h", 0.0);
				body.size.z = bodyJson.value("d", 0.0);
				body.velocity.x = bodyJson.value("vx", 0.0);
				body.velocity.y = bodyJson.value("vy", 0.0);
				body.velocity.z = bodyJson.value("vz", 0.0);
				body.triggerOnly = bodyJson.value("triggerOnly", false);
				body.ccdEnabled = bodyJson.value("ccd", false);
				body.collisionGroup = bodyJson.value("group", 0);
				if (bodyJson.contains("properties"))
					body.properties = bodyJson["properties"].get<std::unordered_map<std::string, std::string>>();
				bodies[body.name] = std::move(body);
			}
		}

		return true;
	}

	bool IsoMap::SaveJSON(const std::filesystem::path& filePath, std::string& errorMsg) const
	{
		json j;
		j["projection"] = ProjectionToString(projection);
		j["tileWidth"] = tileWidth;
		j["tileHeight"] = tileHeight;
		j["mapWidth"] = mapWidth;
		j["mapHeight"] = mapHeight;
		j["chunkWidth"] = chunkWidth;
		j["chunkHeight"] = chunkHeight;
		j["originX"] = originX;
		j["originY"] = originY;
		j["gravity"] = gravity;
		j["friction"] = friction;
		j["bounce"] = bounce;
		j["hexSideLength"] = hexSideLength;
		j["staggerAxis"] = staggerAxis;
		j["staggerIndex"] = staggerIndex;

		for (const auto& layer : layers)
		{
			json layerJson;
			layerJson["name"] = layer.name;
			layerJson["id"] = layer.id;
			layerJson["visible"] = layer.visible;
			layerJson["isObjectLayer"] = layer.isObjectLayer;
			layerJson["offsetX"] = layer.offsetX;
			layerJson["offsetY"] = layer.offsetY;
			layerJson["parallaxX"] = layer.parallaxX;
			layerJson["parallaxY"] = layer.parallaxY;

			if (!layer.isObjectLayer)
			{
				layerJson["chunks"] = json::array();
				for (const auto& chunk : layer.chunks)
				{
					json chunkJson;
					chunkJson["originX"] = chunk.originX;
					chunkJson["originY"] = chunk.originY;
					chunkJson["width"] = chunk.width;
					chunkJson["height"] = chunk.height;
					json tilesJson = json::array();
					for (const auto& tile : chunk.tiles)
					{
						json tileJson;
						tileJson["gid"] = tile.gid;
						tileJson["heights"] = tile.heights;
						tileJson["terrain"] = tile.terrain;
						if (tile.collisionMaskId.has_value())
							tileJson["collisionMaskId"] = tile.collisionMaskId.value();
						tileJson["properties"] = tile.properties;
						tilesJson.emplace_back(tileJson);
					}
					chunkJson["tiles"] = tilesJson;
					layerJson["chunks"].emplace_back(chunkJson);
				}
			}
			else
			{
				json objectsJson = json::array();
				for (const auto& obj : layer.objects)
				{
					json objJson;
					objJson["name"] = obj.name;
					objJson["x"] = obj.position.x;
					objJson["y"] = obj.position.y;
					objJson["z"] = obj.position.z;
					objJson["w"] = obj.size.x;
					objJson["h"] = obj.size.y;
					objJson["d"] = obj.size.z;
					objJson["triggerOnly"] = obj.triggerOnly;
					objJson["ccd"] = obj.ccdEnabled;
					objJson["group"] = obj.collisionGroup;
					objJson["properties"] = obj.properties;
					objectsJson.emplace_back(objJson);
				}
				layerJson["objects"] = objectsJson;
			}

			j["layers"].emplace_back(layerJson);
		}

		for (const auto& [id, mask] : collisionMasks)
		{
			json maskJson;
			switch (mask.type)
			{
				case CollisionMaskType::Rectangle: maskJson["type"] = "rectangle"; break;
				case CollisionMaskType::Ellipse: maskJson["type"] = "ellipse"; break;
				case CollisionMaskType::Polygon: maskJson["type"] = "polygon"; break;
				default: maskJson["type"] = "none"; break;
			}
			maskJson["width"] = mask.width;
			maskJson["height"] = mask.height;
			json poly = json::array();
			for (const auto& pt : mask.polygon)
			{
				json ptJson;
				ptJson["x"] = pt.x;
				ptJson["y"] = pt.y;
				poly.emplace_back(ptJson);
			}
			maskJson["polygon"] = poly;
			j["collisionMasks"][std::to_string(id)] = maskJson;
		}

		for (const auto& [name, body] : bodies)
		{
			json objJson;
			objJson["x"] = body.position.x;
			objJson["y"] = body.position.y;
			objJson["z"] = body.position.z;
			objJson["w"] = body.size.x;
			objJson["h"] = body.size.y;
			objJson["d"] = body.size.z;
			objJson["vx"] = body.velocity.x;
			objJson["vy"] = body.velocity.y;
			objJson["vz"] = body.velocity.z;
			objJson["triggerOnly"] = body.triggerOnly;
			objJson["ccd"] = body.ccdEnabled;
			objJson["group"] = body.collisionGroup;
			objJson["properties"] = body.properties;
			j["bodies"][name] = objJson;
		}

		std::ofstream out(filePath);
		if (!out)
		{
			errorMsg = "Failed to open output JSON.";
			return false;
		}
		out << j.dump(2);
		return true;
	}

	bool IsoMap::ExportTMXLayer(const std::filesystem::path& filePath, const std::string& layerName, std::string& errorMsg) const
	{
		const IsoLayer* layer = FindLayer(layerName);
		if (!layer)
		{
			errorMsg = "Layer not found for export: " + layerName;
			return false;
		}

		pugi::xml_document doc;
		auto mapNode = doc.append_child("map");
		mapNode.append_attribute("orientation").set_value(ProjectionToString(projection).c_str());
		mapNode.append_attribute("tilewidth").set_value(tileWidth);
		mapNode.append_attribute("tileheight").set_value(tileHeight);
		mapNode.append_attribute("renderorder").set_value("right-down");
		mapNode.append_attribute("width").set_value(mapWidth);
		mapNode.append_attribute("height").set_value(mapHeight);

		auto layerNode = mapNode.append_child("layer");
		layerNode.append_attribute("name").set_value(layer->name.c_str());
		layerNode.append_attribute("width").set_value(mapWidth);
		layerNode.append_attribute("height").set_value(mapHeight);
		auto dataNode = layerNode.append_child("data");
		dataNode.append_attribute("encoding").set_value("csv");

		std::ostringstream csv;
		for (int y = 0; y < mapHeight; ++y)
		{
			for (int x = 0; x < mapWidth; ++x)
			{
				const IsoTile* tile = nullptr;
				for (const auto& chunk : layer->chunks)
				{
					if (x < chunk.originX || y < chunk.originY || x >= chunk.originX + chunk.width || y >= chunk.originY + chunk.height)
						continue;
					const auto idx = static_cast<std::size_t>((y - chunk.originY) * chunk.width + (x - chunk.originX));
					if (idx < chunk.tiles.size())
						tile = &chunk.tiles[idx];
					break;
				}
				csv << (tile ? tile->gid : 0);
				if (!(x == mapWidth - 1 && y == mapHeight - 1))
					csv << ",";
			}
			if (y != mapHeight - 1)
				csv << "\n";
		}
		dataNode.append_child(pugi::node_pcdata).set_value(csv.str().c_str());

		if (!doc.save_file(filePath.c_str(), "  ", pugi::format_default, pugi::encoding_utf8))
		{
			errorMsg = "Failed to write TMX to disk.";
			return false;
		}
		return true;
	}

	TileLookup IsoMap::TileAt(const std::string& layerName, int x, int y)
	{
		TileLookup result;
		auto* layer = FindLayer(layerName);
		if (!layer)
			return result;
		for (auto& chunk : layer->chunks)
		{
			if (x < chunk.originX || y < chunk.originY || x >= chunk.originX + chunk.width || y >= chunk.originY + chunk.height)
				continue;
			const auto idx = chunk.Index(x - chunk.originX, y - chunk.originY);
			if (idx < chunk.tiles.size())
			{
				result.layer = layer;
				result.tile = &chunk.tiles[idx];
				return result;
			}
		}
		return result;
	}

	double IsoMap::TileHeightAt(const std::string& layerName, int x, int y) const
	{
		const auto* layer = FindLayer(layerName);
		if (!layer)
			return 0.0;
		for (const auto& chunk : layer->chunks)
		{
			if (x < chunk.originX || y < chunk.originY || x >= chunk.originX + chunk.width || y >= chunk.originY + chunk.height)
				continue;
			const auto idx = static_cast<std::size_t>((y - chunk.originY) * chunk.width + (x - chunk.originX));
			if (idx < chunk.tiles.size())
			{
				const auto& heights = chunk.tiles[idx].heights;
				return (heights[0] + heights[1] + heights[2] + heights[3]) / 4.0;
			}
		}
		return 0.0;
	}

	Vec3 IsoMap::SurfaceNormalAt(const std::string& layerName, int x, int y) const
	{
		const auto* layer = FindLayer(layerName);
		if (!layer)
			return { 0, 0, 1 };
		for (const auto& chunk : layer->chunks)
		{
			if (x < chunk.originX || y < chunk.originY || x >= chunk.originX + chunk.width || y >= chunk.originY + chunk.height)
				continue;
			const auto idx = static_cast<std::size_t>((y - chunk.originY) * chunk.width + (x - chunk.originX));
			if (idx < chunk.tiles.size())
			{
				const auto& h = chunk.tiles[idx].heights;
				const Vec3 p0 { 0, 0, h[0] };
				const Vec3 p1 { static_cast<double>(tileWidth), 0.0, h[1] };
				const Vec3 p2 { 0.0, static_cast<double>(tileHeight), h[2] };
				const Vec3 u { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
				const Vec3 v { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };
				Vec3 normal { (u.y * v.z) - (u.z * v.y), (u.z * v.x) - (u.x * v.z), (u.x * v.y) - (u.y * v.x) };
				const auto magnitude = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
				if (magnitude > 0.0)
				{
					normal.x /= magnitude;
					normal.y /= magnitude;
					normal.z /= magnitude;
				}
				return normal;
			}
		}
		return { 0, 0, 1 };
	}

	Vec2 IsoMap::WorldToTile(double worldX, double worldY) const
	{
		if (projection == Projection::Isometric)
		{
			const double halfW = tileWidth / 2.0;
			const double halfH = tileHeight / 2.0;
			const double tileX = std::floor((worldX / halfW + worldY / halfH) / 2.0);
			const double tileY = std::floor((worldY / halfH - worldX / halfW) / 2.0);
			return { tileX, tileY };
		}
		return { std::floor(worldX / tileWidth), std::floor(worldY / tileHeight) };
	}

	Vec2 IsoMap::WorldToScreen(double worldX, double worldY, double worldZ, const IsoLayer* layer) const
	{
		const double parallaxX = layer ? layer->parallaxX : 1.0;
		const double parallaxY = layer ? layer->parallaxY : 1.0;
		const double offsetX = layer ? layer->offsetX : 0.0;
		const double offsetY = layer ? layer->offsetY : 0.0;

		if (projection == Projection::Isometric || projection == Projection::Staggered)
		{
			const double halfW = tileWidth / 2.0;
			const double halfH = tileHeight / 2.0;
			const double screenX = (worldX - worldY) * halfW * parallaxX + offsetX + originX;
			const double screenY = (worldX + worldY) * halfH * parallaxY - worldZ + offsetY + originY;
			return { screenX, screenY };
		}

		return { worldX * parallaxX + offsetX + originX, worldY * parallaxY - worldZ + offsetY + originY };
	}

	Vec2 IsoMap::ScreenToTile(double screenX, double screenY, const IsoLayer* layer) const
	{
		const double parallaxX = layer ? layer->parallaxX : 1.0;
		const double parallaxY = layer ? layer->parallaxY : 1.0;
		const double offsetX = layer ? layer->offsetX : 0.0;
		const double offsetY = layer ? layer->offsetY : 0.0;

		const double worldX = (screenX - offsetX - originX) / parallaxX;
		const double worldY = (screenY - offsetY - originY) / parallaxY;

		if (projection == Projection::Isometric || projection == Projection::Staggered)
		{
			const double halfW = tileWidth / 2.0;
			const double halfH = tileHeight / 2.0;
			const double tileX = std::floor((worldX / halfW + worldY / halfH) / 2.0);
			const double tileY = std::floor((worldY / halfH - worldX / halfW) / 2.0);
			return { tileX, tileY };
		}

		return { std::floor(worldX / tileWidth), std::floor(worldY / tileHeight) };
	}

	double IsoMap::DepthSortKey(double screenX, double screenY, double z) const
	{
		return screenY + z;
	}

	bool IsoMap::Raycast(const Vec3& start, const Vec3& end, double& outDistance) const
	{
		const int steps = 64;
		Vec3 delta { (end.x - start.x) / steps, (end.y - start.y) / steps, (end.z - start.z) / steps };
		Vec3 pos = start;
		for (int i = 0; i <= steps; ++i)
		{
			const auto tile = WorldToTile(pos.x, pos.y);
			const auto height = TileHeightAt(layers.empty() ? "" : layers.front().name, static_cast<int>(tile.x), static_cast<int>(tile.y));
			if (pos.z <= height)
			{
				const double dx = pos.x - start.x;
				const double dy = pos.y - start.y;
				const double dz = pos.z - start.z;
				outDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
				return true;
			}
			pos.x += delta.x;
			pos.y += delta.y;
			pos.z += delta.z;
		}
		return false;
	}

	bool IsoMap::MoveBody(const std::string& name, const Vec3& delta)
	{
		auto it = bodies.find(name);
		if (it == bodies.end())
			return false;

		auto& body = it->second;
		Vec3 proposed { body.position.x + delta.x, body.position.y + delta.y, body.position.z + delta.z };

		const auto tile = WorldToTile(proposed.x, proposed.y);
		const double groundHeight = TileHeightAt(layers.empty() ? "" : layers.front().name, static_cast<int>(tile.x), static_cast<int>(tile.y));
		bool blocked = false;
		if (!body.triggerOnly && proposed.z < groundHeight)
		{
			proposed.z = groundHeight;
			body.onGround = true;
			body.velocity = { 0, 0, 0 };
			blocked = true;
		}
		else
		{
			body.onGround = false;
		}

		body.position = proposed;
		return !blocked;
	}

	bool IsoMap::TeleportIfClear(const std::string& name, const Vec3& target)
	{
		auto it = bodies.find(name);
		if (it == bodies.end())
			return false;
		const auto tile = WorldToTile(target.x, target.y);
		const double groundHeight = TileHeightAt(layers.empty() ? "" : layers.front().name, static_cast<int>(tile.x), static_cast<int>(tile.y));
		if (!it->second.triggerOnly && target.z < groundHeight)
			return false;
		it->second.position = target;
		return true;
	}

	void IsoMap::SpawnBodiesFromLayer(const std::string& layerName)
	{
		auto* layer = FindLayer(layerName);
		if (!layer || !layer->isObjectLayer)
			return;
		for (const auto& obj : layer->objects)
			bodies[obj.name] = obj;
	}

	bool IsoMap::ParseTilesets(const pugi::xml_node& mapNode, std::string& errorMsg)
	{
		for (auto tsNode : mapNode.children("tileset"))
		{
			IsoTileset tileset;
			tileset.firstGid = tsNode.attribute("firstgid").as_uint();
			if (auto sourceAttr = tsNode.attribute("source"); sourceAttr && sourceAttr.as_string()[0] != 0)
			{
				const auto tsxPath = mapPath.parent_path() / sourceAttr.as_string();
				pugi::xml_document tsxDoc;
				if (!tsxDoc.load_file(tsxPath.c_str()))
				{
					errorMsg = "Failed to load external tileset: " + tsxPath.string();
					return false;
				}
				tsNode = tsxDoc.child("tileset");
			}

			tileset.name = tsNode.attribute("name").as_string();
			tileset.tileWidth = tsNode.attribute("tilewidth").as_uint();
			tileset.tileHeight = tsNode.attribute("tileheight").as_uint();

			for (auto tileNode : tsNode.children("tile"))
			{
				IsoTilesetTile tile;
				tile.localId = tileNode.attribute("id").as_uint();
				tile.terrain = tileNode.attribute("terrain").as_string();
				if (auto properties = tileNode.child("properties"))
				{
					for (auto prop : properties.children("property"))
					{
						const auto key = prop.attribute("name").as_string();
						if (prop.attribute("type") && std::string_view(prop.attribute("type").as_string()) == "int")
						{
							const auto value = prop.attribute("value").as_int();
							if (std::strcmp(key, "height_tl") == 0)
								tile.heights[0] = static_cast<float>(value);
							else if (std::strcmp(key, "height_tr") == 0)
								tile.heights[1] = static_cast<float>(value);
							else if (std::strcmp(key, "height_bl") == 0)
								tile.heights[2] = static_cast<float>(value);
							else if (std::strcmp(key, "height_br") == 0)
								tile.heights[3] = static_cast<float>(value);
						}
						tile.properties.emplace(key, prop.attribute("value").as_string());
					}
				}

				if (auto objGroup = tileNode.child("objectgroup"))
				{
					for (auto obj : objGroup.children("object"))
					{
						IsoCollisionMask mask;
						mask.width = obj.attribute("width").as_double();
						mask.height = obj.attribute("height").as_double();
						if (obj.child("ellipse"))
						{
							mask.type = CollisionMaskType::Ellipse;
						}
						else if (auto poly = obj.child("polygon"))
						{
							mask.type = CollisionMaskType::Polygon;
							std::string pointsStr = poly.attribute("points").as_string();
							std::stringstream ss(pointsStr);
							std::string token;
							while (std::getline(ss, token, ' '))
							{
								auto comma = token.find(',');
								if (comma == std::string::npos)
									continue;
								Vec2 pt;
								pt.x = std::stod(token.substr(0, comma));
								pt.y = std::stod(token.substr(comma + 1));
								mask.polygon.emplace_back(pt);
							}
						}
						else
						{
							mask.type = CollisionMaskType::Rectangle;
						}
						const auto maskId = static_cast<std::uint32_t>(collisionMasks.size() + 1);
						collisionMasks[maskId] = mask;
						tile.collisionMask = mask;
					}
				}
				tileset.tiles[tile.localId] = tile;
			}

			tilesets.emplace_back(std::move(tileset));
		}
		return true;
	}

	bool IsoMap::ParseTileLayers(const pugi::xml_node& mapNode, std::string& errorMsg)
	{
		for (auto layerNode : mapNode.children("layer"))
		{
			IsoLayer layer;
			layer.id = layerNode.attribute("id").as_int();
			layer.name = layerNode.attribute("name").as_string();
			layer.visible = layerNode.attribute("visible").as_bool(true);
			layer.offsetX = ParseDoubleAttr(layerNode, "offsetx");
			layer.offsetY = ParseDoubleAttr(layerNode, "offsety");
			layer.parallaxX = ParseDoubleAttr(layerNode, "parallaxx", 1.0);
			layer.parallaxY = ParseDoubleAttr(layerNode, "parallaxy", 1.0);
			auto dataNode = layerNode.child("data");
			if (!dataNode)
			{
				errorMsg = "Layer missing data node: " + layer.name;
				return false;
			}

			if (!ParseLayerChunks(layer, dataNode, errorMsg))
				return false;

			layers.emplace_back(std::move(layer));
		}
		return true;
	}

	bool IsoMap::ParseObjectLayers(const pugi::xml_node& mapNode, std::string& /*errorMsg*/)
	{
		for (auto objLayer : mapNode.children("objectgroup"))
		{
			IsoLayer layer;
			layer.isObjectLayer = true;
			layer.name = objLayer.attribute("name").as_string();
			layer.id = objLayer.attribute("id").as_int();
			layer.visible = objLayer.attribute("visible").as_bool(true);
			layer.offsetX = ParseDoubleAttr(objLayer, "offsetx");
			layer.offsetY = ParseDoubleAttr(objLayer, "offsety");
			layer.parallaxX = ParseDoubleAttr(objLayer, "parallaxx", 1.0);
			layer.parallaxY = ParseDoubleAttr(objLayer, "parallaxy", 1.0);

			for (auto obj : objLayer.children("object"))
			{
				IsoBody body;
				body.name = obj.attribute("name").as_string();
				body.position.x = obj.attribute("x").as_double();
				body.position.y = obj.attribute("y").as_double();
				body.size.x = obj.attribute("width").as_double();
				body.size.y = obj.attribute("height").as_double();
				body.collisionGroup = obj.attribute("type").as_int(0);

				if (auto properties = obj.child("properties"))
				{
					for (auto prop : properties.children("property"))
					{
						const auto key = prop.attribute("name").as_string();
						const auto value = prop.attribute("value").as_string();
						body.properties.emplace(key, value);
						if (std::strcmp(key, "z") == 0)
							body.position.z = std::atof(value);
						else if (std::strcmp(key, "depth") == 0)
							body.size.z = std::atof(value);
						else if (std::strcmp(key, "trigger") == 0)
							body.triggerOnly = ToLower(value) == "true";
					}
				}
				layer.objects.emplace_back(std::move(body));
			}

			layers.emplace_back(std::move(layer));
		}
		return true;
	}

	bool IsoMap::ParseLayerChunks(IsoLayer& layer, const pugi::xml_node& dataNode, std::string& errorMsg)
	{
		if (auto chunkNode = dataNode.child("chunk"))
		{
			for (; chunkNode; chunkNode = chunkNode.next_sibling("chunk"))
			{
				IsoChunk chunk;
				chunk.originX = chunkNode.attribute("x").as_int();
				chunk.originY = chunkNode.attribute("y").as_int();
				chunk.width = chunkNode.attribute("width").as_int();
				chunk.height = chunkNode.attribute("height").as_int();

				auto gids = DecodeTileData(chunkNode, errorMsg);
				if (!gids)
					return false;
				chunk.tiles.resize(chunk.width * chunk.height);
				for (int y = 0; y < chunk.height; ++y)
				{
					for (int x = 0; x < chunk.width; ++x)
					{
						const auto idx = static_cast<std::size_t>(y * chunk.width + x);
						chunk.tiles[idx].gid = (*gids)[idx];
						PopulateTileHeights(chunk.tiles[idx], chunk.tiles[idx].gid);
					}
				}
				layer.chunks.emplace_back(std::move(chunk));
			}
		}
		else
		{
			IsoChunk chunk;
			chunk.originX = 0;
			chunk.originY = 0;
			chunk.width = dataNode.parent().attribute("width").as_int(mapWidth);
			chunk.height = dataNode.parent().attribute("height").as_int(mapHeight);
			auto gids = DecodeTileData(dataNode, errorMsg);
			if (!gids)
				return false;
			chunk.tiles.resize(chunk.width * chunk.height);
			for (int y = 0; y < chunk.height; ++y)
			{
				for (int x = 0; x < chunk.width; ++x)
				{
					const auto idx = static_cast<std::size_t>(y * chunk.width + x);
					chunk.tiles[idx].gid = (*gids)[idx];
					PopulateTileHeights(chunk.tiles[idx], chunk.tiles[idx].gid);
				}
			}
			layer.chunks.emplace_back(std::move(chunk));
		}

		return true;
	}

	bool IsoMap::PopulateTileHeights(IsoTile& tile, std::uint32_t gid)
	{
		const auto* tileInfo = LookupTilesetTile(gid);
		if (!tileInfo)
			return false;
		tile.heights = tileInfo->heights;
		tile.properties = tileInfo->properties;
		tile.terrain = tileInfo->terrain;
		if (tileInfo->collisionMask)
		{
			const auto maskId = static_cast<std::uint32_t>(collisionMasks.size() + 1);
			collisionMasks[maskId] = *tileInfo->collisionMask;
			tile.collisionMaskId = maskId;
		}
		return true;
	}

	std::optional<std::vector<std::uint32_t>> IsoMap::DecodeTileData(const pugi::xml_node& dataNode, std::string& errorMsg)
	{
		const std::string encoding = dataNode.attribute("encoding").as_string();
		const std::string compression = dataNode.attribute("compression").as_string();
		const auto text = dataNode.text().as_string();

		if (encoding.empty() || encoding == "csv")
		{
			std::vector<std::uint32_t> gids;
			std::stringstream ss(text);
			std::string token;
			while (std::getline(ss, token, ','))
			{
				if (token.empty())
					continue;
				gids.push_back(static_cast<std::uint32_t>(std::stoul(token)));
			}
			return gids;
		}
		else if (encoding == "base64")
		{
			auto bytes = DecodeBase64(text, errorMsg);
			if (!errorMsg.empty())
				return std::nullopt;
			if (!compression.empty())
			{
				const bool gzip = compression == "gzip";
				bytes = Decompress(bytes, gzip, errorMsg);
				if (!errorMsg.empty())
					return std::nullopt;
			}
			std::vector<std::uint32_t> gids;
			for (std::size_t i = 0; i + 3 < bytes.size(); i += 4)
			{
				std::uint32_t value = bytes[i] | (bytes[i + 1] << 8) | (bytes[i + 2] << 16) | (bytes[i + 3] << 24);
				gids.push_back(value);
			}
			return gids;
		}

		errorMsg = "Unsupported encoding: " + encoding;
		return std::nullopt;
	}

	std::vector<std::uint8_t> IsoMap::DecodeBase64(std::string_view text, std::string& errorMsg)
	{
		static constexpr unsigned char kTable[256] = {
			64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
			64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,52,53,54,55,56,57,58,59,60,61,64,64,64, 0,64,64,
			64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
			64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
			64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
			64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
			64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
			64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64 };

		std::vector<std::uint8_t> out;
		std::uint32_t buffer = 0;
		int bitsCollected = 0;
		for (unsigned char c : text)
		{
			if (std::isspace(c))
				continue;
			if (c == '=')
				break;
			const auto val = kTable[c];
			if (val == 64)
			{
				errorMsg = "Invalid base64 character.";
				return {};
			}
			buffer = (buffer << 6) | val;
			bitsCollected += 6;
			if (bitsCollected >= 8)
			{
				bitsCollected -= 8;
				out.push_back(static_cast<std::uint8_t>((buffer >> bitsCollected) & 0xFF));
			}
		}
		return out;
	}

	std::vector<std::uint8_t> IsoMap::Decompress(const std::vector<std::uint8_t>& input, bool gzip, std::string& errorMsg)
	{
		std::vector<std::uint8_t> output;
		z_stream stream {};
		stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
		stream.avail_in = static_cast<uInt>(input.size());

		const int windowBits = gzip ? 16 + MAX_WBITS : MAX_WBITS;
		int status = inflateInit2(&stream, windowBits);
		if (status != Z_OK)
		{
			errorMsg = "inflateInit2 failed with code " + std::to_string(status);
			return {};
		}

		std::vector<std::uint8_t> buffer(4096);
		do
		{
			stream.next_out = buffer.data();
			stream.avail_out = static_cast<uInt>(buffer.size());
			status = inflate(&stream, Z_NO_FLUSH);
			if (status != Z_OK && status != Z_STREAM_END)
			{
				inflateEnd(&stream);
				errorMsg = "inflate failed with code " + std::to_string(status);
				return {};
			}
			const auto produced = buffer.size() - stream.avail_out;
			output.insert(output.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(produced));
		} while (status != Z_STREAM_END);

		inflateEnd(&stream);
		return output;
	}

	const IsoTilesetTile* IsoMap::LookupTilesetTile(std::uint32_t gid) const
	{
		for (auto it = tilesets.rbegin(); it != tilesets.rend(); ++it)
		{
			if (gid >= it->firstGid)
			{
				const auto localId = gid - it->firstGid;
				const auto tileIt = it->tiles.find(localId);
				if (tileIt != it->tiles.end())
					return &tileIt->second;
			}
		}
		return nullptr;
	}

	IsoLayer* IsoMap::FindLayer(const std::string& name)
	{
		for (auto& layer : layers)
		{
			if (layer.name == name)
				return &layer;
		}
		return nullptr;
	}

	const IsoLayer* IsoMap::FindLayer(const std::string& name) const
	{
		for (const auto& layer : layers)
		{
			if (layer.name == name)
				return &layer;
		}
		return nullptr;
	}
}
