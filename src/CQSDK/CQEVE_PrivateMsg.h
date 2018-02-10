#pragma once
#include "CQEVE.h"//不能删除此行...
//#include "CQEVEMsg.h"
//#include "CQTools.h"
#include "CQMsgSend.h"

/*
私聊消息(Type=21)

此函数具有以下参数
subType		子类型，11/来自好友 1/来自在线状态 2/来自群 3/来自讨论组
msgId	消息ID
fromQQ		来源QQ
msg			消息内容
font		字体

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_PrivateMsg_EX(Name)																	\
	void Name(CQ::EVEPrivateMsg & eve);															\
	EVE_PrivateMsg(Name)																		\
	{																							\
		CQ::EVEPrivateMsg tep(subType, msgId, fromQQ, msg, font);							\
		EVETry Name(tep); EVETryEnd(Name,发生了一个错误)										\
		return tep._EVEret;																		\
	}																							\
	void Name(CQ::EVEPrivateMsg & eve)


namespace CQ{
    struct EVEPrivateMsg :public EVEMsg
    {

		EVEPrivateMsg(int subType, int msgId, long long fromQQ, const char* msg, int Font);

        //来自好友
		bool fromPrivate() const;

        //来自在线状态
		bool fromOnlineStatus() const;

        //来自群临时
		bool fromGroup() const;

        //来自讨论组临时
		bool fromDiscuss() const;

		// 通过 EVEMsg 继承
		virtual msg sendMsg() const override;

		virtual int sendMsg(const char *) const override;

		virtual int sendMsg(std::string) const override;

	};
}
