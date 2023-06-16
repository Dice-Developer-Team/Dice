/**
 * lua�ű�Ƕ��
 * �����Զ���ǰ׺ָ���
 * Copyright (C) 2019-2023 String.Empty
 */
#pragma once
#include <string>
#include <unordered_map>

class Lua_State;
class DiceEvent;
bool lua_msg_call(DiceEvent*, const AttrVar&);
bool lua_call_event(AttrObject eve, const AttrVar&);
bool lua_call_task(const AttrVars&);