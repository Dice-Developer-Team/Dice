/**
 * lua script extern
 * for orders e.t.c.
 * Copyright (C) 2019-2026 String.Empty
 */
#pragma once
#include <memory>
#include "DiceAttrVar.h"

class lua_State;
class LuaState {
	lua_State* state;
	//bool isValid;
public:
	LuaState(std::string file);
	LuaState();
	//operator bool()const { return isValid; }
	operator lua_State* () { return state; }
	~LuaState();
	void regist();
	void reboot();
	AttrVar dostring(const string&);
};
class DiceEvent;
bool lua_msg_call(DiceEvent*, const AttrVar&);
bool lua_call_event(const std::shared_ptr<AnysTable>& eve, const AttrVar&);
bool lua_call_task(const AttrVars&);