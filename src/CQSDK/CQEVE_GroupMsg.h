#pragma once
//#include <exception>
#include "CQEVE.h"//不能删除此行...
#include "CQEVEMsg.h"

#include <vector>
/*
群消息(Type=2)

subType 子类型，目前固定为1
msgId 消息ID
fromGroup 来源群号
fromQQ 来源QQ号
fromAnonymous 来源匿名者
msg 消息内容
font 字体

如果消息来自匿名者,isAnonymous() 返回 true, 可使用 getFromAnonymousInfo() 获取 匿名者信息

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_GroupMsg_EX(Name) 																	\
	void Name(CQ::EVEGroupMsg & eve);															\
	EVE_GroupMsg(Name)																		\
	{																							\
		CQ::EVEGroupMsg tep(subType, msgId, fromGroup, fromQQ, fromAnonymous, msg, font);	\
		EVETry Name(tep); EVETryEnd(Name,发生了一个错误)										\
		return tep._EVEret;																		\
	}																							\
	void Name(CQ::EVEGroupMsg & eve)

namespace CQ{
	class GroupMemberInfo;

	// 群匿名信息
	struct AnonymousInfo
	{
		long long AID;
		std::string 代号;
		//std::vector<unsigned char> Token;

		AnonymousInfo(const char* msg);
		AnonymousInfo() = default;
	};

	//群事件
    struct EVEGroupMsg :public EVEMsg
    {
	private:
		AnonymousInfo* fromAnonymousInfo;
	public:
        //群号
        long long fromGroup;
        //禁言用的令牌
        const char* fromAnonymousToken;
		EVEGroupMsg(int subType, int msgId, long long fromGroup, long long fromQQ, const char* fromAnonymous, const char* msg, int Font);

		virtual ~EVEGroupMsg();
		
		bool isAnonymous() const;

		// 通过 EVEMsg 继承
		 int sendMsg(const char *) const override;
		 int sendMsg(std::string) const override;
		 msg sendMsg() const override;

		//获取匿名者信息
		AnonymousInfo&getFromAnonymousInfo() /*throw(std::exception_ptr)*/;

		//置群员移除
		bool setGroupKick(bool refusedAddAgain = false);
		//置群员禁言
		//自动判断是否是匿名
		bool setGroupBan(long long banTime = 60);
		//置群管理员
		bool setGroupAdmin(bool isAdmin);
		//置群成员专属头衔
		bool setGroupSpecialTitle(std::string 专属头衔, long long 过期时间 = -1);

		//置全群禁言
		bool setGroupWholeBan(bool 开启禁言 = true);
		//置群匿名设置
		bool setGroupAnonymous(bool 开启匿名);
		//置群成员名片
		bool setGroupCard(std::string 新名片_昵称);
		//置群退出
		bool setGroupLeave(bool 是否解散);
		//取群成员信息 (支持缓存)
		GroupMemberInfo getGroupMemberInfo(bool 不使用缓存 = false);
		//取群成员列表
		std::vector<GroupMemberInfo> getGroupMemberList();

	};
}
