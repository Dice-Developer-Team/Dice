/*
此文件是所有EX事件的实现
*/
#include "CQEVE_ALL.h"

#include "CQAPI_EX.h"
#include "CQTools.h"
#include "Unpack.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace CQ;

void EVE::message_ignore() { _EVEret = Msg_Ignored; }
void EVE::message_block() { _EVEret = Msg_Blocked; }

bool EVEMsg::isSystem() const { return fromQQ == 1000000; }

Font::Font(const int Font)
{
	RtlMoveMemory(static_cast<void*>(this), reinterpret_cast<const void *>(Font), 20);
}

EVEMsg::EVEMsg(const int subType, const int msgId, const long long fromQQ, std::string message, const int font)
	: subType(subType), msgId(msgId), fromQQ(fromQQ), message(move(message)), font(font)
{
}

//真实用户
bool EVEMsg::isUser() const
{
	switch (fromQQ)
	{
	case 1000000: // 系统提示
	case 80000000: // 匿名
		return false;
	default:
		return true;
	}
}

EVEGroupMsg::EVEGroupMsg(const int subType, const int msgId, const long long fromGroup, const long long fromQQ, const char* fromAnonymous,
                         const char* msg, const int font)
	: EVEMsg(subType, msgId, fromQQ, msg, font), fromAnonymousInfo(), fromGroup(fromGroup),
	  fromAnonymousToken(fromAnonymous)
{
}

EVEGroupMsg::~EVEGroupMsg() { delete fromAnonymousInfo; }


bool EVEGroupMsg::isAnonymous() const { return fromQQ == 80000000; }


AnonymousInfo& EVEGroupMsg::getFromAnonymousInfo() //throw(std::exception_ptr)
{
	if (isAnonymous())
		return
			fromAnonymousInfo != nullptr
				? *fromAnonymousInfo
				: *(fromAnonymousInfo = new AnonymousInfo(fromAnonymousToken));
	throw std::exception_ptr();
}

bool EVEGroupMsg::setGroupKick(const bool refusedAddAgain) const
{
	return !CQ::setGroupKick(fromGroup, fromQQ, refusedAddAgain);
}

bool EVEGroupMsg::setGroupBan(const long long banTime) const
{
	if (isAnonymous())
	{
		return !setGroupAnonymousBan(fromGroup, fromAnonymousToken, banTime);
	}
	return !CQ::setGroupBan(fromGroup, fromQQ, banTime);
}

bool EVEGroupMsg::setGroupAdmin(const bool isAdmin) const
{
	return !CQ::setGroupAdmin(fromGroup, fromQQ, isAdmin);
}

bool EVEGroupMsg::setGroupSpecialTitle(const std::string& Title, const long long ExpireTime) const
{
	return !CQ::setGroupSpecialTitle(fromGroup, fromQQ, Title, ExpireTime);
}

bool EVEGroupMsg::setGroupWholeBan(const bool isBan) const
{
	return CQ::setGroupWholeBan(fromGroup, isBan) != 0;
}

bool EVEGroupMsg::setGroupAnonymous(const bool enableAnonymous) const
{
	return CQ::setGroupAnonymous(fromGroup, enableAnonymous) != 0;
}

bool EVEGroupMsg::setGroupCard(const std::string& newGroupNick) const
{
	return CQ::setGroupCard(fromGroup, fromQQ, newGroupNick) != 0;
}

bool EVEGroupMsg::setGroupLeave(const bool isDismiss) const
{
	return CQ::setGroupLeave(fromGroup, isDismiss) != 0;
}

GroupMemberInfo EVEGroupMsg::getGroupMemberInfo(const bool disableCache) const
{
	return CQ::getGroupMemberInfo(fromGroup, fromQQ, disableCache);
}

std::vector<GroupMemberInfo> EVEGroupMsg::getGroupMemberList() const
{
	return CQ::getGroupMemberList(fromGroup);
}

EVEPrivateMsg::EVEPrivateMsg(const int subType, const int msgId, const long long fromQQ, const char* msg, const int font)
	: EVEMsg(subType, msgId, fromQQ, msg, font)
{
}

//来自好友
bool EVEPrivateMsg::fromPrivate() const { return subType == 11; }

//来自在线状态
bool EVEPrivateMsg::fromOnlineStatus() const { return subType == 1; }

//来自群临时
bool EVEPrivateMsg::fromGroup() const { return subType == 2; }

//来自讨论组临时
bool EVEPrivateMsg::fromDiscuss() const { return subType == 3; }

msg EVEPrivateMsg::sendMsg() const { return msg(fromQQ, Friend); }
msg EVEGroupMsg::sendMsg() const { return msg(fromGroup, Group); }
msg EVEDiscussMsg::sendMsg() const { return msg(fromQQ, Discuss); }

int EVEPrivateMsg::sendMsg(const char* msg) const { return sendPrivateMsg(fromQQ, msg); }
int EVEPrivateMsg::sendMsg(const std::string& msg) const { return sendPrivateMsg(fromQQ, msg); }
int EVEGroupMsg::sendMsg(const char* msg) const { return sendGroupMsg(fromGroup, msg); }
int EVEGroupMsg::sendMsg(const std::string& msg) const { return sendGroupMsg(fromGroup, msg); }
int EVEDiscussMsg::sendMsg(const char* msg) const { return sendDiscussMsg(fromDiscuss, msg); }
int EVEDiscussMsg::sendMsg(const std::string& msg) const { return sendDiscussMsg(fromDiscuss, msg); }

EVEDiscussMsg::EVEDiscussMsg(const int subType, const int msgId, const long long fromDiscuss, const long long fromQQ, const char* msg, const int font)
	: EVEMsg(subType, msgId, fromQQ, msg, font), fromDiscuss(fromDiscuss)
{
}

bool EVEDiscussMsg::leave() const { return !setDiscussLeave(fromDiscuss); }

void EVEStatus::color_green() { color = 1; }
void EVEStatus::color_orange() { color = 2; }
void EVEStatus::color_red() { color = 3; }
void EVEStatus::color_crimson() { color = 4; }
void EVEStatus::color_black() { color = 5; }
void EVEStatus::color_gray() { color = 6; }

std::string CQ::statusEVEreturn(EVEStatus& eve)
{
	Unpack pack;
	std::string _ret = pack.add(eve.data).add(eve.dataf).add(eve.color).getAll();
	_ret = base64_encode(_ret);
	return _ret;
}

EVERequest::EVERequest(const int sendTime, const long long fromQQ, const char* msg, const char* responseFlag)
	: sendTime(sendTime), fromQQ(fromQQ), msg(msg), responseFlag(responseFlag)
{
}

EVERequestAddFriend::EVERequestAddFriend(const int subType, const int sendTime, const long long fromQQ, const char* msg,
                                         const char* responseFlag)
	: EVERequest(sendTime, fromQQ, msg, responseFlag), subType(subType), fromGroup(0)
{
}

void EVERequestAddFriend::pass(const std::string& msg) const
{
	setFriendAddRequest(responseFlag, RequestAccepted, msg.c_str());
}

void EVERequestAddFriend::fail(const std::string& msg) const
{
	setFriendAddRequest(responseFlag, RequestRefused, msg.c_str());
}

EVERequestAddGroup::EVERequestAddGroup(const int subType, const int sendTime, const long long fromGroup, const long long fromQQ,
                                       const char* const msg, const char* const responseFlag)
	: EVERequest(sendTime, fromQQ, msg, responseFlag), subType(subType), fromGroup(fromGroup)
{
}

void EVERequestAddGroup::pass(const std::string& msg) const
{
	setGroupAddRequest(responseFlag, subType, RequestAccepted, msg.c_str());
}

void EVERequestAddGroup::fail(const std::string& msg) const
{
	setGroupAddRequest(responseFlag, subType, RequestRefused, msg.c_str());
}

AnonymousInfo::AnonymousInfo(const char* msg)
{
	if (msg != nullptr && msg[0] != '\0')
	{
		Unpack p(base64_decode(msg));
		AID = p.getLong();
		AnonymousNick = p.getstring();
	}
}

regexMsg::regexMsg(const std::string& msg)
{
	Unpack msgs(base64_decode(msg));
	auto len = msgs.getInt(); //获取参数数量
	while (len-- > 0)
	{
		auto tep = msgs.getUnpack();
		const auto key = tep.getstring();
		const auto value = tep.getstring();
		if (key == "")
		{
			return;
		}
		regexMap[key] = value;
	}
}

std::string regexMsg::get(const std::string& key)
{
	return regexMap[key];
}

std::string regexMsg::operator[](const std::string& key)
{
	return regexMap[key];
}
