#pragma once
#include <string>
#include "CQEVERequest.h"

/*
请求-群添加(Type=302)

subtype 子类型，1/他人申请入群 2/自己(即登录号)受邀入群
sendTime 发送时间(时间戳)
fromGroup 来源群号
fromQQ 来源QQ
msg 附言
responseFlag 反馈标识(处理请求用)

如果 subtype ＝ 1
置群添加请求 (responseFlag, #请求_群添加, #请求_通过)
如果 subtype ＝ 2
置群添加请求 (responseFlag, #请求_群邀请, #请求_通过)

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_Request_AddGroup_EX(Name) \
	void Name(CQ::EVERequestAddGroup & eve);\
	EVE_Request_AddGroup(Name)\
	{\
		CQ::EVERequestAddGroup tep(subType, sendTime, fromGroup, fromQQ, msg, responseFlag);\
		Name(tep);\
		return tep._EVEret;\
	}\
	void Name(CQ::EVERequestAddGroup & eve)

namespace CQ
{
	struct EVERequestAddGroup final : EVERequest
	{
		//子类型
		//1:他人申请入群
		//2:自己(即登录号)受邀入群
		int subType;
		long long fromGroup; // 来源群号
		EVERequestAddGroup(int subType, int sendTime, long long fromGroup, long long fromQQ, const char* msg,
		                   const char* responseFlag);
		void pass(const std::string& msg = "") const override; //通过此请求
		void fail(const std::string& msg = "您由于不满足某些要求被拒绝!") const override; //拒绝此请求
	};
}
