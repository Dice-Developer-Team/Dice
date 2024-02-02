/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123ËÝä§
 * Copyright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs-libc.h"
	long long js_toLongLong(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
#ifdef _MSC_VER
#define QJSDEF(name) __declspec(dllexport) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#else
#define QJSDEF(name) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#endif
	extern JSClassID js_dice_Set_id;
	extern JSClassDef js_dice_Set_class;
	QJSDEF(Set_constructor);
	void js_dice_Set_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_Set_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_Set_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	extern const JSCFunctionListEntry js_dice_funcs[14];
	QJSDEF(log);
	QJSDEF(loadJS);
	QJSDEF(getDiceID);
	QJSDEF(getDiceDir);
	QJSDEF(getSelfData);
	QJSDEF(eventMsg);
	QJSDEF(sendMsg);
	QJSDEF(getGroupAttr);
	QJSDEF(setGroupAttr);
	QJSDEF(getUserAttr);
	QJSDEF(setUserAttr);
	QJSDEF(getUserToday);
	QJSDEF(setUserToday);
	QJSDEF(getPlayerCard);
	//Context
	extern JSClassID js_dice_context_id;
	extern JSClassDef js_dice_context_class;
	void js_dice_context_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_context_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);
	int js_dice_context_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	int js_dice_context_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags);
	QJSDEF(context_get);
	QJSDEF(context_format);
	QJSDEF(context_echo);
	QJSDEF(context_inc);
	extern const JSCFunctionListEntry js_dice_Context_proto_funcs[4];
	extern JSClassID js_dice_selfdata_id;
	extern JSClassDef js_dice_selfdata_class;
	QJSDEF(selfdata_constructor);
	void js_dice_selfdata_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_selfdata_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_selfdata_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	int js_dice_selfdata_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags); 
	int js_dice_selfdata_set(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags);
	QJSDEF(selfdata_append);
	extern const JSCFunctionListEntry js_dice_selfdata_proto_funcs[1];
	//GameTable
	extern JSClassID js_dice_GameTable_id;
	extern JSClassDef js_dice_GameTable_class;
	void js_dice_GameTable_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_GameTable_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_GameTable_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	int js_dice_GameTable_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags);
	extern const JSCFunctionListEntry js_dice_GameTable_proto_funcs[1];
	QJSDEF(GameTable_message);
	//Actor
	extern JSClassID js_dice_actor_id;
	extern JSClassDef js_dice_actor_class;
	void js_dice_actor_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_actor_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_actor_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);
	int js_dice_actor_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	int js_dice_actor_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags);
	extern const JSCFunctionListEntry js_dice_actor_proto_funcs[5];
	QJSDEF(actor_set);
	QJSDEF(actor_rollDice);
	QJSDEF(actor_locked);
	QJSDEF(actor_lock);
	QJSDEF(actor_unlock);
#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#ifdef __cplusplus
#define JS2SET(val) AttrSet& set = *(AttrSet*)JS_GetOpaque(val, js_dice_Set_id)
#define JS2OBJ(val) ptr<AnysTable>& obj = *(ptr<AnysTable>*)JS_GetOpaque(val, js_dice_context_id)
#define JS2DATA(val) ptr<SelfData>& data = *(ptr<SelfData>*)JS_GetOpaque(val, js_dice_selfdata_id)
#define JS2GAME(val) ptr<DiceSession>& game = *(ptr<DiceSession>*)JS_GetOpaque(val, js_dice_GameTable_id)
#define JS2PC(val) PC& pc = *(PC*)JS_GetOpaque(val, js_dice_actor_id)
}
#endif