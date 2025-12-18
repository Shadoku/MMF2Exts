#pragma once
#include "DarkEdif.hpp"
#include "IsoWorldTypes.hpp"

class Extension final
{
public:
	RunHeader* rhPtr;
	RunObjectMultiPlatPtr rdPtr;
#ifdef __ANDROID__
	global<jobject> javaExtPtr;
#elif defined(__APPLE__)
	void* const objCExtPtr;
#endif

	Edif::Runtime Runtime;

	static const int MinimumBuild = 254;
	static const int Version = 1;

	static constexpr OEFLAGS OEFLAGS = OEFLAGS::NONE;
	static constexpr OEPREFS OEPREFS = OEPREFS::NONE;

#ifdef _WIN32
	Extension(RunObject* const rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr);
#elif defined(__ANDROID__)
	Extension(const EDITDATA* const edPtr, const jobject javaExtPtr, const CreateObjectInfo* const cobPtr);
#else
	Extension(const EDITDATA* const edPtr, void* const objCExtPtr, const CreateObjectInfo* const cobPtr);
#endif
	~Extension();

	// State
	Iso::IsoMap map;
	std::tstring textScratch;
	std::string collisionStartBody;
	std::string collisionEndBody;
	std::string clearPathBody;
	std::string landingBody;
	std::string leavingBody;
	bool lastLoadSuccess = false;
	bool lastLoadFailure = false;
	bool debugOverlay = false;

	struct BodyContactState
	{
		bool onGround = false;
		bool landingEvent = false;
		bool leavingEvent = false;
	};

	std::unordered_map<std::string, BodyContactState> bodyContacts;

	// Actions
	void LoadTMX(const TCHAR* path);
	void LoadJSONCache(const TCHAR* path);
	void SaveJSONCache(const TCHAR* path);
	void ExportTMXLayer(const TCHAR* path, const TCHAR* layerName);
	void SetTileHeights(const TCHAR* layerName, int x, int y, int tl, int tr, int bl, int br);
	void SetTileMask(const TCHAR* layerName, int x, int y, int maskId);
	void MoveBody(const TCHAR* name, float dx, float dy, float dz);
	void TeleportIfClear(const TCHAR* name, float x, float y, float z);
	void ClearChunkCache();
	void SpawnBodiesFromLayer(const TCHAR* layerName);
	void EnableCCD(const TCHAR* name, bool enabled);
	void SetBodyPhysics(const TCHAR* name, float gravity, float friction, float bounce);
	void SetDebugOverlay(bool enabled);

	// Conditions
	bool OnTMXLoadSuccess();
	bool OnTMXLoadFailure();
	bool OnCollisionStart(const TCHAR* bodyName);
	bool OnCollisionEnd(const TCHAR* bodyName);
	bool OnClearPath(const TCHAR* bodyName);
	bool OnLanding(const TCHAR* bodyName);
	bool OnLeavingGround(const TCHAR* bodyName);
	bool OnTileTypeInRegion(const TCHAR* layerName, const TCHAR* terrain, int x1, int y1, int x2, int y2);

	// Expressions
	double WorldToScreenX(double worldX, double worldY, double worldZ);
	double WorldToScreenY(double worldX, double worldY, double worldZ);
	double ScreenToTileQ(double screenX, double screenY);
	double ScreenToTileR(double screenX, double screenY);
	double TileHeightAt(const TCHAR* layerName, int x, int y);
	double SurfaceNormalX(const TCHAR* layerName, int x, int y);
	double SurfaceNormalY(const TCHAR* layerName, int x, int y);
	double SurfaceNormalZ(const TCHAR* layerName, int x, int y);
	double RaycastDistance(double x1, double y1, double z1, double x2, double y2, double z2);
	double DepthSortKey(double screenX, double screenY, double z);
	double BodyContactFlags(const TCHAR* name);
	const TCHAR* TileProperty(const TCHAR* layerName, int x, int y, const TCHAR* key);
	const TCHAR* TilesetProperty(std::uint32_t gid, const TCHAR* key);
	double LayerParallaxX(const TCHAR* layerName);
	double LayerParallaxY(const TCHAR* layerName);
	double BodyX(const TCHAR* name);
	double BodyY(const TCHAR* name);
	double BodyZ(const TCHAR* name);

	REFLAG Handle();

	void UnlinkedAction(int ID);
	long UnlinkedCondition(int ID);
	long UnlinkedExpression(int ID);
};
