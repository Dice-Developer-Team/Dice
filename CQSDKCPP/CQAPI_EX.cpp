#include "..\CQSDK\CQAPI_EX.h"

#include "..\CQSDK\CQAPI.h"
#include "..\CQSDK\Unpack.h"
#include "..\CQSDK\CQEVE_GroupMsg.h"
#include "..\CQSDK\CQEVE_PrivateMsg.h"
#include "..\CQSDK\CQTools.h"

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
	昵称 = "";
	名片 = "";
	性别 = 0;
	年龄 = 0;
	地区 = "";
	加群时间 = 0;
	最后发言 = 0;
	等级_名称 = "";
	permissions = 0;
	不良记录成员 = 0;
	专属头衔 = "";
	专属头衔过期时间 = 0;
	允许修改名片 = 0;
}

void CQ::GroupMemberInfo::setdata(Unpack& u)
{
	Group = u.getLong();
	QQID = u.getLong();
	昵称 = u.getstring();
	名片 = u.getstring();
	性别 = u.getInt();
	年龄 = u.getInt();
	地区 = u.getstring();
	加群时间 = u.getInt();
	最后发言 = u.getInt();
	等级_名称 = u.getstring();
	permissions = u.getInt();
	不良记录成员 = u.getInt() == 1;
	专属头衔 = u.getstring();
	专属头衔过期时间 = u.getInt();
	允许修改名片 = u.getInt() == 1;
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
	s += " ,QQID:"; s += to_string(QQID);
	s += " ,昵称:"; s += 昵称;
	s += " ,名片:"; s += 名片;
	s += " ,性别:"; s += (性别 == 255 ? "未知" : 性别 == 1 ? "男" : "女");
	s += " ,年龄:"; s += to_string(年龄);
	s += " ,地区:"; s += 地区;
	s += " ,加群时间:"; s += to_string(加群时间);
	s += " ,最后发言:"; s += to_string(最后发言);
	s += " ,等级_名称:"; s += 等级_名称;
	s += " ,管理权限:"; s += (permissions == 3 ? "群主" : permissions == 2 ? "管理员" : "群员"); s += "("; s += to_string(permissions); s += ")";
	s += " ,不良记录成员:"; s += to_string(不良记录成员);
	s += " ,专属头衔:"; s += 专属头衔;
	s += " ,专属头衔过期时间:"; s += to_string(专属头衔过期时间);
	s += " ,允许修改名片:"; s += to_string(允许修改名片);
	s += "}"; return s;
}

//增加运行日志
int CQ::addLog(int 优先级, const char * 类型, const char * 内容) { return lasterr = CQ_addLog(getAuthCode(), 优先级, 类型, 内容); }

//发送好友消息
int CQ::sendPrivateMsg(long long QQ, const char * msg) { return  CQ_sendPrivateMsg(getAuthCode(), QQ, msg); }

//发送好友消息
int CQ::sendPrivateMsg(long long QQ, std::string & msg) { return sendPrivateMsg(QQ, msg.c_str()); }

//发送好友消息
//int CQ::sendPrivateMsg(EVEPrivateMsg eve, std::string msg) { return sendPrivateMsg(eve.fromQQ, msg.c_str()); }

//发送群消息
int CQ::sendGroupMsg(long long 群号, const char * msg) { return  CQ_sendGroupMsg(getAuthCode(), 群号, msg); }

//发送群消息
int CQ::sendGroupMsg(long long 群号, std::string & msg) { return sendGroupMsg(群号, msg.c_str()); }

int CQ::sendDiscussMsg(long long 讨论组号, const char * msg) { return   CQ_sendDiscussMsg(getAuthCode(), 讨论组号, msg); }

//发送讨论组消息
int CQ::sendDiscussMsg(long long 讨论组号, std::string & msg) { return sendDiscussMsg(讨论组号, msg.c_str()); }

//发送赞
int CQ::sendLike(long long QQID, int times) { return lasterr = CQ_sendLikeV2(getAuthCode(), QQID, times); }

//取Cookies (慎用，此接口需要严格授权)
const char * CQ::getCookies() { return  CQ_getCookies(getAuthCode()); }

//接收语音
const char * CQ::getRecord(const char * file, const char * outformat) { return CQ_getRecord(getAuthCode(), file, outformat); }

//接收语音
std::string CQ::getRecord(std::string & file, std::string outformat) { return getRecord(file.c_str(), outformat.c_str()); }

//取CsrfToken (慎用，此接口需要严格授权)
int CQ::getCsrfToken() { return  CQ_getCsrfToken(getAuthCode()); }

//取应用目录
const char * CQ::getAppDirectory() { return  CQ_getAppDirectory(getAuthCode()); }

//取登录QQ
long long CQ::getLoginQQ() { return  CQ_getLoginQQ(getAuthCode()); }

//取登录昵称
const char * CQ::getLoginNick() { return  CQ_getLoginNick(getAuthCode()); }

//置群员移除
int CQ::setGroupKick(long long 群号, long long QQID, CQBOOL 拒绝再加群) { return lasterr = CQ_setGroupKick(getAuthCode(), 群号, QQID, 拒绝再加群); }

//置群员禁言
int CQ::setGroupBan(long long 群号, long long QQID, long long 禁言时间) { return lasterr = CQ_setGroupBan(getAuthCode(), 群号, QQID, 禁言时间); }

//置群管理员
int CQ::setGroupAdmin(long long 群号, long long QQID, CQBOOL 成为管理员) { return lasterr = CQ_setGroupAdmin(getAuthCode(), 群号, QQID, 成为管理员); }

//置群成员专属头衔
int CQ::setGroupSpecialTitle(long long 群号, long long QQID, const char * 头衔, long long 过期时间) { return lasterr = CQ_setGroupSpecialTitle(getAuthCode(), 群号, QQID, 头衔, 过期时间); }

//置群成员专属头衔
int CQ::setGroupSpecialTitle(long long 群号, long long QQID, std::string & 头衔, long long 过期时间) { return setGroupSpecialTitle(群号, QQID, 头衔.c_str(), 过期时间); }

//置全群禁言
int CQ::setGroupWholeBan(long long 群号, CQBOOL 开启禁言) { return lasterr = CQ_setGroupWholeBan(getAuthCode(), 群号, 开启禁言); }

//置匿名群员禁言
int CQ::setGroupAnonymousBan(long long 群号, const char * 匿名, long long 禁言时间) { return lasterr = CQ_setGroupAnonymousBan(getAuthCode(), 群号, 匿名, 禁言时间); }

//置群匿名设置
int CQ::setGroupAnonymous(long long 群号, CQBOOL 开启匿名) { return lasterr = CQ_setGroupAnonymous(getAuthCode(), 群号, 开启匿名); }

//置群成员名片
int CQ::setGroupCard(long long 群号, long long QQID, const char * 新名片_昵称) { return lasterr = CQ_setGroupCard(getAuthCode(), 群号, QQID, 新名片_昵称); }

//置群成员名片
int CQ::setGroupCard(long long 群号, long long QQID, std::string 新名片_昵称) { return setGroupCard(群号, QQID, 新名片_昵称.c_str()); }

//置群退出
int CQ::setGroupLeave(long long 群号, CQBOOL 是否解散) { return lasterr = CQ_setGroupLeave(getAuthCode(), 群号, 是否解散); }

//置讨论组退出
int CQ::setDiscussLeave(long long 讨论组号) { return lasterr = CQ_setDiscussLeave(getAuthCode(), 讨论组号); }

//置好友添加请求
int CQ::setFriendAddRequest(const char * 请求反馈标识, int 反馈类型, const char * 备注) { return lasterr = CQ_setFriendAddRequest(getAuthCode(), 请求反馈标识, 反馈类型, 备注); }

//置群添加请求
int CQ::setGroupAddRequest(const char * 请求反馈标识, int 请求类型, int 反馈类型, const char * 理由) { return lasterr = CQ_setGroupAddRequestV2(getAuthCode(), 请求反馈标识, 请求类型, 反馈类型, 理由); }

//置致命错误提示
int CQ::setFatal(const char * 错误信息) { return lasterr = CQ_setFatal(getAuthCode(), 错误信息); }

//取群成员信息 (支持缓存)
GroupMemberInfo CQ::getGroupMemberInfo(long long 群号, long long QQID, CQBOOL 不使用缓存) { return GroupMemberInfo(CQ_getGroupMemberInfoV2(getAuthCode(), 群号, QQID, 不使用缓存)); }

//取陌生人信息 (支持缓存)
StrangerInfo CQ::getStrangerInfo(long long QQID, CQBOOL 不使用缓存) { return StrangerInfo(CQ_getStrangerInfo(getAuthCode(), QQID, 不使用缓存)); }

//取群成员列表
std::vector<GroupMemberInfo> CQ::getGroupMemberList(long long 群号) {
	const char * data = CQ_getGroupMemberList(getAuthCode(), 群号);
	vector<GroupMemberInfo> infovector;
	if (data[0] == '\0')return infovector;

	Unpack u(base64_decode(data));
	auto i = u.getInt();
	while (--i && u.len()>0)
	{
		//infovector.push_back(GroupMemberInfo(u.getchars()));
		infovector.emplace_back(u.getUnpack());
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
		long long ID = tep.getLong();//读取群号
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
	case -12:  return "不支持对匿名成员解除禁言";
	case -13:  return "无法解析要禁言的匿名成员数据";
	case -14:  return "由于未知原因，操作失败";
	case -15:  return "群未开启匿名发言功能，或匿名帐号被禁言";
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
	default:   return "未知错误,请向 初音Py2001 报告以升级SDK";
	}
}
