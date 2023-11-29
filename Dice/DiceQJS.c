#include "DiceQJS.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

long long js_toLongLong(JSContext* ctx, JSValueConst val) {
	int64_t num = 0;
	JS_ToInt64(ctx, &num, val);
	return (long long)num;
}
double js_toDouble(JSContext* ctx, JSValueConst val) {
	double num = 0;
	JS_ToFloat64(ctx, &num, val);
	return num;
}
JSClassID js_dice_Set_id;
JSClassExoticMethods js_dice_Set_methods = {
	.get_own_property = js_dice_Set_get_own,
	.delete_property = js_dice_Set_delete,
};
JSClassDef js_dice_Set_class = {
	"Set",
	.finalizer = js_dice_Set_finalizer,
	.exotic = &js_dice_Set_methods,
};
#define JS_DICEDEF(name,len) JS_CFUNC_DEF(#name,len,js_dice_##name)
const JSCFunctionListEntry js_dice_funcs[] = {
	JS_DICEDEF(log,2),
	JS_DICEDEF(loadJS,1),
	JS_DICEDEF(getDiceID,0),
	JS_DICEDEF(getDiceDir,0),
	JS_DICEDEF(getSelfData,1),
	JS_DICEDEF(eventMsg,3),
	JS_DICEDEF(sendMsg,3),
	JS_DICEDEF(getGroupAttr,3),
	JS_DICEDEF(setGroupAttr,3),
	JS_DICEDEF(getUserAttr,3),
	JS_DICEDEF(setUserAttr,3),
	JS_DICEDEF(getUserToday,3),
	JS_DICEDEF(setUserToday,3),
	JS_DICEDEF(getPlayerCard,2),
	JS_DICEDEF(callOneBot,1),
};
JSClassID js_dice_context_id;
JSClassExoticMethods js_dice_context_methods = {
	.get_own_property = js_dice_context_get_own,
	.get_own_property_names = js_dice_context_get_keys,
	.delete_property = js_dice_context_delete,
	.define_own_property = js_dice_context_define,
};
JSClassDef js_dice_context_class = {
	"Context",
	.finalizer = js_dice_context_finalizer,
	.exotic = &js_dice_context_methods,
}; 
const JSCFunctionListEntry js_dice_Context_proto_funcs[] = {
	JS_CFUNC_DEF("get",2,js_dice_context_get),
	JS_CFUNC_DEF("format",2,js_dice_context_format),
	JS_CFUNC_DEF("echo",2,js_dice_context_echo),
	JS_CFUNC_DEF("inc",2,js_dice_context_inc),
};
JSClassID js_dice_selfdata_id;
JSClassExoticMethods js_dice_selfdata_methods = {
	.get_own_property = js_dice_selfdata_get_own,
	.delete_property = js_dice_selfdata_delete,
	.define_own_property = js_dice_selfdata_define,
	.set_property = js_dice_selfdata_set,
};
JSClassDef js_dice_selfdata_class = {
	"SelfData",
	.finalizer = js_dice_selfdata_finalizer,
	.exotic = &js_dice_selfdata_methods,
};
const JSCFunctionListEntry js_dice_selfdata_proto_funcs[] = {
	JS_CFUNC_DEF("append",2,js_dice_selfdata_append),
};
JSClassID js_dice_GameTable_id;
JSClassExoticMethods js_dice_GameTable_methods = {
	.get_own_property = js_dice_GameTable_get_own,
	.delete_property = js_dice_GameTable_delete,
	.define_own_property = js_dice_GameTable_define,
};
JSClassDef js_dice_GameTable_class = {
	"GameTable",
	.finalizer = js_dice_GameTable_finalizer,
	.exotic = &js_dice_GameTable_methods,
};
const JSCFunctionListEntry js_dice_GameTable_proto_funcs[] = {
	JS_CFUNC_DEF("message",1,js_dice_GameTable_message),
};
JSClassID js_dice_actor_id;
JSClassExoticMethods js_dice_actor_methods = {
	.get_own_property = js_dice_actor_get_own,
	.get_own_property_names = js_dice_actor_get_keys,
	.delete_property = js_dice_actor_delete,
	.define_own_property = js_dice_actor_define,
};
JSClassDef js_dice_actor_class = {
	"Actor",
	.finalizer = js_dice_actor_finalizer,
	.exotic = &js_dice_actor_methods,
}; 
const JSCFunctionListEntry js_dice_actor_proto_funcs[] = {
	JS_CFUNC_DEF("set",2,js_dice_actor_set),
	JS_CFUNC_DEF("rollDice",1,js_dice_actor_rollDice),
	JS_CFUNC_DEF("locked",1,js_dice_actor_locked),
	JS_CFUNC_DEF("lock",1,js_dice_actor_lock),
	JS_CFUNC_DEF("unlock",1,js_dice_actor_unlock),
};
/*
static int js_dicemaid_init(JSContext* ctx, JSModuleDef* m) {
	return JS_SetModuleExportList(ctx, m, js_dice_funcs, countof(js_dice_funcs));
}
JSModuleDef* js_init_module_dice(JSContext* ctx, const char* module_name) {
	JSModuleDef* m = JS_NewCModule(ctx, module_name, js_dicemaid_init);
	if (m) {
		JS_AddModuleExport(ctx, m, "Context");
		//JS_AddModuleExportList(ctx, m, js_dice_funcs, countof(js_dice_funcs));
		return m;
	}
	return NULL;
}
*/