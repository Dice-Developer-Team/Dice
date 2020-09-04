#include "CQAPI_EX.h"

#include <ctime>


#include "CQAPI.h"
#include "Unpack.h"
#include "CQEVE_GroupMsg.h"
#include "CQTools.h"

using namespace CQ;
using namespace std;
int lasterr;

StrangerInfo::StrangerInfo(const char* msg)
{
	if (msg != nullptr && msg[0] != '\0')
	{
		Unpack p(base64_decode(msg));
		QQID = p.getLong();
		nick = p.getstring();
		sex = p.getInt();
		age = p.getInt();
	}
}

string StrangerInfo::tostring() const
{
	return string("{")
		+ "QQ:" + to_string(QQID)
		+ " ,昵称:" + nick
		+ " ,性别:" + (sex == 255 ? "未知" : sex == 1 ? "男" : "女")
		+ " ,年龄:" + to_string(age)
		+ "}";
}

void GroupInfo::setdata(Unpack& u)
{
	llGroup = u.getLong();
	strGroupName = u.getstring();
	nGroupSize = u.getInt(); // 群人数
	nMaxGroupSize = u.getInt(); //群规模
	nFriendCnt = u.getInt(); //好友数
}

GroupInfo::GroupInfo(long long group)
{
	const char* data = CQ_getGroupInfo(getAuthCode(), group, true);
	if (!data || data[0] == '\0')return;
	Unpack pack(base64_decode(data));
	if (!pack.len())return;
	setdata(pack);
}

std::string GroupInfo::tostring() const
{
	return strGroupName + "(" + std::to_string(llGroup) + ")[" + std::to_string(nGroupSize) + "/" +
		std::to_string(nMaxGroupSize) + "]";
}

void GroupMemberInfo::setdata(Unpack& u)
{
	Group = u.getLong();
	QQID = u.getLong();
	Nick = u.getstring();
	GroupNick = u.getstring();
	Gender = u.getInt();
	Age = u.getInt();
	Region = u.getstring();
	AddGroupTime = u.getInt();
	LastMsgTime = u.getInt();
	LevelName = u.getstring();
	permissions = u.getInt();
	NaughtyRecord = u.getInt() == 1;
	Title = u.getstring();
	ExpireTime = u.getInt();
	canEditGroupNick = u.getInt() == 1;
}

GroupMemberInfo::GroupMemberInfo(Unpack& msg) { setdata(msg); }

GroupMemberInfo::GroupMemberInfo(const char* msg)
{
	if (msg != nullptr && msg[0] != '\0')
	{
		Unpack u(base64_decode(msg));
		setdata(u);
	}
}

GroupMemberInfo::GroupMemberInfo(const vector<unsigned char>& data)
{
	if (!data.empty())
	{
		Unpack u(data);
		setdata(u);
	}
}

string GroupMemberInfo::tostring() const
{
	string s = "{";
	s += "群号:";
	s += to_string(Group);
	s += " ,QQ号:";
	s += to_string(QQID);
	s += " ,昵称:";
	s += Nick;
	s += " ,名片:";
	s += GroupNick;
	s += " ,性别:";
	s += (Gender == 255 ? "未知" : Gender == 1 ? "男" : "女");
	s += " ,年龄:";
	s += to_string(Age);
	s += " ,地区:";
	s += Region;
	s += " ,加群时间:";
	s += to_string(AddGroupTime);
	s += " ,最后发言:";
	s += to_string(LastMsgTime);
	s += " ,等级_名称:";
	s += LevelName;
	s += " ,管理权限:";
	s += (permissions == 3 ? "群主" : permissions == 2 ? "管理员" : "群员");
	s += "(";
	s += to_string(permissions);
	s += ")";
	s += " ,不良记录成员:";
	s += to_string(NaughtyRecord);
	s += " ,专属头衔:";
	s += Title;
	s += " ,专属头衔过期时间:";
	s += to_string(ExpireTime);
	s += " ,允许修改名片:";
	s += to_string(canEditGroupNick);
	s += "}";
	return s;
}

FriendInfo::FriendInfo(Unpack p)
{
	QQID = p.getLong();
	nick = p.getstring();
	remark = p.getstring();
}

std::string FriendInfo::tostring() const
{
	return remark + '(' + std::to_string(QQID) + ')' + ((remark == nick) ? "" : "（" + nick + "）");
}

//增加运行日志
int CQ::addLog(const int Priorty, const char* Type, const char* Content) noexcept
{
	return lasterr = CQ_addLog(getAuthCode(), Priorty, Type, Content);
}

//发送好友消息
int CQ::sendPrivateMsg(const long long QQ, const char* msg) noexcept
{
	return CQ_sendPrivateMsg(getAuthCode(), QQ, msg);
}

//发送好友消息
int CQ::sendPrivateMsg(const long long QQ, const std::string& msg) noexcept { return sendPrivateMsg(QQ, msg.c_str()); }

//发送群消息
int CQ::sendGroupMsg(const long long GroupID, const char* msg) noexcept
{
	return CQ_sendGroupMsg(getAuthCode(), GroupID, msg);
}

//发送群消息
int CQ::sendGroupMsg(const long long GroupID, const std::string& msg) noexcept
{
	return sendGroupMsg(GroupID, msg.c_str());
}

int CQ::sendDiscussMsg(const long long DiscussID, const char* msg) noexcept
{
	return CQ_sendDiscussMsg(getAuthCode(), DiscussID, msg);
}

//发送讨论组消息
int CQ::sendDiscussMsg(const long long DiscussID, const std::string& msg) noexcept
{
	return sendDiscussMsg(DiscussID, msg.c_str());
}

//发送赞
int CQ::sendLike(const long long QQID, const int times) noexcept
{
	return lasterr = CQ_sendLikeV2(getAuthCode(), QQID, times);
}


//取Cookies (慎用，此接口需要严格授权)
const char* CQ::getCookies(const char* Domain) noexcept { return CQ_getCookiesV2(getAuthCode(), Domain); }

//取Cookies (慎用，此接口需要严格授权)
const char* CQ::getCookies(const std::string& Domain) noexcept { return getCookies(Domain.c_str()); }

//接收语音
const char* CQ::getRecord(const char* file, const char* outformat) noexcept
{
	return CQ_getRecordV2(getAuthCode(), file, outformat);
}

//接收语音
std::string CQ::getRecord(const std::string& file, const std::string& outformat) noexcept
{
	return getRecord(file.c_str(), outformat.c_str());
}

//取CsrfToken (慎用，此接口需要严格授权)
int CQ::getCsrfToken() noexcept { return CQ_getCsrfToken(getAuthCode()); }

//取应用目录
const char* CQ::getAppDirectory() noexcept { return CQ_getAppDirectory(getAuthCode()); }

//取登录QQ
long long CQ::getLoginQQ() noexcept { return CQ_getLoginQQ(getAuthCode()); }

//取登录昵称
const char* CQ::getLoginNick() noexcept { return CQ_getLoginNick(getAuthCode()); }

//置群员移除
int CQ::setGroupKick(const long long GroupID, const long long QQID, const CQBOOL refuseForever) noexcept
{
	return lasterr = CQ_setGroupKick(getAuthCode(), GroupID, QQID, refuseForever);
}

//置群员禁言
int CQ::setGroupBan(const long long GroupID, const long long QQID, const long long banTime) noexcept
{
	return lasterr = CQ_setGroupBan(getAuthCode(), GroupID, QQID, banTime);
}

//置群管理员
int CQ::setGroupAdmin(const long long GroupID, const long long QQID, const CQBOOL isAdmin) noexcept
{
	return lasterr = CQ_setGroupAdmin(getAuthCode(), GroupID, QQID, isAdmin);
}

//置群成员专属头衔
int CQ::setGroupSpecialTitle(const long long GroupID, const long long QQID, const char* Title,
                             const long long ExpireTime) noexcept
{
	return lasterr = CQ_setGroupSpecialTitle(getAuthCode(), GroupID, QQID, Title, ExpireTime);
}

//置群成员专属头衔
int CQ::setGroupSpecialTitle(const long long GroupID, const long long QQID, const std::string& Title,
                             const long long ExpireTime) noexcept
{
	return setGroupSpecialTitle(GroupID, QQID, Title.c_str(), ExpireTime);
}

//置全群禁言
int CQ::setGroupWholeBan(const long long GroupID, const CQBOOL isBan) noexcept
{
	return lasterr = CQ_setGroupWholeBan(getAuthCode(), GroupID, isBan);
}

//置Anonymous群员禁言
int CQ::setGroupAnonymousBan(const long long GroupID, const char* AnonymousToken, const long long banTime) noexcept
{
	return lasterr = CQ_setGroupAnonymousBan(getAuthCode(), GroupID, AnonymousToken, banTime);
}

//置群Anonymous设置
int CQ::setGroupAnonymous(const long long GroupID, const CQBOOL enableAnonymous) noexcept
{
	return lasterr = CQ_setGroupAnonymous(getAuthCode(), GroupID, enableAnonymous);
}

//置群成员名片
int CQ::setGroupCard(const long long GroupID, const long long QQID, const char* newGroupNick) noexcept
{
	return lasterr = CQ_setGroupCard(getAuthCode(), GroupID, QQID, newGroupNick);
}

//置群成员名片
int CQ::setGroupCard(const long long GroupID, const long long QQID, const std::string& newGroupNick) noexcept
{
	return setGroupCard(GroupID, QQID, newGroupNick.c_str());
}

//置群退出
int CQ::setGroupLeave(const long long GroupID, const CQBOOL isDismiss) noexcept
{
	return lasterr = CQ_setGroupLeave(getAuthCode(), GroupID, isDismiss);
}

//置讨论组退出
int CQ::setDiscussLeave(const long long DiscussID) noexcept
{
	return lasterr = CQ_setDiscussLeave(getAuthCode(), DiscussID);
}

//置好友添加请求
int CQ::setFriendAddRequest(const char* RequestToken, const int ReturnType, const char* Remarks) noexcept
{
	return lasterr = CQ_setFriendAddRequest(getAuthCode(), RequestToken, ReturnType, Remarks);
}

//置群添加请求
int CQ::setGroupAddRequest(const char* RequestToken, const int RequestType, const int ReturnType,
                           const char* Reason) noexcept
{
	return lasterr = CQ_setGroupAddRequestV2(getAuthCode(), RequestToken, RequestType, ReturnType, Reason);
}

//置致命错误提示
int CQ::setFatal(const char* ErrorMsg) noexcept { return lasterr = CQ_setFatal(getAuthCode(), ErrorMsg); }

//取群成员信息 (支持缓存)
GroupMemberInfo CQ::getGroupMemberInfo(const long long GroupID, const long long QQID,
                                       const CQBOOL disableCache) noexcept
{
	return GroupMemberInfo(CQ_getGroupMemberInfoV2(getAuthCode(), GroupID, QQID, disableCache));
}

//取陌生人信息 (支持缓存)
StrangerInfo CQ::getStrangerInfo(const long long QQID, const CQBOOL DisableCache) noexcept
{
	return StrangerInfo(CQ_getStrangerInfo(getAuthCode(), QQID, DisableCache));
}

//取群成员列表
std::vector<GroupMemberInfo> CQ::getGroupMemberList(const long long GroupID)
{
	const char* ret = CQ_getGroupMemberList(getAuthCode(), GroupID);
	if (!ret || ret[0] == '\0') return {};
	const string data(base64_decode(ret));
	if (data.empty())return {};
	vector<GroupMemberInfo> infovector;
	Unpack u(data);
	auto i = u.getInt();
	while (--i && u.len() > 0)
	{
		auto tmp = u.getUnpack();
		infovector.emplace_back(tmp);
	}

	return infovector;
}

#include <fstream>
//取群列表
std::map<long long, std::string> CQ::getGroupList(bool disableCache)
{
	static std::map<long long, std::string> ret;
	static time_t lastUpdateTime = 0;

	const time_t timeNow = time(nullptr);
	if (!disableCache && timeNow - lastUpdateTime < 600 && !ret.empty())
	{
		return ret;
	}

	ret.clear();
	lastUpdateTime = timeNow;

	const char* src = CQ_getGroupList(getAuthCode());
	if (!src || src[0] == '\0') return {};
	const auto data(base64_decode(src)); // 解码
	if (data.empty())return {};
	Unpack pack(data); // 转换为Unpack

	pack.getInt(); //获取总群数, 返回值在此并没有用
	while (pack.len() > 0)
	{
		//如果还有剩余数据,就继续读取
		auto tep = pack.getUnpack(); //读取下一个群
		auto ID = tep.getLong(); //读取GroupID
		const auto name = tep.getstring(); //读取群名称
		ret[ID] = name; //写入map
	}
	return ret;
}

//取好友列表
std::map<long long, FriendInfo> CQ::getFriendList(bool disableCache)
{
	static std::map<long long, FriendInfo> ret;
	static time_t lastUpdateTime = 0;
	
	const time_t timeNow = time(nullptr);
	if (!disableCache && timeNow - lastUpdateTime < 600 && !ret.empty())
	{
		return ret;
	}
	
	ret.clear();
	lastUpdateTime = timeNow;
	
	const char* src = CQ_getFriendList(getAuthCode(), false);
	if (!src || src[0] == '\0') return {};
	const auto data(base64_decode(src)); // 解码
	if (data.empty())return {};
	Unpack pack(data); // 获取原始数据转换为Unpack
	int Cnt = pack.getInt(); //获取总数
	while (Cnt--)
	{
		FriendInfo info(pack.getUnpack()); //读取
		ret[info.QQID] = info; //写入map
	}
	return ret;
}

bool CQ::canSendImage() noexcept
{
	return CQ_canSendImage(getAuthCode()) > 0;
}

bool CQ::canSendRecord() noexcept
{
	return CQ_canSendRecord(getAuthCode()) > 0;
}

const char* CQ::getImage(const char* file) noexcept
{
	return CQ_getImage(getAuthCode(), file);
}

const char* CQ::getImage(const std::string& file) noexcept
{
	return getImage(file.c_str());
}

int CQ::deleteMsg(const long long MsgId) noexcept
{
	return lasterr = CQ_deleteMsg(getAuthCode(), MsgId);
}

const char* CQ::getlasterrmsg() noexcept
{
	switch (lasterr)
	{
	case 0: return "操作成功";
	case -1: return "请求发送失败";
	case -2: return "未收到服务器回复，可能未发送成功";
	case -3: return "消息过长或为空";
	case -4: return "消息解析过程异常";
	case -5: return "日志功能未启用";
	case -6: return "日志优先级错误";
	case -7: return "数据入库失败";
	case -8: return "不支持对系统帐号操作";
	case -9: return "帐号不在该群内，消息无法发送";
	case -10: return "该用户不存在/不在群内";
	case -11: return "数据错误，无法请求发送";
	case -12: return "不支持对Anonymous成员解除禁言";
	case -13: return "无法解析要禁言的Anonymous成员数据";
	case -14: return "由于未知原因，操作失败";
	case -15: return "群未开启Anonymous发言功能，或Anonymous帐号被禁言";
	case -16: return "帐号不在群内或网络错误，无法退出/解散该群";
	case -17: return "帐号为群主，无法退出该群";
	case -18: return "帐号非群主，无法解散该群";
	case -19: return "临时消息已失效或未建立";
	case -20: return "参数错误";
	case -21: return "临时消息已失效或未建立";
	case -22: return "获取QQ信息失败";
	case -23: return "找不到与目标QQ的关系，消息无法发送";
	case -99: return "您调用的功能无法在此版本上实现";
	case -101: return "应用过大";
	case -102: return "不是合法的应用";
	case -103: return "不是合法的应用";
	case -104: return "应用不存在公开的Information函数";
	case -105: return "无法载入应用信息";
	case -106: return "文件名与应用ID不同";
	case -107: return "返回信息解析错误";
	case -108: return "AppInfo返回的Api版本不支持直接加载，仅支持Api版本为9(及以上)的应用直接加载";
	case -109: return "AppInfo返回的AppID错误";
	case -110: return "缺失AppInfo返回的AppID对应的[Appid].json文件";
	case -111: return "[Appid].json文件内的AppID与其文件名不同";
	case -120: return "无Api授权接收函数(Initialize)";
	case -121: return "Api授权接收函数(Initialize)返回值非0";
	case -122: return "尝试恶意修改酷Q配置文件，将取消加载并关闭酷Q";
	case -150: return "无法载入应用信息";
	case -151: return "应用信息Json串解析失败，请检查Json串是否正确";
	case -152: return "Api版本过旧或过新";
	case -153: return "应用信息错误或存在缺失";
	case -154: return "Appid不合法";
	case -160: return "事件类型(Type)错误或缺失";
	case -161: return "事件函数(Function)错误或缺失";
	case -162: return "应用优先级不为10000、20000、30000、40000中的一个";
	case -163: return "事件类型(Api)不支持应用Api版本";
	case -164: return "应用Api版本大于8，但使用了新版本已停用的事件类型(Type)：1(好友消息)、3(临时消息)";
	case -165: return "事件类型为2(群消息)、4(讨论组消息)、21(私聊消息)，但缺少正则表达式(regex)的表达式部分(expression)";
	case -166: return "存在为空的正则表达式(regex)的key";
	case -167: return "存在为空的正则表达式(regex)的表达式部分(expression)";
	case -168: return "应用事件(event)id参数不存在或为0";
	case -169: return "应用事件(event)id参数有重复";
	case -180: return "应用状态(status)id参数不存在或为0";
	case -181: return "应用状态(status)period参数不存在或设置错误";
	case -182: return "应用状态(status)id参数有重复";
	case -201: return "无法载入应用，可能是应用文件已损坏";
	case -202: return "Api版本过旧或过新";
	case -997: return "应用未启用";
	case -998: return "应用调用在Auth声明之外的 酷Q Api。";
	default: return "未知错误";
	}
}
