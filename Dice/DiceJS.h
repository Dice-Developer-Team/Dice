/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * CoJSright (C) 2018-2021 w4123ËÝä§
 * CoJSright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a coJS of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "DiceAttrVar.h"
class JSRuntime;
class JSContext;
typedef uint64_t JSValue;
class DiceEvent;
class js_context {
	JSContext* ctx;
public:
	js_context();
	~js_context();
	operator JSContext* () { return ctx; }
	string getException();
	AttrVar getValue(JSValue);
	void setContext(const std::string&, const ptr<AnysTable>& context);
	JSValue evalString(const std::string& s, const string& title);
	JSValue evalStringLocal(const std::string& s, const string& title, const ptr<AnysTable>& context);
	JSValue evalFile(const std::filesystem::path&);
	JSValue evalFileLocal(const std::string& s, const ptr<AnysTable>& context);
};
JSValue js_newAttr(JSContext*, const AttrVar& var);
void js_global_init();
void js_global_end();
bool js_call_event(const ptr<AnysTable>&, const AttrVar&);
void js_msg_call(DiceEvent*, const AttrVar&);
AttrVar js_simple_eval(const std::string&);
AttrVar js_context_eval(const std::string&, const ptr<AnysTable>& context);