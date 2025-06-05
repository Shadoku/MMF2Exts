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
