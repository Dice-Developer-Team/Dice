#pragma once
#ifndef _APPINFO_
#define _APPINFO_
#ifdef _DEBUG
#pragma comment(lib,"..\\..\\res\\SDK_Debug.lib")
#else
#pragma comment(lib,"..\\..\\res\\SDK_Release.lib")
#endif /*_DEBUG*/

#define CQAPPID "com.w4123.Dice" //请修改AppID，规则见 http://d.cqp.me/Pro/开发/基础信息
#define CQAPPINFO CQAPIVERTEXT "," CQAPPID
#endif /*_APPINFO_*/
