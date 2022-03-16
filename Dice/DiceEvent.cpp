#include "DDAPI.h"
#include "DiceEvent.h"
#include "Jsonio.h"
#include "MsgFormat.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "ManagerSystem.h"
#include "BlackListManager.h"
#include "CharacterCard.h"
#include "DiceSession.h"
#include "GetRule.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "DiceGUI.h"
#include "DiceStatic.hpp"
#include <memory>
#include <ctime>
using namespace std;

AttrVar idx_at(AttrObject& eve) {
	if (eve.has("at"))return eve["at"];
	if (!eve.has("uid"))return {};
	return eve["at"] = eve.has("gid")
		? "[CQ:at,id=" + eve.get_str("uid") + "]"
		: idx_nick(eve);
}

AttrIndexs MsgIndexs{
	{"nick", idx_nick},
	{"pc", idx_pc},
	{"at", idx_at},
};

FromMsg::FromMsg(const AttrVars& var, const chatInfo& ct)
	:DiceJobDetail(var, ct), strMsg(vars["fromMsg"].text) {
	if (fromChat.gid) {
		pGrp = &chat(fromChat.gid);
		fromSession = fromChat.gid;
	}
	else {
		fromSession = ~fromChat.uid;
	}
	
}
bool FromMsg::isPrivate()const {
	return !fromChat.gid;
}
bool FromMsg::isFromMaster()const {
	return fromChat.uid == console.master();
}
bool FromMsg::isChannel()const {
	return fromChat.chid;
}

void FromMsg::formatReply() {
	if (msgMatch.ready())strReply = convert_realw2a(msgMatch.format(convert_a2realw(strReply.c_str())).c_str());
	strReply = fmt->format(strReply, vars);
}

void FromMsg::reply(const std::string& msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(const char* msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(bool isFormat) {
	if (isVirtual && fromChat.uid == console.DiceMaid && isPrivate())return;
	isAns = true;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	if (isFormat)
		formatReply();
	logEcho();
	if (console["ReferMsgReply"] && vars.has("msgid"))strReply = "[CQ:reply,id=" + vars.get_str("msgid") + "]" + strReply;
	AddMsgToQueue(strReply, fromChat);
}
void FromMsg::replyMsg(const std::string& key) {
	if (isVirtual && fromChat.uid == console.DiceMaid && isPrivate())return;
	isAns = true;
	strReply = getMsg(key, vars);
	logEcho();
	if (console["ReferMsgReply"] && vars.has("msgid"))strReply = "[CQ:reply,id=" + vars.get_str("msgid") + "]" + strReply;
	AddMsgToQueue(strReply, fromChat);
}

void FromMsg::replyHidden(const std::string& msgReply) {
	strReply = msgReply;
	replyHidden();
}
void FromMsg::replyHidden() {
	isAns = true;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	formatReply();
	if (LogList.count(fromSession) && gm->session(fromSession).is_logging()) {
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(getMsg("strSelfName")) + "(" + to_string(console.DiceMaid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(filter_CQcode(strReply, fromChat.gid)) << endl << endl;
	}
	strReply = "在" + printChat(fromChat) + "中 " + forward_filter(strReply);
	AddMsgToQueue(strReply, fromChat.uid);
	if (gm->has_session(fromSession)) {
		for (auto qq : gm->session(fromSession).get_ob()) {
			if (qq != fromChat.uid) {
				AddMsgToQueue(strReply, qq);
			}
		}
	}
}

void FromMsg::logEcho(){
	if (!isPrivate()
		&& (isChannel() ? console["ListenChannelEcho"] : console["ListenGroupEcho"]))return;
	if (LinkList.count(fromSession) && LinkList[fromSession].second
		&& strLowerMessage.find(".link") != 0) {
		string strFwd{ getMsg("strSelfCall") + ":"
			+ forward_filter(strReply) };
		if (long long aim = LinkList[fromSession].first; aim < 0) {
			AddMsgToQueue(strFwd, ~aim);
		}
		else if (ChatList.count(aim)) {
			AddMsgToQueue(strFwd, { 0,aim });
		}
	}
	if (LogList.count(fromSession)
		&& strLowerMessage.find(".log") != 0) {
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(getMsg("strSelfName")) + "(" + to_string(console.DiceMaid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(filter_CQcode(strReply, fromChat.gid)) << endl << endl;
	}
}
void FromMsg::fwdMsg()
{
	if (LinkList.count(fromSession) && LinkList[fromSession].second
		&& fromChat.uid != console.DiceMaid
		&& strLowerMessage.find(".link") != 0){
		string strFwd;
		if (trusted < 5)strFwd += printFrom();
		strFwd += forward_filter(strMsg);
		if (long long aim = LinkList[fromSession].first;aim < 0) {
			AddMsgToQueue(strFwd, ~aim);
		}
		else if (ChatList.count(aim)) {
			AddMsgToQueue(strFwd, { 0,aim });
		}
	}
	if (LogList.count(fromSession) && strLowerMessage.find(".log") != 0) {
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(idx_pc(vars).to_str()) + "(" + to_string(fromChat.uid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(filter_CQcode(strMsg, fromChat.gid)) << endl << endl;
	}
}

void FromMsg::note(std::string strMsg, int note_lv)
{
	strMsg = fmt->format(strMsg, vars);
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
int FromMsg::AdminEvent(const string& strOption)
{
	if (strOption == "isban")
	{
		vars["target"] = readDigit();
		blacklist->isban(this);
		return 1;
	}
	if (strOption == "state")
	{
		ResList res;
		res << "Servant:" + printUser(console.DiceMaid)
			<< "Master:" + printUser(console.master())
			<< (console["Private"] ? "私用模式" : "公用模式");
		if (console["LeaveDiscuss"])res << "禁用讨论组";
		if (console["DisabledGlobal"])res << "全局静默中";
		if (console["DisabledMe"])res << "全局禁用.me";
		if (console["DisabledJrrp"])res << "全局禁用.jrrp";
		if (console["DisabledDraw"])res << "全局禁用.draw";
		if (console["DisabledSend"])res << "全局禁用.send";
		if (trusted > 3)
			res << "所在群聊数：" + to_string(DD::getGroupIDList().size())
			<< "群记录数：" + to_string(ChatList.size())
			<< "好友数：" + to_string(DD::getFriendQQList().size())
			<< "用户记录数：" + to_string(UserList.size())
			<< "今日用户量：" + to_string(today->cntUser())
			<< (!PList.empty() ? "角色卡记录数：" + to_string(PList.size()) : "")
			<< "黑名单用户数：" + to_string(blacklist->mQQDanger.size())
			<< "黑名单群数：" + to_string(blacklist->mGroupDanger.size())
			<< (censor.size() ? "敏感词库规模：" + to_string(censor.size()) : "")
			<< console.listClock().dot("\t").show();
		reply(getMsg("strSelfName") + "的当前情况" + res.show());
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
	if (strOption == "dicelist")
	{
		getDiceList();
		strReply = "当前骰娘列表：";
		for (auto& [diceQQ, masterQQ] : mDiceList)
		{
			strReply += "\n" + printUser(diceQQ);
		}
		reply();
		return 1;
	}
	if (strOption == "censor") {
		readSkipSpace();
		if (strMsg[intMsgCnt] == '+') {
			intMsgCnt++;
			vars["danger_level"] = readToColon();
			Censor::Level danger_level = censor.get_level(vars["danger_level"].to_str());
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
			reply(fmt->get_help("censor"));
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
		Console::Clock cc{0, 0};
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
			int intLV;
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
	if (strOption == "ext")
	{
		try
		{
			string action = readPara();	
			if (action == "install")
			{
				string package = readRest();
				ExtensionManagerInstance->installPackage(GBKtoUTF8(package));
				reply("已成功安装" + package);
			}
			else if (action == "query")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryPackage(GBKtoUTF8(package)));
			}
			else if (action == "update")
			{
				ExtensionManagerInstance->refreshIndex();
				reply("已成功刷新软件包缓存，" + to_string(ExtensionManagerInstance->getIndexCount()) + "个拓展可用，"
					+ to_string(ExtensionManagerInstance->getUpgradableCount()) + "个可升级");
			}
			else if (action == "list")
			{
				string re = "可用拓展:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.name) + " ";
				}
				reply(re);
			}
			else if (action == "search")
			{
				string package = readRest();
				string re = "搜索结果:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.name);
					if(GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}		
				}
				reply(re);
			}
			else if (action == "listinstalled")
			{
				string re = "已安装拓展:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.first.name) + " ";
				}
				reply(re);
			}
			else if (action == "remove")
			{
				string package = readRest();
				ExtensionManagerInstance->removePackage(GBKtoUTF8(package));
				reply("已成功卸载" + package);
			}
			else if (action == "queryinstalled")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryInstalledPackage(GBKtoUTF8(package)).first);
			}
			else if (action == "searchinstalled")
			{
				string package = readRest();
				string re = "搜索结果:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.first.name);
					if (GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}		
				}
				reply(re);
			}
			else if (action == "upgrade")
			{
				string package = readRest();
				if (package.empty())
				{
					int cnt = ExtensionManagerInstance->upgradeAllPackages();
					reply("成功升级" + std::to_string(cnt) + "个拓展");
				}
				else
				{
					if (ExtensionManagerInstance->upgradePackage(GBKtoUTF8(package)))
					{
						reply(package + "已成功被升级");
					}
					else
					{
						reply(package + "无需升级");
					}
				}
			}
			else 
			{
				reply("Unknown command");
			}
		}
		catch (const std::exception& e)
		{
			reply(UTF8toGBK(e.what(), true));
		}
		return 1;
	}
	if (strOption == "recorder")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatInfo cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("当前通知窗口" + to_string(list.size()) + "个：" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				note("已停止发送" + printChat(cTarget) + "√", 0b1);
				console.redNotice(cTarget, 0b1);
			}
			else
			{
				reply("该窗口不接受日志通知！");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				reply("该窗口已接收日志通知！");
			}
			else
			{
				console.addNotice(cTarget, 0b11011);
				reply("已添加日志窗口" + printChat(cTarget) + "√");
			}
		}
		return 1;
	}
	if (strOption == "monitor")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatInfo cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("当前通知窗口" + to_string(list.size()) + "个：" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				console.redNotice(cTarget, 0b100000);
				note("已移除监视窗口" + printChat(cTarget) + "√", 0b1);
			}
			else
			{
				reply("该窗口不存在于监视列表！");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				reply("该窗口已存在于监视列表！");
			}
			else
			{
				console.addNotice(cTarget, 0b100000);
				note("已添加监视窗口" + printChat(cTarget) + "√", 0b1);
			}
		}
		return 1;
	}
	if (strOption == "blackfriend")
	{
		ResList res;
		set<long long> uids{ DD::getFriendQQList() };
		for(long long uid: uids){
			if (blacklist->get_qq_danger(uid))
				res << printUser(uid);
		}
		if (res.empty())
		{
			reply("好友列表内无黑名单用户√", false);
		}
		else
		{
			reply("好友列表内黑名单用户：" + res.show(), false);
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
					if (!chat(llGroup).isset("已退") && !chat(llGroup).isset("未进"))AddMsgToQueue(
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
		vars["note"] = readPara();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llTargetID = readID();
		if (strOption == "dismiss")
		{
			if (ChatList.count(llTargetID))
			{
				note("已令" + getMsg("strSelfName") + "退出" + printChat(chat(llTargetID)), 0b10);
				chat(llTargetID).reset("免清").leave();
			}
			else
			{
				replyMsg("strGroupGetErr");
			}
			return 1;
		}
		else if (strOption == "boton")
		{
			if (ChatList.count(llTargetID))
			{
				if (groupset(llTargetID, "停用指令") > 0)
				{
					chat(llTargetID).reset("停用指令");
					note("已令" + getMsg("strSelfName") + "在" + printGroup(llTargetID) + "启用指令√");
				}
				else reply(getMsg("strSelfName") + "已在该群启用指令!");
			}
			else
			{
				replyMsg("strGroupGetErr");
			}
		}
		else if (strOption == "botoff")
		{
			if (groupset(llTargetID, "停用指令") < 1)
			{
				chat(llTargetID).set("停用指令");
				note("已令" + getMsg("strSelfName") + "在" + printGroup(llTargetID) + "停用指令√", 0b1);
			}
			else reply(getMsg("strSelfName") + "已在该群停用指令!");
			return 1;
		}
		else if (strOption == "blackgroup")
		{
			if (llTargetID == 0)
			{
				ResList res;
				for (auto [each, danger] : blacklist->mGroupDanger) {
					res << printGroup(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			vars["time"] = printSTNow();
			do
			{
				if (boolErase)
				{
					blacklist->rm_black_group(llTargetID, this);
				}
				else
				{
					blacklist->add_black_group(llTargetID, this);
				}
			} 
			while ((llTargetID = readID()));
			return 1;
		}
		else if (strOption == "whiteqq")
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
						vars["user_nick"] = getName(llTargetID);
						AddMsgToQueue(getMsg("strWhiteQQAddNotice", vars), llTargetID);
					}
				}
			}
			while ((llTargetID = readID()));
			return 1;
		}
		else if (strOption == "blackqq")
		{
			if (llTargetID == 0) 
			{
				ResList res;
				for (auto [each, danger] : blacklist->mQQDanger) 
				{
					res << printUser(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			vars["time"] = printSTNow();
			do 
			{
				if (boolErase)
				{
					blacklist->rm_black_qq(llTargetID, this);
				}
				else
				{
					blacklist->add_black_qq(llTargetID, this);
				}
			}
			while ((llTargetID = readID()));
			return 1;
		}
		else replyMsg("strAdminOptionEmpty");
		return 0;
	}
}

int FromMsg::MasterSet() 
{
	const std::string strOption = readPara();
	if (strOption.empty())
	{
		replyMsg("strAdminOptionEmpty");
		return -1;
	}
	if (strOption == "groupclr")
	{
		vars["clear_mode"] = readRest();
		cmd_key = "clrgroup";
		sch.push_job(*this);
		return 1;
	}
	if (strOption == "delete")
	{
		if (console.master() != fromChat.uid)
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
		if (console.master() != fromChat.uid)
		{
			replyMsg("strNotMaster");
			return 1;
		}
		const string strMaster = readDigit();
		if (strMaster.empty() || stoll(strMaster) == console.master())
		{
			reply("Master不要消遣{strSelfCall}!");
		}
		else
		{
			console.newMaster(stoll(strMaster));
			note("已将Master转让给" + printUser(console.master()));
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

int FromMsg::BasicOrder()
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
		if (trusted > 0)
		{
			pGrp->set("许可使用").reset("未审核").reset("协议无效");
			note("已授权" + printGroup(pGrp->ID) + "许可使用", 1);
			AddMsgToQueue(getMsg("strGroupAuthorized", vars), { 0,pGrp->ID });
		}
		else
		{
			if (!console["CheckGroupLicense"] && !console["Private"] && !isCalled)return 0;
			string strInfo = readRest();
			if (strInfo.empty())console.log(printUser(fromChat.uid) + "申请" + printGroup(pGrp->ID) + "许可使用", 0b10, printSTNow());
			else console.log(printUser(fromChat.uid) + "申请" + printGroup(pGrp->ID) + "许可使用；附言：" + strInfo, 0b100, printSTNow());
			replyMsg("strGroupLicenseApply");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		if (isPrivate())
		{
			string QQNum = readDigit();
			if (QQNum.empty())
			{
				replyMsg("strDismissPrivate");
				return -1;
			}
			long long llGroup = stoll(QQNum);
			if (!ChatList.count(llGroup))
			{
				replyMsg("strGroupNotFound");
				return 1;
			}
			Chat& grp = chat(llGroup);
			if (grp.isset("已退") || grp.isset("未进"))
			{
				replyMsg("strGroupAway");
			}
			if (trustedQQ(fromChat.uid) > 2) {
				grp.leave(getMsg("strAdminDismiss", vars));
				replyMsg("strGroupExit");
			}
			else if(DD::isGroupAdmin(llGroup, fromChat.uid, true) || (grp.inviter == fromChat.uid))
			{
				replyMsg("strDismiss");
			}
			else
			{
				replyMsg("strPermissionDeniedErr");
			}
			return 1;
		}
		string QQNum = readDigit();
		bool isTarget{ QQNum == to_string(console.DiceMaid)
			|| (QQNum.length() == 4 && stoll(QQNum) == console.DiceMaid % 10000) };
		if (QQNum.empty() || isTarget){
			if (trusted > 2) 
			{
				pGrp->leave(getMsg("strAdminDismiss", vars));
				return 1;
			}
			if (pGrp->isset("协议无效") && !isTarget)return 0;
			if (canRoomHost())
			{
				pGrp->leave(getMsg("strDismiss"));
			}
			else
			{
				if (!isCalled && (pGrp->isset("停用指令") || DD::getGroupSize(fromChat.gid).currSize > 200))AddMsgToQueue(getMsg("strPermissionDeniedErr", vars), fromChat.uid);
				else replyMsg("strPermissionDeniedErr");
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "warning")
	{
		intMsgCnt += 7;
		string strWarning = readRest();
		AddWarning(strWarning, fromChat.uid, fromChat.gid);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "master" && console.isMasterMode)
	{
		intMsgCnt += 6;
		if (!console.master())
		{
			string strOption = readRest();
			if (strOption == "public"){
				console.set("BelieveDiceList", 1);
				console.set("LeaveBlackQQ", 1);
				console.set("BannedBanInviter", 1);
				console.set("KickedBanInviter", 1);
			}
			else{
				console.set("Private", 1);
			}
			console.newMaster(fromChat.uid);
		}
		else if (trusted > 4 || console.master() == fromChat.uid)
		{
			return MasterSet();
		}
		else
		{
			if (isCalled)replyMsg("strNotMaster");
			return 1;
		}
		return 1;
	}
	else if (!isPrivate() && pGrp->isset("协议无效")){
		return 1;
	}
	if (blacklist->get_qq_danger(fromChat.uid) || (!isPrivate() && blacklist->get_group_danger(fromChat.gid)))
	{
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		string Command = readPara();
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(console.DiceMaid) 
			|| (QQNum.length() == 4 && stoll(QQNum) == console.DiceMaid % 10000))
		{
			if (Command == "on" && !isPrivate())
			{
				if ((console["CheckGroupLicense"] && pGrp->isset("未审核")) || (console["CheckGroupLicense"] == 2 && !pGrp->isset("许可使用")))
					replyMsg("strGroupLicenseDeny");
				else {
					if (canRoomHost() || trusted > 2)
					{
						if (groupset(fromChat.gid, "停用指令") > 0)
						{
							chat(fromChat.gid).reset("停用指令");
							replyMsg("strBotOn");
						}
						else
						{
							replyMsg("strBotOnAlready");
						}
					}
					else
					{
						if (groupset(fromChat.gid, "停用指令") > 0 && DD::getGroupSize(fromChat.gid).currSize > 200)AddMsgToQueue(
							getMsg("strPermissionDeniedErr", vars), fromChat.uid);
						else replyMsg("strPermissionDeniedErr");
					}
				}
			}
			else if (Command == "off" && !isPrivate())
			{
				if (canRoomHost() || trusted > 2)
				{
					if (groupset(fromChat.gid, "停用指令"))
					{
						if (!isCalled && QQNum.empty() && pGrp->isGroup && DD::getGroupSize(fromChat.gid).currSize > 200)AddMsgToQueue(getMsg("strBotOffAlready", vars), fromChat.uid);
						else replyMsg("strBotOffAlready");
					}
					else 
					{
						chat(fromChat.gid).set("停用指令");
						replyMsg("strBotOff");
					}
				}
				else
				{
					if (groupset(fromChat.gid, "停用指令"))AddMsgToQueue(getMsg("strPermissionDeniedErr", vars), fromChat.uid);
					else replyMsg("strPermissionDeniedErr");
				}
			}
			else if (!Command.empty() && !isCalled && pGrp->isset("停用指令"))
			{
				return 0;
			}
			else if (!isPrivate() && pGrp->isset("停用指令") && DD::getGroupSize(fromChat.gid).currSize > 500 && !isCalled)
			{
				AddMsgToQueue(getMsg("strBotHeader") + Dice_Full_Ver_On + getMsg("strBotMsg"), fromChat.uid);
			}
			else
			{
				this_thread::sleep_for(1s);
				reply(getMsg("strBotHeader") + Dice_Full_Ver_On + getMsg("strBotMsg"));
			}
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
	if (isDisabled || (!isCalled || !console["DisabledListenAt"]) && (groupset(fromChat.gid, "停用指令") > 0))
	{
		if (isPrivate())
		{
			replyMsg("strGlobalOff");
			return 1;
		}
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "helpdoc" && trusted > 3)
	{
		intMsgCnt += 7;
		while (strMsg[intMsgCnt] == ' ')
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			replyMsg("strHlpNameEmpty");
			return true;
		}
		string key{ readUntilSpace() };
		vars["key"] = key;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			CustomHelp.erase(key);
			if (auto it = HelpDoc.find(key); it != HelpDoc.end())
				fmt->set_help(it->first, it->second);
			else
				fmt->rm_help(key);
			replyMsg("strHlpReset");
		}
		else
		{
			string strHelpdoc = strMsg.substr(intMsgCnt);
			CustomHelp[key] = strHelpdoc;
			fmt->set_help(key, strHelpdoc);
			replyMsg("strHlpSet");
		}
		saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
		return true;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		vars["help_word"] = readRest();
		if (!isPrivate())
		{
			if (!canRoomHost() && (vars["help_word"] == "on" || vars["help_word"] == "off"))
			{
				replyMsg("strPermissionDeniedErr");
				return 1;
			}
			vars["option"] = "禁用help";
			if (vars["help_word"] == "off")
			{
				if (groupset(fromChat.gid, vars["option"].to_str()) < 1)
				{
					chat(fromChat.gid).set(vars["option"].to_str());
					replyMsg("strGroupSetOn");
				}
				else
				{
					replyMsg("strGroupSetOnAlready");
				}
				return 1;
			}
			if (vars["help_word"] == "on")
			{
				if (groupset(fromChat.gid, vars["option"].to_str()) > 0)
				{
					chat(fromChat.gid).reset(vars["option"].to_str());
					replyMsg("strGroupSetOff");
				}
				else
				{
					replyMsg("strGroupSetOffAlready");
				}
				return 1;
			}
			if (groupset(fromChat.gid, vars["option"].to_str()) > 0)
			{
				replyMsg("strGroupSetOnAlready");
				return 1;
			}
		}
		std::thread th(&DiceModManager::_help, fmt.get(), shared_from_this());
		th.detach();
		return true;
	}
	return 0;
}

int FromMsg::InnerOrder() {
	if (strMsg[0] != '.')return 0;
	if (WordCensor()) {
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("welcome"));
			return 1;
		}
		if (isPrivate()) {
			replyMsg("strWelcomePrivate");
			return 1;
		}
		if (canRoomHost()) {
			string strWelcomeMsg = strMsg.substr(intMsgCnt);
			if (strWelcomeMsg == "clr") {
				if (chat(fromChat.gid).confs.count("入群欢迎")) {
					chat(fromChat.gid).reset("入群欢迎");
					replyMsg("strWelcomeMsgClearNotice");
				}
				else {
					replyMsg("strWelcomeMsgClearErr");
				}
			}
			else if (strWelcomeMsg == "show") {
				string strWelcome{ chat(fromChat.gid).confs["入群欢迎"].to_str() };
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
			vars["list_mode"] = readPara();
			cmd_key = "lsgroup";
			sch.push_job(*this);
		}
		else if (strOption == "clr") {
			if (trusted < 5) {
				replyMsg("strNotMaster");
				return 1;
			}
			int cnt = clearGroup();
			note("已清理过期群记录" + to_string(cnt) + "条", 0b10);
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
			if (isPrivate()) {
				if (User& user{ getUser(fromChat.uid) }; user.confs.count("rc房规"))
				vars["rule"] = user.confs["rc房规"];
			}
			else if (pGrp->isset("rc房规")) {
				vars["rule"] = pGrp->confs["rc房规"];
			}
			if (vars.has("rule")) {
				replyMsg("strDefaultCOCShow");
			}
			else {
				vars["rule"] = console["DefaultCOCRoomRule"];
				replyMsg("strDefaultCOCShowDefault");
			}
			return 1;
		}
		else if (action == "clr") {
			if (isPrivate())getUser(fromChat.uid).rmConf("rc房规");
			else chat(fromChat.gid).reset("rc房规");
			replyMsg("strDefaultCOCClr");
			return 1;
		}
		string strRule = readDigit();
		if (strRule.empty()) {
			reply(fmt->get_help("setcoc"));
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
#ifdef _WIN32
		if (strOption == "gui") {
			reply("Dice! GUI已停止更新，请考虑使用Dice! WebUI https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming");
			thread th(GUIMain);
			th.detach();
			return 1;
		}
#endif
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
				<< "今日指令量:" + to_string(today->get("frq"))
				<< "启动后指令量:" + to_string(FrqMonitor::sumFrqTotal);
			reply(res.show());
			return 1;
		}
		if (strOption == "clrimg") {
			reply("非酷Q框架不需要此功能");
			return -1;
		}
		else if (strOption == "reload") {
			if (trusted < 5 && fromChat.uid != console.master()) {
				replyMsg("strNotMaster");
				return -1;
			}
			cmd_key = "reload";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "remake") {
			
			if (trusted < 5 && fromChat.uid != console.master()) {
				replyMsg("strNotMaster");
				return -1;
			}
			cmd_key = "remake";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "die") {
			if (trusted < 5 && fromChat.uid != console.master()) {
				replyMsg("strNotMaster");
				return -1;
			}
			cmd_key = "die";
			sch.push_job(*this);
			return 1;
		}
		if (strOption == "rexplorer")
		{
#ifdef _WIN32
			if (trusted < 5 && fromChat.uid != console.master())
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
			if (fromChat.uid != console.master())
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
	else if (strLowerMessage.substr(intMsgCnt, 5) == "admin") {
		intMsgCnt += 5;
		return AdminEvent(readPara());
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "cloud") {
		intMsgCnt += 5;
		string strOpt = readPara();
		if (trusted < 4 && fromChat.uid != console.master()) {
			replyMsg("strNotAdmin");
			return 1;
		}
		if (strOpt == "update") {
			vars["ver"] = readPara();
			if (vars["ver"].str_empty()) {
				Cloud::checkUpdate(this);
			}
			else if (vars["ver"] == "dev" || vars["ver"] == "release") {
				cmd_key = "update";
				sch.push_job(*this);
			}
			return 1;
		}
		else if (strOpt == "black") {
			cmd_key = "cloudblack";
			sch.push_job(*this);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd") {
		vars["res"] = COC7D();
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d") {
		vars["res"] = COC6D();
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group") {
		intMsgCnt += 5;
		long long llGroup(fromChat.gid);
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("group"));
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
				string strOption{ (vars["option"] = readRest()).to_str() };
				if (!mChatConf.count(vars["option"].to_str())) {
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
					vars["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
				else {
					for (auto& [id, grp] : ChatList) {
						if (!grp.isset(strOption))continue;
						grp.reset(strOption);
						Cnt++;
					}
					vars["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
			}
			return 1;
		}
		if (!(vars["group_id"] = readDigit(false)).str_empty()) {
			llGroup = stoll(vars["group_id"].to_str());
			if (!ChatList.count(llGroup)) {
				replyMsg("strGroupNotFound");
				return 1;
			}
			if (getGroupAuth(llGroup) < 0) {
				replyMsg("strGroupDenied");
				return 1;
			}
		}
		else if (isPrivate())return 0;
		else vars["group_id"] = to_string(fromChat.gid);
		Chat& grp = chat(llGroup);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if(strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'){
			int cntSet{ 0 };
			bool isSet{ strMsg[intMsgCnt] == '+' };
			do{
				isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				const string option{ (vars["option"] = readPara()).to_str() };
				readSkipSpace();
				if (!mChatConf.count(vars["option"].to_str())) {
					replyMsg("strGroupSetInvalid");
					continue;
				}
				if (getGroupAuth(llGroup) >= get<string, short>(mChatConf, option, 0)) {
					if (isSet) {
						if (groupset(llGroup, vars["option"].to_str()) < 1) {
							chat(llGroup).set(vars["option"].to_str());
							++cntSet;
							if (vars["Option"] == "许可使用") {
								AddMsgToQueue(getMsg("strGroupAuthorized", vars), { 0, fromChat.uid });
							}
						}
						else {
							replyMsg("strGroupSetOnAlready");
						}
					}
					else if (grp.isset(vars["option"].to_str())) {
						++cntSet;
						chat(llGroup).reset(vars["option"].to_str());
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
				vars["opt_list"] = grp.listBoolConf();
				replyMsg("strGroupMultiSet");
			}
			return 1;
		}
		bool isInGroup{ fromChat.gid == llGroup || DD::isGroupMember(llGroup,console.DiceMaid,true) };
		string Command = readPara();
		vars["group"] = DD::printGroupInfo(llGroup);
		if (Command.empty()) {
			reply(fmt->get_help("group"));
			return 1;
		}
		else if (Command == "state") {
			ResList res;
			res << "在{group}：";
			res << grp.listBoolConf();
			res << "记录创建：" + printDate(grp.tCreated);
			res << "最后记录：" + printDate(grp.tUpdated);
			if (grp.inviter)res << "邀请者：" + printUser(grp.inviter);
			res << string("入群欢迎：") + (grp.isset("入群欢迎") ? "已设置" : "无");
			reply(getMsg("strSelfName") + res.show());
			return 1;
		}
		if (!grp.isGroup || (fromChat.gid == llGroup && isPrivate())) {
			replyMsg("strGroupNot");
			return 1;
		}
		else if (Command == "info") {
			reply(DD::printGroupInfo(llGroup), false);
			return 1;
		}
		else if (!isInGroup) {
			replyMsg("strGroupNotIn");
			return 1;
		}
		else if (Command == "survey") {
			int cntDiver(0);
			long long dayMax(0);
			set<string> sBlackQQ;
			int cntUser(0);
			size_t cntDice(0);
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			set<long long> list{ DD::getGroupMemberList(llGroup) };
			if (list.empty()) {
				reply("{self}加载成员列表失败！");
				return 1;
			}
			for (auto each : list) {
				if (each == console.DiceMaid)continue;
				if (long long lst{ DD::getGroupLastMsg(llGroup,each) }; lst > 0) {
					long long dayDive((tNow - lst) / intTDay);
					if (dayDive > dayMax)dayMax = dayDive;
					if (dayDive > 30)++cntDiver;
				}
				if (blacklist->get_qq_danger(each) > 1) {
					sBlackQQ.insert(printUser(each));
				}
				if (UserList.count(each))++cntUser;
				if (DD::isDiceMaid(each))++cntDice;
				++cntSize;
			}
			ResList res;
			res << "在{group}内"
				<< "{self}用户占比: " + to_string(cntUser * 100 / (cntSize)) + "%"
				<< (cntDice ? "同系骰娘: " + to_string(cntDice) : "")
				<< (cntDiver ? "30天潜水群员: " + to_string(cntDiver) : "");
			if (!sBlackQQ.empty()) {
				if (sBlackQQ.size() > 8)
					res << getMsg("strSelfName") + "的黑名单成员" + to_string(sBlackQQ.size()) + "名";
				else {
					res << getMsg("strSelfName") + "的黑名单成员:{blackqq}";
					ResList blacks;
					for (const auto& each : sBlackQQ) {
						blacks << each;
					}
					vars["blackqq"] = blacks.show();
				}
			}
			reply(res.show());
			return 1;
		}
		else if (Command == "diver") {
			bool bForKick = false;
			if (strLowerMessage.substr(intMsgCnt, 5) == "4kick") {
				bForKick = true;
				intMsgCnt += 5;
			}
			std::priority_queue<std::pair<time_t, string>> qDiver;
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			for (auto each : DD::getGroupMemberList(llGroup)) {
				long long lst{ DD::getGroupLastMsg(llGroup,each) };
				time_t intLastMsg = (tNow - lst) / intTDay;
				if (lst > 0 || intLastMsg > 30) {
					qDiver.emplace(intLastMsg, (bForKick ? to_string(each)
												: printUser(each)));
				}
				++cntSize;
			}
			if (!cntSize) {
				reply("{self}加载成员列表失败！");
				return 1;
			}
			else if (qDiver.empty()) {
				reply("{self}未发现潜水群成员！");
				return 1;
			}
			int intCnt(0);
			ResList res;
			while (!qDiver.empty()) {
				res << (bForKick ? qDiver.top().second
						: (qDiver.top().second + to_string(qDiver.top().first) + "天"));
				if (++intCnt > 15 && intCnt > cntSize / 80)break;
				qDiver.pop();
			}
			bForKick ? reply("(.group " + to_string(llGroup) + " kick " + res.show(1))
				: reply("潜水成员列表:" + res.show(1));
			return 1;
		}
		if (bool isAdmin = DD::isGroupAdmin(llGroup, fromChat.uid, true); Command == "pause") {
			if (!isAdmin && trusted < 4) {
				replyMsg("strPermissionDeniedErr");
				return 1;
			}
			int secDuring(-1);
			string strDuring{ readDigit() };
			if (!strDuring.empty())secDuring = stoi(strDuring);
			DD::setGroupWholeBan(llGroup, secDuring);
			replyMsg("strGroupWholeBan");
			return 1;
		}
		else if (Command == "restart") {
			if (!isAdmin && trusted < 4) {
				replyMsg("strPermissionDeniedErr");
				return 1;
			}
			DD::setGroupWholeBan(llGroup, 0);
			replyMsg("strGroupWholeUnban");
			return 1;
		}
		else if (Command == "card") {
			if (long long llqq = readID()) {
				if (trusted < 4 && !isAdmin && llqq != fromChat.uid) {
					replyMsg("strPermissionDeniedErr");
					return 1;
				}
				if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
					replyMsg("strSelfPermissionErr");
					return 1;
				}
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				vars["card"] = readRest();
				vars["target"] = getName(llqq, llGroup);
				DD::setGroupCard(llGroup, llqq, vars["card"].to_str());
				replyMsg("strGroupCardSet");
			}
			else {
				replyMsg("strUidEmpty");
			}
			return 1;
		}
		else if ((!isAdmin && (!DD::isGroupOwner(llGroup, console.DiceMaid, true) || trusted < 5))) {
			replyMsg("strPermissionDeniedErr");
			return 1;
		}
		else if (Command == "ban") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				replyMsg("strSelfPermissionErr");
				return 1;
			}
			string QQNum = readDigit();
			if (QQNum.empty()) {
				replyMsg("strUidEmpty");
				return -1;
			}
			long long llMemberQQ = stoll(QQNum);
			vars["member"] = getName(llMemberQQ, llGroup);
			string strMainDice = readDice();
			if (strMainDice.empty()) {
				replyMsg("strValueErr");
				return -1;
			}
			const int intDefaultDice = getUser(fromChat.uid).getConf("默认骰", 100);
			RD rdMainDice(strMainDice, intDefaultDice);
			rdMainDice.Roll();
			int intDuration{ rdMainDice.intTotal };
			vars["res"] = rdMainDice.FormShortString();
			DD::setGroupBan(llGroup, llMemberQQ, intDuration * 60);
			if (intDuration <= 0)
				replyMsg("strGroupUnban");
			else replyMsg("strGroupBan");
		}
		else if (Command == "kick") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				replyMsg("strSelfPermissionErr");
				return 1;
			}
			long long llMemberQQ = readID();
			if (!llMemberQQ) {
				replyMsg("strUidEmpty");
				return -1;
			}
			ResList resKicked, resDenied, resNotFound;
			do {
				if (int auth{ DD::getGroupAuth(llGroup, llMemberQQ,0) }) {
					if (auth > 1) {
						resDenied << printUser(llMemberQQ);
						continue;
					}
					DD::setGroupKick(llGroup, llMemberQQ);
					resKicked << printUser(llMemberQQ);
				}
				else resNotFound << printUser(llMemberQQ);
			} while ((llMemberQQ = readID()));
			strReply = getMsg("strSelfName");
			if (!resKicked.empty())strReply += "已移出群员：" + resKicked.show() + "\n";
			if (!resDenied.empty())strReply += "移出失败：" + resDenied.show() + "\n";
			if (!resNotFound.empty())strReply += "找不到对象：" + resNotFound.show();
			reply();
			return 1;
		}
		else if (Command == "title") {
			if (!DD::isGroupOwner(llGroup, console.DiceMaid,true)) {
				replyMsg("strSelfPermissionErr");
				return 1;
			}
			if (long long llqq = readID()) {
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				vars["title"] = readRest();
				DD::setGroupTitle(llGroup, llqq, vars["title"].to_str());
				vars["target"] = getName(llqq, llGroup);
				replyMsg("strGroupTitleSet");
			}
			else {
				replyMsg("strUidEmpty");
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "reply") {
		intMsgCnt += 5;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("reply"));
			return 1;
		}
		unsigned int intMsgTmpCnt{ intMsgCnt };
		string action{ readPara() };
		if (action == "on" && fromChat.gid) {
			const string option{ (vars["option"] = "禁用回复").to_str() };
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
			const string option{ (vars["option"] = "禁用回复").to_str() };
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
		else if (action == "show"){
			if (trusted < 2) {
				replyMsg("strNotAdmin");
				return -1;
			}
			vars["key"] = readRest();
			fmt->reply_show(shared_from_this());
			return 1;
		}
		else if (action == "get") {
			if (trusted < 2) {
				replyMsg("strNotAdmin");
				return -1;
			}
			vars["key"] = readRest();
			fmt->reply_get(shared_from_this());
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
					if(DiceMsgReply::sType.count(type))trigger->type = (DiceMsgReply::Type)DiceMsgReply::sType[type];
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
						}
						catch (const std::regex_error& e) {
							vars["err"] = e.what();
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
						while (intMsgCnt < strMsg.length()) {
							string item = readItem();
							if (!item.empty())trigger->deck.push_back(item);
						}
					}
					else {
						if(trigger->echo== DiceMsgReply::Echo::Lua && trusted < 5) {
							replyMsg("strNotMaster");
							return -1;
						}
						trigger->text = readRest();
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
				vars["key"] = trigger->title;
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
			vars["res"] = fmt->list_reply();
			replyMsg("strReplyList");
			return 1;
		}
		else if (action == "del") {
			if (trusted < 4) {
				replyMsg("strNotAdmin");
				return -1;
			}
			vars["key"] = readRest();
			if (fmt->del_reply(vars["key"].to_str())) {
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
		const string& key{ (vars["key"] = readUntilSpace()).text };
		if (key.empty()) {
			reply(fmt->get_help("reply"));
			return -1;
		}
		rep->keyMatch[MatchMode] = std::make_unique<vector<string>>(getLines(key, '|'));
		if(MatchMode == 3)
		{
			try
			{
				std::wregex re(convert_a2realw(key.c_str()), std::regex::ECMAScript);
			}
			catch (const std::regex_error& e)
			{
				vars["err"] = e.what();
				replyMsg("strRegexInvalid");
				return -1;
			}
		}
		readItems(rep->deck);
		if (rep->deck.empty()) {
			fmt->del_reply(key);
			replyMsg("strReplyDel");
		}
		else {
			fmt->set_reply(key, rep);
			replyMsg("strReplySet");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules") {
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rules"));
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
			string strReturn;
			if (getUser(fromChat.uid).confs.count("默认规则") && strSearch.find(':') == string::npos &&
				GetRule::get(getUser(fromChat.uid).confs["默认规则"].to_str(), strSearch, strReturn)) {
				reply(strReturn);
			}
			else if (GetRule::analyze(strSearch, strReturn)) {
				reply(strReturn);
			}
			else {
				reply(getMsg("strRuleErr") + strReturn);
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6") {
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
		vars["res"] = COC6(intNum);
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "deck") {
		if (trusted < 4 && console["DisabledDeck"]) {
			replyMsg("strDisabledDeckGlobal");
			return 1;
		}
		intMsgCnt += 4;
		string strRoom = readDigit(false);
		long long llRoom = strRoom.empty() ? fromSession : stoll(strRoom);
		if (llRoom == 0)llRoom = fromSession;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("deck"));
			return 1;
		}
		string strPara = readPara();
		if (strPara == "show") {
			if (gm->has_session(llRoom))
				gm->session(llRoom).deck_show(this);
			else replyMsg("strDeckListEmpty");
		}
		else if ((!canRoomHost() || llRoom != fromSession) && !trusted) {
			replyMsg("strWhiteQQDenied");
		}
		else if (strPara == "set") {
			gm->session(llRoom).deck_set(this);
		}
		else if (strPara == "reset") {
			gm->session(llRoom).deck_reset(this);
		}
		else if (strPara == "del") {
			gm->session(llRoom).deck_del(this);
		}
		else if (strPara == "clr") {
			gm->session(llRoom).deck_clr(this);
		}
		else if (strPara == "new") {
			gm->session(llRoom).deck_new(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {
		if (trusted < 4 && console["DisabledDraw"]) {
			replyMsg("strDisabledDrawGlobal");
			return 1;
		}
		vars["option"] = "禁用draw";
		if (isPrivate() && groupset(fromChat.gid, vars["option"].to_str()) > 0) {
			replyMsg("strGroupSetOnAlready");
			return 1;
		}
		intMsgCnt += 4;
		bool isPrivate(false);
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			vars["hidden"];
			isPrivate = true;
			++intMsgCnt;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		vector<string> ProDeck;
		vector<string>* TempDeck = nullptr;
		string& key{ (vars["deck_name"] = readAttrName()).text };
		while (!key.empty() && key[0] == '_') {
			isPrivate = true;
			vars["hidden"];
			key.erase(key.begin());
		}
		if (key.empty()) {
			reply(fmt->get_help("draw"));
			return 1;
		}
		else {
			if (gm->has_session(fromSession) && gm->session(fromSession).has_deck(key)) {
				gm->session(fromSession)._draw(this);
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
		vars["res"] = Res.dot("|").show();
		vars["cnt"] = to_string(Res.size());
		if (isPrivate) {
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
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init") {
		intMsgCnt += 4;
		vars["table_name"] = "先攻";
		string strCmd = readPara();
		if (strCmd.empty()|| isPrivate()) {
			reply(fmt->get_help("init"));
		}
		else if (!gm->has_session(fromSession) || !gm->session(fromSession).table_count("先攻")) {
			replyMsg("strGMTableNotExist");
		}
		else if (strCmd == "show" || strCmd == "list") {
			vars["res"] = gm->session(fromSession).table_prior_show("先攻");
			replyMsg("strGMTableShow");
		}
		else if (strCmd == "del") {
			vars["table_item"] = readRest();
			if (vars["table_item"].str_empty())
				replyMsg("strGMTableItemEmpty");
			else if (gm->session(fromSession).table_del("先攻", vars["table_item"].to_str()))
				replyMsg("strGMTableItemDel");
			else
				replyMsg("strGMTableItemNotFound");
		}
		else if (strCmd == "clr") {
			gm->session(fromSession).table_clr("先攻");
			replyMsg("strGMTableClr");
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp") {
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
		vars["res"] = to_string(today->getJrrp(fromChat.uid));
		replyMsg("strJrrp");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "link") {
		intMsgCnt += 4;
		if (trusted < 3) {
			replyMsg("strNotAdmin");
			return true;
		}
		vars["option"] = readPara();
		if (vars["option"] == "close") {
			gm->session(fromSession).link_close(this);
		}
		else if (vars["option"] == "start") {
			gm->session(fromSession).link_start(this);
		}
		else if (vars["option"] == "with" || vars["option"] == "from" || vars["option"] == "to") {
			gm->session(fromSession).link_new(this);
		}
		else {
			reply(fmt->get_help("link"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name") {
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
		vars["res"] = Res.dot("、").show();
		replyMsg("strNameGenerator");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "send") {
		intMsgCnt += 4;
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("send"));
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
					AddMsgToQueue(strFwd, ct);
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
	else if (strLowerMessage.substr(intMsgCnt, 4) == "user") {
		intMsgCnt += 4;
		string strOption = readPara();
		if (strOption.empty())return 0;
		if (strOption == "state") {
			User& user = getUser(fromChat.uid);
			vars["user"] = printUser(fromChat.uid);
			ResList rep;
			rep << "信任级别：" + to_string(trusted)
				<< "和{nick}的第一印象大约是在" + printDate(user.tCreated)
				<< (!(user.strNick.empty()) ? "正记录{nick}的" + to_string(user.strNick.size()) + "个称呼" : "没有记录{nick}的称呼")
				<< ((PList.count(fromChat.uid)) ? "这里有{nick}的" + to_string(PList[fromChat.uid].size()) + "张角色卡" : "无角色卡记录");
			reply("{user}" + rep.show());
			return 1;
		}
		if (strOption == "trust") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				replyMsg("strNotAdmin");
				return 1;
			}
			string strTarget = readDigit();
			if (strTarget.empty()) {
				replyMsg("strUidEmpty");
				return 1;
			}
			long long llTarget = stoll(strTarget);
			if (trustedQQ(llTarget) >= trusted && !console.is_self(fromChat.uid) && fromChat.uid != llTarget) {
				replyMsg("strUserTrustDenied");
				return 1;
			}
			vars["user"] = printUser(llTarget);
			vars["trust"] = readDigit();
			if (vars["trust"].str_empty()) {
				if (!UserList.count(llTarget)) {
					replyMsg("strUserNotFound");
					return 1;
				}
				vars["trust"] = trustedQQ(llTarget);
				replyMsg("strUserTrustShow");
				return 1;
			}
			User& user = getUser(llTarget);
			if (int intTrust = vars["trust"].to_int(); intTrust < 0 || intTrust > 255 || (intTrust >= trusted && fromChat.uid
																						   != console.master())) {
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
				reply(UTF8toGBK(to_json(getUser(target).confs).dump()), false);
			}
			else {
				reply("{self}无" + printUser(target) + "的用户记录×");
			}
			return 1;
		}
		if (strOption == "diss") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				replyMsg("strNotAdmin");
				return 1;
			}
			vars["note"] = readPara();
			long long llTargetID(readID());
			if (!llTargetID) {
				replyMsg("strUidEmpty");
			}
			else if (trustedQQ(llTargetID) >= trusted) {
				replyMsg("strUserTrustDenied");
			}
			else {
				blacklist->add_black_qq(llTargetID, this);
				UserList.erase(llTargetID);
				PList.erase(llTargetID);
			}
			return 1;
		}
		if (strOption == "kill") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				replyMsg("strNotAdmin");
				return 1;
			}
			long long llTarget = readID();
			if (trustedQQ(llTarget) >= trusted && fromChat.uid != console.master()) {
				replyMsg("strUserTrustDenied");
				return 1;
			}
			vars["user"] = printUser(llTarget);
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
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc") {
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
		vars["res"] = COC7(intNum);
		replyMsg("strCOCBuild");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd") {
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
		vars["res"] = DND(intNum);
		replyMsg("strDNDBuild");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "log") {
		intMsgCnt += 3;
		string strPara = readPara();
		if (strPara.empty()) {
			reply(fmt->get_help("log"));
		}
		else if (DiceSession& game = gm->session(fromSession); strPara == "new") {
			game.log_new(this);
		}
		else if (strPara == "on") {
			game.log_on(this);
		}
		else if (strPara == "off") {
			game.log_off(this);
		}
		else if (strPara == "end") {
			game.log_end(this);
		}
		else {
			reply(fmt->get_help("log"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string type = readPara();
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("随机姓名_" + type)) ? "随机姓名_" + type : "随机姓名";
		vars["old_nick"] = idx_nick(vars);
		vars["new_nick"] = strip(CardDeck::drawCard(CardDeck::mPublicDeck[strDeckName], true));
		getUser(fromChat.uid).setNick(fromChat.gid, vars["new_nick"].to_str());
		replyMsg("strNameSet");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string& strDice{ (vars["default"] = readDigit()).text };
		while (strDice[0] == '0')
			strDice.erase(strDice.begin());
		if (strDice.empty())
			strDice = "100";
		for (auto charNumElement : strDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement))) {
				replyMsg("strSetInvalid");
				return 1;
			}
		if (strDice.length() > 4) {
			replyMsg("strSetTooBig");
			return 1;
		}
		const int intDefaultDice = stoi(strDice);
		if (PList.count(fromChat.uid)) {
			PList[fromChat.uid][fromChat.gid]["__DefaultDice"] = intDefaultDice;
			replyMsg("strSetDefaultDice");
			return 1;
		}
		if (intDefaultDice == 100)
			getUser(fromChat.uid).rmConf("默认骰");
		else
			getUser(fromChat.uid).setConf("默认骰", intDefaultDice);
		replyMsg("strSetDefaultDice");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str" && trusted > 3) {
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
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ak") {
	intMsgCnt += 2;
	readSkipSpace();
	if (intMsgCnt == strMsg.length()) {
		reply(fmt->get_help("ak"));
		return 1;
	}
	Session& room{ gm->session(fromSession) };
	char sign{ strMsg[intMsgCnt] };
	string action{ readPara() };
	if (sign == '#' || action == "new") {
		if (sign == '#')++intMsgCnt;
		room.get_deck().erase("__Ank");
		if (string strTitle{ strMsg.substr(intMsgCnt,strMsg.find('+') - intMsgCnt) }; !strTitle.empty()) {
			intMsgCnt += strTitle.length();
			room.setConf("AkFork", vars["fork"] = strTitle);
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
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		if (!readItems(deck)) {
			replyMsg("strAkAddEmpty");
			return 1;
		}
		vars["fork"] = room.conf["AkFork"].to_str();
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		replyMsg("strAkAdd");
		room.save();
	}
	else if (sign == '-' || action == "del") {
		if (sign == '-')++intMsgCnt;
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		int nNo{ 0 };
		if (readNum(nNo) || nNo <= 0 || nNo > deck.size()) {
			replyMsg("strAkNumErr");
			return 1;
		}
		deck.erase(deck.begin() + nNo - 1);
		vars["fork"] = room.conf["AkFork"];
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		replyMsg("strAkDel");
		room.save();
	}
	else if (sign == '=' || action == "get") {
		if (DeckInfo& deck{ room.get_deck("__Ank") }; !deck.meta.empty()) {
			vars["fork"] = room.conf["AkFork"];
			room.rmConf("AkFork");
			size_t res{ (size_t)RandomGenerator::Randint(0,deck.meta.size() - 1) };
			vars["get"] = to_string(res + 1) + ". " + deck.meta[res];
			ResList list;
			list.order();
			for (auto& val : deck.meta) {
				list << val;
			}
			vars["li"] = list.linebreak().show();
			room.get_deck().erase("__Ank");
			replyMsg("strAkGet");
		}
		else {
			replyMsg("strAkOptEmptyErr");
		}
		room.save();
	}
	else if (action == "show") {
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		vars["fork"] = room.conf["AkFork"];
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		replyMsg("strAkShow");
	}
	else if (action == "clr") {
		room.get_deck().erase("__Ank");
		vars["fork"] = room.conf["AkFork"];
		room.rmConf("AkFork");
		replyMsg("strAkClr");
		room.save();
	}
	if(strReply.empty())reply(fmt->get_help("ak"));
	return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en") {
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (strMsg.length() == intMsgCnt) {
		reply(fmt->get_help("en"));
		return 1;
	}
	string& strAttr{ (vars["attr"] = readAttrName()).text };
	string strCurrentValue{ readDigit(false) };
	CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
	int intVal{ 0 };
		//获取技能原值
		if (strCurrentValue.empty()) {
			if (pc && !strAttr.empty() && (pc->stored(strAttr))) {
				intVal = getPlayer(fromChat.uid)[fromChat.gid][strAttr].to_int();
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
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//成长检定仅计入掷骰统计，不计入检定统计
		if (pc)pc->cntRollStat(intTmpRollRes, 100);
		string& res{ (vars["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intVal) + " ").text };
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
			vars["change"] = rdEnFail.FormCompleteString();
			vars["final"] = to_string(intVal);
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
			vars["change"] = rdEnSuc.FormCompleteString();
			vars["final"] = to_string(intVal);
			replyMsg("strEnRollSuccess");
		}
		if (pc)pc->set(strAttr, intVal);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li") {
		LongInsane(*vars);
		replyMsg("strLongInsane");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me") {
		if (trusted < 4 && console["DisabledMe"]) {
			replyMsg("strDisabledMeGlobal");
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (isPrivate()) {
			string strGroupID = readDigit();
			if (strGroupID.empty()) {
				replyMsg("strGroupIDEmpty");
				return 1;
			}
			const long long llGroupID = stoll(strGroupID);
			if (groupset(llGroupID, "停用指令") && trusted < 4) {
				replyMsg("strDisabledErr");
				return 1;
			}
			if (groupset(llGroupID, "禁用me") && trusted < 5) {
				replyMsg("strMEDisabledErr");
				return 1;
			}
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strAction = strip(readRest());
			if (strAction.empty()) {
				replyMsg("strActionEmpty");
				return 1;
			}
			string strReply = (trusted > 4 ? getName(fromChat.uid, llGroupID) : "") + strAction;
			DD::sendGroupMsg(llGroupID, strReply);
			replyMsg("strSendSuccess");
			return 1;
		}
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (!canRoomHost() && (strAction == "on" || strAction == "off")) {
			replyMsg("strPermissionDeniedErr");
			return 1;
		}
		if (strAction == "off") {
			if (groupset(fromChat.gid, "禁用me") < 1) {
				chat(fromChat.gid).set("禁用me");
				replyMsg("strMeOff");
			}
			else {
				replyMsg("strMeOffAlready");
			}
			return 1;
		}
		if (strAction == "on") {
			if (groupset(fromChat.gid, "禁用me") > 0) {
				chat(fromChat.gid).reset("禁用me");
				replyMsg("strMeOn");
			}
			else {
				replyMsg("strMeOnAlready");
			}
			return 1;
		}
		if (groupset(fromChat.gid, "禁用me")) {
			replyMsg("strMEDisabledErr");
			return 1;
		}
		strAction = strip(readRest());
		if (strAction.empty()) {
			replyMsg("strActionEmpty");
			return 1;
		}
		trusted > 4 ? reply(strAction, false) : reply(idx_pc(vars).to_str() + strAction, false);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		vars["old_nick"] = idx_nick(vars);
		string& strNN{ (vars["new_nick"] = strip(filter_CQcode(strMsg.substr(intMsgCnt),fromChat.gid))).text };
		if (strNN.length() > 50) {
			replyMsg("strNameTooLongErr");
			return 1;
		}
		if (!strNN.empty()) {
			getUser(fromChat.uid).setNick(fromChat.gid, strNN);
			replyMsg("strNameSet");
		}
		else {
			if (getUser(fromChat.uid).rmNick(fromChat.gid)) {
				replyMsg("strNameClr");
			}
			else {
				replyMsg("strNameDelEmpty");
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob") {
		if (isPrivate()) {
			reply(fmt->get_help("ob"));
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string strOption = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);

		if (!canRoomHost() && (strOption == "on" || strOption == "off")) {
			replyMsg("strPermissionDeniedErr");
			return 1;
		}
		vars["option"] = "禁用ob";
		if (strOption == "off") {
			if (groupset(fromChat.gid, vars["option"].to_str()) < 1) {
				chat(fromChat.gid).set(vars["option"].to_str());
				gm->session(fromSession).clear_ob();
				replyMsg("strObOff");
			}
			else {
				replyMsg("strObOffAlready");
			}
			return 1;
		}
		if (strOption == "on") {
			if (groupset(fromChat.gid, vars["option"].to_str()) > 0) {
				chat(fromChat.gid).reset(vars["option"].to_str());
				replyMsg("strObOn");
			}
			else {
				replyMsg("strObOnAlready");
			}
			return 1;
		}
		if (groupset(fromChat.gid, vars["option"].to_str()) > 0) {
			replyMsg("strObOffAlready");
			return 1;
		}
		if (strOption == "list") {
			gm->session(fromSession).ob_list(this);
		}
		else if (strOption == "clr") {
			if (canRoomHost()) {
				gm->session(fromSession).ob_clr(this);
			}
			else {
				replyMsg("strPermissionDeniedErr");
			}
		}
		else if (strOption == "exit") {
			gm->session(fromSession).ob_exit(this);
		}
		else {
			gm->session(fromSession).ob_enter(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "pc") {
		intMsgCnt += 2;
		string strOption = readPara();
		if (strOption.empty()) {
			reply(fmt->get_help("pc"));
			return 1;
		}
		Player& pl = getPlayer(fromChat.uid);
		if (strOption == "tag") {
			vars["char"] = readRest();
			switch (pl.changeCard(vars["char"].to_str(), fromChat.gid)) {
			case 1:
				replyMsg("strPcCardReset");
				break;
			case 0:
				replyMsg("strPcCardSet");
				break;
			case -5:
				replyMsg("strPcNameNotExist");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "show") {
			string strName = readRest();
			CharaCard& pc{ pl.getCard(strName, fromChat.gid) };
			vars["char"] = pc.getName();
			vars["type"] = pc.Attr["__Type"].to_str();
			vars["show"] = pc.show(true);
			replyMsg("strPcCardShow");
			return 1;
		}
		if (strOption == "new") {
			string& strPC{ (vars["char"] = strip(filter_CQcode(readRest(), fromChat.gid))).text };
			switch (pl.newCard(strPC, fromChat.gid)) {
			case 0:
				vars["type"] = pl[fromChat.gid].Attr["__Type"].to_str();
				vars["show"] = pl[fromChat.gid].show(true);
				if (vars["show"].str_empty())replyMsg("strPcNewEmptyCard");
				else replyMsg("strPcNewCardShow");
				break;
			case -1:
				replyMsg("strPcCardFull");
				break;
			case -4:
				replyMsg("strPcNameExist");
				break;
			case -6:
				replyMsg("strPcNameInvalid");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "build") {
			string& strPC{ (vars["char"] = strip(filter_CQcode(readRest(), fromChat.gid))).text };
			switch (pl.buildCard(strPC, false, fromChat.gid)) {
			case 0:
				vars["show"] = pl[strPC].show(true);
				replyMsg("strPcCardBuild");
				break;
			case -1:
				replyMsg("strPcCardFull");
				break;
			case -2:
				replyMsg("strPcTempInvalid");
				break;
			case -6:
				replyMsg("strPCNameInvalid");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "list") {
			vars["show"] = pl.listCard();
			replyMsg("strPcCardList");
			return 1;
		}
		if (strOption == "nn") {
			string& strPC{ (vars["new_name"] = strip(filter_CQcode(readRest(),fromChat.gid))).text };
			vars["old_name"] = pl[fromChat.gid].getName();
			switch (pl.renameCard(vars["old_name"].to_str(), strPC)) {
			case 0:
				replyMsg("strPcCardRename");
				break;
			case -3:
				replyMsg("strPCNameEmpty");
				break;
			case -4:
				replyMsg("strPCNameExist");
				break;
			case -6:
				replyMsg("strPCNameInvalid");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "del") {
			vars["char"] = strip(readRest());
			switch (pl.removeCard(vars["char"].to_str())) {
			case 0:
				replyMsg("strPcCardDel");
				break;
			case -5:
				replyMsg("strPcNameNotExist");
				break;
			case -7:
				replyMsg("strPcInitDelErr");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "redo") {
			vars["char"] = strip(readRest());
			pl.buildCard(vars["char"].text, true, fromChat.gid);
			vars["show"] = pl[vars["char"].to_str()].show(true);
			replyMsg("strPcCardRedo");
			return 1;
		}
		if (strOption == "grp") {
			vars["show"] = pl.listMap();
			replyMsg("strPcGroupList");
			return 1;
		}
		if (strOption == "cpy") {
			string strName = strip(filter_CQcode(readRest(), fromChat.gid));
			string& strPC1{ (vars["char1"] = strName.substr(0, strName.find('='))).text };
			vars["char2"] = (strPC1.length() + 1 < strName.length())
				? strip(strName.substr(strPC1.length() + 1))
				: pl[fromChat.gid].getName();
			switch (pl.copyCard(strPC1, vars["char2"].to_str(), fromChat.gid)) {
			case 0:
				replyMsg("strPcCardCpy");
				break;
			case -1:
				replyMsg("strPcCardFull");
				break;
			case -3:
				replyMsg("strPcNameEmpty");
				break;
			case -6:
				replyMsg("strPcNameInvalid");
				break;
			default:
				replyMsg("strUnknownErr");
				break;
			}
			return 1;
		}
		if (strOption == "stat") {
			CharaCard& pc{ pl[fromChat.gid] };
			bool isEmpty{ true };
			ResList res;
			int intFace{ pc.count("__DefaultDice")
				? pc.call(string("__DefaultDice"))
				: getUser(fromChat.uid).getConf("默认骰",100) };
			string strFace{ to_string(intFace) };
			string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//掷骰次数
			if (intFace <= 100 && pc.count(keyStatCnt)) {
				int cntRoll{ pc[keyStatCnt].to_int() };	
				if (cntRoll > 0) {
					isEmpty = false;
					res << "D" + strFace + "统计次数: " + to_string(cntRoll);
					int sumRes{ pc["__StatD" + strFace + "Sum"].to_int() };		//点数和
					int sumResSqr{ pc["__StatD" + strFace + "SqrSum"].to_int() };	//点数平方和
					DiceEst stat{ intFace,cntRoll,sumRes,sumResSqr };
					if (stat.estMean > 0)
						res << "均值: " + toString(stat.estMean, 2, true) + " [" + toString(stat.expMean) + "]";
					if (stat.pNormDist) {
						if (stat.pNormDist < 0.5)res << "均值低于" + toString(100 - stat.pNormDist * 100, 2) + "%的用户";
						else res << "均值高于" + toString(stat.pNormDist * 100, 2) + "%的用户";
					}
					if (stat.estStd > 0) {
						res << "标准差: " + toString(stat.estStd, 2) + " [" + toString(stat.expStd) + "]";
					}
					/*if (stat.pZtest > 0) {
						res << "Z检验“偏心”水平: " + toString(stat.pZtest * 100, 2) + "%";
					}*/
				}
			}
			string keyRcCnt{ "__StatRcCnt" };	//rc/sc检定次数
			if (pc.count(keyRcCnt)) {
				int cntRc{ pc["__StatRcCnt"].to_int() };
				if (cntRc > 0) {
					isEmpty = false;
					res << "检定统计次数: " + to_string(cntRc);
					int sumRcSuc{ pc["__StatRcSumSuc"].to_int() };//实际成功数
					int sumRcRate{ pc["__StatRcSumRate"].to_int() };//总成功率
					res << "检定[期望]成功率: " + toString((double)sumRcSuc / cntRc * 100) + "%" + "(" + to_string(sumRcSuc) + ") [" + toString((double)sumRcRate / cntRc) + "%]";
					if (pc.count("__StatRcCnt5") || pc.count("__StatRcCnt96"))
						res << "5- | 96+ 出现率: " + toString((double)pc["__StatRcCnt5"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt5"].to_str() + ") | " + toString((double)pc["__StatRcCnt96"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt96"].to_str() + ")";
					if(pc.count("__StatRcCnt1")|| pc.count("__StatRcCnt100"))
						res << "1 | 100 出现次数: " + to_string(pc["__StatRcCnt1"].to_int()) + " | " + to_string(pc["__StatRcCnt100"].to_int());
				}
			}
			if (isEmpty) {
				replyMsg("strPcStatEmpty");
			}
			else {
				vars["stat"] = res.show();
				replyMsg("strPcStatShow");
			}
			return 1;
		}
		if (strOption == "clr") {
			PList.erase(fromChat.uid);
			replyMsg("strPcClr");
			return 1;
		}
		if (strOption == "type") {
			vars["new_type"] = strip(readRest());
			if (vars["new_type"].str_empty()) {
				vars["attr"] = "模板类";
				vars["val"] = pl[fromChat.gid].Attr["__Type"].to_str();
				replyMsg("strProp");
			}
			else {
				pl[fromChat.gid].setType(vars["new_type"].to_str());
				replyMsg("strSetPropSuccess");
			}
			return 1;
		}
		if (strOption == "temp") {
			CardTemp& temp{ *pl[fromChat.gid].pTemplet};
			reply(temp.show());
			return 1;
		}
		reply(fmt->get_help("pc"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra" || strLowerMessage.substr(intMsgCnt, 2) == "rc") {
		intMsgCnt += 2;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rc"));
			return 1;
		}
		int intRule = isPrivate()
			? getUser(fromChat.uid).getConf("rc房规")
			: chat(fromChat.gid).getConf("rc房规");
		int intTurnCnt = 1;
		bool isHidden(false);
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			isHidden = true;
			++intMsgCnt;
		}
		else if (readSkipSpace(); strMsg[intMsgCnt] == '_') {
			isHidden = true;
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
		CharaCard* pc{ isStatic ? &PList[fromChat.uid][fromChat.gid] : nullptr };
		if ((strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') && strLowerMessage[intMsgCnt - 1] != ' ') {
			isStatic = false;
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		readSkipSpace();
		if (strMsg[intMsgCnt] == '_') {
			isHidden = true;
			++intMsgCnt;
		}
		if (strMsg.length() == intMsgCnt) {
			vars["attr"] = getMsg("strEnDefaultName");
			replyMsg("strUnknownPropErr");
			return 1;
		}
		string attr{ strMsg.substr(intMsgCnt) };
		vars["attr"] = attr;
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
		if (pc && pc->count(attr))intMsgCnt = strMsg.length();
		else {
			vars["attr"] = attr = readAttrName();
		}
		if (strLowerMessage[intMsgCnt] == '*' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
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
			   strLowerMessage[intMsgCnt] ==
			   ':')
			intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			intMsgCnt++;
		}
		vars["reason"] = readRest();
		int intSkillVal;
		if (strSkillVal.empty()) {
			if (pc && pc->count(attr)) {
				intSkillVal = PList[fromChat.uid][fromChat.gid].call(attr);
			}
			else {
				if (!pc && SkillNameReplace.count(attr)) {
					vars["attr"] = SkillNameReplace[attr];
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
		vars["attr"] = strDifficulty + attr + (
			(intSkillMultiple != 1) ? "×" + to_string(intSkillMultiple) : "") + strSkillModify + ((intSkillDivisor != 1)
																								  ? "/" + to_string(
																									  intSkillDivisor)
																								  : "");				 
		if (vars["reason"].str_empty()) {
			strReply = getMsg("strRollSkill", vars);
		}
		else strReply = getMsg("strRollSkillReason", vars);
		ResList Res;
		string strAns;
		if (intTurnCnt == 1) {
			rdMainDice.Roll();
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
				rdMainDice.Roll();
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
		if (isHidden) {
			replyHidden();
			replyMsg("strRollSkillHidden");
		}
		else
			reply();
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri") {
		if (isPrivate()) {
			reply(fmt->get_help("ri"));
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
		readSkipSpace();
		vars["char"] = strip(strMsg.substr(intMsgCnt));
		if (vars["char"].str_empty()) {
			vars["char"] = idx_pc(vars).to_str();
		}
		RD initdice(strinit, 20);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err) {
			replyMsg("strValueErr");
			return 1;
		}
		if (intFirstTimeRes == Input_Err) {
			replyMsg("strInputErr");
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err) {
			replyMsg("strZeroDiceErr");
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err) {
			replyMsg("strZeroTypeErr");
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			replyMsg("strDiceTooBigErr");
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err) {
			replyMsg("strTypeTooBigErr");
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err) {
			replyMsg("strAddDiceValErr");
			return 1;
		}
		if (intFirstTimeRes != 0) {
			replyMsg("strUnknownErr");
			return 1;
		}
		gm->session(fromSession).table_add("先攻", initdice.intTotal, vars["char"].to_str());
		vars["res"] = initdice.FormCompleteString();
		replyMsg("strRollInit");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc") {
		intMsgCnt += 2;
		string SanCost = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (SanCost.empty()) {
			reply(fmt->get_help("sc"));
			return 1;
		}
		if (SanCost.find('/') == string::npos) {
			replyMsg("strSanCostInvalid");
			return 1;
		}
		string attr = "理智";
		int intSan = 0;
		CharaCard* pc{ nullptr };
		if (readNum(intSan)) {
			if (PList.count(fromChat.uid)
				&& (pc = &getPlayer(fromChat.uid)[fromChat.gid])->count(attr)) {
				intSan = pc->call(attr);
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
		for (const auto& character : SanCost.substr(SanCost.find('/') + 1)) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				replyMsg("strSanCostInvalid");
				return 1;
			}
		}
		RD rdSuc(strSanCostSuc);
		RD rdFail(strSanCostFail);
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0) {
			replyMsg("strSanCostInvalid");
			return 1;
		}
		if (intSan <= 0) {
			replyMsg("strSanInvalid");
			return 1;
		}
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//理智检定计入统计
		if (pc) {
			pc->cntRollStat(intTmpRollRes, 100);
			pc->cntRcStat(intTmpRollRes, intSan);
		}
		string& strRes{ (vars["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intSan) + " ").text };
		//调用房规
		int intRule = fromChat.gid
			? chat(fromChat.gid).getConf("rc房规")
			: getUser(fromChat.uid).getConf("rc房规");
		switch (RollSuccessLevel(intTmpRollRes, intSan, intRule)) {
		case 5:
		case 4:
		case 3:
		case 2:
			strRes += getMsg("strSuccess");
			vars["change"] = rdSuc.FormCompleteString();
			intSan = max(0, intSan - rdSuc.intTotal);
			break;
		case 1:
			strRes += getMsg("strFailure");
			vars["change"] = rdFail.FormCompleteString();
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		case 0:
			strRes += getMsg("strFumble");
			rdFail.Max();
			vars["change"] = rdFail.strDice + "最大值=" + to_string(rdFail.intTotal);
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		}
		vars["final"] = to_string(intSan);
		if (pc)pc->set(attr, intSan);
		replyMsg("strSanRollRes");
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length()) {
			reply(fmt->get_help("st"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr") {
			if (!PList.count(fromChat.uid)) {
				replyMsg("strPcNotExistErr");
				return 1;
			}
			getPlayer(fromChat.uid)[fromChat.gid].clear();
			vars["char"] = getPlayer(fromChat.uid)[fromChat.gid].getName();
			replyMsg("strPropCleared");
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
			if (strMsg[intMsgCnt] == '&') {
				intMsgCnt++;
			}
			vars["attr"] = readAttrName();
			if (getPlayer(fromChat.uid)[fromChat.gid].erase(vars["attr"].text)) {
				replyMsg("strPropDeleted");
			}
			else {
				replyMsg("strPropNotFound");
			}
			return 1;
		}
		CharaCard& pc = getPlayer(fromChat.uid)[fromChat.gid];
		if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			vars["attr"] = readAttrName();
			if (vars["attr"].str_empty()) {
				vars["char"] = pc.getName();
				vars["type"] = pc.Attr["__Type"].to_str();
				vars["show"] = pc.show(false);
				replyMsg("strPropList");
				return 1;
			}
			if (string val; pc.show(vars["attr"].to_str(), val) > -1) {
				vars["val"] = val;
				replyMsg("strProp");
			}
			else {
				replyMsg("strPropNotFound");
			}
			return 1;
		}
		bool boolError = false;
		bool isDetail = false;
		bool isModify = false;
		//循环录入
		int cntInput{ 0 };
		while (intMsgCnt != strLowerMessage.length()) {
			readSkipSpace();
			//判定录入表达式
			if (strMsg[intMsgCnt] == '&') {
				vars["attr"] = readToColon(); 
				if (vars["attr"].str_empty()) {
					continue;
				}
				if (pc.set(vars["attr"].to_str(), readExp())) {
					replyMsg("strPcTextTooLong");
					return 1;
				}
				++cntInput;
				continue;
			}
			//读取属性名
			string strSkillName = readAttrName();
			if (strSkillName.empty()) {
				readSkipSpace();
				while (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '+' ||
			           strMsg[intMsgCnt] == '-' || strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/')
				{
					intMsgCnt++;
				}
				readDigit(false);
				continue;
			}
			strSkillName = pc.standard(strSkillName);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
				'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			//判定所录入为文本
			if (bool isSqr{ strMsg.substr(intMsgCnt, 2) == "【" }; pc.pTemplet->sInfoList.count(strSkillName) || isSqr) {
				string strVal;
				if (auto pos{ strMsg.find("】",intMsgCnt) }; pos != string::npos) {
					strVal = strMsg.substr(intMsgCnt + 2, pos - intMsgCnt - 2);
					intMsgCnt = pos + 2;
				}
				else {
					strVal = readUntilTab();
				}
				if (pc.set(strSkillName, strVal)) {
					replyMsg("strPcTextTooLong");
					return 1;
				}
				++cntInput;
				continue;
			}
			if (strSkillName == "note") {
				if (pc.setNote(readRest())) {
					replyMsg("strPcNoteTooLong");
					return 1;
				}
				++cntInput;
				break;
			}
			readSkipSpace();
			//判定数值修改
			if ((strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+')) {
				isDetail = true;
				isModify = true;
				AttrVar& nVal{ pc[strSkillName] };
				RD Mod((nVal.to_int() == 0 ? "" : nVal.to_str()) + readDice());
				if (Mod.Roll()) {
					replyMsg("strValueErr");
					return 1;
				}
				strReply += "\n" + strSkillName + "：" + Mod.FormCompleteString();
				if (Mod.intTotal < -32767) {
					strReply += "→-32767";
					nVal = -32767;
				}
				else if (Mod.intTotal > 32767) {
					strReply += "→32767";
					nVal = 32767;
				}
				else nVal = Mod.intTotal;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					   '|')intMsgCnt++;
				++cntInput;
				continue;
			}
			string strSkillVal = readDigit();
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 5) {
				boolError = true;
				break;
			}
			int intSkillVal = std::clamp(stoi(strSkillVal), -32767, 32767);
			//录入纯数值
			pc.set(strSkillName, intSkillVal);
			++cntInput;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
				intMsgCnt++;
		}
		if (boolError) {
			replyMsg("strPropErr");
		}
		else if (isModify) {
			vars["change"] = strReply;
			replyMsg("strStModify");
		}
		else if(cntInput){
			vars["cnt"] = to_string(cntInput);
			replyMsg("strSetPropSuccess");
		}
		else {
			reply(fmt->get_help("st"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti") {
		TempInsane(*vars);
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
			reply(fmt->get_help("ww"));
			return 1;
		}
		if (!fromChat.gid)isHidden = false;
		CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
		string strMainDice;
		string& strReason{ (vars["reason"] = "").text };
		string strAttr;
		if (pc) {	//调用角色卡属性或表达式
			while (intMsgCnt < len && !isspace(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
				if (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))
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
					if (pc->count(strAttr)) {
						strMainDice += pc->getExp(strAttr);
						if (!pc->count("&" + strAttr) && (*pc)[strAttr].type == AttrVar::AttrType::Integer)strMainDice += 'a';
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
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes != 0) {
				if (intRdTurnCntRes == Value_Err) {
					replyMsg("strValueErr");
					return 1;
				}
				if (intRdTurnCntRes == Input_Err) {
					replyMsg("strInputErr");
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err) {
					replyMsg("strZeroDiceErr");
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err) {
					replyMsg("strZeroTypeErr");
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err) {
					replyMsg("strDiceTooBigErr");
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err) {
					replyMsg("strTypeTooBigErr");
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err) {
					replyMsg("strAddDiceValErr");
					return 1;
				}
				replyMsg("strUnknownErr");
				return 1;
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
				vars["turn"] = rdTurnCnt.FormShortString();
				replyHidden(getMsg("strRollTurn", vars));
			}
		}
		if (strMainDice.empty()) {
			reply(fmt->get_help("ww"));
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

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes != 0) {
			if (intFirstTimeRes == Value_Err) {
				replyMsg("strValueErr");
				return 1;
			}
			if (intFirstTimeRes == Input_Err) {
				replyMsg("strInputErr");
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err) {
				replyMsg("strZeroDiceErr");
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err) {
				replyMsg("strZeroTypeErr");
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err) {
				replyMsg("strDiceTooBigErr");
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err) {
				replyMsg("strTypeTooBigErr");
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err) {
				replyMsg("strAddDiceValErr");
				return 1;
			}
			replyMsg("strUnknownErr");
			return 1;
		}
		if (!boolDetail && intTurnCnt != 1) {
			if (strReason.empty())strReply = getMsg("strRollMuiltDice");
			else strReply = getMsg("strRollMuiltDiceReason");
			vector<int> vintExVal;
			string& strRes{ (vars["res"] = "{ ").text };
			while (intTurnCnt--) {
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strRes += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					vars["res"] = ",";
			}
			strRes += " }";
			if (!vintExVal.empty()) {
				strRes += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					strRes += to_string(*it);
					if (it != vintExVal.cend() - 1)strRes += ",";
				}
			}
			if (!isHidden) {
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
				vars["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
				if (strReason.empty())
					strReply = getMsg("strRollDice");
				else strReply = getMsg("strRollDiceReason");
				if (!isHidden) {
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
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (strMsg[intMsgCnt] == 's') {
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h') {
			isHidden = true;
			intMsgCnt += 1;
		}
		if (!fromChat.gid)isHidden = false;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
		string& strReason{ (vars["reason"] = strMsg.substr(intMsgCnt)).text };
		if (strReason.empty()) {
			string key{ "__DefaultDiceExp" };
			if (pc && pc->countExp(key)) {
				strMainDice = pc->getExp(key);
			}
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
			if (isExp)vars["reason"] = readRest();
			else strMainDice.clear();
		}
		int intTurnCnt = 1;
		const int intDefaultDice = (pc && pc->count("__DefaultDice")) 
			? (*pc)["__DefaultDice"].to_int()
			: getUser(fromChat.uid).getConf("默认骰", 100);
		if (strMainDice.find('#') != string::npos) {
			string& turn{ (vars["turn"] = strMainDice.substr(0, strMainDice.find('#'))).text };
			if (turn.empty())
				turn = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(turn, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			switch (intRdTurnCntRes) {
			case 0: break;
			case Value_Err:
				replyMsg("strValueErr");
				return 1;
			case Input_Err:
				replyMsg("strInputErr");
				return 1;
			case ZeroDice_Err:
				replyMsg("strZeroDiceErr");
				return 1;
			case ZeroType_Err:
				replyMsg("strZeroTypeErr");
				return 1;
			case DiceTooBig_Err:
				replyMsg("strDiceTooBigErr");
				return 1;
			case TypeTooBig_Err:
				replyMsg("strTypeTooBigErr");
				return 1;
			case AddDiceVal_Err:
				replyMsg("strAddDiceValErr");
				return 1;
			default:
				replyMsg("strUnknownErr");
				return 1;
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
				if (!isHidden) {
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
		const int intFirstTimeRes = rdMainDice.Roll();
		switch (intFirstTimeRes) {
		case 0: break;
		case Value_Err:
			replyMsg("strValueErr");
			return 1;
		case Input_Err:
			replyMsg("strInputErr");
			return 1;
		case ZeroDice_Err:
			replyMsg("strZeroDiceErr");
			return 1;
		case ZeroType_Err:
			replyMsg("strZeroTypeErr");
			return 1;
		case DiceTooBig_Err:
			replyMsg("strDiceTooBigErr");
			return 1;
		case TypeTooBig_Err:
			replyMsg("strTypeTooBigErr");
			return 1;
		case AddDiceVal_Err:
			replyMsg("strAddDiceValErr");
			return 1;
		default:
			replyMsg("strUnknownErr");
			return 1;
		}
		vars["dice_exp"] = rdMainDice.strDice;
		//仅统计与默认骰一致的掷骰
		bool isStatic{ intDefaultDice <= 100 && pc && rdMainDice.strDice == ("D" + to_string(intDefaultDice)) };
		string strType = (intTurnCnt != 1
						  ? (vars["reason"].str_empty() ? "strRollMultiDice" : "strRollMultiDiceReason")
						  : (vars["reason"].str_empty() ? "strRollDice" : "strRollDiceReason"));
		if (!boolDetail && intTurnCnt != 1) {
			vector<int> vintExVal;
			string& res{ (vars["res"] = "{ ").text };
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
			if (!isHidden) {
				replyMsg(strType);
			}
			else {
				replyHidden(getMsg(strType, vars));
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
				vars["res"] = dices.dot(", ").line(7).show();
			}
			else {
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				vars["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
			}
			if (!isHidden) {
				replyMsg(strType);
			}
			else {
				replyHidden(getMsg(strType,vars));
			}
		}
		if (isHidden) {
			replyMsg("strRollHidden");
		}
		return 1;
	}
	return 0;
}

//判断是否响应
bool FromMsg::DiceFilter()
{
	while (isspace(static_cast<unsigned char>(strMsg[0])))
		strMsg.erase(strMsg.begin());
	init(strMsg);
	bool isOtherCalled = false;
	bool isSummoned{ false };
	string strAt{ CQ_AT + to_string(console.DiceMaid) + "]" };
	size_t r{ 0 };
	while ((r = strMsg.find(']')) != string::npos && strMsg.find(CQ_AT) == 0 || strMsg.find(CQ_QQAT) == 0)
	{
		if (string strTarget{ strMsg.substr(10,r - 10) }; strTarget == "all") {
			isCalled = true;
		}
		else if (strTarget == to_string(console.DiceMaid))
		{
			isCalled = true;
		}
		else if (User& self{ getUser(console.DiceMaid) }; self.confs.count("tinyID") && self.confs["tinyID"] == strTarget)
		{
			isCalled = true;
		}
		else {
			isOtherCalled = true;
		}
		strMsg = strMsg.substr(r + 1);
		while (isspace(static_cast<unsigned char>(strMsg[0])))
			strMsg.erase(strMsg.begin());
	}
	string strSummon{ getMsg("strSummonWord") };
	if (!strSummon.empty() && strMsg.find(strSummon) == 0) {
		isSummoned = true;
		if(isChannel())strMsg = strMsg.substr(strSummon.length());
	}
	init2(strMsg);
	strLowerMessage = strMsg;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(),
				   [](unsigned char c) { return tolower(c); });
	trusted = trustedQQ(fromChat.uid);
	fwdMsg();
	if (isOtherCalled && !isCalled)return false;
	if (isPrivate()) isCalled = true;
	isDisabled = ((console["DisabledGlobal"] && trusted < 4) || groupset(fromChat.gid, "协议无效") > 0);
	if (BasicOrder()) 
	{
		if (isAns) {
			if (!isVirtual) {
				AddFrq(fromChat, fromTime, strMsg);
				getUser(fromChat.uid).update(fromTime);
				if (!isPrivate())chat(fromChat.gid).update(fromTime);
			}
		}
		return 1;
	}
	if (!isPrivate() && ((console["CheckGroupLicense"] > 0 && pGrp->isset("未审核"))
											  || (console["CheckGroupLicense"] == 2 && !pGrp->isset("许可使用")) 
											  || blacklist->get_group_danger(fromChat.gid))) {
		isDisabled = true;
	}
	if (blacklist->get_qq_danger(fromChat.uid))isDisabled = true;
	if (isDisabled)return console["DisabledBlock"];
	if (int chon{ isChannel() && pGrp ? pGrp->getChConf(fromChat.chid,"order",0):0 }; isCalled || chon > 0 ||
		!(chon || pGrp->isset("停用指令"))) {
		if (fmt->listen_order(this) || InnerOrder()) {
			if (!isVirtual && !vars["ignored"]) {
				AddFrq(fromChat, fromTime,  strMsg);
				getUser(fromChat.uid).update(fromTime);
				if (!isPrivate())chat(fromChat.gid).update(fromTime);
			}
			return true;
		}
	}
	if (fmt->listen_reply(this)) {
		if (!isVirtual && !vars["ignored"]) {
			AddFrq(fromChat, fromTime, strMsg);
			getUser(fromChat.uid).update(fromTime);
			if (!isPrivate())chat(fromChat.gid).update(fromTime);
		}
		return true;
	}
	if (isSummoned && (strMsg.empty() || strMsg == strSummon))replyMsg("strSummonEmpty");
	if (isCalled && WordCensor()) {
		return true;
	}
	return false;
}
bool FromMsg::WordCensor() {
	//信任小于4的用户进行敏感词检测
	if (trusted < 4) {
		vector<string>sens_words;
		switch (int danger = censor.search(strMsg, sens_words) - 1) {
		case 3:
			if (trusted < danger++) {
				console.log("警告:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName") + "发送了含敏感词消息:\n" + strMsg, 0b1000,
							printTTime(fromTime));
				replyMsg("strCensorDanger");
				return 1;
			}
		case 2:
			if (trusted < danger++) {
				console.log("警告:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
					+ "发送了含敏感词消息(" + listItem(sens_words) + "):\n" + strMsg, 0b10, printTTime(fromTime));
				replyMsg("strCensorWarning");
				break;
			}
		case 1:
			if (trusted < danger++) {
				console.log("提醒:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
					+ "发送了含敏感词消息("	+ listItem(sens_words) + "):\n" + strMsg, 0b10,
							printTTime(fromTime));
				replyMsg("strCensorCaution");
				break;
			}
		case 0:
			console.log("提醒:" + printUser(fromChat.uid) + "对" + getMsg("strSelfName")
				+ "发送了含敏感词消息(" + listItem(sens_words) +"):\n" + strMsg, 1,
						printTTime(fromTime));
			break;
		default:
			break;
		}
	}
	return false;
}

void FromMsg::virtualCall() {
	isVirtual = true;
	isCalled = true;
	DiceFilter();
}
bool FromMsg::canRoomHost() {
	if (!vars.has("canRoomHost")) {
		return bool(vars["canRoomHost"] = trusted > 3
			|| isChannel() || isPrivate()
			|| DD::isGroupAdmin(fromChat.gid, fromChat.uid, true) || pGrp->inviter == fromChat.uid);
	}
	return bool(vars["canRoomHost"]);
}

int FromMsg::getGroupAuth(long long group) {
	if (trusted > 0)return trusted;
	if (ChatList.count(group)) {
		return DD::isGroupAdmin(group, fromChat.uid, true) ? 0 : -1;
	}
	return -2;
}
void FromMsg::readSkipColon() {
	readSkipSpace();
	while (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '='))intMsgCnt++;
}

int FromMsg::readNum(int& num)
{
	string strNum;
	while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])) && strMsg[intMsgCnt] != '-')intMsgCnt++;
	if (strMsg[intMsgCnt] == '-')
	{
		strNum += '-';
		intMsgCnt++;
	}
	if (intMsgCnt >= strMsg.length())return -1;
	while (intMsgCnt < strMsg.length() && isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
	{
		strNum += strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 9)return -2;
	if (strNum.empty() || strNum == "-")return -3;
	num = stoi(strNum);
	return 0;
}

int FromMsg::readChat(chatInfo& ct, bool isReroll)
{
	const int intFormor = intMsgCnt;
	if (const string strT = readPara(); strT == "me")
	{
		ct = { fromChat.uid, 0,0 };
		return 0;
	}
	else if (strT == "this")
	{
		ct = fromChat.gid ? chatInfo(0, fromChat.gid, fromChat.chid) : fromChat;
		return 0;
	}
	else if (const long long llID{ readID() }; !llID) {
		if (isReroll)intMsgCnt = intFormor;
		return -2;
	}
	else if (strT == "qq") 
	{
		ct.type = msgtype::Private;
		ct.uid = llID;
		return 0;
	}
	else if (strT == "group")
	{
		ct.type = msgtype::Group;
		ct.gid = llID;
		return 0;
	}
	if (isReroll)intMsgCnt = intFormor;
	return -1;
}

string FromMsg::readItem()
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
int FromMsg::readItems(vector<string>& vItem) {
	int cnt{ 0 };
	string strItem;
	while (intMsgCnt < strMsg.length() && !(strItem = readItem()).empty()) {
		vItem.push_back(strItem);
		++cnt;
	}
	return cnt;
}

std::string FromMsg::printFrom()
{
	std::string strFwd;
	if (!isPrivate())strFwd += isChannel()
		? ("[频道:" + to_string(fromChat.gid) + "]")
		: ("[群:" + to_string(fromChat.gid) + "]");
	strFwd += getName(fromChat.uid, fromChat.gid) + "(" + to_string(fromChat.uid) + "):";
	return strFwd;
}

void reply(AttrObject& msg, string strReply) {
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	strReply = fmt->format(strReply, msg);
	if (console["ReferMsgReply"] && msg["msgid"])strReply = "[CQ:reply,id=" + msg["msgid"].to_str() + "]" + strReply;
	long long uid{ msg.get_ll("uid") };
	long long gid{ msg.get_ll("gid") };
	long long chid{ msg.get_ll("chid") };
	if (uid || gid || chid)
		AddMsgToQueue(strReply, chatInfo{ uid,gid,chid });
}