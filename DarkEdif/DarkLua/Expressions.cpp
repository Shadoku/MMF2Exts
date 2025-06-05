#include "Common.hpp"

const TCHAR* Extension::EvalLua(const TCHAR* ExpressionText)
{
    std::string utf8 = DarkEdif::TStringToUTF8(ExpressionText);
    if (luaL_dostring(L, utf8.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        DarkEdif::MsgBox::Error(_T("Lua Error"), DarkEdif::UTF8ToTString(err).c_str());
        lua_pop(L, 1);
        return Runtime.CopyString(_T(""));
    }
    const char* res = lua_tostring(L, -1);
    std::tstring tres = DarkEdif::UTF8ToTString(res ? res : "");
    lua_pop(L, 1);
    return Runtime.CopyString(tres.c_str());
}

int Extension::GetCurrentContext()
{
    return currentContext;
}
