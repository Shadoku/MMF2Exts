#include "Common.hpp"

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
	rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr) :
	javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
	Runtime(this, this->javaExtPtr)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr) :
	objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr)
#endif
{
	using namespace std::literals;
	LinkAction(0, LoadTMX);
	LinkAction(1, LoadJSONCache);
	LinkAction(2, SaveJSONCache);
	LinkAction(3, ExportTMXLayer);
	LinkAction(4, SetTileHeights);
	LinkAction(5, SetTileMask);
	LinkAction(6, MoveBody);
	LinkAction(7, TeleportIfClear);
	LinkAction(8, ClearChunkCache);
	LinkAction(9, SpawnBodiesFromLayer);
	LinkAction(10, EnableCCD);
	LinkAction(11, SetBodyPhysics);
	LinkAction(12, SetDebugOverlay);

	LinkCondition(0, OnTMXLoadSuccess);
	LinkCondition(1, OnTMXLoadFailure);
	LinkCondition(2, OnCollisionStart);
	LinkCondition(3, OnCollisionEnd);
	LinkCondition(4, OnClearPath);
	LinkCondition(5, OnLanding);
	LinkCondition(6, OnLeavingGround);
	LinkCondition(7, OnTileTypeInRegion);

	LinkExpression(0, WorldToScreenX);
	LinkExpression(1, WorldToScreenY);
	LinkExpression(2, ScreenToTileQ);
	LinkExpression(3, ScreenToTileR);
	LinkExpression(4, TileHeightAt);
	LinkExpression(5, SurfaceNormalX);
	LinkExpression(6, SurfaceNormalY);
	LinkExpression(7, SurfaceNormalZ);
	LinkExpression(8, RaycastDistance);
	LinkExpression(9, DepthSortKey);
	LinkExpression(10, BodyContactFlags);
	LinkExpression(11, TileProperty);
	LinkExpression(12, TilesetProperty);
	LinkExpression(13, LayerParallaxX);
	LinkExpression(14, LayerParallaxY);
	LinkExpression(15, BodyX);
	LinkExpression(16, BodyY);
	LinkExpression(17, BodyZ);

	// Apply edittime defaults if provided
	if (edPtr)
	{
		map.tileWidth = edPtr->Props.GetPropertyNum("Tile Width"sv);
		map.tileHeight = edPtr->Props.GetPropertyNum("Tile Height"sv);
		map.chunkWidth = edPtr->Props.GetPropertyNum("Chunk Width"sv);
		map.chunkHeight = edPtr->Props.GetPropertyNum("Chunk Height"sv);
		map.originX = edPtr->Props.GetPropertyNum("Origin X"sv);
		map.originY = edPtr->Props.GetPropertyNum("Origin Y"sv);
		const auto projection = edPtr->Props.GetPropertyStr("Projection"sv);
		if (projection == _T("isometric"))
			map.projection = Iso::Projection::Isometric;
		else if (projection == _T("staggered"))
			map.projection = Iso::Projection::Staggered;
		else if (projection == _T("hexagonal"))
			map.projection = Iso::Projection::Hexagonal;
		else
			map.projection = Iso::Projection::Orthogonal;
	}
}

Extension::~Extension() = default;
