#include "Common.hpp"

///
/// EXTENSION CONSTRUCTOR/DESTRUCTOR
///

#ifdef _WIN32
Extension::Extension(RunObject* const _rdPtr, const EDITDATA* const edPtr, const CreateObjectInfo* const cobPtr) :
	rdPtr(_rdPtr), rhPtr(_rdPtr->get_rHo()->get_AdRunHeader()), Runtime(this), FusionDebugger(this)
#elif defined(__ANDROID__)
Extension::Extension(const EDITDATA* const edPtr, const jobject javaExtPtr) :
	javaExtPtr(javaExtPtr, "Extension::javaExtPtr from Extension ctor"),
	Runtime(this, this->javaExtPtr), FusionDebugger(this)
#else
Extension::Extension(const EDITDATA* const edPtr, void* const objCExtPtr) :
	objCExtPtr(objCExtPtr), Runtime(this, objCExtPtr), FusionDebugger(this)
#endif
{
	/*
		Link all your action/condition/expression functions to their IDs to match the
		IDs in the JSON here
	*/

        LinkAction(0, RunLuaScript);
        LinkAction(1, RunLuaScriptFile);
        LinkAction(2, CreateContext);
        LinkAction(3, SwitchContext);

        LinkCondition(0, DummyCondition);

        LinkExpression(0, EvalLua);
        LinkExpression(1, GetCurrentContext);

	/*
		This is where you'd do anything you'd do in CreateRunObject in the original SDK

		It's the only place you'll get access to edPtr at runtime, so you should transfer
		anything from edPtr to the extension class here.

	*/

	// Don't use "this" inside these lambda functions, always ext.
	// There can be nothing in the [] section of the lambda.
	// If you're not sure about lambdas, you can remove this debugger stuff without any side effects;
	// it's just an example of how to use the debugger. You can view it in Fusion itself to see.
        FusionDebugger.AddItemToDebugger(
                // reader function for your debug item
                [](Extension *ext, std::tstring &writeTo) {
                        writeTo = _T("My text is: ") + ext->exampleDebuggerTextItem;
                },
		// writer function (can be null if you don't want user to be able to edit it in debugger)
		[](Extension *ext, std::tstring &newText)
		{
			ext->exampleDebuggerTextItem = newText;
			return true; // accept the changes
		}, 500, NULL
        );

        currentContext = 0;
        contexts.push_back(luaL_newstate());
        L = contexts[0];
        luaL_openlibs(L);
        RegisterFusionFunctions(L);


	// Read object DarkEdif properties; you can pass property name, or property index
	// This will work on all platforms the same way.
	bool checkboxWithinFolder = edPtr->Props.IsPropChecked("Checkbox within folder"sv);
	std::tstring editable6Text = edPtr->Props.GetPropertyStr("Editable 6"sv);

	// These lines do nothing, but prevent the compiler warning the variables are unused
	(void)checkboxWithinFolder;
	(void)editable6Text;
}

Extension::~Extension()
{
        for (lua_State* state : contexts)
        {
                if (state)
                        lua_close(state);
        }
        contexts.clear();
        L = nullptr;
}


REFLAG Extension::Handle()
{
	/*
		If your extension will draw to the MMF window you should first
		check if anything about its display has changed :

			if (rdPtr->roc.rcChanged)
			  return REFLAG::DISPLAY;
			else
			  return REFLAG::NONE;

		You will also need to make sure you change this flag yourself
		to 1 whenever you want to redraw your object

		If your extension won't draw to the window, but it still needs
		to do something every MMF loop use :

			return REFLAG::NONE;

		If you don't need to do something every loop, use :

			return REFLAG::ONE_SHOT;

		This doesn't mean this function can never run again. If you want MMF
		to handle your object again (causing this code to run) use this function:

			Runtime.Rehandle();

		At the end of the loop this code will run

	*/

	// Will not be called next loop
	return REFLAG::ONE_SHOT;
}


REFLAG Extension::Display()
{
	/*
		If you return REFLAG_DISPLAY in Handle() this routine will run.
	*/

	// Ok
	return REFLAG::DISPLAY;
}

short Extension::FusionRuntimePaused()
{

	// Ok
	return 0;
}

short Extension::FusionRuntimeContinued()
{

	// Ok
	return 0;
}


// These are called if there's no function linked to an ID

void Extension::UnlinkedAction(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedAction() called"), _T("Running a fallback for action ID %d. Make sure you ran LinkAction()."), ID);
}

long Extension::UnlinkedCondition(int ID)
{
	DarkEdif::MsgBox::Error(_T("Extension::UnlinkedCondition() called"), _T("Running a fallback for condition ID %d. Make sure you ran LinkCondition()."), ID);
	return 0;
}

long Extension::UnlinkedExpression(int ID)
{
        DarkEdif::MsgBox::Error(_T("Extension::UnlinkedExpression() called"), _T("Running a fallback for expression ID %d. Make sure you ran LinkExpression()."), ID);
        // Unlinked A/C/E is fatal error, but try not to return null string and definitely crash it
        if ((size_t)ID < Edif::SDK->ExpressionInfos.size() && Edif::SDK->ExpressionInfos[ID]->Flags.ef == ExpReturnType::String)
                return (long)Runtime.CopyString(_T(""));
        return 0;
}

int Extension::Lua_ListObjects(lua_State* L)
{
        Extension* ext = (Extension*)lua_touserdata(L, lua_upvalueindex(1));
        int oi = (int)luaL_checkinteger(L, 1);
        lua_newtable(L);
        int idx = 1;
        for (auto ro : DarkEdif::ObjectIterator(ext->rhPtr, oi, DarkEdif::Selection::Explicit))
        {
                lua_pushinteger(L, idx++);
                lua_pushinteger(L, ro->get_rHo()->GetFixedValue());
                lua_settable(L, -3);
        }
        return 1;
}

int Extension::Lua_GetPosition(lua_State* L)
{
        Extension* ext = (Extension*)lua_touserdata(L, lua_upvalueindex(1));
        int fv = (int)luaL_checkinteger(L, 1);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro)
        {
                lua_pushnil(L);
                return 1;
        }
        HeaderObject* ho = ro->get_rHo();
        lua_pushinteger(L, ho->X);
        lua_pushinteger(L, ho->Y);
        return 2;
}

int Extension::Lua_SetPosition(lua_State* L)
{
        Extension* ext = (Extension*)lua_touserdata(L, lua_upvalueindex(1));
        int fv = (int)luaL_checkinteger(L, 1);
        int x = (int)luaL_checkinteger(L, 2);
        int y = (int)luaL_checkinteger(L, 3);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro)
                return 0;
        HeaderObject* ho = ro->get_rHo();
        ho->X = x;
        ho->Y = y;
        if (ro->get_roc())
                ro->get_roc()->rcChanged = true;
        return 0;
}

int Extension::Lua_GetAltValue(lua_State* L)
{
        Extension* ext = (Extension*)lua_touserdata(L, lua_upvalueindex(1));
        int fv = (int)luaL_checkinteger(L, 1);
        int index = (int)luaL_checkinteger(L, 2);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro || !ro->get_rov())
        {
                lua_pushnil(L);
                return 1;
        }
        const CValueMultiPlat* val = ro->get_rov()->GetAltValueAtIndex(index);
        if (!val)
        {
                lua_pushnil(L);
                return 1;
        }
        if (val->m_type == TYPE_STRING)
        {
                lua_pushstring(L, DarkEdif::TStringToUTF8(val->m_pString ? val->m_pString : _T("")).c_str());
        }
        else if (val->m_type == TYPE_FLOAT)
        {
                lua_pushnumber(L, val->m_double);
        }
        else
        {
                lua_pushinteger(L, val->m_long);
        }
        return 1;
}

int Extension::Lua_SetAltValue(lua_State* L)
{
        Extension* ext = (Extension*)lua_touserdata(L, lua_upvalueindex(1));
        int fv = (int)luaL_checkinteger(L, 1);
        int index = (int)luaL_checkinteger(L, 2);
        RunObjectMultiPlatPtr ro = ext->Runtime.RunObjPtrFromFixed(fv);
        if (!ro || !ro->get_rov())
                return 0;

        if (lua_type(L, 3) == LUA_TSTRING)
        {
                const char* s = lua_tostring(L, 3);
                ro->get_rov()->SetAltStringAtIndex(index, DarkEdif::UTF8ToTString(s));
        }
        else if (lua_isinteger(L, 3))
        {
                ro->get_rov()->SetAltValueAtIndex(index, (int)lua_tointeger(L, 3));
        }
        else if (lua_isnumber(L, 3))
        {
                ro->get_rov()->SetAltValueAtIndex(index, lua_tonumber(L, 3));
        }
        return 0;
}

void Extension::RegisterFusionFunctions(lua_State* state)
{
        lua_newtable(state);
        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, Lua_ListObjects, 1);
        lua_setfield(state, -2, "listObjects");

        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, Lua_GetPosition, 1);
        lua_setfield(state, -2, "getPos");

        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, Lua_SetPosition, 1);
        lua_setfield(state, -2, "setPos");

        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, Lua_GetAltValue, 1);
        lua_setfield(state, -2, "getAltValue");

        lua_pushlightuserdata(state, this);
        lua_pushcclosure(state, Lua_SetAltValue, 1);
        lua_setfield(state, -2, "setAltValue");

        lua_setglobal(state, "fusion");
}
