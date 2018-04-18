#pragma once
#ifndef _APPINFO_
#define _APPINFO_
#ifdef _DEBUG
#pragma comment(lib,"..\\SDK_Debug.lib")
#else
#pragma comment(lib,"..\\SDK_Release.lib")
#endif /*_DEBUG*/

#define CQAPPID "com.w4123.Dice" 
#define CQAPPINFO CQAPIVERTEXT "," CQAPPID
#endif /*_APPINFO_*/
