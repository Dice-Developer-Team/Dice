#pragma once
#include <string>
#include "CQEVERequest.h"

/*
请求-好友添加(Type=301)

subtype 子类型，目前固定为1
sendTime 发送时间(时间戳)
fromQQ 来源QQ
msg 附言
responseFlag 反馈标识(处理请求用)

置好友添加请求 (responseFlag, #请求_通过)

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_Request_AddFriend_EX(Name) \
	void Name(CQ::EVERequestAddFriend & eve);\
	EVE_Request_AddFriend(Name)\
	{\
		CQ::EVERequestAddFriend tep(subType, sendTime, fromGroup, fromQQ, msg, responseFlag);\
		Name(tep);\
		return tep._EVEret;\
	}\
	void Name(CQ::EVERequestAddFriend & eve)

namespace CQ
{
	struct EVERequestAddFriend final : EVERequest
	{
		//子类型
		//1:固定为1
		int subType;
		long long fromGroup; // 来源群号
		EVERequestAddFriend(int subType, int sendTime, long long fromQQ, const char* msg, const char* responseFlag);
		void pass(const std::string& msg = "") const override; //通过此请求
		void fail(const std::string& msg = "您由于不满足某些要求被拒绝!") const override; //拒绝此请求
	};
}
