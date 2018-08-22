#include "CQAPI_EX.h"

#include "CQAPI.h"
#include "Unpack.h"
#include "CQEVE_GroupMsg.h"
#include "CQEVE_PrivateMsg.h"
#include "CQTools.h"

using namespace CQ;
using namespace std;
int lasterr;

CQ::StrangerInfo::StrangerInfo(const char* msg)
{
	if (msg[0] == '\0')
	{
		QQID = 0; sex = 255; age = -1; nick = "";
	}
	else
	{
		Unpack p(base64_decode(msg));
		QQID = p.getLong();
		nick = p.getstring();
		sex = p.getInt();
		age = p.getInt();
	}
}

string CQ::StrangerInfo::tostring() const
{
	return string("{")
		+ "QQ:" + to_string(QQID)
		+ " ,昵称:" + nick
		+ " ,性别:" + (sex == 255 ? "未知" : sex == 1 ? "男" : "女")
		+ " ,年龄:" + to_string(age)
		+ "}"
		;
}

void CQ::GroupMemberInfo::Void()
{
	Group = 0;
	QQID = 0;
	Nick = "";
	GroupNick = "";
	Gender = 0;
	Age = 0;
	Region = "";
	AddGroupTime = 0;
	LastMsgTime = 0;
	LevelName = "";
	permissions = 0;
	NaughtyRecord = 0;
	Title = "";
	ExpireTime = 0;
	canEditGroupNick = 0;
}

void CQ::GroupMemberInfo::setdata(Unpack& u)
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

CQ::GroupMemberInfo::GroupMemberInfo(Unpack & msg) { setdata(msg); }

CQ::GroupMemberInfo::GroupMemberInfo(const char* msg)
{
	if (msg[0] == '0')
	{
		Void();
	}
	else
	{
		Unpack u(base64_decode(msg));
		setdata(u);
		//setdata(Unpack(base64_decode(msg)));
	}
}

CQ::GroupMemberInfo::GroupMemberInfo(vector<unsigned char> data)
{
	if (data.size() <= 0)
	{
		Void();
	}
	else
	{
		Unpack u(data);
		setdata(u);
		//setdata(Unpack(base64_decode(msg)));
	}
}

string CQ::GroupMemberInfo::tostring() const
{
	string s = "{";
	s += "群号:"; s += to_string(Group);
	s += " ,QQ号:"; s += to_string(QQID);
	s += " ,昵称:"; s += Nick;
	s += " ,名片:"; s += GroupNick;
	s += " ,性别:"; s += (Gender == 255 ? "未知" : Gender == 1 ? "男" : "女");
	s += " ,年龄:"; s += to_string(Age);
	s += " ,地区:"; s += Region;
	s += " ,加群时间:"; s += to_string(AddGroupTime);
	s += " ,最后发言:"; s += to_string(LastMsgTime);
	s += " ,等级_名称:"; s += LevelName;
	s += " ,管理权限:"; s += (permissions == 3 ? "群主" : permissions == 2 ? "管理员" : "群员"); s += "("; s += to_string(permissions); s += ")";
	s += " ,不良记录成员:"; s += to_string(NaughtyRecord);
	s += " ,专属头衔:"; s += Title;
	s += " ,专属头衔过期时间:"; s += to_string(ExpireTime);
	s += " ,允许修改名片:"; s += to_string(canEditGroupNick);
	s += "}"; return s;
}

//增加运行日志
int CQ::addLog(int Priorty, const char* Type, const char* Content) { return lasterr = CQ_addLog(getAuthCode(), Priorty, Type, Content); }

//发送好友消息
int CQ::sendPrivateMsg(long long QQ, const char* msg) { return  CQ_sendPrivateMsg(getAuthCode(), QQ, msg); }

//发送好友消息
int CQ::sendPrivateMsg(long long QQ, std::string & msg) { return sendPrivateMsg(QQ, msg.c_str()); }

//发送好友消息
//int CQ::sendPrivateMsg(EVEPrivateMsg eve, std::string msg) { return sendPrivateMsg(eve.fromQQ, msg.c_str()); }

//发送群消息
int CQ::sendGroupMsg(long long GroupID, const char* msg) { return  CQ_sendGroupMsg(getAuthCode(), GroupID, msg); }

//发送群消息
int CQ::sendGroupMsg(long long GroupID, std::string & msg) { return sendGroupMsg(GroupID, msg.c_str()); }

int CQ::sendDiscussMsg(long long DiscussID, const char* msg) { return   CQ_sendDiscussMsg(getAuthCode(), DiscussID, msg); }

//发送讨论组消息
int CQ::sendDiscussMsg(long long DiscussID, std::string & msg) { return sendDiscussMsg(DiscussID, msg.c_str()); }

//发送赞
int CQ::sendLike(long long QQID, int times) { return lasterr = CQ_sendLikeV2(getAuthCode(), QQID, times); }

//取Cookies (慎用，此接口需要严格授权)
const char* CQ::getCookies() { return  CQ_getCookies(getAuthCode()); }

//接收语音
const char* CQ::getRecord(const char* file, const char* outformat) { return CQ_getRecord(getAuthCode(), file, outformat); }

//接收语音
std::string CQ::getRecord(std::string & file, std::string outformat) { return getRecord(file.c_str(), outformat.c_str()); }

//取CsrfToken (慎用，此接口需要严格授权)
int CQ::getCsrfToken() { return  CQ_getCsrfToken(getAuthCode()); }

//取应用目录
const char* CQ::getAppDirectory() { return  CQ_getAppDirectory(getAuthCode()); }

//取登录QQ
long long CQ::getLoginQQ() { return  CQ_getLoginQQ(getAuthCode()); }

//取登录昵称
const char* CQ::getLoginNick() { return  CQ_getLoginNick(getAuthCode()); }

//置群员移除
int CQ::setGroupKick(long long GroupID, long long QQID, CQBOOL refuseForever) { return lasterr = CQ_setGroupKick(getAuthCode(), GroupID, QQID, refuseForever); }

//置群员禁言
int CQ::setGroupBan(long long GroupID, long long QQID, long long banTime) { return lasterr = CQ_setGroupBan(getAuthCode(), GroupID, QQID, banTime); }

//置群管理员
int CQ::setGroupAdmin(long long GroupID, long long QQID, CQBOOL isAdmin) { return lasterr = CQ_setGroupAdmin(getAuthCode(), GroupID, QQID, isAdmin); }

//置群成员专属头衔
int CQ::setGroupSpecialTitle(long long GroupID, long long QQID, const char* Title, long long ExpireTime) { return lasterr = CQ_setGroupSpecialTitle(getAuthCode(), GroupID, QQID, Title, ExpireTime); }

//置群成员专属头衔
int CQ::setGroupSpecialTitle(long long GroupID, long long QQID, std::string & Title, long long ExpireTime) { return setGroupSpecialTitle(GroupID, QQID, Title.c_str(), ExpireTime); }

//置全群禁言
int CQ::setGroupWholeBan(long long GroupID, CQBOOL isBan) { return lasterr = CQ_setGroupWholeBan(getAuthCode(), GroupID, isBan); }

//置Anonymous群员禁言
int CQ::setGroupAnonymousBan(long long GroupID, const char* Anonymous, long long banTime) { return lasterr = CQ_setGroupAnonymousBan(getAuthCode(), GroupID, Anonymous, banTime); }

//置群Anonymous设置
int CQ::setGroupAnonymous(long long GroupID, CQBOOL enableAnonymous) { return lasterr = CQ_setGroupAnonymous(getAuthCode(), GroupID, enableAnonymous); }

//置群成员名片
int CQ::setGroupCard(long long GroupID, long long QQID, const char* newGroupNick) { return lasterr = CQ_setGroupCard(getAuthCode(), GroupID, QQID, newGroupNick); }

//置群成员名片
int CQ::setGroupCard(long long GroupID, long long QQID, std::string newGroupNick) { return setGroupCard(GroupID, QQID, newGroupNick.c_str()); }

//置群退出
int CQ::setGroupLeave(long long GroupID, CQBOOL isDismiss) { return lasterr = CQ_setGroupLeave(getAuthCode(), GroupID, isDismiss); }

//置讨论组退出
int CQ::setDiscussLeave(long long DiscussID) { return lasterr = CQ_setDiscussLeave(getAuthCode(), DiscussID); }

//置好友添加请求
int CQ::setFriendAddRequest(const char* RequestToken, int ReturnType, const char* Remarks) { return lasterr = CQ_setFriendAddRequest(getAuthCode(), RequestToken, ReturnType, Remarks); }

//置群添加请求
int CQ::setGroupAddRequest(const char* RequestToken, int RequestType, int ReturnType, const char* Reason) { return lasterr = CQ_setGroupAddRequestV2(getAuthCode(), RequestToken, RequestType, ReturnType, Reason); }

//置致命错误提示
int CQ::setFatal(const char* ErrorMsg) { return lasterr = CQ_setFatal(getAuthCode(), ErrorMsg); }

//取群成员信息 (支持缓存)
GroupMemberInfo CQ::getGroupMemberInfo(long long GroupID, long long QQID, CQBOOL disableCache) { return GroupMemberInfo(CQ_getGroupMemberInfoV2(getAuthCode(), GroupID, QQID, disableCache)); }

//取陌生人信息 (支持缓存)
StrangerInfo CQ::getStrangerInfo(long long QQID, CQBOOL disableCache) { return StrangerInfo(CQ_getStrangerInfo(getAuthCode(), QQID, disableCache)); }

//取群成员列表
std::vector<GroupMemberInfo> CQ::getGroupMemberList(long long GroupID) {
	const char* data = CQ_getGroupMemberList(getAuthCode(), GroupID);
	vector<GroupMemberInfo> infovector;
	if (data[0] == '\0')return infovector;

	Unpack u(base64_decode(data));
	auto i = u.getInt();
	while (--i && u.len()>0)
	{
		//infovector.push_back(GroupMemberInfo(u.getchars()));
		auto tmp = u.getUnpack();
		infovector.emplace_back(tmp);
	}

	return infovector;
}

//取群列表
std::map<long long, std::string> CQ::getGroupList() {
	string source(CQ_getGroupList(getAuthCode()));// 获取原始数据
	string data(base64_decode(source)); // 解码
	Unpack pack(data);// 转换为Unpack

	int len = pack.getInt();//获取总群数
	std::map<long long, std::string> ret;
	while (pack.len() > 0) {//如果还有剩余数据,就继续读取
		auto tep = pack.getUnpack();//读取下一个群
		long long ID = tep.getLong();//读取GroupID
		string name = tep.getstring();//读取群名称
		ret[ID] = name;//写入map
	}
	return ret;
}

int CQ::deleteMsg(long long MsgId)
{
	return lasterr = CQ_deleteMsg(getAuthCode(), MsgId);
}

const char * CQ::getlasterrmsg()
{
	switch (lasterr)
	{
	case 0:    return "操作成功";
	case -1:   return "请求发送失败";
	case -2:   return "未收到服务器回复，可能未发送成功";
	case -3:   return "消息过长或为空";
	case -4:   return "消息解析过程异常";
	case -5:   return "日志功能未启用";
	case -6:   return "日志优先级错误";
	case -7:   return "数据入库失败";
	case -8:   return "不支持对系统帐号操作";
	case -9:   return "帐号不在该群内，消息无法发送";
	case -10:  return "该用户不存在/不在群内";
	case -11:  return "数据错误，无法请求发送";
	case -12:  return "不支持对Anonymous成员解除禁言";
	case -13:  return "无法解析要禁言的Anonymous成员数据";
	case -14:  return "由于未知原因，操作失败";
	case -15:  return "群未开启Anonymous发言功能，或Anonymous帐号被禁言";
	case -16:  return "帐号不在群内或网络错误，无法退出/解散该群";
	case -17:  return "帐号为群主，无法退出该群";
	case -18:  return "帐号非群主，无法解散该群";
	case -19:  return "临时消息已失效或未建立";
	case -20:  return "参数错误";
	case -21:  return "临时消息已失效或未建立";
	case -22:  return "获取QQ信息失败";
	case -23:  return "找不到与目标QQ的关系，消息无法发送";
	case -99:  return "您调用的功能无法在此版本上实现";
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
	default:   return "未知错误";
	}
}
