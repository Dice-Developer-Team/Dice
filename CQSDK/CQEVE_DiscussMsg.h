#pragma once

//#include "CQEVE.h"//不能删除此行...
#include "CQMsgSend.h"

/*
讨论组消息(Type=4)

subtype		子类型，目前固定为1
msgId	消息ID
fromDiscuss	来源讨论组
fromQQ		来源QQ号
msg			消息内容
font		字体

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_DiscussMsg_EX(Name)																	\
	void Name(CQ::EVEDiscussMsg & eve);															\
	EVE_DiscussMsg(Name)																		\
	{																							\
		CQ::EVEDiscussMsg tep(subType, msgId, fromDiscuss, fromQQ, msg, font);					\
		EVETry Name(tep); EVETryEnd(Name,发生了一个错误)										\
		return tep._EVEret;																		\
	}																							\
	void Name(CQ::EVEDiscussMsg & eve)


namespace CQ {
	struct EVEDiscussMsg :public EVEMsg
	{
		long long fromDiscuss;//讨论组号

		EVEDiscussMsg(int subType, int msgId, long long fromDiscuss, long long fromQQ, const char* msg, int font);

		bool leave() const;//退出讨论组

		// 通过 EVEMsg 继承
		virtual msg sendMsg() const override;
		virtual int sendMsg(const char *) const override;
		virtual int sendMsg(std::string) const override;
	};
}