/**
 * lua脚本嵌入
 * 用于自定义前缀指令等
 * Copyright (C) 2019-2021 String.Empty
 */
#pragma once
#include <string>
#include <unordered_map>

class Lua_State;
class FromMsg;
bool lua_msg_order(FromMsg*, const char*, const char*);
bool lua_msg_reply(FromMsg*, const string&);
bool lua_call_event(AttrObject eve, const string&);
bool lua_call_task(const char*, const char*);
int lua_readStringTable(const char*, const char*, std::unordered_map<std::string, std::string>&);