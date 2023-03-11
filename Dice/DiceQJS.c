#include "DiceQJS.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

long long js_toBigInt(JSContext* ctx, JSValueConst val) {
	int64_t num = 0;
	JS_ToBigInt64(ctx, &num, val);
	return (long long)num;
}
double js_toDouble(JSContext* ctx, JSValueConst val) {
	double num = 0;
	JS_ToFloat64(ctx, &num, val);
	return num;
}

const JSCFunctionListEntry js_dice_funcs[] = {
	JS_CFUNC_DEF("log",2,js_dice_log),
	JS_CFUNC_DEF("getDiceID",0,js_dice_getDiceID),
};
JSClassID js_dice_context_id;
JSClassExoticMethods js_dice_context_methods = {
	.get_own_property = js_dice_context_get_own,
	.delete_property = js_dice_context_delete,
	.get_property = js_dice_context_get,
};
JSClassDef js_dice_context_class = {
	"Context",
	.finalizer = js_dice_context_finalizer,
	.exotic = &js_dice_context_methods,
}; 
const JSCFunctionListEntry js_dice_context_proto_funcs[1] = {
	JS_CFUNC_DEF("echo",2,js_dice_context_echo),
};
static int js_dice_init(JSContext* ctx, JSModuleDef* m) {
	JSValue proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, proto, js_dice_context_proto_funcs,
		countof(js_dice_context_proto_funcs));
	JS_SetClassProto(ctx, js_dice_context_id, proto);
	return JS_SetModuleExportList(ctx, m, js_dice_funcs,
		countof(js_dice_funcs));
}
JSModuleDef* js_init_module_dice(JSContext* ctx, const char* module_name) {
	JSModuleDef* m = JS_NewCModule(ctx, module_name, js_dice_init);
	if (m) {
		JS_AddModuleExport(ctx, m, "Context");
		JS_AddModuleExportList(ctx, m, js_dice_funcs, countof(js_dice_funcs));
		return m;
	}
	return NULL;
}
void js_init(JSRuntime* rt, JSContext* ctx) {
	js_std_add_helpers(ctx, 0, NULL);
	/* system modules */
	js_init_module_std(ctx, "std");
	js_init_module_os(ctx, "os");
	js_init_module_dice(ctx, "dice");
	JSValue proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, proto, js_dice_context_proto_funcs,
		countof(js_dice_context_proto_funcs));
	JS_SetClassProto(ctx, js_dice_context_id, proto);
}
JSValue js_newException() {
	return JS_EXCEPTION;
}
JSValue js_newUndefined() {
	return JS_UNDEFINED;
}