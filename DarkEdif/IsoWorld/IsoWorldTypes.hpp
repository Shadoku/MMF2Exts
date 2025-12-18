#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define PUGIXML_HEADER_ONLY
#include "thirdparty/pugixml.hpp"

#include "thirdparty/json.hpp"

#ifdef _WIN32
#pragma comment(lib, "..\\Lib\\Windows\\zlib.lib")
#include "..\\Inc\\Windows\\zlib.h"
#else
#include <zlib.h>
#endif

namespace Iso
{
	enum class Projection
	{
		Orthogonal,
		Isometric,
		Staggered,
		Hexagonal
	};

	enum class CollisionMaskType
	{
		None,
		Rectangle,
		Ellipse,
		Polygon
	};

	struct Vec2
	{
		double x = 0.0;
		double y = 0.0;
	};

	struct Vec3
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
	};

	struct IsoCollisionMask
	{
		CollisionMaskType type = CollisionMaskType::None;
		std::vector<Vec2> polygon;
		double width = 0.0;
		double height = 0.0;
	};

	struct IsoTile
	{
		std::uint32_t gid = 0;
		std::array<float, 4> heights { 0.f, 0.f, 0.f, 0.f };
		std::optional<std::uint32_t> collisionMaskId;
		std::unordered_map<std::string, std::string> properties;
		std::string terrain;
	};

	struct IsoChunk
	{
		int originX = 0;
		int originY = 0;
		int width = 0;
		int height = 0;
		std::vector<IsoTile> tiles;

		std::size_t Index(int x, int y) const
		{
			return static_cast<std::size_t>(y * width + x);
		}
	};

	struct IsoBody
	{
		std::string name;
		Vec3 position {};
		Vec3 size {};
		Vec3 velocity {};
		bool triggerOnly = false;
		bool ccdEnabled = false;
		bool onGround = false;
		int collisionGroup = 0;
		std::unordered_map<std::string, std::string> properties;
	};

	struct IsoLayer
	{
		std::string name;
		int id = -1;
		bool visible = true;
		bool isObjectLayer = false;
		double offsetX = 0.0;
		double offsetY = 0.0;
		double parallaxX = 1.0;
		double parallaxY = 1.0;
		std::vector<IsoChunk> chunks;
		std::vector<IsoBody> objects;
	};

	struct IsoTilesetTile
	{
		std::uint32_t localId = 0;
		std::unordered_map<std::string, std::string> properties;
		std::optional<IsoCollisionMask> collisionMask;
		std::array<float, 4> heights { 0.f, 0.f, 0.f, 0.f };
		std::string terrain;
	};

	struct IsoTileset
	{
		std::string name;
		std::uint32_t firstGid = 0;
		std::uint32_t tileWidth = 0;
		std::uint32_t tileHeight = 0;
		std::unordered_map<std::uint32_t, IsoTilesetTile> tiles;
	};

	struct TileLookup
	{
		IsoLayer* layer = nullptr;
		IsoTile* tile = nullptr;
	};

	class IsoMap
	{
	public:
		std::filesystem::path mapPath;
		Projection projection = Projection::Orthogonal;
		int tileWidth = 0;
		int tileHeight = 0;
		int mapWidth = 0;
		int mapHeight = 0;
		int chunkWidth = 16;
		int chunkHeight = 16;
		int hexSideLength = 0;
		std::string staggerAxis;
		std::string staggerIndex;
		double originX = 0.0;
		double originY = 0.0;
		double gravity = 900.0;
		double friction = 0.0;
		double bounce = 0.0;

		std::vector<IsoLayer> layers;
		std::unordered_map<std::string, IsoBody> bodies;
		std::unordered_map<std::uint32_t, IsoCollisionMask> collisionMasks;
		std::vector<IsoTileset> tilesets;

		bool LoadTMX(const std::filesystem::path& filePath, std::string& errorMsg);
		bool LoadJSON(const std::filesystem::path& filePath, std::string& errorMsg);
		bool SaveJSON(const std::filesystem::path& filePath, std::string& errorMsg) const;
		bool ExportTMXLayer(const std::filesystem::path& filePath, const std::string& layerName, std::string& errorMsg) const;

		TileLookup TileAt(const std::string& layerName, int x, int y);
		double TileHeightAt(const std::string& layerName, int x, int y) const;
		Vec3 SurfaceNormalAt(const std::string& layerName, int x, int y) const;

		Vec2 WorldToTile(double worldX, double worldY) const;
		Vec2 WorldToScreen(double worldX, double worldY, double worldZ, const IsoLayer* layer = nullptr) const;
		Vec2 ScreenToTile(double screenX, double screenY, const IsoLayer* layer = nullptr) const;
		double DepthSortKey(double screenX, double screenY, double z) const;

		bool Raycast(const Vec3& start, const Vec3& end, double& outDistance) const;
		bool MoveBody(const std::string& name, const Vec3& delta);
		bool TeleportIfClear(const std::string& name, const Vec3& target);
		void SpawnBodiesFromLayer(const std::string& layerName);

	private:
		bool ParseTilesets(const pugi::xml_node& mapNode, std::string& errorMsg);
		bool ParseTileLayers(const pugi::xml_node& mapNode, std::string& errorMsg);
		bool ParseObjectLayers(const pugi::xml_node& mapNode, std::string& errorMsg);
		bool ParseLayerChunks(IsoLayer& layer, const pugi::xml_node& dataNode, std::string& errorMsg);
		bool PopulateTileHeights(IsoTile& tile, std::uint32_t gid);
		static std::optional<std::vector<std::uint32_t>> DecodeTileData(const pugi::xml_node& dataNode, std::string& errorMsg);
		static std::vector<std::uint8_t> DecodeBase64(std::string_view text, std::string& errorMsg);
		static std::vector<std::uint8_t> Decompress(const std::vector<std::uint8_t>& input, bool gzip, std::string& errorMsg);
		const IsoTilesetTile* LookupTilesetTile(std::uint32_t gid) const;
		IsoLayer* FindLayer(const std::string& name);
		const IsoLayer* FindLayer(const std::string& name) const;
	};

} // namespace Iso
