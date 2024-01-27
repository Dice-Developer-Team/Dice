/**
 * lua脚本嵌入
 * 用于自定义前缀指令等
 * Copyright (C) 2019-2024 String.Empty
 */
#pragma once
#include <string>
#include <unordered_map>
#include <memory>

class Lua_State;
class DiceEvent;
bool lua_msg_call(DiceEvent*, const AttrVar&);
bool lua_call_event(AttrObject eve, const AttrVar&);
bool lua_call_task(const AttrVars&);