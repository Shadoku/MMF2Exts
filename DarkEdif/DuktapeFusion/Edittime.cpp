// ============================================================================
// This file contains routines that are handled only during the Edittime,
// under the Frame and Event editors.
//
// Including creating, display, and setting up your object.
// ============================================================================
#include "Common.hpp"

// ============================================================================
// ROUTINES USED UNDER FRAME EDITOR
// ============================================================================

#if EditorBuild

// Called once object is created or modified, just after setup.
// Also called before showing the "Insert an object" dialog if your object
// has no icon resource
int FusionAPI MakeIconEx(mv * mV, cSurface * pIconSf, TCHAR * lpName, ObjInfo * oiPtr, EDITDATA * edPtr)
{
#pragma DllExportHint
    pIconSf->Delete();
    pIconSf->Clone(*Edif::SDK->Icon);

    pIconSf->SetTransparentColor(RGB(255, 0, 255));
    return 0;
}

// Called when you choose "Create new object". It should display the setup box
// and initialize everything in the datazone.
int FusionAPI CreateObject(mv * mV, LevelObject * loPtr, EDITDATA * edPtr)
{
#pragma DllExportHint
    if (!Edif::IS_COMPATIBLE(mV))
        return -1;

    Edif::Init(mV, edPtr);
    return DarkEdif::DLL::DLL_CreateObject(mV, loPtr, edPtr);
}

// Displays the object under the frame editor
void FusionAPI EditorDisplay(mv *mV, ObjectInfo * oiPtr, LevelObject * loPtr, EDITDATA * edPtr, RECT * rc)
{
#pragma DllExportHint
    cSurface * Surface = WinGetSurface((int) mV->IdEditWin);
    if (!Surface)
        return;

    // If you don't have this function run in Edittime.cpp, SDK Updater will be disabled for your ext
    // Don't comment or preprocessor-it out if you're removing it; delete the line entirely.
    DarkEdif::SDKUpdater::RunUpdateNotifs(mV, edPtr);

    Edif::SDK->Icon->Blit(*Surface, rc->left, rc->top, BMODE_TRANSP, BOP_COPY, 0);
}

// Inserts properties into the properties of the object.
BOOL FusionAPI GetProperties(mv * mV, EDITDATA * edPtr, BOOL bMasterItem)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetProperties(mV, edPtr, bMasterItem);
}

// Called when the properties are removed from the property window.
void FusionAPI ReleaseProperties(mv * mV, EDITDATA * edPtr, BOOL bMasterItem)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_ReleaseProperties(mV, edPtr, bMasterItem);
}

// When the object is removed from the frame editor, this is called to clean up
void FusionAPI RemoveObject(mv * mV, EDITDATA * edPtr, int code, BOOL bMasterItem)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_RemoveObject(mV, edPtr, code, bMasterItem);
}

// Called when Fusion copies your object during runtime (animation list, others)
// Use to deep-copy if your EDITDATA struct holds dynamic allocated data.
void FusionAPI DuplicateObject(mv * mV, EDITDATA * destEdPtr, EDITDATA * sourceEdPtr)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_DuplicateObject(mV, destEdPtr, sourceEdPtr);
}

#endif // EditorBuild

// ============================================================================
// GENERAL ROUTINES
// ============================================================================

void FusionAPI GetObjInfos(mv * mV, EDITDATA * edPtr, tagObjInfo * oiPtr)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_GetObjInfos(mV, edPtr, oiPtr);
}

#ifdef _WIN32
BOOL FusionAPI GetRunObjectData(mv * mV, LevelObject * loPtr, EDITDATA * edPtr, void *file, int size)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetRunObjectData(mV, loPtr, edPtr, file, size);
}
#endif

BOOL FusionAPI PutRunObjectData(mv * mV, EDITDATA * edPtr, void *file, int *psize)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_PutRunObjectData(mV, edPtr, file, psize);
}

int FusionAPI GetRID(ID * pID)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetRID(pID);
}

HGLOBAL FusionAPI Malloc(DWORD dwBytes)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_Malloc(dwBytes);
}

int FusionAPI Free(HGLOBAL hMem)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_Free(hMem);
}

long FusionAPI GetRunObjectInfos(mv * mV, EDITDATA * edPtr, fpcob cobPtr)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetRunObjectInfos(mV, edPtr, cobPtr);
}

void FusionAPI enumStorage(mv * mV, fpcob cobPtr, void (FusionAPI * cb)(mv*, fpcob, LPSTR, int, int))
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_enumStorage(mV, cobPtr, cb);
}

long FusionAPI LoadObject(mv * mV, EDITDATA * edPtr, fpcob cobPtr, int version)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_LoadObject(mV, edPtr, cobPtr, version);
}

// ============================================================================
// UNICODE SUPPORT
// ============================================================================

int FusionAPI Initialize(mv * mV)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_Initialize(mV);
}

#ifdef _WIN32
int FusionAPI UpdateEditStructure(mv * mV, void * OldEdPtr, void * NewEdPtr)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_UpdateEditStructure(mV, OldEdPtr, NewEdPtr);
}
#endif

int FusionAPI GetRegNo(mv * mV)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetRegNo(mV);
}

const TCHAR * FusionAPI GetHelpFileName()
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_GetHelpFileName();
}

// Called when Edif uses ICEx to add ACEs
void FusionAPI AddToToolbarList(mv * mV, short col, LPCSTR text, LPCSTR tooltip)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_AddToToolBarList(mV, col, text, tooltip);
}

#ifdef _WIN32
BOOL FusionAPI IsToolBarSupported()
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_IsToolBarSupported();
}
#endif

void FusionAPI ReleaseToolbar(mv * mV, void * Buffer)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_ReleaseToolBar(mV, Buffer);
}

// Called after editing the menu
void FusionAPI MenuEditorDisplay(mv * mV, EDITDATA * edPtr, void * Base, RECT * rc, long colEdit, long colSel)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_MenuEditorDisplay(mV, edPtr, Base, rc, colEdit, colSel);
}

#ifdef _WIN32
void FusionAPI GetPopupMenuPos(EDITDATA* edPtr, POINT* pt)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_GetPopupMenuPos(edPtr, pt);
}
#endif

void FusionAPI UpdatePopupMenu(EDITDATA* edPtr, HMENU* pMenu)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_UpdatePopupMenu(edPtr, pMenu);
}

BOOL FusionAPI CreateFromFile(mv * mV, LPCSTR fileName, EDITDATA * edPtr)
{
#pragma DllExportHint
    return DarkEdif::DLL::DLL_CreateFromFile(mV, fileName, edPtr);
}

void FusionAPI EditorFlatStructure(mv * mV, EDITDATA * edPtr, LPSTR Buffer, int* nSize, BOOL Unicode)
{
#pragma DllExportHint
    DarkEdif::DLL::DLL_EditorFlatStructure(mV, edPtr, Buffer, nSize, Unicode);
}

#endif
