#pragma once
#ifndef __BOTENVIRONMENT__
#define __BOTENVIRONMENT__

// 如果你在使用Mirai环境，设置全局__MIRAI__宏或取消下面一句的注释
// #define __MIRAI__


#ifdef __MIRAI__
#pragma comment(lib, "CQP_Mirai.lib")
#else
#pragma comment(lib, "CQP.lib")
#endif

#endif