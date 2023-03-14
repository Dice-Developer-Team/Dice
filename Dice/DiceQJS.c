#include "DiceQJS.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

long long js_toBigInt(JSContext* ctx, JSValueConst val) {
	int64_t num = 0;
	JS_ToInt64(ctx, &num, val);
	return (long long)num;
}
double js_toDouble(JSContext* ctx, JSValueConst val) {
	double num = 0;
	JS_ToFloat64(ctx, &num, val);
	return num;
}
#define JS_DICEDEF(name,len) JS_CFUNC_DEF(#name,2,js_dice_##name)
const JSCFunctionListEntry js_dice_funcs[] = {
	JS_DICEDEF(log,2),
	JS_DICEDEF(getDiceID,0),
	JS_DICEDEF(getDiceDir,0),
	JS_DICEDEF(eventMsg,3),
	JS_DICEDEF(sendMsg,3),
	JS_DICEDEF(getGroupAttr,3),
	JS_DICEDEF(setGroupAttr,3),
	JS_DICEDEF(getUserAttr,3),
	JS_DICEDEF(setUserAttr,3),
	JS_DICEDEF(getUserToday,3),
	JS_DICEDEF(setUserToday,3),
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
const JSCFunctionListEntry js_dice_context_proto_funcs[4] = {
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
const JSCFunctionListEntry js_dice_selfdata_proto_funcs[1] = {
	JS_CFUNC_DEF("append",2,js_dice_selfdata_append),
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