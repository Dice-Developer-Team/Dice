#pragma once
namespace QQ {
#if (defined _WIN32 && !defined _WIN64)
#ifdef _MSC_VER
#define QQEVENT(ReturnType, Name, Size) __pragma(comment(linker, "/EXPORT:" #Name "=_" #Name "@" #Size))\
	extern "C" __declspec(dllexport) ReturnType __stdcall Name
#else
#define QQEVENT(ReturnType, Name, Size)\
	extern "C" __attribute__((dllexport)) ReturnType __attribute__((__stdcall__)) Name
#endif /*_MSC_VER*/
#elif defined _WIN64
#ifdef _MSC_VER
#define QQEVENT(ReturnType, Name, Size)\
	extern "C" __declspec(dllexport) ReturnType __stdcall Name
#else
#define QQEVENT(ReturnType, Name, Size)\
	extern "C" __attribute__((dllexport)) Name
#endif /*_MSC_VER*/
#else
#define QQEVENT(ReturnType, Name, Size)\
	extern "C" __attribute__((visibility ("default"))) ReturnType Name
#endif

/*
初始化事件，加载api并获取QQ
*/
#define EVE_Startup(Name) QQEVENT(void, Name, 12)(void* initApi, long long botQQ)
/*
启用Dice
*/
#define EVE_Enable(Name) QQEVENT(void, Name, 0)()
#define EVE_Disable(Name) QQEVENT(void, Name, 0)()
#define EVE_Exit(Name) QQEVENT(void, Name, 0)()
#define EVE_PrivateMsg(Name) QQEVENT(int, Name, 16)(int msgId, long long fromQQ, const char* message)
#define EVE_GroupMsg(Name) QQEVENT(int, Name, 24)(int msgId, long long fromGroup, long long fromQQ, const char* message)
#define EVE_DiscussMsg(Name) QQEVENT(int, Name, 24)(int msgId, long long fromDiscuss, long long fromQQ, const char* message)
#define EVE_GroupMemberKicked(Name) QQEVENT(int, Name, 24)(long long fromGroup, long long beingOperateQQ, long long fromQQ)
#define EVE_GroupMemberIncrease(Name) QQEVENT(int, Name, 24)(long long fromGroup, long long fromQQ, long long operatorQQ)
#define EVE_GroupBan(Name) QQEVENT(int, Name, 28)(long long fromGroup, long long beingOperateQQ, long long operatorQQ, const char* duration)
#define EVE_GroupInvited(Name) QQEVENT(int, Name, 16)(long long fromGroup, long long fromQQ)
#define EVE_FriendRequest(Name) QQEVENT(int, Name, 12)(long long fromQQ, const char* message)
#define EVE_FriendAdded(Name) QQEVENT(int, Name, 8)(long long fromQQ)

//菜单，待设置
#define EVE_Menu(Name) QQEVENT(int, Name, 0)()
//悬浮窗，暂时无用
//#define EVE_Status_EX(Name) QQEVENT(const char*, Name, 0)()
}