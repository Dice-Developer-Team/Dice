#include "quickjs-libc.h"
#include "DiceQJS.h"

const JSCFunctionListEntry js_dice_funcs[] = {
	JS_CFUNC_DEF("log",0,js_dice_log),
	JS_CFUNC_DEF("getDiceID",0,js_dice_getDiceID),
};