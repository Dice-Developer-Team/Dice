#include "QQEvent.h"
#include "DDAPI.h"
using namespace QQ;

EVE_Startup(eventStartUp) {
	if (!botQQ)return;
	loginQQ = botQQ;
	DD::ApiList = reinterpret_cast<const api_list&(*)()>(initApi)();
	DD::debugLog("Dice" + std::to_string(botQQ) + ".init");
}