#include "OneBotAPI.h"
#include "DiceEvent.h"
#include "Jsonio.h"
#include "MsgFormat.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "MsgMonitor.h"
#include "CharacterCard.h"
#include "DiceSession.h"
#include "GetRule.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "DiceStatic.hpp"
#include <memory>
#include <ctime>
using namespace std; 
static bool is_digit(char c) { return c >= '0' && c <= '9'; }

AttrVar idx_at(AttrObject& eve) {
	if (eve.has("at"))return eve["at"];
	if (!eve.has("uid"))return {};
	return eve["at"] = eve.has("gid")
		? AttrVar("[CQ:at,qq=" + eve.get_str("uid") + "]")
		: idx_nick(eve);
}
AttrVar idx_gAuth(AttrObject& eve) {
	if (!eve.has("uid")|| !eve.has("gid"))return {};
	if (int auth{ api::getGroupAuth(eve.get_ll("gid"),eve.get_ll("uid"),0) })
		return eve["grpAuth"] = auth;
	return {};
}

AttrGetters MsgIndexs{
	{"nick", idx_nick},
	{"pc", idx_pc},
	{"at", idx_at},
	{"@", idx_at},
	{"gender",  [](AttrObject& vars) {
		return vars.has("uid") ? vars["gender"] = getUserItem(vars.get_ll("uid"),"gender") : AttrVar();
	}},
	{"grpAuth", idx_gAuth},
	{"fromUser", [](AttrObject& vars) {
		return vars.has("uid") ? vars["fromUser"] = vars.get_str("uid") : "";
	}},
	{"fromQQ", [](AttrObject& vars) {
		return vars.has("uid") ? vars["fromQQ"] = vars.get_str("uid") : "";
	}},
	{"fromGroup", [](AttrObject& vars) {
		return vars.has("gid") ? vars["fromGroup"] = vars.get_str("gid") : "";
	}},
};

DiceEvent::DiceEvent(const AttrVars& var, const chatInfo& ct)
	:AttrObject(var), strMsg(at("fromMsg").text), fromChat(ct) {
	if (fromChat.gid) {
		pGrp = &chat(fromChat.gid);
	}
	thisGame = sessions.get_if(fromChat);
}
DiceEvent::DiceEvent(const AttrObject& var)
	:AttrObject(var), strMsg(at("fromMsg").text) {
	fromChat = { get_ll("uid") ,get_ll("gid") ,get_ll("chid") };
	if (fromChat.gid) {
		pGrp = &chat(fromChat.gid);
	}
	thisGame = sessions.get_if(fromChat);
}
bool DiceEvent::isPrivate()const {
	return !fromChat.gid;
}
bool DiceEvent::isFromMaster()const {
	return fromChat.uid == console;
}
bool DiceEvent::isChannel()const {
	return fromChat.chid;
}

void DiceEvent::formatReply() {
	if (msgMatch.ready())strReply = convert_realw2a(msgMatch.format(convert_a2realw(strReply.c_str())).c_str());
	strReply = fmt->format(strReply, *this);
}

void DiceEvent::reply(const std::string& msgReply, bool isFormat) {
	if (!msgReply.empty()) {
		strReply = msgReply;
		reply(isFormat);
	}
}

void DiceEvent::reply(bool isFormat) {
	if (isVirtual && isPrivate())return;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	if (isFormat)
		formatReply();
	logEcho();
	//if (console["ReferMsgReply"] && get_ll("msgid"))strReply = "[CQ:reply,id=" + get_str("msgid") + "]" + strReply;
	api::printLog("msg reply:" + strReply);
	api::printLog("type:" + to_string((int)fromChat.type));
	AddMsgToQueue(strReply, fromChat);
}
void DiceEvent::replyMsg(const std::string& key) {
	if (isVirtual && fromChat.uid == console.DiceMaid && isPrivate())return;
	strReply = getMsg(key, *this);
	logEcho();
	if (console["ReferMsgReply"] && get_ll("msgid"))strReply = "[CQ:reply,id=" + get_str("msgid") + "]" + strReply;
	AddMsgToQueue(strReply, fromChat);
}
void DiceEvent::replyHelp(const std::string& key) {
	if (isVirtual && fromChat.uid == console.DiceMaid && isPrivate())return;
	strReply = fmt->get_help(key, *this);
	logEcho();
	if (console["ReferMsgReply"] && get_ll("msgid"))strReply = "[CQ:reply,id=" + get_str("msgid") + "]" + strReply;
	AddMsgToQueue(strReply, fromChat);
}
void DiceEvent::replyRollDiceErr(int err, const RD& rd) {
	switch (err) {
	case 0: break;
	case Value_Err:
		replyMsg("strValueErr");
		break;
	case Input_Err:
		set("dice_exp", rd.strDice);
		replyMsg("strInputErr");
		break;
	case ZeroDice_Err:
		replyMsg("strZeroDiceErr");
		break;
	case ZeroType_Err:
		replyMsg("strZeroTypeErr");
		break;
	case DiceTooBig_Err:
		replyMsg("strDiceTooBigErr");
		break;
	case TypeTooBig_Err:
		replyMsg("strTypeTooBigErr");
		break;
	case AddDiceVal_Err:
		replyMsg("strAddDiceValErr");
		break;
	default:
		replyMsg("strUnknownErr");
		break;
	}
}

void DiceEvent::replyHidden(const std::string& msgReply) {
	strReply = msgReply;
	replyHidden();
}
void DiceEvent::replyHidden() {
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	formatReply();
	auto here{ fromChat.locate() };
	if (thisGame && thisGame->is_logging()) {
		ofstream logout(thisGame->log_path(), ios::out | ios::app);
		logout << getMsg("strSelfName") + "(" + to_string(console.DiceMaid) + ") " + printTTime((time_t)get_ll("time")) << endl
			<< filter_CQcode(strReply, fromChat.gid) << endl << endl;
	}
	strReply = "在" + printChat(fromChat) + "中 " + forward_filter(strReply);
	if (!pGrp || !pGrp->isset("Rh盲骰")) {
		AddMsgToQueue(strReply, fromChat.uid);
	}
	if (thisGame) {
		for (auto& id : *thisGame->get_gm()) {
			if (id.to_double() != fromChat.uid) {
				AddMsgToQueue(strReply, (long long)id.to_double());
			}
		}
		for (auto& id : *thisGame->get_ob()) {
			if (id.to_double() != fromChat.uid) {
				AddMsgToQueue(strReply, (long long)id.to_double());
			}
		}
	}
}

void DiceEvent::logEcho(){
	if (!isPrivate()
		&& (isChannel() ? console["ListenChannelEcho"] : console["ListenGroupEcho"]))return;
	auto here{ fromChat.locate() };
	if (thisGame && thisGame->is_logging()
		&& strLowerMessage.find(".log") != 0
		&& (thisGame->is_simple() || thisGame->is_part(fromChat.uid))) {
		ofstream logout(thisGame->log_path(), ios::out | ios::app);
		logout << getMsg("strSelfName") + "(" + to_string(console.DiceMaid) + ") " + printTTime((time_t)get_ll("time")) << endl
			<< filter_CQcode(strReply, fromChat.gid) << endl << endl;
	}
}
void DiceEvent::fwdMsg(){
	auto here{ fromChat.locate() };
	if (thisGame && thisGame->is_logging()
		&& strLowerMessage.find(".log") != 0
		&& (thisGame->is_simple() || thisGame->is_part(fromChat.uid))) {
		ofstream logout(thisGame->log_path(), ios::out | ios::app);
		logout << idx_pc(*this).to_str() + "(" + to_string(fromChat.uid) + ") " + printTTime((time_t)get_ll("time")) << endl
			<< filter_CQcode(strMsg, fromChat.gid) << endl << endl;
	}
}

void DiceEvent::note(std::string strMsg, int note_lv)
{
	strMsg = fmt->format(strMsg, *this);
	ofstream fout(DiceDir / "audit" / ("log" + to_string(console.DiceMaid) + "_" + printDate() + ".txt"),
		ios::out | ios::app);
	fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	reply(strMsg);
	const string note = getName(fromChat.uid) + strMsg;
	for (const auto& [ct, level] : console.NoticeList)
	{
		if (!(level & note_lv) || ct.uid == fromChat.uid || ct.gid == fromChat.gid)continue;
		AddMsgToQueue(note, ct);
	}
}
int DiceEvent::AdminEvent(const string& strOption){
	if (strOption == "state")
	{
		ResList res;
		res << "Servant:" + printUser(console.DiceMaid)
			<< "Master:" + printUser(console)
			<< (console["Private"] ? "私用模式" : "公用模式");
		if (console["LeaveDiscuss"])res << "禁用讨论组";
		if (console["DisabledGlobal"])res << "全局静默中";
		if (console["DisabledMe"])res << "全局禁用.me";
		if (console["DisabledJrrp"])res << "全局禁用.jrrp";
		if (console["DisabledDraw"])res << "全局禁用.draw";
		if (console["DisabledSend"])res << "全局禁用.send";
		if (trusted > 3)
			res //<< "所在群聊数：" + to_string(DD::getGroupIDList().size())
			<< "群记录数：" + to_string(ChatList.size())
			//<< "好友数：" + to_string(DD::getFriendQQList().size())
			<< "用户记录数：" + to_string(UserList.size())
			<< "今日用户量：" + to_string(today->cntUser())
			<< (!PList.empty() ? "角色卡记录数：" + to_string(PList.size()) : "")
			<< (censor.size() ? "敏感词库规模：" + to_string(censor.size()) : "")
			<< console.listClock().dot("\t").show();
		reply(res.show(), false);
		return 1;
	}
	if (trusted < 4)
	{
		replyMsg("strNotAdmin");
		return -1;
	}
	if (auto it = Console::intDefault.find(strOption);it != Console::intDefault.end())
	{
		int intSet = 0;
		switch (readNum(intSet))
		{
		case 0:
			console.set(it->first, intSet);
			note("已将" + getMsg("strSelfName") + "的" + it->first + "设置为" + to_string(intSet), 0b10);
			break;
		case -1:
			reply(getMsg("strSelfName") + "该项为" + to_string(console[strOption.c_str()]));
			break;
		case -2:
			reply("{nick}设置参数超出范围×");
			break;
		}
		return 1;
	}
	if (strOption == "delete")
	{
		note("已经放弃管理员权限√", 0b100);
		getUser(fromChat.uid).trust(3);
		console.NoticeList.erase({ fromChat.uid, 0,0 });
		return 1;
	}
	if (strOption == "on")
	{
		if (console["DisabledGlobal"])
		{
			console.set("DisabledGlobal", 0);
			note("已全局开启" + getMsg("strSelfName"), 3);
		}
		else
		{
			reply(getMsg("strSelfName") + "不在静默中！");
		}
		return 1;
	}
	if (strOption == "off")
	{
		if (console["DisabledGlobal"])
		{
			reply(getMsg("strSelfName") + "已经静默！");
		}
		else
		{
			console.set("DisabledGlobal", 1);
			note("已全局关闭" + getMsg("strSelfName"), 0b10);
		}
		return 1;
	}
	if (strOption == "censor") {
		readSkipSpace();
		if (strMsg[intMsgCnt] == '+') {
			intMsgCnt++;
			set("danger_level", readToColon());
			Censor::Level danger_level = censor.get_level(get_str("danger_level"));
			readSkipColon();
			ResList res;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					censor.add_word(item, danger_level);
					res << item;
				}
			}
			if (res.empty()) {
				reply("{nick}未输入待添加敏感词！");
			}
			else {
				note("已添加{danger_level}级敏感词" + to_string(res.size()) + "个:" + res.show(), 1);
			}
		}
		else if (strMsg[intMsgCnt] == '-') {
			intMsgCnt++;
			ResList res,resErr;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					if (censor.rm_word(item))
						res << item;
					else
						resErr << item;
				}
			}
			if (res.empty()) {
				reply("{nick}未输入待移除敏感词！");
			}
			else {
				note("已移除敏感词" + to_string(res.size()) + "个:" + res.show(), 1);
			}
			if (!resErr.empty())
				reply("{nick}移除不存在敏感词" + to_string(resErr.size()) + "个:" + resErr.show());
		}
		else
			replyHelp("censor");
		return 1;
	}
	if (strOption == "only")
	{
		if (console["Private"])
		{
			reply(getMsg("strSelfName") + "已成为私用骰娘！");
		}
		else
		{
			console.set("Private", 1);
			note("已将" + getMsg("strSelfName") + "变为私用√", 0b10);
		}
		return 1;
	}
	if (strOption == "public")
	{
		if (console["Private"])
		{
			console.set("Private", 0);
			note("已将" + getMsg("strSelfName") + "变为公用√", 0b10);
		}
		else
		{
			reply(getMsg("strSelfName") + "已成为公用骰娘！");
		}
		return 1;
	}
	if (strOption == "clock")
	{
		bool isErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			isErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+')
		{
			intMsgCnt++;
		}
		string strType = readPara();
		if (strType.empty())
		{
			reply(getMsg("strSelfName") + "的定时列表：" + console.listClock().show());
			return 1;
		}
		Clock cc{0, 0};
		switch (readClock(cc))
		{
		case 0:
			if (isErase)
			{
				if (console.rmClock(cc, strType))reply(
					getMsg("strSelfName") + "无此定时项目");
				else note("已移除" + getMsg("strSelfName") + "在" + printClock(cc) + "的定时" + strType, 0b10);
			}
			else
			{
				console.setClock(cc, strType);
				note("已设置" + getMsg("strSelfName") + "在" + printClock(cc) + "的定时" + strType, 0b10);
			}
			break;
		case -1:
			replyMsg("strParaEmpty");
			break;
		case -2:
			replyMsg("strParaIllegal");
			break;
		default: break;
		}
		return 1;
	}
	if (strOption == "notice")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		else if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		if (chatInfo cTarget; readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("当前通知窗口" + to_string(list.size()) + "个：" + list.show());
			return 1;
		}
		else
		{
			if (boolErase)
			{
				console.rmNotice(cTarget);
				note("已将" + getMsg("strSelfName") + "的通知窗口" + printChat(cTarget) + "移除", 0b1);
				return 1;
			}
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-')
			{
				int intAdd = 0;
				int intReduce = 0;
				while (intMsgCnt < strMsg.length())
				{
					bool isReduce = strMsg[intMsgCnt] == '-';
					string strNum = readDigit();
					if (strNum.empty() || strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 9)continue;
					else
					{
						if (isReduce)intReduce |= (1 << intNum);
						else intAdd |= (1 << intNum);
					}
					readSkipSpace();
				}
				if (intAdd)console.addNotice(cTarget, intAdd);
				if (intReduce)console.redNotice(cTarget, intReduce);
				if (intAdd | intReduce)note(
					"已将" + getMsg("strSelfName") + "对窗口" + printChat(cTarget) + "通知级别调整为" + to_binary(
						console.showNotice(cTarget)), 0b1);
				else replyMsg("strParaIllegal");
				return 1;
			}
			int intLV{ 0 };
			switch (readNum(intLV))
			{
			case 0:
				if (intLV < 0 || intLV > 63)
				{
					replyMsg("strParaIllegal");
					return 1;
				}
				console.setNotice(cTarget, intLV);
				note("已将" + getMsg("strSelfName") + "对窗口" + printChat(cTarget) + "通知级别调整为" + to_string(intLV), 0b1);
				break;
			case -1:
				reply("窗口" + printChat(cTarget) + "在" + getMsg("strSelfName") + "处的通知级别为：" + to_binary(
					console.showNotice(cTarget)));
				break;
			case -2:
				replyMsg("strParaIllegal");
				break;
			}
		}
		return 1;
	}
	if (strOption == "whitegroup")
	{
		readSkipSpace();
		bool isErase = false;
		if (strMsg[intMsgCnt] == '-')
		{
			intMsgCnt++;
			isErase = true;
		}
		if (string strGroup; !(strGroup = readDigit()).empty())
		{
			long long llGroup = stoll(strGroup);
			if (isErase)
			{
				if (groupset(llGroup, "许可使用") > 0 || groupset(llGroup, "免清") > 0)
				{
					chat(llGroup).reset("许可使用").reset("免清");
					note("已移除" + printGroup(llGroup) + "在" + getMsg("strSelfName") + "的使用许可");
				}
				else
				{
					reply("该群未拥有" + getMsg("strSelfName") + "的使用许可！");
				}
			}
			else
			{
				if (groupset(llGroup, "许可使用") > 0)
				{
					reply("该群已拥有" + getMsg("strSelfName") + "的使用许可！");
				}
				else
				{
					chat(llGroup).set("许可使用").reset("未审核");
					if (chat(llGroup).getLst())AddMsgToQueue(
						getMsg("strAuthorized"), { 0,llGroup });
					note("已添加" + printGroup(llGroup) + "在" + getMsg("strSelfName") + "的使用许可");
				}
			}
			return 1;
		}
		ResList res;
		for (auto& [id, grp] : ChatList)
		{
			string strGroup;
			if (grp.isset("许可使用") || grp.isset("免清") || grp.isset("免黑"))
			{
				strGroup = printChat(grp);
				if (grp.isset("许可使用"))strGroup += "-许可使用";
				if (grp.isset("免清"))strGroup += "-免清";
				if (grp.isset("免黑"))strGroup += "-免黑";
				res << strGroup;
			}
		}
		reply("当前白名单群" + to_string(res.size()) + "个：" + res.show());
		return 1;
	}
	if (strOption == "frq")
	{
		reply("当前总指令频度" + to_string(FrqMonitor::getFrqTotal()));
		return 1;
	}
	else 
	{
		bool boolErase = false;
		set("note", readPara());
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llTargetID = readID();
		if (strOption == "whiteqq")
		{
			if (llTargetID == 0)
			{
				strReply = "当前白名单用户列表：";
				for (auto& [uid, user] : UserList)
				{
					if (user.nTrust)strReply += "\n" + printUser(uid) + ":" + to_string(user.nTrust);
				}
				reply();
				return 1;
			}
			do
			{
				if (boolErase)
				{
					if (trustedQQ(llTargetID))
					{
						if (trusted <= trustedQQ(llTargetID))
						{
							replyMsg("strUserTrustDenied");
						}
						else 
						{
							getUser(llTargetID).trust(0);
							note("已收回" + getMsg("strSelfName") + "对" + printUser(llTargetID) + "的信任√", 0b1);
						}
					}
					else
					{
						reply(printUser(llTargetID) + "并不在" + getMsg("strSelfName") + "的白名单！");
					}
				}
				else 
				{
					if (trustedQQ(llTargetID))
					{
						reply(printUser(llTargetID) + "已加入" + getMsg("strSelfName") + "的白名单!");
					}
					else
					{
						getUser(llTargetID).trust(1);
						note("已添加" + getMsg("strSelfName") + "对" + printUser(llTargetID) + "的信任√", 0b1);
						set("user_nick", getName(llTargetID));
						AddMsgToQueue(getMsg("strWhiteQQAddNotice", *this), llTargetID);
					}
				}
			}
			while ((llTargetID = readID()));
			return 1;
		}
		else replyMsg("strSummonEmpty");
		return 0;
	}
}

int DiceEvent::MasterSet() 
{
	const std::string strOption = readPara();
	if (strOption.empty())
	{
		replyMsg("strAdminOptionEmpty");
		return -1;
	}
	if (strOption == "groupclr")
	{
		set("clear_mode", readRest());
		set("cmd", "clrgroup");
		sch.push_job(*this);
		return 1;
	}
	if (strOption == "delete")
	{
		if (console != fromChat.uid)
		{
			replyMsg("strNotMaster");
			return 1;
		}
		reply("你不再是" + getMsg("strSelfName") + "的Master！");
		console.killMaster();
		return 1;
	}
	else if (strOption == "reset")
	{
		if (console != fromChat.uid)
		{
			replyMsg("strNotMaster");
			return 1;
		}
		const string strMaster = readDigit();
		if (strMaster.empty() || stoll(strMaster) == console)
		{
			reply("Master不要消遣{strSelfCall}!");
		}
		else
		{
			console.newMaster(stoll(strMaster));
			note("已将Master转让给" + printUser(console));
		}
		return 1;
	}
	if (strOption == "admin")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llAdmin = readID();
		if (llAdmin)
		{
			if (boolErase)
			{
				if (trustedQQ(llAdmin) > 3)
				{
					note("已收回" + printUser(llAdmin) + "对" + getMsg("strSelfName") + "的管理权限√", 0b100);
					console.rmNotice({ llAdmin,0,0 });
					getUser(llAdmin).trust(0);
				}
				else
				{
					reply("该用户无管理权限！");
				}
			}
			else
			{
				if (trustedQQ(llAdmin) > 3)
				{
					reply("该用户已有管理权限！");
				}
				else
				{
					getUser(llAdmin).trust(4);
					console.addNotice({ llAdmin,0,0 }, 0b1110);
					note("已添加" + printUser(llAdmin) + "对" + getMsg("strSelfName") + "的管理权限√", 0b100);
				}
			}
			return 1;
		}
		ResList list;
		for (const auto& [uid, user] : UserList)
		{
			if (user.nTrust > 3)list << printUser(uid);
		}
		reply(getMsg("strSelfName") + "的管理权限拥有者共" + to_string(list.size()) + "位：" + list.show());
		return 1;
	}
	return AdminEvent(strOption);
}

int DiceEvent::BasicOrder()
{
	if (strMsg[0] != '.')return 0;
	intMsgCnt++;
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
		intMsgCnt++;
	//指令匹配
	if (console["DebugMode"])console.log("listen:" + strMsg, 0, printSTNow());
	if (strLowerMessage.substr(intMsgCnt, 9) == "authorize")
	{
		intMsgCnt += 9;
		readSkipSpace();
		if (isPrivate() || strMsg[intMsgCnt] == '+')
		{
			long long llTarget = readID();
			if (llTarget)
			{
				pGrp = &chat(llTarget);
			}
			else
			{
				replyMsg("strGroupIDEmpty");
				return 1;
			}
		}
		if (pGrp->isset("许可使用") && !pGrp->isset("未审核") && !pGrp->isset("协议无效"))return 0;
		string strInfo = readRest();
		if (fmt->call_hook_event(merge({
			{"hook","GroupAuthorize"},
			{"aimGroup",to_string(pGrp->ID)},
			{"AttachInfo",strInfo},
			{"aim_gid",pGrp->ID},
			})))return 1;
		if (trusted > 0)
		{
			pGrp->set("许可使用").reset("未审核").reset("协议无效");
			note("已授权" + printGroup(pGrp->ID) + "许可使用", 1);
			AddMsgToQueue(getMsg("strGroupAuthorized", *this), { 0,pGrp->ID });
		}
		else
		{
			if (!console["CheckGroupLicense"] && !console["Private"] && !isCalled)return 0;
			if (strInfo.empty())console.log(printUser(fromChat.uid) + "申请" + printGroup(pGrp->ID) + "许可使用", 0b10, printSTNow());
			else console.log(printUser(fromChat.uid) + "申请" + printGroup(pGrp->ID) + "许可使用；附言：" + strInfo, 0b100, printSTNow());
			replyMsg("strGroupLicenseApply");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "master")
	{
		intMsgCnt += 6;
		if (!console)
		{
			string strOption = readRest();
			if (strOption == console.authkey_pub){
				console.set("Private", 0);
				console.newMaster(fromChat.uid);
			}
			else if(strOption == console.authkey_pri) {
				console.set("Private", 1);
				console.newMaster(fromChat.uid);
			}
			else {
				reply("请{nick}以{strSelfCall}初始化时提供的口令完成认主！");
			}
		}
		else if (trusted > 4 || isFromMaster()) {
			return MasterSet();
		}
		else {
			if (isCalled)replyMsg("strNotMaster");
		}
		return 1;
	}
	else if (!isPrivate() && pGrp->isset("协议无效")){
		set("ignored");
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot"){
		//intMsgCnt += 3;
		reply(getMsg("strBotHeader") + Dice_Full_Ver_On + getMsg("strBotMsg"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "reply") {
		intMsgCnt += 5;
		if (strMsg.length() == intMsgCnt) {
			replyHelp("reply");
			return 1;
		}
		unsigned int intMsgTmpCnt{ intMsgCnt };
		string action{ readPara() };
		if (action == "on" && fromChat.gid) {
			const string& option{ (at("option") = "禁用回复").text };
			if (!chat(fromChat.gid).isset(option)) {
				replyMsg("strGroupSetOffAlready");
			}
			else if (trusted > 0 || canRoomHost()) {
				chat(fromChat.gid).reset(option);
				replyMsg("strReplyOn");
			}
			else {
				replyMsg("strWhiteQQDenied");
			}
			return 1;
		}
		else if (action == "off" && fromChat.gid) {
			const string& option{ (at("option") = "禁用回复").text };
			if (chat(fromChat.gid).isset(option)) {
				replyMsg("strGroupSetOnAlready");
			}
			else if (trusted > 0 || canRoomHost()) {
				chat(fromChat.gid).set(option);
				replyMsg("strReplyOff");
			}
			else {
				replyMsg("strWhiteQQDenied");
			}
			return 1;
		}
		else if (is("order_off")) {
			return 0;
		}
		else if (action == "show") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			set("key", readRest());
			fmt->reply_show(this);
			return 1;
		}
		else if (action == "get") {
			if (trusted < 2) {
				replyMsg("strNotAdmin");
				return -1;
			}
			set("key", readRest());
			fmt->reply_get(this);
			return 1;
		}
		else if (action == "set") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			ptr<DiceMsgReply> trigger{ make_shared<DiceMsgReply>() };
			string keyword;
			string attr{ readToColon() };
			while (!attr.empty()) {
				if (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':'))intMsgCnt++;
				if (attr == "Title") {
					trigger->title = readUntilTab();
				}
				else if (attr == "Type") {	//Type=Order|Reply
					string type{ readUntilTab() };
					if (DiceMsgReply::sType.count(type))trigger->type = (DiceMsgReply::Type)DiceMsgReply::sType[type];
				}
				else if (attr == "Limit") { //trigger limit
					string content{ readUntilTab() };
					trigger->limit.parse(content);
				}
				else if (DiceMsgReply::sMode.count(attr)) {	//Mode=Key
					size_t mode{ DiceMsgReply::sMode[attr] };
					keyword = readUntilTab();
					if (mode == 3) {
						try {
							std::wregex re(convert_a2realw(keyword.c_str()), std::regex::ECMAScript);
						} catch (const std::regex_error& e) {
							set("err", e.what());
							replyMsg("strRegexInvalid");
							return -1;
						}
						trigger->keyMatch[mode] = std::make_unique<vector<string>>(
							vector<string>{ keyword });
					}
					else {
						trigger->keyMatch[mode] = std::make_unique<vector<string>>(
							getLines(keyword, '|'));
					}
				}
				else if (DiceMsgReply::sEcho.count(attr)) {	//Echo=Reply
					trigger->echo = (DiceMsgReply::Echo)DiceMsgReply::sEcho[attr];
					if (trigger->echo == DiceMsgReply::Echo::Deck) {
						VarArray deck;
						while (intMsgCnt < strMsg.length()) {
							deck.push_back(readItem());
						}
						trigger->answer = AttrObject(deck);
					}
					else {
						if (trigger->echo == DiceMsgReply::Echo::Lua) {
							if (trusted < 5) {
								replyMsg("strNotMaster");
								return -1;
							}
							trigger->answer = AttrVars{ {"lua",readRest()} };
						}
						else if (trigger->echo == DiceMsgReply::Echo::JavaScript) {
							if (trusted < 5) {
								replyMsg("strNotMaster");
								return -1;
							}
							trigger->answer = AttrVars{ {"js",readRest()} };
						}
						else if (trigger->echo == DiceMsgReply::Echo::Python) {
							if (trusted < 5) {
								replyMsg("strNotMaster");
								return -1;
							}
							trigger->answer = AttrVars{ {"py",readRest()} };
						}
						else trigger->answer.set("text", readRest());
					}
					break;
				}
				attr = readToColon();
			}
			if (keyword.empty()) {
				replyMsg("strReplyKeyEmpty");
			}
			else {
				if (trigger->title.empty())trigger->title = keyword;
				set("key", trigger->title);
				fmt->set_reply(trigger->title, trigger);
				replyMsg("strReplySet");
			}
			return 1;
		}
		else if (action == "list") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			set("res", fmt->list_reply(2));
			replyMsg("strReplyList");
			return 1;
		}
		else if (action == "del") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			set("key", readRest());
			if (fmt->del_reply(get_str("key"))) {
				replyMsg("strReplyDel");
			}
			else {
				replyMsg("strReplyKeyNotFound");
			}
			return 1;
		}
		intMsgCnt = intMsgTmpCnt;
		ptr<DiceMsgReply> rep{ make_shared<DiceMsgReply>() };
		int MatchMode{ 0 };
		if (strLowerMessage.substr(intMsgCnt, 2) == "re") {
			intMsgCnt += 2;
			MatchMode = 3;
		}
		if (trusted < 4) {
			replyMsg("strNotAdmin");
			return -1;
		}
		at("key") = rep->title = readUntilSpace();
		if (rep->title.empty()) {
			replyHelp("reply");
			return -1;
		}
		rep->keyMatch[MatchMode] = std::make_unique<vector<string>>(getLines(rep->title, '|'));
		if (MatchMode == 3) {
			try {
				std::wregex re(convert_a2realw(rep->title.c_str()), std::regex::ECMAScript);
			} catch (const std::regex_error& e) {
				set("err", e.what());
				replyMsg("strRegexInvalid");
				return -1;
			}
		}
		if (vector<string> deck; readItems(deck)) {
			rep->answer = deck;
			fmt->set_reply(rep->title, rep);
			replyMsg("strReplySet");
		}
		else {
			fmt->del_reply(rep->title);
			replyMsg("strReplyDel");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "chan") {
		if (isPrivate())return 0;
		intMsgCnt += 4;
		string strPara{ readPara() };
		string QQNum = readDigit();
		if (!QQNum.empty() && QQNum != to_string(console.DiceMaid)
			&& (QQNum.length() != 4 || stoll(QQNum) != console.DiceMaid % 10000))return 0;
		if (strPara == "on") {
			pGrp->ChConf[fromChat.chid]["order"] = 1;
			replyMsg("strBotChannelOn");
		}
		else if (strPara == "off") {
			pGrp->ChConf[fromChat.chid]["order"] = -1;
			replyMsg("strBotChannelOff");
		}
	}
	if (is("order_off")) {
		if (isPrivate()){
			replyMsg("strGlobalOff");
			return 1;
		}
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "helpdoc" && trusted > 3)
	{
		intMsgCnt += 7;
		readSkipSpace();
		if (intMsgCnt == strMsg.length())
		{
			replyMsg("strHlpNameEmpty");
			return true;
		}
		string& key{ (at("key") = readUntilSpace()).text };
		string strHelp = readRest();
		if (strHelp.empty()){
			reply(fmt->prev_help(key),false);
		}
		else if (strHelp == "reset") {
			fmt->rm_help(key);
			replyMsg("strHlpReset");
		}
		else{
			fmt->set_help(key, strHelp);
			replyMsg("strHlpSet");
		}
		return true;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		set("help_word", readRest());
		if (!isPrivate())
		{
			if (!canRoomHost() && (at("help_word") == "on" || at("help_word") == "off"))
			{
				replyMsg("strPermissionDeniedErr");
				return 1;
			}
			set("option", "禁用help");
			if (at("help_word") == "off")
			{
				if (groupset(fromChat.gid, get_str("option")) < 1)
				{
					chat(fromChat.gid).set(get_str("option"));
					replyMsg("strGroupSetOn");
				}
				else
				{
					replyMsg("strGroupSetOnAlready");
				}
				return 1;
			}
			if (at("help_word") == "on")
			{
				if (groupset(fromChat.gid, get_str("option")) > 0)
				{
					chat(fromChat.gid).reset(get_str("option"));
					replyMsg("strGroupSetOff");
				}
				else
				{
					replyMsg("strGroupSetOffAlready");
				}
				return 1;
			}
			if (groupset(fromChat.gid, get_str("option")) > 0)
			{
				replyMsg("strGroupSetOnAlready");
				return 1;
			}
		}
		fmt->_help(this);
		return true;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "game") {
		intMsgCnt += 4;
		string action{ readPara() };
		if (action == "new") {
			if (canRoomHost()) {
				set("game_id", sessions.newGame(readFileName(), fromChat)->name);
				replyMsg("strGameNew");
			}
			else {
				replyMsg("strGameMasterDenied");
			}
			return 1;
		}
		else if (action == "over") {
			if (thisGame) {
				if (thisGame->is_gm(fromChat.uid) || api::isGroupAdmin(fromChat.gid, fromChat.uid, false)) {
					if (thisGame->is_logging())thisGame->log_end(this);
					set("game_id", thisGame->name);
					sessions.over(fromChat);
					replyMsg("strGameOver");
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else {
				replyMsg("strGameVoidHere");
			}
			return 1;
		}
		else if (action == "open") {
			string& name{ (at("game_id") = readFileName()).text };
			if (auto game{ sessions.getByName(name) }) {
				if (game->is_gm(fromChat.uid)) {
					sessions.open(game, fromChat);
					replyMsg("strGameAreaOpen");
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else {
				replyMsg("strGameNotExit");
			}
			return 1;
		}
		else if (action == "close") {
			if (thisGame) {
				if (thisGame->is_gm(fromChat.uid) || api::isGroupAdmin(fromChat.gid, fromChat.uid, false)) {
					set("game_id", thisGame->name);
					sessions.close(fromChat);
					replyMsg("strGameAreaClosed");
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else {
				replyMsg("strGameVoidHere");
			}
			return 1;
		}
		else if (action == "state") {
			if (thisGame) {
				reply(thisGame->show(), false);
			}
			else {
				replyMsg("strGameVoidHere");
			}
			return 1;
		}
		if (!thisGame)thisGame = sessions.get(fromChat);
		if (action == "join") {
			if (thisGame->add_pl(fromChat.uid)) {
				replyMsg("strGameJoined");
			}
			else {
				replyMsg("strGamePlayerAlready");
			}
		}
		else if (action == "call") {
			if (thisGame->is_gm(fromChat.uid)) {
				if (auto pls{ thisGame->get_pl() }; !pls->empty()) {
					ShowList res;
					for (auto& uid : *pls) {
						res << "[CQ:at,id=" + uid.to_string() + "]";
					}
					set("items", res.show("\n"));
					replyMsg("strGamePlayerCall");
				}
				else {
					replyMsg("strGamePlayerEmpty");
				}
			}
			else {
				replyMsg("strGameNotMaster");
			}
		}
		else if (action == "set") {
			if (thisGame->is_gm(fromChat.uid)) {
				auto [strItem, strVal] = readini(strMsg.substr(intMsgCnt));
				if (!strItem.empty()) {
					set("set_item", strItem);
					if (!strVal.empty()) {
						AttrVar& val{ at("set_val") = AttrVar::parse(strVal) };
						thisGame->setAttr(strItem, val);
						replyMsg("strGameItemSet");
					}
					else {
						set("set_val", print(thisGame->getAttr(strItem)));
						replyMsg("strGameItemShow");
					}
				}
				else {
					replyMsg("strGameItemEmpty");
				}
			}
			else {
				replyMsg("strGameNotMaster");
			}
		}
		else if (action == "exit") {
			if (thisGame->del_pl(fromChat.uid) || thisGame->del_gm(fromChat.uid)) {
				replyMsg("strGameExited");
			}
			else {
				replyMsg("strGameNotJoined");
			}
		}
		else if (action == "kick") {
			if (thisGame->is_gm(fromChat.uid)) {
				auto target_id{ readID() };
				set("tid", target_id ? AttrVar(target_id) : "");
				if (thisGame->del_pl(target_id) || thisGame->del_ob(target_id)) {
					replyMsg("strGameKicked");
				}
				else {
					replyMsg("strGameKickNotPlayer");
				}
			}
			else {
				replyMsg("strGameNotMaster");
			}
		}
		else if (action == "master") {
			auto gms{ thisGame->get_gm() };
			if (!thisGame->is_gm(fromChat.uid)) {
				if (gms->empty() ? canRoomHost() : api::isGroupAdmin(fromChat.gid, fromChat.uid, false)) {
					thisGame->add_gm(fromChat.uid);
					replyMsg("strGameMastered");
				}
				else {
					replyMsg("strGameMasterDenied");
				}
			}
			else {
				ShowList res;
				for (auto& uid : *gms) {
					res << printUser((long long)uid.to_double());
				}
				set("items", res.show("\n"));
				replyMsg("strGameMasterList");
			}
		}
		else if (action == "rou") {
			readSkipSpace();
			if (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
				if (thisGame->is_gm(fromChat.uid)) {
					if (int nFace = 100; !readNum(nFace) && nFace <= 100) {
						set("face", nFace);
						if (int nCopy = 1; '*' == strMsg[intMsgCnt] && !readNum(nCopy)) {
							if (nCopy <= 0) {
								replyMsg("strZeroDiceErr");
								return 1;
							}
							else if (nFace * nCopy > 100) {
								replyMsg("strGameRouletteTooBig");
								return 1;
							}
							else {
								thisGame->roulette[nFace] = DiceRoulette((size_t)nFace, (size_t)nCopy);
								thisGame->update();
							}
						}
						else {
							thisGame->roulette[nFace] = DiceRoulette((size_t)nFace);
							thisGame->update();
						}
						replyMsg("strGameRouletteSet");
					}
					else {
						replyMsg("strGameRouletteTooBig");
					}
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else if ((action = readPara()) == "hist") {
				if (auto& rous{ thisGame->roulette }; !rous.empty()) {
					ShowList hist;
					for (auto& [face, rou] : rous) {
						hist << "D" + to_string(face) + "=" + rou.hist();
					}
					set("hist", hist.show("\n"));
					replyMsg("strGameRouletteHistory");
				}
				else {
					replyMsg("strGameRouletteEmpty");
				}
			}
			else if ((action = readPara()) == "reset") {
				if (thisGame->is_gm(fromChat.uid)) {
					for (auto& [n, rou] : thisGame->roulette) {
						rou.reset();
					}
					replyMsg("strGameRouletteReset");
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else if (action == "clr") {
				if (thisGame->is_gm(fromChat.uid)) {
					thisGame->roulette.clear();
					replyMsg("strGameRouletteClear");
				}
				else {
					replyMsg("strGameNotMaster");
				}
			}
			else replyHelp("roulette");
		}
		else replyHelp("game");
		return 1;
	}
	return 0;
}

int DiceEvent::InnerOrder() {
	if (strMsg[0] != '.')return 0;
	if (WordCensor()) {
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			replyHelp("welcome");
			return 1;
		}
		if (isPrivate()) {
			replyMsg("strWelcomePrivate");
			return 1;
		}
		if (canRoomHost()) {
			string strWelcomeMsg = strMsg.substr(intMsgCnt);
			if (strWelcomeMsg == "clr") {
				if (chat(fromChat.gid).isset("入群欢迎")) {
					chat(fromChat.gid).reset("入群欢迎");
					replyMsg("strWelcomeMsgClearNotice");
				}
				else {
					replyMsg("strWelcomeMsgClearErr");
				}
			}
			else if (strWelcomeMsg == "show") {
				string strWelcome{ chat(fromChat.gid).confs.get_str("入群欢迎") };
				if (strWelcome.empty())replyMsg("strWelcomeMsgEmpty");
				else reply(strWelcome, false);	//转义有注入风险
			}
			else if (readPara() == "set") {
				chat(fromChat.gid).set("入群欢迎", strip(readRest()));
				replyMsg("strWelcomeMsgUpdateNotice");
			}
			else {
				chat(fromChat.gid).set("入群欢迎", strWelcomeMsg);
				replyMsg("strWelcomeMsgUpdateNotice");
			}
		}
		else {
			replyMsg("strPermissionDeniedErr");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "groups") {
		if (trusted < 4) {
			replyMsg("strNotAdmin");
			return 1;
		}
		intMsgCnt += 6;
		string strOption = readPara();
		if (strOption == "list") {
			set("list_mode", readPara());
			set("cmd", "lsgroup");
			sch.push_job(*this);
		}
		else if (strOption == "clr") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return 1;
			}
			note("已清理过期群记录" + to_string(clearGroup()) + "条", 0b10);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "setcoc") {
		if (!canRoomHost()) {
			replyMsg("strPermissionDeniedErr");
			return 1;
		}
		intMsgCnt += 6;
		string action{ readPara() };
		if (action == "show") {
			if ((thisGame || (thisGame = sessions.get_if(fromChat)))
				&& thisGame->attrs.has("rr_rc")) {
				set("rule", thisGame->getAttr("rr_rc"));
			}
			else if (isPrivate()) {
				if (User& user{ getUser(fromChat.uid) }; user.isset("rc房规"))
				set("rule", user.confs["rc房规"]);
			}
			else if (pGrp->isset("rc房规")) {
				set("rule", pGrp->confs["rc房规"]);
			}
			if (has("rule")) {
				replyMsg("strDefaultCOCShow");
			}
			else {
				set("rule", console["DefaultCOCRoomRule"]);
				replyMsg("strDefaultCOCShowDefault");
			}
			return 1;
		}
		else if (action == "clr") {
			if (thisGame
				&& thisGame->attrs.has("rr_rc")) {
				thisGame->rmAttr("rr_rc");
			}
			else if (isPrivate())getUser(fromChat.uid).rmConf("rc房规");
			else chat(fromChat.gid).reset("rc房规");
			replyMsg("strDefaultCOCClr");
			return 1;
		}
		string strRule = readDigit();
		if (strRule.empty()) {
			replyHelp("setcoc");
			return 1;
		}
		if (strRule.length() > 1) {
			replyMsg("strDefaultCOCNotFound");
			return 1;
		}
		int intRule = stoi(strRule);
		switch (intRule) {
		case 0:
			reply(getMsg("strDefaultCOCSet") + "0 规则书\n出1大成功\n不满50出96-100大失败，满50出100大失败");
			break;
		case 1:
			reply(getMsg("strDefaultCOCSet") + "1\n不满50出1大成功，满50出1-5大成功\n不满50出96-100大失败，满50出100大失败");
			break;
		case 2:
			reply(getMsg("strDefaultCOCSet") + "2\n出1-5且<=成功率大成功\n出100或出96-99且>成功率大失败");
			break;
		case 3:
			reply(getMsg("strDefaultCOCSet") + "3\n出1-5大成功\n出96-100大失败");
			break;
		case 4:
			reply(getMsg("strDefaultCOCSet") + "4\n出1-5且<=十分之一大成功\n不满50出>=96+十分之一大失败，满50出100大失败");
			break;
		case 5:
			reply(getMsg("strDefaultCOCSet") + "5\n出1-2且<五分之一大成功\n不满50出96-100大失败，满50出99-100大失败");
			break;
		case 6:
			reply(getMsg("strDefaultCOCSet") + "6\n绿色三角洲\n出1或出个位十位相同且<=成功率大成功\n出100或出个位十位相同且>成功率大失败");
			break;
		default:
			replyMsg("strDefaultCOCNotFound");
			return 1;
		}
		if (isPrivate())getUser(fromChat.uid).setConf("rc房规", intRule); 
		else chat(fromChat.gid).set("rc房规", intRule);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "system") {
		intMsgCnt += 6;
		if (console && trusted < 4) {
			replyMsg("strNotAdmin");
			return -1;
		}
		string strOption = readPara();
		if (strOption == "save") {
			dataBackUp();
			note("已手动保存{self}的数据√", 0b1);
			return 1;
		}
		if (strOption == "load") {
			loadData();
			note("已手动加载{self}的配置√", 0b1);
			return 1;
		}
		if (strOption == "state")
		{
			time_t tt = time(nullptr);
#ifdef _MSC_VER
			localtime_s(&stNow, &tt);
#else
			localtime_r(&tt, &stNow);
#endif
#ifdef _WIN32
			double mbFreeBytes = 0, mbTotalBytes = 0;
			long long milDisk(getDiskUsage(mbFreeBytes, mbTotalBytes));
#endif
			ResList res;
			res << "本地时间:" + printSTime(stNow)
#ifdef _WIN32
				<< "内存占用:" + to_string(getRamPort()) + "%"
				<< "CPU占用:" + toString(getWinCpuUsage() / 10.0) + "%"
				<< "硬盘占用:" + toString(milDisk / 10.0) + "%(空余:" + toString(mbFreeBytes) + "GB/ " + toString(mbTotalBytes) + "GB)"
#endif
				<< "运行时长:" + printDuringTime(time(nullptr) - llStartTime)
				<< "今日指令量:" + today->get("frq").to_str()
				<< "启动后指令量:" + to_string(FrqMonitor::sumFrqTotal);
			reply(res.show());
			return 1;
		}
		if (strOption == "clrimg") {
			reply("非酷Q框架不需要此功能");
			return -1;
		}
		else if (strOption == "reload") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return -1;
			}
			set("cmd", "reload");
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "remake") {
			
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return -1;
			}
			set("cmd", "remake");
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "die") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return -1;
			}
			set("cmd", "die");
			sch.push_job(*this);
			return 1;
		}
		if (strOption == "rexplorer")
		{
#ifdef _WIN32
			if (trusted < 5)
			{
				replyMsg("strNotMaster");
				return -1;
			}
			system(R"(taskkill /f /fi "username eq %username%" /im explorer.exe)");
			system(R"(start %SystemRoot%\explorer.exe)");
			this_thread::sleep_for(3s);
			note("已重启资源管理器√\n当前内存占用：" + to_string(getRamPort()) + "%");
#endif
		}
		else if (strOption == "cmd")
		{
#ifdef _WIN32
			if (isFromMaster())
			{
				replyMsg("strNotMaster");
				return -1;
			}
			string strCMD = readRest() + "\ntimeout /t 10";
			system(strCMD.c_str());
			reply("已启动命令行√");
			return 1;
#endif
		}
	}
	else if (string pref5{ strLowerMessage.substr(intMsgCnt, 5) }; pref5 == "admin") {
		intMsgCnt += 5;
		return AdminEvent(readPara());
	}
	else if (pref5 == "cloud") {
		intMsgCnt += 5;
		string strOpt = readPara();
		if (trusted < 4) {
			replyMsg("strNotAdmin");
			return 1;
		}
		if (strOpt == "update") {
			set("ver", readPara());
			set("cmd", "update");
			sch.push_job(*this);
			return 1;
		}
		else if (strOpt == "black") {
			set("cmd", "cloudblack");
			sch.push_job(*this);
			return 1;
		}
	}
	else if (pref5 == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd") {
		set("res", COC7D());
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (pref5 == "coc6d") {
		set("res", COC6D());
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (pref5 == "group") {
		intMsgCnt += 5;
		long long llGroup(fromChat.gid);
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			replyHelp("group");
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "all") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return 1;
			}
			intMsgCnt += 3;
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-') {
				bool isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				string strOption{ (at("option") = readRest()).to_str() };
				if (!mChatConf.count(get_str("option"))) {
					replyMsg("strGroupSetNotExist");
					return 1;
				}
				int Cnt = 0;
				if (isSet) {
					for (auto& [id, grp] : ChatList) {
						if (grp.isset(strOption))continue;
						grp.set(strOption);
						Cnt++;
					}
					set("cnt", Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
				else {
					for (auto& [id, grp] : ChatList) {
						if (!grp.isset(strOption))continue;
						grp.reset(strOption);
						Cnt++;
					}
					set("cnt", Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
			}
			return 1;
		}
		if (!(at("group_id") = readDigit(false)).str_empty()) {
			llGroup = stoll(get_str("group_id"));
			if (!ChatList.count(llGroup)) {
				replyMsg("strGroupNotFound");
				return 1;
			}
			if (getGroupTrust(llGroup) < 0) {
				replyMsg("strGroupDenied");
				return 1;
			}
		}
		else if (isPrivate())return 0;
		else set("group_id", to_string(fromChat.gid));
		Chat& grp = chat(llGroup);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if(strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'){
			int cntSet{ 0 };
			bool isSet{ strMsg[intMsgCnt] == '+' };
			do{
				isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				const string option{ (at("option") = readPara()).to_str() };
				readSkipSpace();
				if (!mChatConf.count(get_str("option"))) {
					replyMsg("strGroupSetInvalid");
					continue;
				}
				if (getGroupTrust(llGroup) >= ::get<string, short>(mChatConf, option, 0)) {
					if (isSet) {
						if (groupset(llGroup, get_str("option")) < 1) {
							chat(llGroup).set(get_str("option"));
							++cntSet;
							if (at("option") == "许可使用") {
								AddMsgToQueue(getMsg("strGroupAuthorized", *this), { 0, fromChat.uid });
							}
						}
						else {
							replyMsg("strGroupSetOnAlready");
						}
					}
					else if (grp.isset(get_str("option"))) {
						++cntSet;
						chat(llGroup).reset(get_str("option"));
					}
					else {
						replyMsg("strGroupSetOffAlready");
					}
				}
				else {
					replyMsg("strGroupSetDenied");
				}
			} while (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-');
			if (cntSet == 1) {
				isSet ? replyMsg("strGroupSetOn") : replyMsg("strGroupSetOff");
			}
			else if(cntSet > 1) {
				set("opt_list", grp.listBoolConf());
				replyMsg("strGroupMultiSet");
			}
			return 1;
		}
		bool isInGroup{ fromChat.gid == llGroup };
		string Command = readPara();
		set("group", printGroup(llGroup));
		if (Command.empty()) {
			replyHelp("group");
			return 1;
		}
		else if (Command == "state") {
			ResList res;
			res << "在{group}：";
			res << grp.listBoolConf();
			res << "记录创建：" + printDate(grp.tCreated);
			res << "最后记录：" + printDate(grp.updated());
			if (grp.inviter)res << "邀请者：" + printUser(grp.inviter);
			res << string("入群欢迎：") + (grp.isset("入群欢迎") ? "已设置" : "无");
			reply(getMsg("strSelfName") + res.show());
			return 1;
		}
		if (!grp.isGroup || (fromChat.gid == llGroup && isPrivate())) {
			replyMsg("strGroupNot");
			return 1;
		}
		else if (Command == "tojson") {
			if (trusted < 4 && !console.is_self(fromChat.uid)) {
				replyMsg("strNotAdmin");
				return 1;
			}
			if (ChatList.count(llGroup)) {
				reply(chat(llGroup).confs.to_json().dump(), false);
			}
			else {
				reply("{self}无" + printGroup(llGroup) + "的群聊记录×");
			}
			return 1;
		}
		else if (!isInGroup) {
			replyMsg("strGroupNotIn");
			return 1;
		}
		if (Command == "pause") {
			int secDuring(-1);
			string strDuring{ readDigit() };
			if (!strDuring.empty())secDuring = stoi(strDuring);
			//DD::setGroupWholeBan(llGroup, secDuring);
			replyMsg("strGroupWholeBan");
			return 1;
		}
		else if (Command == "restart") {
			//DD::setGroupWholeBan(llGroup, 0);
			replyMsg("strGroupWholeUnban");
			return 1;
		}
		return 1;
	}
	else if (pref5 == "rules") {
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			replyHelp("rules");
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = strMsg.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				getUser(fromChat.uid).rmConf("默认规则");
				replyMsg("strRuleReset");
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				getUser(fromChat.uid).setConf("默认规则", strDefaultRule);
				replyMsg("strRuleSet");
			}
		}
		else {
			string strSearch = strMsg.substr(intMsgCnt);
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			if (auto rule{ getGameRule() }; GetRule::get(*rule, strSearch, strReply)) {
				reply();
			}
			else if (getUser(fromChat.uid).isset("默认规则") && strSearch.find(':') == string::npos &&
				GetRule::get(getUser(fromChat.uid).confs.get_str("默认规则"), strSearch, strReply)) {
				reply();
			}
			else if (GetRule::analyze(strSearch, strReply)) {
				reply();
			}
			else {
				reply(getMsg("strRuleErr") + strReply);
			}
		}
		return 1;
	}
	else if (string pref4{ strLowerMessage.substr(intMsgCnt, 4) }; pref4 == "coc6") {
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2) {
			replyMsg("strCharacterTooBig");
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10) {
			replyMsg("strCharacterTooBig");
			return 1;
		}
		if (intNum == 0) {
			replyMsg("strCharacterCannotBeZero");
			return 1;
		}
		set("res",COC6(intNum));
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (pref4 == "deck") {
		if (trusted < 4 && console["DisabledDeck"]) {
			replyMsg("strDisabledDeckGlobal");
			return 1;
		}
		intMsgCnt += 4;
		if (strMsg.length() == intMsgCnt) {
			replyHelp("deck");
			return 1;
		}
		string strPara = readPara();
		if (strPara == "show") {
			if (thisGame)thisGame->deck_show(this);
			else replyMsg("strDeckListEmpty");
		}
		else if (!canRoomHost()  && !trusted) {
			replyMsg("strWhiteQQDenied");
		}
		else if (strPara == "new") {
			if (!thisGame)thisGame = sessions.get(fromChat);
			thisGame->deck_new(this);
		}
		else if (strPara == "set") {
			if (!thisGame)thisGame = sessions.get(fromChat);
			thisGame->deck_set(this);
		}
		else if (!thisGame) {
			replyMsg("strDeckListEmpty");
		}
		else if (strPara == "reset") {
			thisGame->deck_reset(this);
		}
		else if (strPara == "del") {
			thisGame->deck_del(this);
		}
		else if (strPara == "clr") {
			thisGame->deck_clr(this);
		}
		return 1;
	}
	else if (pref4 == "draw") {
		if (trusted < 4 && console["DisabledDraw"]) {
			replyMsg("strDisabledDrawGlobal");
			return 1;
		}
		set("option","禁用draw");
		if (!isPrivate() && groupset(fromChat.gid, "禁用draw") > 0) {
			replyMsg("strGroupSetOnAlready");
			return 1;
		}
		intMsgCnt += 4;
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			set("hidden");
			++intMsgCnt;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		vector<string> ProDeck;
		vector<string>* TempDeck = nullptr;
		string& key{ (at("deck_name") = readAttrName()).text };
		while (!key.empty() && key[0] == '_') {
			set("hidden");
			key.erase(key.begin());
		}
		if (key.empty()) {
			replyHelp("draw");
			return 1;
		}
		else {
			if (thisGame && thisGame->has_deck(key)) {
				thisGame->_draw(this);
				return 1;
			}
			else if (CardDeck::findDeck(key) == 0) {
				strReply = getMsg("strDeckNotFound");
				reply(strReply);
				return 1;
			}
			ProDeck = CardDeck::mPublicDeck[key];
			TempDeck = &ProDeck;
		}
		int intCardNum = 1;
		switch (readNum(intCardNum)) {
		case 0:
			if (intCardNum == 0) {
				replyMsg("strNumCannotBeZero");
				return 1;
			}
			break;
		case -1: break;
		case -2:
			replyMsg("strParaIllegal");
			console.log("提醒:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName") + "使用了非法指令参数\n" + strMsg, 1,
						printSTNow());
			return 1;
		}
		ResList Res;
		while (intCardNum--) {
			Res << CardDeck::drawCard(*TempDeck);
			if (TempDeck->empty())break;
		}
		set("res",Res.dot("|").show());
		set("cnt",to_string(Res.size()));
		if (is("hidden")) {
			replyMsg("strDrawHidden");
			replyHidden(getMsg("strDrawCard"));
		}
		else
			replyMsg("strDrawCard");
		if (intCardNum > 0) {
			replyMsg("strDeckEmpty");
			return 1;
		}
		return 1;
	}
	else if (pref4 == "init") {
		intMsgCnt += 4;
		set("table_name","先攻");
		string strCmd = readPara();
		if (strCmd.empty()|| isPrivate()) {
			replyHelp("init");
		}
		else if (!thisGame || !thisGame->table_count("先攻")) {
			replyMsg("strGMTableNotExist");
		}
		else if (strCmd == "show" || strCmd == "list") {
			set("res", thisGame->table_prior_show("先攻"));
			replyMsg("strGMTableShow");
		}
		else if (strCmd == "del") {
			set("table_item",readRest());
			if (is_empty("table_item"))
				replyMsg("strGMTableItemEmpty");
			else if (thisGame->table_del("先攻", get_str("table_item")))
				replyMsg("strGMTableItemDel");
			else
				replyMsg("strGMTableItemNotFound");
		}
		else if (strCmd == "clr") {
			thisGame->table_clr("先攻");
			replyMsg("strGMTableClr");
		}
		return 1;
	}
	else if (pref4 == "jrrp") {
		if (console["DisabledJrrp"]) {
			reply("&strDisabledJrrpGlobal");
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (!isPrivate()) {
			if (Command == "on") {
				if (canRoomHost()) {
					if (groupset(fromChat.gid, "禁用jrrp") > 0) {
						chat(fromChat.gid).reset("禁用jrrp");
						reply("成功在本群中启用JRRP!");
					}
					else {
						reply("在本群中JRRP没有被禁用!");
					}
				}
				else {
					replyMsg("strPermissionDeniedErr");
				}
				return 1;
			}
			if (Command == "off") {
				if (canRoomHost()) {
					if (groupset(fromChat.gid, "禁用jrrp") < 1) {
						chat(fromChat.gid).set("禁用jrrp");
						reply("成功在本群中禁用JRRP!");
					}
					else {
						reply("在本群中JRRP没有被启用!");
					}
				}
				else {
					replyMsg("strPermissionDeniedErr");
				}
				return 1;
			}
			if (groupset(fromChat.gid, "禁用jrrp") > 0) {
				reply("在本群中JRRP功能已被禁用");
				return 1;
			}
		}
		set("res",today->getJrrp(fromChat.uid));
		replyMsg("strJrrp");
		return 1;
	}
	else if (pref4 == "name") {
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;

		string type = readPara();
		string strNum = readDigit();
		if (strNum.length() > 1 && strNum != "10") {
			replyMsg("strNameNumTooBig");
			return 1;
		}
		int intNum = strNum.empty() ? 1 : stoi(strNum);
		if (intNum == 0) {
			replyMsg("strNameNumCannotBeZero");
			return 1;
		}
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("随机姓名_" + type)) ? "随机姓名_" + type : "随机姓名";
		vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
		ResList Res;
		while (intNum--) {
			Res << CardDeck::drawCard(TempDeck, true);
		}
		set("res",Res.dot("、").show());
		replyMsg("strNameGenerator");
		return 1;
	}
	else if (pref4 == "send") {
		intMsgCnt += 4;
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			replyHelp("send");
			return 1;
		}
		//先考虑Master带参数向指定目标发送
		if (trusted > 2) {
			chatInfo ct;
			if (!readChat(ct, true)) {
				readSkipColon();
				string strFwd(readRest());
				if (strFwd.empty()) {
					replyMsg("strSendMsgEmpty");
				}
				else {
					AddMsgToQueue(fmt->format(strFwd, ct.gid ?
						AttrVars{ {"gid",ct.gid}} : AttrVars{ {"uid",ct.uid} }), ct);
					replyMsg("strSendMsg");
				}
				return 1;
			}
			else if (strLowerMessage.substr(intMsgCnt, 6) == "notice" && trusted > 3) {
				intMsgCnt += 6;
				int intLv = 0;
				string strNum{ readDigit(false) };
				while (!strNum.empty()){
					if (strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 9)continue;
					else 					{
						intLv |= (1 << intNum);
					}
					if (strLowerMessage[intMsgCnt] == '+')++intMsgCnt;
					strNum = readDigit(false);
				}
				string strNotice(readRest());
				if (intLv && !strNotice.empty()){
					console.log(strNotice, intLv);
					replyMsg("strSendMsg");
				}
				else replyMsg("strParaEmpty");
				return 1;
			}
			readSkipColon();
		}
		else if (!console) {
			replyMsg("strSendMsgInvalid");
			return 1;
		}
		else if (console["DisabledSend"] && trusted < 3) {
			replyMsg("strDisabledSendGlobal");
			return 1;
		}
		string strInfo = readRest();
		if (strInfo.empty()) {
			replyMsg("strSendMsgEmpty");
			return 1;
		}
		string strFwd = ((trusted > 4) ? "| " : ("| " + printFrom())) + strInfo;
		console.log(strFwd, 0b100, printSTNow());
		replyMsg("strSendMasterMsg");
		return 1;
	}
	else if (pref4 == "user") {
		intMsgCnt += 4;
		string strOption = readPara();
		if (strOption.empty())return 0;
		if (strOption == "state") {
			User& user = getUser(fromChat.uid);
			set("user",printUser(fromChat.uid));
			ResList rep;
			rep << "信任级别：" + to_string(trusted)
				<< "和{nick}的第一印象大约是在" + printDate(user.tCreated)
				<< (!(user.strNick.empty()) ? "正记录{nick}的" + to_string(user.strNick.size()) + "个称呼" : "没有记录{nick}的称呼")
				<< ((PList.count(fromChat.uid)) ? "这里有{nick}的" + to_string(PList[fromChat.uid].size()) + "张角色卡" : "无角色卡记录");
			reply("{user}" + rep.show());
			return 1;
		}
		if (strOption == "trust") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return 1;
			}
			string strTarget = readDigit();
			if (strTarget.empty()) {
				replyMsg("strUidEmpty");
				return 1;
			}
			long long llTarget = stoll(strTarget);
			if ((trustedQQ(llTarget) >= trusted && !console.is_self(fromChat.uid) && fromChat.uid != llTarget) || isVirtual) {
				replyMsg("strUserTrustDenied");
				return 1;
			}
			set("user",printUser(llTarget));
			set("trust",readDigit());
			if (is_empty("trust")) {
				if (!UserList.count(llTarget)) {
					replyMsg("strUserNotFound");
					return 1;
				}
				set("trust",trustedQQ(llTarget));
				replyMsg("strUserTrustShow");
				return 1;
			}
			User& user = getUser(llTarget);
			if (int intTrust = get_int("trust"); intTrust < 0 || intTrust > 255 || (intTrust >= trusted && isFromMaster())) {
				replyMsg("strUserTrustIllegal");
				return 1;
			}
			else {
				user.trust(intTrust);
			}
			replyMsg("strUserTrusted");
			return 1;
		}
		if (strOption == "tojson") {
			if (trusted < 4 && !console.is_self(fromChat.uid)) {
				replyMsg("strNotAdmin");
				return 1;
			}
			long long target{ readID() };
			if (!target)target = fromChat.uid;
			if(UserList.count(target)){
				reply(getUser(target).confs.to_json().dump(), false);
			}
			else {
				reply("{self}无" + printUser(target) + "的用户记录×");
			}
			return 1;
		}
		if (strOption == "diss") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return 1;
			}
			set("note",readPara());
			long long llTargetID(readID());
			if (!llTargetID) {
				replyMsg("strUidEmpty");
			}
			else if (trustedQQ(llTargetID) >= trusted) {
				replyMsg("strUserTrustDenied");
			}
			else {
				//blacklist->add_black_qq(llTargetID, this);
				UserList.erase(llTargetID);
				PList.erase(llTargetID);
			}
			return 1;
		}
		if (strOption == "kill") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return 1;
			}
			long long llTarget = readID();
			if (trustedQQ(llTarget) >= trusted && isFromMaster()) {
				replyMsg("strUserTrustDenied");
				return 1;
			}
			set("user",printUser(llTarget));
			if (!llTarget || !UserList.count(llTarget)) {
				replyMsg("strUserNotFound");
				return 1;
			}
			UserList.erase(llTarget);
			reply("已抹除{user}的用户记录");
			return 1;
		}
		if (strOption == "clr") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return 1;
			}
			int cnt = clearUser();
			note("已清理无效或过期用户记录" + to_string(cnt) + "条", 0b10);
			return 1;
		}
	}
	else if (string pref3{ strLowerMessage.substr(intMsgCnt, 3) }; pref3 == "coc") {
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			replyMsg("strCharacterTooBig");
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			replyMsg("strCharacterCannotBeZero");
			return 1;
		}
		set("res",COC7(intNum));
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (pref3 == "dnd") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			replyMsg("strCharacterTooBig");
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			replyMsg("strCharacterCannotBeZero");
			return 1;
		}
		set("res",DND(intNum));
		replyMsg("strDNDBuild");
		return 1;
	}
	else if (pref3 == "log") {
		intMsgCnt += 3;
		string strPara = readPara();
		if (strPara.empty()) {
			replyHelp("log");
		}
		else if (strPara == "new") {
			sessions.get(fromChat)->log_new(this);
		}
		else if (strPara == "on") {
			sessions.get(fromChat)->log_on(this);
		}
		else if (strPara == "off") {
			sessions.get(fromChat)->log_off(this);
		}
		else if (strPara == "end") {
			sessions.get(fromChat)->log_end(this);
		}
		else {
			replyHelp("log");
		}
		return 1;
	}
	else if (pref3 == "mod") {
	if (trusted < 4) {
		replyMsg("strNotAdmin");
		return 1;
	}
	intMsgCnt += 3;
	string strPara = readPara();
	if (strPara.empty()) {
		replyHelp("mod");
	}
	else if (strPara == "list") {
		set("li",fmt->list_mod());
		replyMsg("strModList");
	}
	else if (string& modName{ (at("mod") = readRest()).text }; modName.empty()) {
		replyMsg("strModNameEmpty");
	}
	else if (strPara == "get") {
		fmt->mod_install(*this);
	}
	else if (strPara == "update") {
		fmt->has_mod(modName) ? fmt->mod_update(*this) : fmt->mod_install(*this);
	}
	else if (!fmt->has_mod(modName)) {
		replyMsg("strModNotFound");
	}
	else if (strPara == "on") {
		fmt->mod_on(this);
	}
	else if (strPara == "off") {
		fmt->mod_off(this);
	}
	else if (strPara == "info") {
		set("mod_desc", fmt->get_mod(modName)->desc());
		replyMsg("strModDescLocal");
	}
	else if (strPara == "detail") {
		set("mod_detail", fmt->get_mod(modName)->detail());
		replyMsg("strModDetail");
	}
	else if (strPara == "reload") {
		string err;
		if (fmt->get_mod(modName)->reload(err)) {
			fmt->build();
			replyMsg("strModReload");
		}
		else {
			set("err", err);
			replyMsg("strModLoadErr");
		}
	}
	else if (strPara == "del") {
		fmt->uninstall(modName);
		note("{strModDelete}", 1);
	}
	else if (strPara == "reinstall") {
		fmt->mod_reinstall(*this);
	}
	else replyHelp("mod");
	return 1;
}
	else if (pref3 == "nnn") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string type = readPara();
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("随机姓名_" + type)) ? "随机姓名_" + type : "随机姓名";
		set("old_nick",idx_nick(*this));
		set("new_nick",strip(CardDeck::drawCard(CardDeck::mPublicDeck[strDeckName], true)));
		getUser(fromChat.uid).setNick(fromChat.gid, get_str("new_nick"));
		replyMsg("strNameSet");
		return 1;
	}
	else if (pref3 == "set") {
		intMsgCnt += 3;
		readSkipSpace();
		string& strDice{ (at("default") = readDigit()).text};
		while (strDice[0] == '0')
			strDice.erase(strDice.begin());
		if (strDice.empty())
			strDice = "100";
		if (strDice.length() > 4) {
			replyMsg("strSetTooBig");
			return 1;
		}
		const int intDefaultDice = stoi(strDice);
		if (PList.count(fromChat.uid)) {
			PList[fromChat.uid][fromChat.gid]->set("__DefaultDice", intDefaultDice);
			replyMsg("strSetDefaultDice");
			return 1;
		}
		else if (intDefaultDice == 100)
			getUser(fromChat.uid).rmConf("默认骰");
		else
			getUser(fromChat.uid).setConf("默认骰", intDefaultDice);
		replyMsg("strSetDefaultDice");
		return 1;
	}
	else if (pref3 == "str" && trusted > 3) {
		string strName;
		while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length()
			   ) {
			strName += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		while (strMsg[intMsgCnt] == ' ')intMsgCnt++;
		if (intMsgCnt == strMsg.length() || strMsg.substr(intMsgCnt) == "show") {
			AddMsgToQueue(fmt->msg_get(strName), fromChat);
			return 1;
		}
		string strMessage = strMsg.substr(intMsgCnt);
		if (strMessage == "reset") {
			fmt->msg_reset(strName);
			note("已重置" + strName + "的自定义。", 0b1);
		}
		else {
			if (strMessage == "NULL")strMessage = "";
			fmt->msg_edit(strName, strMessage);
			note("已自定义" + strName + "的文本", 0b1);
		}
		return 1;
	}
	else if (string pref2{ strLowerMessage.substr(intMsgCnt, 2) }; pref2 == "ak") {
	intMsgCnt += 2;
	readSkipSpace();
	if (intMsgCnt == strMsg.length()) {
		replyHelp("ak");
		return 1;
	}
	auto s{ sessions.get(fromChat) };
	char sign{ strMsg[intMsgCnt] };
	string action{ readPara() };
	if (sign == '#' || action == "new") {
		if (sign == '#')++intMsgCnt;
		s->get_deck().erase("__Ank");
		if (string strTitle{ strMsg.substr(intMsgCnt,strMsg.find('+') - intMsgCnt) }; !strTitle.empty()) {
			intMsgCnt += strTitle.length();
			s->setAttr("AkFork", at("fork") = strTitle);
		}
		if (intMsgCnt == strMsg.length()) {
			replyMsg("strAkForkNew");
		}
		else {
			sign = strMsg[intMsgCnt];
		}
	}
	if (sign == '+' || action == "add") {
		if (sign == '+')++intMsgCnt;
		std::vector<string>& deck{ s->get_deck("__Ank").meta };
		if (!readItems(deck)) {
			replyMsg("strAkAddEmpty");
			return 1;
		}
		set("fork",s->attrs["AkFork"]);
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		set("li",list.linebreak().show());
		replyMsg("strAkAdd");
		s->update();
	}
	else if (sign == '-' || action == "del") {
		if (sign == '-')++intMsgCnt;
		std::vector<string>& deck{ s->get_deck("__Ank").meta };
		int nNo{ 0 };
		if (readNum(nNo) || nNo <= 0 || nNo > deck.size()) {
			replyMsg("strAkNumErr");
			return 1;
		}
		deck.erase(deck.begin() + nNo - 1);
		set("fork",s->attrs["AkFork"]);
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		set("li",list.linebreak().show());
		replyMsg("strAkDel");
		s->update();
	}
	else if (sign == '=' || action == "get") {
		if (DeckInfo& deck{ s->get_deck("__Ank") }; !deck.meta.empty()) {
			set("fork",s->attrs["AkFork"]);
			s->rmAttr("AkFork");
			size_t res{ (size_t)RandomGenerator::Randint(0,deck.meta.size() - 1) };
			set("get",to_string(res + 1) + ". " + deck.meta[res]);
			ResList list;
			list.order();
			for (auto& val : deck.meta) {
				list << val;
			}
			set("li",list.linebreak().show());
			s->get_deck().erase("__Ank");
			replyMsg("strAkGet");
		}
		else {
			replyMsg("strAkOptEmptyErr");
		}
		s->update();
	}
	else if (action == "show") {
		std::vector<string>& deck{ s->get_deck("__Ank").meta };
		set("fork",s->attrs["AkFork"]);
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		set("li",list.linebreak().show());
		replyMsg("strAkShow");
	}
	else if (action == "clr") {
		s->get_deck().erase("__Ank");
		set("fork",s->attrs["AkFork"]);
		s->rmAttr("AkFork");
		replyMsg("strAkClr");
		s->update();
	}
	if(strReply.empty())replyHelp("ak");
	return 1;
	}
	else if (pref2 == "en") {
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (strMsg.length() == intMsgCnt) {
		replyHelp("en");
		return 1;
	}
	string& strAttr{ (at("attr") = readAttrName()).text};
	string strCurrentValue{ readDigit(false) };
	PC pc{ PList.count(fromChat.uid) ? getPlayer(fromChat.uid)[fromChat.gid] : std::make_shared<CharaCard>()};
	int intVal{ 0 };
		//获取技能原值
		if (strCurrentValue.empty()) {
			if (pc && !strAttr.empty() && (pc->stored(strAttr))) {
				intVal = pc->get(strAttr).to_int();
			}
			else {
				replyMsg("strEnValEmpty");
				return 1;
			}
		}
		else {
			if (strCurrentValue.length() > 3) {
				replyMsg("strEnValInvalid");
				return 1;
			}
			intVal = stoi(strCurrentValue);
		}
		readSkipSpace();
		//可变成长值表达式
		string strEnChange;
		string strEnFail;
		string strEnSuc = "1D10";
		//以加减号做开头确保与技能值相区分
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strEnChange = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			//没有'/'时默认成功变化值
			if (strEnChange.find('/') != std::string::npos) {
				strEnFail = strEnChange.substr(0, strEnChange.find('/'));
				strEnSuc = strEnChange.substr(strEnChange.find('/') + 1);
			}
			else strEnSuc = strEnChange;
		}
		if (strAttr.empty())strAttr = getMsg("strEnDefaultName");
		const int intTmpRollRes = (thisGame && thisGame->is_part(fromChat.uid)) ? thisGame->roll(100) : RandomGenerator::Randint(1, 100);
		//成长检定仅计入掷骰统计，不计入检定统计
		if (pc)pc->cntRollStat(intTmpRollRes, 100);
		string& res{ (at("res") = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intVal) + " ").text};
		if (intTmpRollRes <= intVal && intTmpRollRes <= 95) {
			if (strEnFail.empty()) {
				res += getMsg("strFailure");
				replyMsg("strEnRollNotChange");
				return 1;
			}
			res += getMsg("strFailure");
			RD rdEnFail(strEnFail);
			if (rdEnFail.Roll()) {
				replyMsg("strValueErr");
				return 1;
			}
			intVal = intVal + rdEnFail.intTotal;
			set("change",rdEnFail.FormCompleteString());
			set("final", intVal);
			replyMsg("strEnRollFailure");
		}
		else {
			res += getMsg("strSuccess");
			RD rdEnSuc(strEnSuc);
			if (rdEnSuc.Roll()) {
				replyMsg("strValueErr");
				return 1;
			}
			intVal = intVal + rdEnSuc.intTotal;
			set("change",rdEnSuc.FormCompleteString());
			set("final",intVal);
			replyMsg("strEnRollSuccess");
		}
		if (pc)pc->set(strAttr, intVal);
		return 1;
	}
	else if (pref2 == "li") {
		LongInsane(*this);
		replyMsg("strLongInsane");
		return 1;
	}
	else if (pref2 == "nn") {
		intMsgCnt += 2;
		readSkipSpace();
		if (intMsgCnt == strMsg.length()) {
			replyHelp("nn");
			return 1;
		}
		set("old_nick",idx_nick(*this));
		string& strNN{ (at("new_nick") = strip(filter_CQcode(strMsg.substr(intMsgCnt),fromChat.gid))).text};
		if (strNN.length() > 50) {
			replyMsg("strNameTooLongErr");
			return 1;
		}
		if (strNN.empty()) {
			replyHelp("nn");
		}
		else if(strNN == "del") {
			if (getUser(fromChat.uid).rmNick(fromChat.gid)) {
				replyMsg("strNameDel");
			}
			else {
				replyMsg("strNameDelEmpty");
			}
		}
		else if (strNN == "clr") {
			getUser(fromChat.uid).clrNick();
			replyMsg("strNameClr");
		}
		else {
			getUser(fromChat.uid).setNick(fromChat.gid, strNN);
			replyMsg("strNameSet");
		}
		return 1;
	}
	else if (pref2 == "ob") {
		if (isPrivate()) {
			replyHelp("ob");
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string strOption = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (strOption.empty()) {
			replyHelp("ob");
			return 1;
		}
		if (!canRoomHost() && (strOption == "on" || strOption == "off")) {
			replyMsg("strPermissionDeniedErr");
			return 1;
		}
		string& option { (at("option") = "禁用ob").text };
		if (strOption == "off") {
			if (groupset(fromChat.gid, option) < 1) {
				chat(fromChat.gid).set(option);
				if (thisGame)thisGame->clear_ob();
				replyMsg("strObOff");
			}
			else {
				replyMsg("strObOffAlready");
			}
			return 1;
		}
		if (strOption == "on") {
			if (groupset(fromChat.gid, option) > 0) {
				chat(fromChat.gid).reset(option);
				replyMsg("strObOn");
			}
			else {
				replyMsg("strObOnAlready");
			}
			return 1;
		}
		if (groupset(fromChat.gid, option) > 0) {
			replyMsg("strObOffAlready");
			return 1;
		}
		if (strOption == "join") {
			sessions.get(fromChat)->ob_enter(this);
		}
		else if (!thisGame) {
			replyMsg("strObListEmpty");
		}
		else if (strOption == "list") {
			thisGame->ob_list(this);
		}
		else if (strOption == "clr") {
			if (canRoomHost()) {
				thisGame->ob_clr(this);
			}
			else {
				replyMsg("strPermissionDeniedErr");
			}
		}
		else if (strOption == "exit") {
			thisGame->ob_exit(this);
		}
		else {
			replyHelp("ob");
		}
		return 1;
	}
	else if (pref2 == "pc") {
		intMsgCnt += 2;
		string strOption = readPara();
		if (strOption.empty()) {
			replyHelp("pc");
			return 1;
		}
		Player& pl = getPlayer(fromChat.uid);
		int resno = 0;
		if (strOption == "tag") {
			set("char",readRest());
			switch (resno = pl.changeCard(get_str("char"), fromChat.gid)) {
			case 1:
				replyMsg("strPcCardReset");
				return 1;
				break;
			case 0:
				replyMsg("strPcCardSet");
				break;
			}
		}
		else if (strOption == "show") {
			string strName = readRest();
			auto pc{ pl.getCard(strName, fromChat.gid) };
			set("char",pc->getName());
			set("type",pc->get_str("__Type"));
			set("show",pc->show(true));
			replyMsg("strPcCardShow");
			return 1;
		}
		else if (strOption == "new") {
			string& strPC{ (at("char") = strip(filter_CQcode(readRest(), fromChat.gid))).text};
			if (!(resno = pl.newCard(strPC, fromChat.gid))) {
				set("type", pl[fromChat.gid]->get_str("__Type"));
				set("show", pl[fromChat.gid]->show(true));
				if (is_empty("show"))replyMsg("strPcNewEmptyCard");
				else replyMsg("strPcNewCardShow");
			}
		}
		else if (strOption == "build") {
			string& strPC{ (at("char") = strip(filter_CQcode(readRest(), fromChat.gid))).text};
			if (!(resno = pl.buildCard(strPC, false, fromChat.gid))) {
				set("show", pl[strPC]->show(true));
				replyMsg("strPcCardBuild");
			}
		}
		else if (strOption == "list") {
			set("show",pl.listCard());
			replyMsg("strPcCardList");
			return 1;
		}
		else if (strOption == "nn") {
			string& strPC{ (at("new_name") = strip(filter_CQcode(readRest(),fromChat.gid))).text};
			set("old_name",pl[fromChat.gid]->getName());
			if (!(resno = pl.renameCard(get_str("old_name"), strPC)))replyMsg("strPcCardRename");
		}
		else if (strOption == "del") {
			set("char",strip(readRest()));
			if (!(resno = pl.removeCard(get_str("char"))))replyMsg("strPcCardDel");
		}
		else if (strOption == "redo") {
			pl.buildCard((at("char") = strip(readRest())).text, true, fromChat.gid);
			set("show",pl[get_str("char")]->show(true));
			replyMsg("strPcCardRedo");
			return 1;
		}
		else if (strOption == "grp") {
			set("show",pl.listMap());
			replyMsg("strPcGroupList");
			return 1;
		}
		else if (strOption == "cpy") {
			auto [namePC1, namePC2] = readini(strip(filter_CQcode(readRest(), fromChat.gid)));
			set("char1", namePC1);
			set("char2", namePC2 = namePC2.empty() ? pl[fromChat.gid]->getName() : strip(namePC2));
			if (!(resno = pl.copyCard(namePC1, namePC2, fromChat.gid)))replyMsg("strPcCardCpy");
			else if (resno == -5)set("char", namePC2);
		}
		else if (strOption == "stat") {
			auto pc{ pl[fromChat.gid] };
			bool isEmpty{ true };
			ResList res;
			int intFace{ pc->has("__DefaultDice")
				? pc->get("__DefaultDice").to_int()
				: getUser(fromChat.uid).getConf("默认骰",100) };
			string strFace{ to_string(intFace) };
			string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//掷骰次数
			if (intFace <= 100 && pc->has(keyStatCnt)) {
				int cntRoll{ pc->get_int(keyStatCnt) };
				if (cntRoll > 0) {
					isEmpty = false;
					res << "D" + strFace + "统计次数: " + to_string(cntRoll);
					int sumRes{ pc->get_int("__StatD" + strFace + "Sum") };		//点数和
					int sumResSqr{ pc->get_int("__StatD" + strFace + "SqrSum") };	//点数平方和
					DiceEst stat{ intFace,cntRoll,sumRes,sumResSqr };
					if (stat.estMean > 0)
						res << "均值[期望]: " + toString(stat.estMean, 2, true) + " [" + toString(stat.expMean) + "]";
					if (stat.pNormDist) {
						if (stat.pNormDist < 0.5)res << "低于" + toString(100 - stat.pNormDist * 100, 2) + "%的用户";
						else res << "高于" + toString(stat.pNormDist * 100, 2) + "%的用户";
					}
					if (stat.estStd > 0) {
						res << "标准差[期望]: " + toString(stat.estStd, 2) + " [" + toString(stat.expStd) + "]";
					}
				}
			}
			string keyRcCnt{ "__StatRcCnt" };	//rc/sc检定次数
			if (pc->has(keyRcCnt)) {
				int cntRc{ pc->get_int("__StatRcCnt") };
				if (cntRc > 0) {
					isEmpty = false;
					int sumRcSuc{ pc->get_int("__StatRcSumSuc") };//实际成功数
					res << "检定成功统计: " + to_string(sumRcSuc) + "/" + to_string(cntRc);
					int sumRcRate{ pc->get_int("__StatRcSumRate") };//总成功率
					res << "成功率[期望]: " + toString((double)sumRcSuc / cntRc * 100) + "% [" + toString((double)sumRcRate / cntRc) + "%]";
					double cnt5{ pc->get_num("__StatRcCnt5") }, cnt96{ pc->get_num("__StatRcCnt96") };
					res << "5- | 96+ 出现率: " + (cnt5 ? toString(cnt5 / cntRc * 100) + "%(" + pc->get_str("__StatRcCnt5") + ")" : "0%")
						+ " | " + (cnt96 ? toString(cnt96 / cntRc * 100) + "%(" + pc->get_str("__StatRcCnt96") + ")" : "0%");
					if(pc->has("__StatRcCnt1")|| pc->has("__StatRcCnt100"))
						res << "1 | 100 出现数: " + pc->get_str("__StatRcCnt1") + " | " + pc->get_str("__StatRcCnt100");
				}
			}
			if (isEmpty) {
				replyMsg("strPcStatEmpty");
			}
			else {
				set("stat",res.show());
				replyMsg("strPcStatShow");
			}
			return 1;
		}
		else if (strOption == "clr") {
			PList.erase(fromChat.uid);
			replyMsg("strPcClr");
			return 1;
		}
		else if (strOption == "type") {
			if ((at("new_type") = strip(readRest())).str_empty()) {
				set("attr","模板类");
				set("val",pl[fromChat.gid]->get_str("__Type"));
				replyMsg("strProp");
			}
			else {
				pl[fromChat.gid]->setType(get_str("new_type"));
				replyMsg("strPcTempChange");
			}
			return 1;
		}
		else if (strOption == "temp") {
			reply(pl[fromChat.gid]->getTemplet()->show());
			return 1;
		}
		else if (strOption == "tojson") {
			string strName = readRest();
			auto pc{ pl.getCard(strName, fromChat.gid) };
			set("char", pc->getName());
			set("type", pc->get_str("__Type"));
			set("show", pc->to_json().dump());
			replyMsg("strPcCardShow");
			return 1;
		}
		else replyHelp("pc");
		if (resno) replyMsg(PlayerErrors.count(resno) ? PlayerErrors.at(resno) : "strUnknownErr");
		return 1;
	}
	else if (pref2 == "ra" || pref2 == "rc") {
		intMsgCnt += 2;
		if (strMsg.length() == intMsgCnt) {
			replyHelp("rc");
			return 1;
		}
		int intRule = isPrivate()
			? getUser(fromChat.uid).getConf("rc房规", console["DefaultCOCRoomRule"])
			: chat(fromChat.gid).getConf("rc房规", console["DefaultCOCRoomRule"]);
		int intTurnCnt = 1;
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			set("hidden");
			++intMsgCnt;
		}
		else if (readSkipSpace(); strMsg[intMsgCnt] == '_') {
			set("hidden");
			++intMsgCnt;
		}
		readSkipSpace();
		if (strMsg.find('#') != string::npos) {
			string strTurnCnt = strMsg.substr(intMsgCnt, strMsg.find('#') - intMsgCnt);
			//#能否识别有效
			if (strTurnCnt.empty())intMsgCnt++;
			else if ((strTurnCnt.length() == 1 && isdigit(static_cast<unsigned char>(strTurnCnt[0]))) || strTurnCnt ==
					 "10") {
				intMsgCnt += strTurnCnt.length() + 1;
				intTurnCnt = stoi(strTurnCnt);
			}
		}
		string strMainDice = "D100";
		string strSkillModify;
		//困难等级
		string strDifficulty;
		int intDifficulty = 1;
		int intSkillModify = 0;
		//乘数
		int intSkillMultiple = 1;
		//除数
		int intSkillDivisor = 1;
		//自动成功
		bool isAutomatic = false;
		//D100且有角色卡时计入统计
		bool isStatic = PList.count(fromChat.uid);
		bool isRoulette = thisGame && thisGame->is_part(fromChat.uid) && thisGame->roulette.count(100);
		PC pc{ isStatic ? PList[fromChat.uid][fromChat.gid] : std::make_shared<CharaCard>()};
		if ((strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') && strLowerMessage[intMsgCnt - 1] != ' ') {
			isStatic = false;
			strMainDice = strLowerMessage[intMsgCnt];
			++intMsgCnt;
			strMainDice += readDigit(false);
		}
		readSkipSpace();
		if (strMsg[intMsgCnt] == '_') {
			set("hidden");
			++intMsgCnt;
		}
		if (strMsg.length() == intMsgCnt) {
			replyMsg("strUnknownPropErr");
			return 1;
		}
		string& attr{ (at("attr") = readAttrName()).text };
		if (pc) {
			if (strMsg[intMsgCnt] == ':') {
				if(PList[fromChat.uid].count(attr)){
					while (strMsg[intMsgCnt] == ':')++intMsgCnt;
					pc = PList[fromChat.uid][attr];
					set("pc", pc->getName());
					attr = readAttrName();
				}
				else {
					set("char", attr);
					replyMsg("strPcNameNotExist");
				}
			}
			else if (size_t pos{ attr.find("的") }; pos != string::npos) {
				string strGenitive = attr.substr(0, pos);
				if (PList[fromChat.uid].count(strGenitive)) {
					pc = PList[fromChat.uid][strGenitive];
					set("pc", pc->getName());
					attr = attr.substr(pos + 2);
				}
			}
		}
		if (attr.find("自动成功") == 0) {
			strDifficulty = attr.substr(0, 8);
			attr = attr.substr(8);
			isAutomatic = true;
		}
		if (attr.find("困难") == 0 || attr.find("极难") == 0) {
			strDifficulty += attr.substr(0, 4);
			intDifficulty = (attr.substr(0, 4) == "困难") ? 2 : 5;
			attr = attr.substr(4);
		}
		if (pc) {
			attr = pc->standard(attr);
		}
		else {
			if (SkillNameReplace.count(attr))attr = SkillNameReplace[attr];
		}
		if (strLowerMessage[intMsgCnt] == '*' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			++intMsgCnt;
			readNum(intSkillMultiple);
		}
		while ((strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') && isdigit(
			strLowerMessage[intMsgCnt + 1])) {
			if (!readNum(intSkillModify))strSkillModify = to_signed_string(intSkillModify);
		}
		if (strLowerMessage[intMsgCnt] == '/' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
			readNum(intSkillDivisor);
			if (intSkillDivisor == 0) {
				replyMsg("strValueErr");
				return 1;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			   strLowerMessage[intMsgCnt] == ':') intMsgCnt++;
		string strSkillVal = readDigit(false);
		set("reason",readRest());
		int intSkillVal;
		if (strSkillVal.empty()) {
			if (pc && pc->has(attr)) {
				intSkillVal = pc->get(attr).to_int();
			}
			else {
				if (!pc && SkillNameReplace.count(attr)) {
					set("attr",SkillNameReplace[attr]);
				}
				if (!pc && SkillDefaultVal.count(attr)) {
					intSkillVal = SkillDefaultVal[attr];
				}
				else {
					replyMsg("strUnknownPropErr");
					return 1;
				}
			}
		}
		else if (strSkillVal.length() > 3) {
			replyMsg("strPropErr");
			return 1;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		//最终成功率计入检定统计
		int intFianlSkillVal = (intSkillVal * intSkillMultiple + intSkillModify) / intSkillDivisor / intDifficulty;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000) {
			replyMsg("strSuccessRateErr");
			return 1;
		}
		RD rdMainDice(strMainDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err) {
			replyMsg("strZeroDiceErr");
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			replyMsg("strDiceTooBigErr");
			return 1;
		}
		set("attr", strDifficulty + attr + (
			(intSkillMultiple != 1) ? "×" + to_string(intSkillMultiple) : "") + strSkillModify + ((intSkillDivisor != 1)
				? "/" + to_string(intSkillDivisor)
				: ""));
		if (is_empty("reason")) {
			strReply = getMsg("strRollSkill", *this);
		}
		else strReply = getMsg("strRollSkillReason", *this);
		ResList Res;
		string strAns;
		if (intTurnCnt == 1) {
			isRoulette ? rdMainDice.Roll(thisGame) : rdMainDice.Roll();
			if (isStatic) {
				pc->cntRollStat(rdMainDice.intTotal, 100);
				pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
			}
			strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
			int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
			switch (intRes) {
			case 0: strAns += getMsg("strRollFumble");
				break;
			case 1: strAns += isAutomatic ? getMsg("strRollRegularSuccess") : getMsg("strRollFailure");
				break;
			case 5: strAns += getMsg("strRollCriticalSuccess");
				break;
			case 4: if (intDifficulty == 1) {
				strAns += getMsg("strRollExtremeSuccess");
				break;
			}
			case 3: if (intDifficulty == 1) {
				strAns += getMsg("strRollHardSuccess");
				break;
			}
			case 2: strAns += getMsg("strRollRegularSuccess");
				break;
			}
			strReply += strAns;
		}
		else {
			Res.dot("\n");
			while (intTurnCnt--) {
				isRoulette ? rdMainDice.Roll(thisGame) : rdMainDice.Roll();
				if (isStatic) {
					pc->cntRollStat(rdMainDice.intTotal, 100);
					pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
				}
				strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
				int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
				switch (intRes) {
				case 0: strAns += getMsg("strFumble");
					break;
				case 1: strAns += isAutomatic ? getMsg("strSuccess") : getMsg("strFailure");
					break;
				case 5: strAns += getMsg("strCriticalSuccess");
					break;
				case 4: if (intDifficulty == 1) {
					strAns += getMsg("strExtremeSuccess");
					break;
				}
				case 3: if (intDifficulty == 1) {
					strAns += getMsg("strHardSuccess");
					break;
				}
				case 2: strAns += getMsg("strSuccess");
					break;
				}
				Res << strAns;
			}
			strReply += Res.show();
		}
		if (is("hidden")) {
			replyHidden();
			replyMsg("strRollSkillHidden");
		}
		else
			reply();
		return 1;
	}
	else if (pref2 == "ri") {
		if (isPrivate()) {
			replyHelp("ri");
			return 1;
		}
		intMsgCnt += 2;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strinit += readDice();
		}
		else if (isRollDice()) {
			strinit = readDice();
		}
		set("char",strip(readRest()));
		if (is_empty("char")) {
			set("char",idx_pc(*this));
		}
		RD initdice(strinit, 20);
		if (const auto intFirstTimeRes = initdice.Roll()) {
			replyRollDiceErr(intFirstTimeRes, initdice);
			return 1;
		}
		sessions.get(fromChat)->table_add("先攻", initdice.intTotal, get_str("char"));
		set("res",initdice.FormCompleteString());
		replyMsg("strRollInit");
		return 1;
	}
	else if (pref2 == "sc") {
		intMsgCnt += 2;
		string SanCost = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (SanCost.empty()) {
			replyHelp("sc");
			return 1;
		}
		if (SanCost.find('/') == string::npos) {
			replyMsg("strSanCostInvalid");
			return 1;
		}
		string attr = "理智";
		int intSan = 0, sanLoss = 0;
		PC pc;
		if (readNum(intSan)) {
			if (PList.count(fromChat.uid)
				&& (pc = getPlayer(fromChat.uid)[fromChat.gid])->has(attr)) {
				intSan = pc->get(attr).to_int();
			}
			else {
				replyMsg("strSanEmpty");
				return 1;
			}
		}
		string strSanCostSuc = SanCost.substr(0, SanCost.find('/'));
		string strSanCostFail = SanCost.substr(SanCost.find('/') + 1);
		for (const auto& character : strSanCostSuc) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				replyMsg("strSanCostInvalid");
				return 1;
			}
		}
		for (const auto& character : strSanCostFail) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				replyMsg("strSanCostInvalid");
				return 1;
			}
		}
		std::optional<RD> rdLoss;
		if (intSan <= 0) {
			replyMsg("strSanInvalid");
			return 1;
		}
		const int intTmpRollRes = (thisGame && thisGame->is_part(fromChat.uid))
			? thisGame->roll(100) : RandomGenerator::Randint(1, 100);
		//理智检定计入统计
		if (pc) {
			pc->cntRollStat(intTmpRollRes, 100);
			pc->cntRcStat(intTmpRollRes, intSan);
		}
		string& strRes{ (at("res") = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intSan)).text};
		//调用房规
		int intRule = fromChat.gid
			? chat(fromChat.gid).getConf("rc房规", console["DefaultCOCRoomRule"])
			: getUser(fromChat.uid).getConf("rc房规", console["DefaultCOCRoomRule"]);
		int res = RollSuccessLevel(intTmpRollRes, intSan, intRule);
		switch (res) {
		case 5:
		case 4:
		case 3:
		case 2:
			rdLoss = RD(strSanCostSuc);
			if (rdLoss->Roll() != 0) {
				replyMsg("strSanCostInvalid");
				return 1;
			}
			set("change", rdLoss->FormShortString());
			break;
		case 1:
			rdLoss = RD(strSanCostFail);
			if (rdLoss->Roll() != 0) {
				replyMsg("strSanCostInvalid");
				return 1;
			}
			set("change", rdLoss->FormShortString());
			break;
		case 0:
			rdLoss = RD(strSanCostFail);
			if (rdLoss->Max() != 0) {
				replyMsg("strSanCostInvalid");
				return 1;
			}
			set("change","Max{" + rdLoss->strDice + "}=" + to_string(rdLoss->intTotal));
			break;
		}
		set("loss", sanLoss = rdLoss->intTotal);
		intSan = max(0, intSan - sanLoss);
		set("rank", res);
		set("final",intSan);
		if (pc)pc->set(attr, intSan);
		replyMsg("strSanityRoll");
		return 1;
	}
	else if (pref2 == "st") {
		intMsgCnt += 2;
		readSkipSpace();
		if (intMsgCnt == strLowerMessage.length()) {
			replyHelp("st");
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr") {
			if (!PList.count(fromChat.uid)) {
				replyMsg("strPcNotExistErr");
				return 1;
			}
			auto pc = getPlayer(fromChat.uid)[fromChat.gid];
			if (!pc->locked("w")) {
				pc->clear();
				set("char", getPlayer(fromChat.uid)[fromChat.gid]->getName());
				replyMsg("strPropCleared");
			}
			else {
				replyMsg("strPcLockedWrite");
			}
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			if (!PList.count(fromChat.uid)) {
				replyMsg("strPcNotExistErr");
				return 1;
			}
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			set("attr",readAttrName());
			auto pc = getPlayer(fromChat.uid)[fromChat.gid];
			if (pc->locked("w")) {
				replyMsg("strPcLockedWrite");
			}
			else if (pc->erase(at("attr").text)) {
				replyMsg("strPropDeleted");
			}
			else {
				replyMsg("strPropNotFound");
			}
			return 1;
		}
		auto& pl = getPlayer(fromChat.uid);
		PC pc = pl[fromChat.gid];
		if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			readSkipSpace();
			string& attr = (at("attr") = readAttrName()).text;
			if (strMsg[intMsgCnt] == ':') {
				if (PList[fromChat.uid].count(attr)) {
					while (strMsg[intMsgCnt] == ':')++intMsgCnt;
					pc = pl[attr];
					set("pc", pc->getName());
					attr = readAttrName();
				}
				else {
					set("char", attr);
					replyMsg("strPcNameNotExist");
				}
			}
			else if (size_t pos{ attr.find("的") }; pos != string::npos) {
				string strGenitive = attr.substr(0, pos);
				if (PList[fromChat.uid].count(strGenitive)) {
					pc = PList[fromChat.uid][strGenitive];
					set("pc", pc->getName());
					attr = attr.substr(pos + 2);
				}
			}
			if (pc->locked("r")) {
				replyMsg("strPcLockedRead");
			}
			else if (attr.empty()) {
				set("char",pc->getName());
				set("type",pc->get_str("__Type"));
				set("show",pc->show(false));
				replyMsg("strPropList");
			}
			else if (auto val{ pc->show(attr) }) {
				set("val",*val);
				replyMsg("strProp");
			}
			else {
				replyMsg("strPropNotFound");
			}
			return 1;
		}
		if (size_t pos = strMsg.find("::", intMsgCnt); pos != string::npos
			//|| (pos = strMsg.find("的", intMsgCnt)) != string::npos
			) {
			string name{ strip(filter_CQcode(strMsg.substr(intMsgCnt,pos - intMsgCnt), fromChat.gid)) };
			intMsgCnt = pos + 2;
			if (!name.empty()) {
				if (!pl.count(name)) {
					string type{ pc->get_str("__Type") };
					switch (pl.emptyCard(name, fromChat.gid, type)) {
					case 0:
						if (!pl.count(fromChat.gid)) {
							pl.changeCard(name, fromChat.gid);
						}
						break;
					case -1:
						replyMsg("strPcCardFull");
						return 1;
					case -6:
						replyMsg("strPcNameInvalid");
						return 1;
					default:
						replyMsg("strUnknownErr");
						return 1;
					}
				}
				pc = pl[name];
				set("pc", name);
			}
		}
		if (pc->locked("w")) {
			replyMsg("strPcLockedWrite");
			return 1;
		}
		ShowList changes;
		ShowList rolls;
		ShowList errs;
		//循环录入
		while (intMsgCnt != strLowerMessage.length()) {
			readSkipSpace();
			//判定录入表达式
			if (strMsg[intMsgCnt] == '&') {
				set("attr",readToColon()); 
				if (at("attr").str_empty()) {
					continue;
				}
				if (pc->set(get_str("attr"), readExp())) {
					replyMsg("strPcTextTooLong");
					set("error");
				}
				else inc("cnt");
				continue;
			}
			//读取Attr
			string strAttr = readAttrName();
			if (strAttr.empty()) {
				readSkipSpace();
				while (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '+' ||
			           strMsg[intMsgCnt] == '-' || strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/'){
					intMsgCnt++;
				}
				readDigit(false);
				continue;
			}
			strAttr = pc->standard(strAttr);
			while (strLowerMessage[intMsgCnt] ==
				'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			//判定数值修改
			if ((strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+')) {
				AttrVar nVal{ pc->get(strAttr)};
				RD Mod(nVal.to_str() + readDice());
				if (!Mod.Roll()) {
					changes << strAttr + ": " + Mod.FormCompleteString();
					pc->set(strAttr, Mod.intTotal);
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
						'|')intMsgCnt++;
					inc("cnt");
				}
				else
					errs << strAttr + ": 表达式出错";
				continue;
			}
			//判定录入文本
			else if (!isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
				&& !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				if (string strVal{ trustedQQ(fromChat.uid) > 0 ? readUntilSpace() : filter_CQcode(readUntilSpace()) };
					!pc->set(strAttr, strVal)) {
					inc("cnt");
				}
				else
					errs << strAttr + ": 文本过长";
				continue;
			}
			//录入纯数值
			string strSkillVal = readDigit();
			if (strSkillVal.empty()) {
				errs << strAttr + ": 属性为空";
				break;
			}
			else if (strAttr.empty()) {
				errs << strSkillVal + "无属性名";
			}
			else if (strSkillVal.length() > 9) {
				errs << strAttr + ": 数值过大";
				break;
			}
			else {
				int intSkillVal = stoi(strSkillVal);
				if (!pc->set(strAttr, intSkillVal)) inc("cnt");
			}
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
				intMsgCnt++;
		}
		if (!changes.empty()) {
			set("change", changes.show("\n"));
			replyMsg("strStModify");
		}
		else if (has("cnt")) {
			replyMsg("strSetPropSuccess");
		}
		if (!errs.empty()) {
			set("err", errs.show("\n"));
			replyMsg("strStErr");
		}
		return 1;
	}
	else if (pref2 == "ti") {
		TempInsane(*this);
		replyMsg("strTempInsane");
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'w') {
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w') {
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h') {
			isHidden = true;
			intMsgCnt += 1;
		}
		readSkipSpace();
		const unsigned int len{ (unsigned int)strMsg.length() };
		if (intMsgCnt == len) {
			replyHelp("ww");
			return 1;
		}
		if (!fromChat.gid)isHidden = false;
		PC pc{ PList.count(fromChat.uid) ? getPlayer(fromChat.uid)[fromChat.gid] : std::make_shared<CharaCard>()};
		string strMainDice;
		string& strReason{ (at("reason") = "").text};
		string strAttr;
		if (pc) {	//调用角色卡属性或表达式
			while (intMsgCnt < len && !isspace(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
				if (is_digit(strMsg[intMsgCnt])
					|| strMsg[intMsgCnt] == 'a'
					|| strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'
					|| strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/') {
					strMainDice += strMsg[intMsgCnt++];
				}
				else if (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':') {
					intMsgCnt++;
				}
				else {
					strAttr = readAttrName();
					if (pc->countExp(strAttr)) {
						strMainDice += pc->getExp(strAttr);
						if (!pc->has("&" + strAttr) && pc->get(strAttr).type == AttrVar::Type::Integer)strMainDice += 'a';
					}
					else {
						strReason = strAttr;
					}
				}
			}
		}
		else {
			strMainDice = readDice(); 	//ww的表达式可以是纯数字
		}
		strReason += readRest();
		int intTurnCnt = 1;
		const int intDefaultDice = getUser(fromChat.uid).getConf("默认骰", 100);
		//处理.ww[次数]#[表达式]
		if (size_t pos{ strMainDice.find('#') }; pos != string::npos) {
			string strTurnCnt = strMainDice.substr(0, pos);
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(pos + 1);
			RD rdTurnCnt(strTurnCnt, intDefaultDice);
			if (const int intRdTurnCntRes = rdTurnCnt.Roll(); intRdTurnCntRes != 0) {
				replyRollDiceErr(intRdTurnCntRes, rdTurnCnt);
			}
			if (rdTurnCnt.intTotal > 10) {
				replyMsg("strRollTimeExceeded");
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				replyMsg("strRollTimeErr");
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find('d') != string::npos) {
				set("turn",rdTurnCnt.FormShortString());
				replyHidden(getMsg("strRollTurn", *this));
			}
		}
		if (strMainDice.empty()) {
			replyHelp("ww");
			return 1;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
												 ? strMainDice.find('+')
												 : strMainDice.find('-'));
		strFirstDice = strFirstDice.substr(0, strFirstDice.find('x') < strFirstDice.find('*')
										   ? strFirstDice.find('x')
										   : strFirstDice.find('*'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice) {
			if (!isdigit(static_cast<unsigned char>(i))) {
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, intDefaultDice);
		if (const int intFirstTimeRes = rdMainDice.Roll(); intFirstTimeRes != 0) {
			replyRollDiceErr(intFirstTimeRes, rdMainDice);
		}
		if (!boolDetail && intTurnCnt != 1) {
			if (strReason.empty())strReply = getMsg("strRollMuiltDice");
			else strReply = getMsg("strRollMuiltDiceReason");
			vector<int> vintExVal;
			string& strRes{ (at("res") = "{ ").text};
			while (intTurnCnt--) {
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strRes += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)strRes += ",";
			}
			strRes += " }";
			if (!vintExVal.empty()) {
				strRes += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					strRes += to_string(*it);
					if (it != vintExVal.cend() - 1)strRes += ",";
				}
			}
			if (!is("hidden")) {
				reply();
			}
			else {
				replyHidden();
			}
		}
		else {
			while (intTurnCnt--) {
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				set("res",boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (strReason.empty())
					strReply = getMsg("strRollDice");
				else strReply = getMsg("strRollDiceReason");
				if (!is("hidden")) {
					reply();
				}
				else {
					replyHidden();
				}
			}
		}
		if (isHidden) {
			replyMsg("strRollHidden");
		}
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'h') {
		if (strLowerMessage[intMsgCnt] == 'h')set("hidden");
		intMsgCnt += 1;
		bool boolDetail = true;
		if (strMsg[intMsgCnt] == 's') {
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h') {
			set("hidden");
			intMsgCnt += 1;
		}
		if (!fromChat.gid)reset("hidden");
		readSkipSpace();
		string strMainDice;
		PC pc{ PList.count(fromChat.uid) ? getPlayer(fromChat.uid)[fromChat.gid] : PC() };
		string& strReason{ (at("reason") = strMsg.substr(intMsgCnt)).text};
		if (static string key{ "__DefaultDiceExp" }; strReason.empty() && pc && pc->countExp(key)) {
			strMainDice = pc->getExp(key);
		}
		else if (pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		else {
			strMainDice = readDice();
			bool isExp = false;
			for (auto ch : strMainDice) {
				if (!isdigit(ch)) {
					isExp = true;
					break;
				}
			}
			if (isExp)strReason = readRest();
			else strMainDice.clear();
		}
		int intTurnCnt = 1;
		const int intDefaultDice = (pc && pc->has("__DefaultDice"))
			? pc->get("__DefaultDice").to_int()
			: getUser(fromChat.uid).getConf("默认骰", 100);
		if (strMainDice.find('#') != string::npos) {
			string& turn{ (at("turn") = strMainDice.substr(0, strMainDice.find('#'))).text };
			if (turn.empty())
				turn = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(turn, intDefaultDice);
			if (const int intRdTurnCntRes = rdTurnCnt.Roll()) {
				replyRollDiceErr(intRdTurnCntRes, rdTurnCnt);
			}
			if (rdTurnCnt.intTotal > 10) {
				replyMsg("strRollTimeExceeded");
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				replyMsg("strRollTimeErr");
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (turn.find('d') != string::npos) {
				turn = rdTurnCnt.FormShortString();
				if (!is("hidden")) {
					replyMsg("strRollTurn");
				}
				else {
					replyHidden(getMsg("strRollTurn"));
				}
			}
		}
		if (strMainDice.empty() && pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		RD rdMainDice(strMainDice, intDefaultDice);
		if (const int intFirstTimeRes = (thisGame && thisGame->is_part(fromChat.uid)) ? rdMainDice.Roll(thisGame) : rdMainDice.Roll()) {
			replyRollDiceErr(intFirstTimeRes, rdMainDice);
			return 1;
		}
		set("dice_exp",rdMainDice.strDice);
		//仅统计与默认骰一致的掷骰
		bool isStatic{ intDefaultDice <= 100 && pc && rdMainDice.strDice == ("D" + to_string(intDefaultDice)) };
		string strType = (intTurnCnt != 1
						  ? (is_empty("reason") ? "strRollMultiDice" : "strRollMultiDiceReason")
						  : (is_empty("reason") ? "strRollDice" : "strRollDiceReason"));
		if (!boolDetail && intTurnCnt != 1) {
			vector<int> vintExVal;
			string& res{ (at("res") = "{ ").text};
			while (intTurnCnt--) {
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				res += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					res += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
																						rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			res += " }";
			if (!vintExVal.empty()) {
				res += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					res += to_string(*it);
					if (it != vintExVal.cend() - 1)
						res += ",";
				}
			}
			if (!is("hidden")) {
				replyMsg(strType);
			}
			else {
				replyHidden(getMsg(strType, *this));
			}
		}
		else {
			ResList dices;
			if (intTurnCnt > 1) {
				while (intTurnCnt--) {
					rdMainDice.Roll();
					if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
					string strForm = to_string(rdMainDice.intTotal);
					if (boolDetail) {
						string strCombined = rdMainDice.FormStringCombined();
						string strSeparate = rdMainDice.FormStringSeparate();
						if (strCombined != strForm)strForm = strCombined + "=" + strForm;
						if (strSeparate != strMainDice && strSeparate != strCombined)strForm = strSeparate + "=" +
							strForm;
					}
					dices << strForm;
				}
				set("res",dices.dot(", ").line(7).show());
			}
			else {
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				set("res",boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
			}
			if (!is("hidden")) {
				replyMsg(strType);
			}
			else {
				replyHidden(getMsg(strType, *this));
			}
		}
		if (is("hidden")) {
			replyMsg("strRollHidden");
		}
		return 1;
	}
	return 0;
}
bool DiceEvent::monitorFrq() {
	if (!isVirtual && !is("ignored")) {
		AddFrq(*this);
		getUser(fromChat.uid).update((time_t)get_ll("time"));
		if (pGrp)pGrp->update((time_t)get_ll("time"));
	}
	return true;
}
//判断是否响应
void DiceEvent::DiceFilter(){
	api::printLog("msg filter");
	while (isspace(static_cast<unsigned char>(strMsg[0])))
		strMsg.erase(strMsg.begin());
	init(strMsg);
	string strSummon{ getMsg("strSummonWord") };
	init2(strMsg);
	strLowerMessage = toLower(strMsg);
	trusted = isVirtual ? 255 : trustedQQ(fromChat.uid);
	fwdMsg();
	api::printLog("BasicOrder");
	if (BasicOrder()) {
		return;
	}
	else if (is("ignored"))return;
	api::printLog("listen_order");
	if (fmt->listen_order(this) || InnerOrder()) {
		return;
	}
	if (auto ruleName{ getGameRule() }; ruleName
		&& (thisGame->is_part(fromChat.uid))
		&& ruleset->has_rule(*ruleName)
		&& ((thisGame->attrs.has("tape") && ruleset->get_rule(*ruleName)->listen_cassette(thisGame->attrs.get_str("tape"), this))
			|| ruleset->get_rule(*ruleName)->listen_order(this))) {
		return;
	}
	else if (thisGame && (thisGame->is_part(fromChat.uid))
		&& fmt->listen_game(this)) {
		return;
	}
	api::printLog("listen_reply");
	if (fmt->listen_reply(this)) {
		return;
	}
	if ((strMsg.empty() || strMsg == strSummon))replyMsg("strSummonEmpty");
	return;
}
bool DiceEvent::WordCensor() {
	//信任小于4的用户进行敏感词检测
	if (trusted < 4) {
		vector<string>sens_words;
		if (int danger = censor.search(strMsg, sens_words)) {
			set("hook", "WordCensored");
			set("danger", danger);
			if (fmt->call_hook_event(*this)) {
				return is("break");
			}
			else if (danger < 2 || trusted > danger) {
				console.log("提醒:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
					+ "发送了含敏感词消息(" + listItem(sens_words) + "):\n" + strMsg, 1,
					printTTime((time_t)get_ll("time")));
			}
			else if (danger > 3) {
				console.log("警告:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName") + "发送了含敏感词消息:\n" + strMsg, 0b1000,
					printTTime((time_t)get_ll("time")));
				replyMsg("strCensorDanger");
				return 1;
			}
			else if (danger == 3) {
				console.log("警告:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
					+ "发送了含敏感词消息(" + listItem(sens_words) + "):\n" + strMsg, 0b10, printTTime((time_t)get_ll("time")));
				replyMsg("strCensorWarning");
				return 1;
			}
			else if (danger == 2) {
				console.log("提醒:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
					+ "发送了含敏感词消息(" + listItem(sens_words) + "):\n" + strMsg, 0b10,
					printTTime((time_t)get_ll("time")));
				replyMsg("strCensorCaution");
				return 1;
			}
		}
	}
	return false;
}

void DiceEvent::virtualCall() {
	isVirtual = true;
	isCalled = true;
	DiceFilter();
}
std::optional<string> DiceEvent::getGameRule() {
	if (thisGame && thisGame->attrs.has("rule")) {
		return thisGame->attrs.get_str("rule");
	}
	return std::nullopt;
}
bool DiceEvent::canRoomHost() {
	if (!has("canRoomHost")) {
		set("canRoomHost",AttrVar(trusted > 3
			|| isChannel() || isPrivate()
			|| api::isGroupAdmin(fromChat.gid, fromChat.uid, true) || pGrp->inviter == fromChat.uid));
	}
	return is("canRoomHost");
}

int DiceEvent::getGroupTrust(long long group) {
	if (trusted > 0)return trusted;
	if (ChatList.count(group)) {
		return api::isGroupAdmin(group, fromChat.uid, true) ? 0 : -1;
	}
	return -2;
}
void DiceEvent::readSkipColon() {
	readSkipSpace();
	while (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '='))intMsgCnt++;
}
string DiceEvent::readDigit(bool isForce)
{
	string strMum;
	if (isForce)while (intMsgCnt < strMsg.length() && !is_digit(strMsg[intMsgCnt]))
	{
		if (strMsg[intMsgCnt] < 0)intMsgCnt++;
		intMsgCnt++;
	}
	else while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
	while (intMsgCnt < strMsg.length() && is_digit(strMsg[intMsgCnt]))
	{
		strMum += strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (intMsgCnt < strMsg.length() && strMsg[intMsgCnt] == ']')intMsgCnt++;
	return strMum;
}

int DiceEvent::readNum(int& num)
{
	string strNum;
	while (intMsgCnt < strMsg.length() && !is_digit(strMsg[intMsgCnt]) && strMsg[intMsgCnt] != '-')intMsgCnt++;
	if (strMsg[intMsgCnt] == '-')
	{
		strNum += '-';
		intMsgCnt++;
	}
	if (intMsgCnt >= strMsg.length())return -1;
	while (intMsgCnt < strMsg.length() && is_digit(strMsg[intMsgCnt]))
	{
		strNum += strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 9)return -2;
	if (strNum.empty() || strNum == "-")return -3;
	num = stoi(strNum);
	return 0;
}
string DiceEvent::readAttrName()
{
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
	const auto intBegin = intMsgCnt;
	int intEnd = intBegin;
	const auto len = strMsg.length();
	while (intMsgCnt < len && !is_digit(strMsg[intMsgCnt])
		&& strMsg[intMsgCnt] != '=' && strMsg[intMsgCnt] != ':'
		&& strMsg[intMsgCnt] != '+' && strMsg[intMsgCnt] != '-' && strMsg[intMsgCnt] != '*'
		&& strMsg[intMsgCnt] != '/')
	{
		if (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || (!isspace(
			static_cast<unsigned char>(strMsg[intEnd]))))intEnd = intMsgCnt;
		if (strMsg[intMsgCnt] < 0)intMsgCnt += 2;
		else intMsgCnt++;
	}
	if (intMsgCnt == strLowerMessage.length() && strLowerMessage.find(' ', intBegin) != string::npos)
	{
		intMsgCnt = strLowerMessage.find(' ', intBegin);
	}
	else if (isspace(static_cast<unsigned char>(strMsg[intEnd])))intMsgCnt = intEnd;
	return strMsg.substr(intBegin, intMsgCnt - intBegin);
}
string DiceEvent::readFileName(){
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
	const size_t intBegin{ intMsgCnt }, len{ strMsg.length() };
	while (intMsgCnt < len
		&& strMsg[intMsgCnt] != '<' && strMsg[intMsgCnt] != '>'
		&& strMsg[intMsgCnt] != ':' && strMsg[intMsgCnt] != '"'
		&& strMsg[intMsgCnt] != '*' && strMsg[intMsgCnt] != '|' && strMsg[intMsgCnt] != '?'
		&& strMsg[intMsgCnt] != '/' && strMsg[intMsgCnt] != '\\'){
		intMsgCnt++;
	}
	return strMsg.substr(intBegin, intMsgCnt - intBegin);
}
int DiceEvent::readChat(chatInfo& ct, bool isReroll)
{
	const int intFormor = intMsgCnt;
	if (string strT{ readPara() }; strT == "me") {
		ct = { fromChat.uid, 0,0 };
		return 0;
	}
	else if (strT == "this"){
		ct = fromChat.locate();
		return 0;
	}
	else if (const long long llID{ readID() }; !llID) {
		if (isReroll)intMsgCnt = intFormor;
		return -2;
	}
	else if (strT == "group" || strT == "g"){
		ct.type = msgtype::Group;
		ct.gid = llID;
		return 0;
	}
	else if (strT == "qq" || strT == "u" || strT.empty()){
		ct.type = msgtype::Private;
		ct.uid = llID;
		return 0;
	}
	if (isReroll)intMsgCnt = intFormor;
	return -1;
}
int DiceEvent::readClock(Clock& cc)
{
	const string strHour = readDigit();
	if (strHour.empty())return -1;
	const unsigned short nHour = stoi(strHour);
	if (nHour > 23)return -2;
	cc.first = nHour;
	if (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '.')intMsgCnt++;
	if (strMsg.substr(intMsgCnt, 2) == "：")intMsgCnt += 2;
	readSkipSpace();
	if (intMsgCnt >= strMsg.length() || !is_digit(strMsg[intMsgCnt]))
	{
		cc.second = 0;
		return 0;
	}
	const string strMin = readDigit();
	const unsigned short nMin = stoi(strMin);
	if (nMin > 59)return -2;
	cc.second = nMin;
	return 0;
}

string DiceEvent::readItem()
{
	size_t len{ strMsg.length() };
	if (intMsgCnt >= len)return{};
	while (intMsgCnt < len &&
		(isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == '|'))intMsgCnt++;
	size_t intBegin{ intMsgCnt };
	size_t intEnd{ strMsg.find('|',intMsgCnt) };
	if (intEnd > len) {
		intMsgCnt = intEnd = len;
	}
	else intMsgCnt = intEnd + 1;
	intEnd = strMsg.find_last_not_of(" \t\r\n", intEnd - 1) + 1;
	//while (isspace(static_cast<unsigned char>(strMsg[intEnd - 1])))--intEnd;
	return strMsg.substr(intBegin, intEnd - intBegin);
}
int DiceEvent::readItems(vector<string>& vItem) {
	int cnt{ 0 };
	string strItem;
	while (intMsgCnt < strMsg.length() && !(strItem = readItem()).empty()) {
		vItem.push_back(strItem);
		++cnt;
	}
	return cnt;
}

std::string DiceEvent::printFrom()
{
	std::string strFwd;
	if (!isPrivate())strFwd += isChannel()
		? ("[频道:" + to_string(fromChat.gid) + "]")
		: ("[群:" + to_string(fromChat.gid) + "]");
	strFwd += getName(fromChat.uid, fromChat.gid) + "(" + to_string(fromChat.uid) + "):";
	return strFwd;
}

void reply(AttrObject& msg, string strReply, bool isFormat) {
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	if(isFormat)strReply = fmt->format(strReply, msg);
	if (console["ReferMsgReply"] && msg.get_int("msgid"))strReply = "[CQ:reply,id=" + msg.get_str("msgid") + "]" + strReply;
	long long uid{ msg.get_ll("uid") };
	long long gid{ msg.get_ll("gid") };
	long long chid{ msg.get_ll("chid") };
	if (uid || gid || chid)
		AddMsgToQueue(strReply, chatInfo{ uid,gid,chid });
}
void MsgNote(AttrObject& msg, string strReply, int note_lv) {
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	strReply = fmt->format(strReply, msg);
	if (console["ReferMsgReply"] && msg.get_int("msgid"))strReply = "[CQ:reply,id=" + msg.get_str("msgid") + "]" + strReply;
	long long uid{ msg.get_ll("uid") };
	long long gid{ msg.get_ll("gid") };
	long long chid{ msg.get_ll("chid") };
	if (uid || gid || chid)
		AddMsgToQueue(strReply, chatInfo{ uid,gid,chid });
	strReply = getName(uid) + strReply;
	for (const auto& [ct, level] : console.NoticeList)
	{
		if (!(level & note_lv) || ct.uid == uid || ct.gid == gid)continue;
		AddMsgToQueue(strReply, ct);
	}
}