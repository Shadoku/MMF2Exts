#include "Common.hpp"

void Extension::RunLuaScript(const TCHAR* ScriptText)
{
    std::string utf8 = DarkEdif::TStringToUTF8(ScriptText);
    if (luaL_dostring(L, utf8.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        DarkEdif::MsgBox::Error(_T("Lua Error"), DarkEdif::UTF8ToTString(err).c_str());
        lua_pop(L, 1);
    }
}

void Extension::RunLuaScriptFile(const TCHAR* FilePath)
{
    std::string path = DarkEdif::TStringToUTF8(FilePath);
    if (luaL_dofile(L, path.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        DarkEdif::MsgBox::Error(_T("Lua Error"), DarkEdif::UTF8ToTString(err).c_str());
        lua_pop(L, 1);
    }
}

void Extension::CreateContext()
{
    lua_State* newL = luaL_newstate();
    luaL_openlibs(newL);
    RegisterFusionFunctions(newL);
    contexts.push_back(newL);
    currentContext = (int)contexts.size() - 1;
    L = newL;
}

void Extension::SwitchContext(int Index)
{
    if (Index >= 0 && Index < (int)contexts.size()) {
        currentContext = Index;
        L = contexts[Index];
    }
}
