#include "DiceQJS.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

const JSCFunctionListEntry js_dice_funcs[] = {
	JS_CFUNC_DEF("log",0,js_dice_log),
	JS_CFUNC_DEF("getDiceID",0,js_dice_getDiceID),
};

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

static int js_dice_init(JSContext* ctx, JSModuleDef* m) {
	return JS_SetModuleExportList(ctx, m, js_dice_funcs,
		countof(js_dice_funcs));
}
JSModuleDef* js_init_module_dice(JSContext* ctx, const char* module_name) {
	JSModuleDef* m = JS_NewCModule(ctx, module_name, js_dice_init);
	if (m) {
		JS_AddModuleExportList(ctx, m, js_dice_funcs, countof(js_dice_funcs));
		return m;
	}
	return NULL;
}
void js_init(JSRuntime* rt, JSContext* ctx) {
	js_std_init_handlers(rt);

	/* loader for ES6 modules */
	JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

	js_std_add_helpers(ctx, 0, NULL);

	/* system modules */
	js_init_module_std(ctx, "std");
	js_init_module_os(ctx, "os");
	js_init_module_dice(ctx, "dice");
}
JSValue js_newException() {
	return JS_EXCEPTION;
}
JSValue js_newUndefined() {
	return JS_UNDEFINED;
}