/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2020 w4123溯洄
 * Copyright (C) 2019-2020 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <mutex>
#include <unordered_map>

#include "APPINFO.h"
#include "DiceFile.hpp"
#include "Jsonio.h"
#include "CQEVE_ALL.h"
#include "ManagerSystem.h"
#include "DiceMod.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"
#include "DiceCloud.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "CharacterCard.h"
#include "DiceEvent.h"
#include "DiceSession.h"
#include "DiceGUI.h"
#include "S3PutObject.h"

#pragma warning(disable:4996)
#pragma warning(disable:6031)

using namespace std;
using namespace CQ;

unordered_map<long long, User> UserList{};
ThreadFactory threads;
string strFileLoc;

constexpr auto msgInit{ R"(欢迎使用Dice!掷骰机器人！
请从菜单【Dice!】->【综合管理】开启骰娘的后台面板
开启Master模式通过认主后即可成为我的主人~
可发送.help查看帮助
参考文档参看.help链接)" };

//加载数据
void loadData()
{
	mkDir(DiceDir);
	string strLog;
	loadDir(loadXML<CardTemp>, string(DiceDir + "\\CardTemp\\"), getmCardTemplet(), strLog, true);
	if (loadJMap(DiceDir + "\\conf\\CustomReply.json", CardDeck::mReplyDeck) < 0 && loadJMap(
		strFileLoc + "ReplyDeck.json", CardDeck::mReplyDeck) > 0)
	{
		console.log("迁移自定义回复" + to_string(CardDeck::mReplyDeck.size()) + "条", 1);
		saveJMap(DiceDir + "\\conf\\CustomReply.json", CardDeck::mReplyDeck);
	}
	fmt->set_help("回复列表", "回复触发词列表:" + listKey(CardDeck::mReplyDeck));
	if (loadDir(loadJMap, string(DiceDir + "\\PublicDeck\\"), CardDeck::mExternPublicDeck, strLog) < 1)
	{
		loadJMap(strFileLoc + "PublicDeck.json", CardDeck::mExternPublicDeck);
		loadJMap(strFileLoc + "ExternDeck.json", CardDeck::mExternPublicDeck);
	}
	map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
	//读取帮助文档
	fmt->load(strLog);
	if (int cnt; (cnt = loadJMap(DiceDir + "\\conf\\CustomHelp.json", CustomHelp)) < 0)
	{
		if (cnt == -1)console.log("自定义帮助文件json解析失败！", 1);
		ifstream ifstreamHelpDoc(strFileLoc + "HelpDoc.txt");
		if (ifstreamHelpDoc)
		{
			string strName, strMsg;
			while (getline(ifstreamHelpDoc, strName) && getline(ifstreamHelpDoc, strMsg))
			{
				while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
				while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
				while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
				CustomHelp[strName] = strMsg;
			}
			if (!CustomHelp.empty())
			{
				saveJMap(DiceDir + "\\conf\\CustomHelp.json", CustomHelp);
				console.log("初始化自定义帮助词条" + to_string(CustomHelp.size()) + "条", 1);
			}
		}
		ifstreamHelpDoc.close();
	}
	map_merge(fmt->helpdoc, CustomHelp);
	if (!strLog.empty())
	{
		strLog += "扩展配置读取完毕√";
		console.log(strLog, 1, printSTNow());
	}
}

//初始化
void dataInit()
{
	gm = make_unique<DiceTableMaster>();
	if (gm->load() < 0)
	{
		multimap<long long, long long> ObserveGroup;
		loadFile(strFileLoc + "ObserveDiscuss.RDconf", ObserveGroup);
		loadFile(strFileLoc + "ObserveGroup.RDconf", ObserveGroup);
		for (auto [grp, qq] : ObserveGroup)
		{
			gm->session(grp).sOB.insert(qq);
			gm->session(grp).update();
		}
		ifstream ifINIT(strFileLoc + "INIT.DiceDB");
		if (ifINIT)
		{
			long long Group(0);
			int value;
			string nickname;
			while (ifINIT >> Group >> nickname >> value)
			{
				gm->session(Group).mTable["先攻"].emplace(base64_decode(nickname), value);
				gm->session(Group).update();
			}
		}
		ifINIT.close();
		console.log("初始化旁观与先攻记录" + to_string(gm->mSession.size()) + "条", 1);
	}
	today = make_unique<DiceToday>(DiceDir + "/user/DiceToday.json");
}

//备份数据
void dataBackUp()
{
	mkDir(DiceDir + "\\conf");
	mkDir(DiceDir + "\\user");
	mkDir(DiceDir + "\\audit");
	//保存卡牌
	saveJMap(strFileLoc + "GroupDeck.json", CardDeck::mGroupDeck);
	saveJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	saveJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	saveJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	//备份列表
	saveBFile(DiceDir + "\\user\\PlayerCards.RDconf", PList);
	saveFile(DiceDir + "\\user\\ChatList.txt", ChatList);
	saveBFile(DiceDir + "\\user\\ChatConf.RDconf", ChatList);
	saveFile(DiceDir + "\\user\\UserList.txt", UserList);
	saveBFile(DiceDir + "\\user\\UserConf.RDconf", UserList);
}

EVE_Enable(eventEnable)
{
	llStartTime = clock();
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	strModulePath = pathStr;
	pathStr = pathStr.substr(pathStr.rfind("\\") + 1);
	std::transform(pathStr.begin(), pathStr.end(), pathStr.begin(), [](unsigned char c) { return tolower(c); });
	if (pathStr.substr(0, 4) == "java")
	{
		Mirai = true;
		Dice_Full_Ver_For = Dice_Full_Ver + " For Mirai]";
		char buffer[MAX_PATH];
		const DWORD length = GetModuleFileNameA(nullptr, buffer, sizeof buffer);
		std::string pathSelf(buffer, length);
		dirExe = pathSelf.substr(0, pathSelf.find("jre\\bin\\java.exe"));
		DiceDir = "Dice" + to_string(getLoginQQ());	
		filesystem::path pathDir(dirExe + DiceDir);
		DiceDir = dirExe + DiceDir;
		if (!exists(pathDir)) {
			filesystem::path pathDirOld(dirExe + "DiceData");
			if (exists(pathDirOld))rename(pathDirOld, pathDir);
			else filesystem::create_directory(pathDir);
		}
		this_thread::sleep_for(3s); //确保Mirai异步信息加载执行完毕
	}
	console.setPath(DiceDir + "\\conf\\Console.xml");
	strFileLoc = getAppDirectory();
	mkDir(strFileLoc); // Mirai不会自动创建文件夹
	console.DiceMaid = getLoginQQ();
	GlobalMsg["strSelfName"] = getLoginNick();
	if (GlobalMsg["strSelfName"].empty() && Mirai)
	{
		GlobalMsg["strSelfName"] = "骰娘[" + toString(console.DiceMaid % 1000, 4) + "]";
	}
	mkDir(DiceDir + "\\conf");
	mkDir(DiceDir + "\\user");
	mkDir(DiceDir + "\\audit");
	if (!console.load())
	{
		ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
		if (ifstreamMaster)
		{
			std::pair<int, int> ClockToWork{}, ClockOffWork{};
			int iDisabledGlobal, iDisabledMe, iPrivate, iDisabledJrrp, iLeaveDiscuss;
			ifstreamMaster >> console.masterQQ >> console.isMasterMode >> iDisabledGlobal >> iDisabledMe >> iPrivate >>
				iDisabledJrrp >> iLeaveDiscuss
				>> ClockToWork.first >> ClockToWork.second >> ClockOffWork.first >> ClockOffWork.second;
			console.set("DisabledGlobal", iDisabledGlobal);
			console.set("DisabledMe", iDisabledMe);
			console.set("Private", iPrivate);
			console.set("DisabledJrrp", iDisabledJrrp);
			console.set("LeaveDiscuss", iLeaveDiscuss);
			console.setClock(ClockToWork, ClockEvent::on);
			console.setClock(ClockOffWork, ClockEvent::off);
		}
		else
		{
			sendPrivateMsg(console.DiceMaid, msgInit);
		}
		ifstreamMaster.close();
		std::map<string, int> boolConsole;
		loadJMap(strFileLoc + "boolConsole.json", boolConsole);
		for (auto& [key, val] : boolConsole)
		{
			console.set(key, val);
		}
		console.setClock({ 11, 45 }, ClockEvent::clear);
		console.loadNotice();
		console.save();
	}
	//读取聊天列表
	if (loadBFile(DiceDir + "\\user\\UserConf.RDconf", UserList) < 1)
	{
		map<long long, int> DefaultDice;
		if (loadFile(strFileLoc + "Default.RDconf", DefaultDice) > 0)
			for (auto p : DefaultDice)
			{
				getUser(p.first).create(NEWYEAR).intConf["默认骰"] = p.second;
			}
		map<long long, string> DefaultRule;
		if (loadFile(strFileLoc + "DefaultRule.RDconf", DefaultRule) > 0)
			for (auto p : DefaultRule)
			{
				if (isdigit(static_cast<unsigned char>(p.second[0])))break;
				getUser(p.first).create(NEWYEAR).strConf["默认规则"] = p.second;
			}
		ifstream ifName(strFileLoc + "Name.dicedb");
		if (ifName)
		{
			long long GroupID = 0, QQ = 0;
			string name;
			while (ifName >> GroupID >> QQ >> name)
			{
				name = base64_decode(name);
				getUser(QQ).create(NEWYEAR).setNick(GroupID, name);
			}
		}
	}
	if (loadFile(DiceDir + "\\user\\UserList.txt", UserList) < 1)
	{
		set<long long> WhiteQQ;
		if (loadFile(strFileLoc + "WhiteQQ.RDconf", WhiteQQ) > 0)
			for (auto qq : WhiteQQ)
			{
				getUser(qq).create(NEWYEAR).trust(1);
			}
		//读取管理员列表
		set<long long> AdminQQ;
		if (loadFile(strFileLoc + "AdminQQ.RDconf", AdminQQ) > 0)
			for (auto qq : AdminQQ)
			{
				getUser(qq).create(NEWYEAR).trust(4);
			}
		if (console.master())getUser(console.master()).create(NEWYEAR).trust(5);
		console.log("初始化用户记录" + to_string(UserList.size()) + "条", 1);
	}
	if (loadBFile(DiceDir + "\\user\\ChatConf.RDconf", ChatList) < 1)
	{
		set<long long> GroupList;
		if (loadFile(strFileLoc + "DisabledDiscuss.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).discuss().set("停用指令");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledJRRPDiscuss.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).discuss().set("禁用jrrp");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledMEGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).discuss().set("禁用me");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledHELPDiscuss.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).discuss().set("禁用help");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledOBDiscuss.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).discuss().set("禁用ob");
			}
		GroupList.clear();
		map<chatType, int> mDefault;
		if (loadFile(strFileLoc + "DefaultCOC.MYmap", mDefault) > 0)
			for (auto it : mDefault)
			{
				if (it.first.second == msgtype::Private)getUser(it.first.first)
				                                        .create(NEWYEAR).setConf("rc房规", it.second);
				else chat(it.first.first).create(NEWYEAR).setConf("rc房规", it.second);
			}
		if (loadFile(strFileLoc + "DisabledGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("停用指令");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledJRRPGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("禁用jrrp");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledMEDiscuss.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("禁用me");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledHELPGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("禁用help");
			}
		GroupList.clear();
		if (loadFile(strFileLoc + "DisabledOBGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("禁用ob");
			}
		GroupList.clear();
		map<long long, string> WelcomeMsg;
		if (loadFile(strFileLoc + "WelcomeMsg.RDconf", WelcomeMsg) > 0)
		{
			for (const auto& p : WelcomeMsg)
			{
				chat(p.first).group().setText("入群欢迎", p.second);
			}
		}
		if (loadFile(strFileLoc + "WhiteGroup.RDconf", GroupList) > 0)
		{
			for (auto g : GroupList)
			{
				chat(g).group().set("许可使用").set("免清");
			}
		}
	}
	if (loadFile(DiceDir + "\\user\\ChatList.txt", ChatList) < 1)
	{
		map<chatType, time_t> mLastMsgList;
		for (auto it : mLastMsgList) 
		{
			if (it.first.second == msgtype::Private)getUser(it.first.first).create(it.second);
			else chat(it.first.first).create(it.second).lastmsg(it.second).isGroup = 2 - int(it.first.second);
		}
		std::map<long long, long long> mGroupInviter;
		if (loadFile(strFileLoc + "GroupInviter.RDconf", mGroupInviter) < 1)
		{
			for (auto it : mGroupInviter)
			{
				chat(it.first).group().inviter = it.second;
			}
		}
		console.log("初始化群记录" + to_string(ChatList.size()) + "条", 1);
	}
	for (auto& [gid,gname] : getGroupList())
	{
		chat(gid).group().name(gname).reset("已退");
	}
	blacklist = make_unique<DDBlackManager>();
	if (blacklist->loadJson(DiceDir + "\\conf\\BlackList.json") < 0)
	{
		blacklist->loadJson(strFileLoc + "BlackMarks.json");
		int cnt = blacklist->loadHistory(strFileLoc);
		blacklist->saveJson(DiceDir + "\\conf\\BlackList.json");
		console.log("初始化不良记录" + to_string(cnt) + "条", 1);
	}
	else {
		blacklist->loadJson(DiceDir + "\\conf\\BlackListEx.json", true);
	}
	fmt = make_unique<DiceModManager>();
	if (loadJMap(DiceDir + "\\conf\\CustomMsg.json", EditedMsg) < 0)loadJMap(strFileLoc + "CustomMsg.json", EditedMsg);
	//预修改出场回复文本
	if (EditedMsg.count("strSelfName"))GlobalMsg["strSelfName"] = EditedMsg["strSelfName"];
	for (auto it : EditedMsg)
	{
		while (it.second.find("本机器人") != string::npos)it.second.replace(it.second.find("本机器人"), 8,
		                                                                GlobalMsg["strSelfName"]);
		GlobalMsg[it.first] = it.second;
	}
	loadData();
	if (loadBFile(DiceDir + "\\user\\PlayerCards.RDconf", PList) < 1)
	{
		ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
		if (ifstreamCharacterProp)
		{
			long long QQ, GrouporDiscussID;
			int Type, Value;
			string SkillName;
			while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value)
			{
				if (SkillName == "智力/灵感")SkillName = "智力";
				getPlayer(QQ)[0].set(SkillName, Value);
			}
			console.log("初始化角色卡记录" + to_string(PList.size()) + "条", 1);
		}
		ifstreamCharacterProp.close();
	}
	for (const auto& pl : PList)
	{
		if (!UserList.count(pl.first))getUser(pl.first).create(NEWYEAR);
	}
	//读取卡牌
	loadJMap(strFileLoc + "GroupDeck.json", CardDeck::mGroupDeck);
	loadJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	loadJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	loadJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	dataInit();
	// 确保线程执行结束
	while (msgSendThreadRunning)Sleep(10);
	Aws::InitAPI(options);
	Enabled = true;
	threads(SendMsg);
	threads(ConsoleTimer);
	threads(warningHandler);
	threads(frqHandler);
	sch.start();
	console.log(GlobalMsg["strSelfName"] + "初始化完成，用时" + to_string((clock() - llStartTime) / 1000) + "秒", 0b1,
				printSTNow());
	//骰娘网络
	getDiceList();
	getExceptGroup();
	llStartTime = clock();
	return 0;
}

mutex GroupAddMutex;

bool eve_GroupAdd(Chat& grp)
{
	{
		unique_lock<std::mutex> lock_queue(GroupAddMutex);
		if (grp.isset("未进") || grp.isset("已退"))grp.reset("未进").reset("已退");
		else return false;
		if (ChatList.size() == 1 && !console.isMasterMode)sendGroupMsg(grp.ID, msgInit);
	}
	GroupInfo ginf(grp.ID);
	grp.Name = ginf.strGroupName;
	if (grp.boolConf.empty() && ginf.nGroupSize > 499) {
		grp.set("协议无效");
	}
	if (!console["ListenGroupAdd"] || grp.isset("忽略"))return 0;
	long long fromGroup = grp.ID;
	string strNow = printSTNow();
	string strMsg(GlobalMsg["strSelfName"]);
	try 
	{
		strMsg += "新加入:" + ginf.tostring();
		if (blacklist->get_group_danger(fromGroup)) 
		{
			grp.leave(blacklist->list_group_warning(fromGroup));
			strMsg += "为黑名单群，已退群";
			console.log(strMsg, 0b10, printSTNow());
			return true;
		}
		if (grp.isset("使用许可"))strMsg += "（已获使用许可）";
		else if(grp.isset("协议无效"))strMsg += "（默认协议无效）";
		if (grp.inviter) {
			strMsg += ",邀请者" + printQQ(chat(fromGroup).inviter);
		}
		console.log(strMsg, 0, strNow);
		int max_trust = 0;
		float ave_trust(0);
		//int max_danger = 0;
		long long ownerQQ = 0;
		ResList blacks;
		std::vector<GroupMemberInfo> list = getGroupMemberList(fromGroup);
		if (list.empty())
		{
			strMsg += "，群员名单未加载；";
		}
		else 
		{
			int cntUser(0), cntMember(0);
			for (auto& each : list) 
			{
				if (each.QQID == console.DiceMaid)continue;
				cntMember++;
				if (UserList.count(each.QQID)) 
				{
					cntUser++;
					ave_trust += getUser(each.QQID).nTrust;
				}
				if (each.permissions > 1) 
				{
					max_trust |= (1 << trustedQQ(each.QQID));
					if (blacklist->get_qq_danger(each.QQID) > 1)
					{
						strMsg += ",发现黑名单管理员" + printQQ(each.QQID);
						if (grp.isset("免黑"))
						{
							strMsg += "（群免黑）";
						}
						else
						{
							sendGroupMsg(fromGroup, blacklist->list_qq_warning(each.QQID));
							grp.leave("发现黑名单管理员" + printQQ(each.QQID) + "将预防性退群");
							strMsg += "，已退群";
							console.log(strMsg, 0b10, strNow);
							return true;
						}
					}
					if (each.permissions == 3)
					{
						ownerQQ = each.QQID;
						ave_trust += ginf.nGroupSize * trustedQQ(each.QQID);
						strMsg += "，群主" + printQQ(each.QQID) + "；";
					}
					else
					{
						ave_trust += ginf.nGroupSize * trustedQQ(each.QQID) / 10;
					}
				}
				else if (blacklist->get_qq_danger(each.QQID) > 1)
				{
					//max_trust |= 1;
					blacks << printQQ(each.QQID);
					if (blacklist->get_qq_danger(each.QQID)) 
					{
						AddMsgToQueue(blacklist->list_self_qq_warning(each.QQID), fromGroup, msgtype::Group);
					}
				}
			}
			if (!chat(fromGroup).inviter && list.size() == 2 && ownerQQ)
			{
				chat(fromGroup).inviter = ownerQQ;
				strMsg += "邀请者" + printQQ(ownerQQ);
			}
			if (!cntMember) 
			{
				strMsg += "，群员名单未加载；";
			}
			else 
			{
				ave_trust /= cntMember;
				strMsg += "\n用户浓度" + to_string(cntUser * 100 / cntMember) + "% (" + to_string(cntUser) + "/" + to_string(cntMember) + "), 信任度" + toString(ave_trust);
			}
		}
		if (!blacks.empty())
		{
			string strNote = "发现黑名单群员" + blacks.show();
			strMsg += strNote;
		}
		if (console["Private"] && !grp.isset("许可使用"))
		{	
			//避免小群绕过邀请没加上白名单
			if (max_trust > 1 || ave_trust > 0.5)
			{
				grp.set("许可使用");
				strMsg += "\n已自动追加使用许可";
			}
			else 
			{
				strMsg += "\n无许可使用，已退群";
				console.log(strMsg, 1, strNow);
				grp.leave(getMsg("strPreserve"));
				return true;
			}
		}
		if (!blacks.empty())console.log(strMsg, 0b10, strNow);
		else console.log(strMsg, 1, strNow);
	}
	catch (...)
	{
		console.log(strMsg + "\n群" + to_string(fromGroup) + "信息获取失败！", 0b1, printSTNow());
		return true;
	}
	if (grp.isset("协议无效")) return 0;
	if (!GlobalMsg["strAddGroup"].empty())
	{
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strAddGroup"), {fromGroup, msgtype::Group});
	}
	if (console["CheckGroupLicense"] && !grp.isset("许可使用"))
	{
		grp.set("未审核");
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strGroupLicenseDeny"), {fromGroup, msgtype::Group});
	}
	return false;
}

//处理骰子指令

EVE_PrivateMsg_EX(eventPrivateMsg)
{
	if (!Enabled)return;
	FromMsg Msg(eve.message, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (!Enabled)return;
	if (eve.isAnonymous())return;
	if (eve.isSystem())return;
	Chat& grp = chat(eve.fromGroup).group().lastmsg(time(nullptr));
	if (grp.isset("未进") || grp.isset("已退"))eve_GroupAdd(grp);
	if (!grp.isset("忽略"))
	{
		FromMsg Msg(eve.message, eve.fromGroup, msgtype::Group, eve.fromQQ);
		if (Msg.DiceFilter())eve.message_block();
	}
	if (grp.isset("拦截消息"))eve.message_block();
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	if (!Enabled)return;
	//time_t tNow = time(NULL);
	if (console["LeaveDiscuss"])
	{
		sendDiscussMsg(eve.fromDiscuss, getMsg("strLeaveDiscuss"));
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	Chat& grp = chat(eve.fromDiscuss).discuss().lastmsg(time(nullptr));
	if (blacklist->get_qq_danger(eve.fromQQ) && console["AutoClearBlack"])
	{
		const string strMsg = "发现黑名单用户" + printQQ(eve.fromQQ) + "，自动执行退群";
		console.log(printChat({eve.fromDiscuss, msgtype::Discuss}) + strMsg, 0b10, printSTNow());
		grp.leave(strMsg);
		return;
	}
	FromMsg Msg(eve.message, eve.fromDiscuss, msgtype::Discuss, eve.fromQQ);
	if (Msg.DiceFilter() || grp.isset("拦截消息"))eve.message_block();
}

EVE_System_GroupMemberIncrease(eventGroupMemberIncrease)
{
	Chat& grp = chat(fromGroup);
	if (grp.isset("忽略"))return 0;
	if (beingOperateQQ != console.DiceMaid)
	{
		if (chat(fromGroup).strConf.count("入群欢迎"))
		{
			string strReply = chat(fromGroup).strConf["入群欢迎"];
			while (strReply.find("{at}") != string::npos)
			{
				strReply.replace(strReply.find("{at}"), 4, "[CQ:at,qq=" + to_string(beingOperateQQ) + "]");
			}
			while (strReply.find("{@}") != string::npos)
			{
				strReply.replace(strReply.find("{@}"), 3, "[CQ:at,qq=" + to_string(beingOperateQQ) + "]");
			}
			while (strReply.find("{nick}") != string::npos)
			{
				strReply.replace(strReply.find("{nick}"), 6, getStrangerInfo(beingOperateQQ).nick);
			}
			while (strReply.find("{age}") != string::npos)
			{
				strReply.replace(strReply.find("{age}"), 5, to_string(getStrangerInfo(beingOperateQQ).age));
			}
			while (strReply.find("{sex}") != string::npos)
			{
				strReply.replace(strReply.find("{sex}"), 5,
				                 getStrangerInfo(beingOperateQQ).sex == 0
					                 ? "男"
					                 : getStrangerInfo(beingOperateQQ).sex == 1
					                 ? "女"
					                 : "未知");
			}
			while (strReply.find("{qq}") != string::npos)
			{
				strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
			}
			grp.update(time(nullptr));
			AddMsgToQueue(strReply, fromGroup, msgtype::Group);
		}
		if (blacklist->get_qq_danger(beingOperateQQ))
		{
			const string strNow = printSTNow();
			string strNote = printGroup(fromGroup) + "发现" + GlobalMsg["strSelfName"] + "的黑名单用户" + printQQ(
				beingOperateQQ) + "入群";
			AddMsgToQueue(blacklist->list_self_qq_warning(beingOperateQQ), fromGroup, msgtype::Group);
			if (grp.isset("免清"))strNote += "（群免清）";
			else if (grp.isset("免黑"))strNote += "（群免黑）";
			else if (getGroupMemberInfo(fromGroup, console.DiceMaid).permissions > 1)strNote += "（群内有权限）";
			else if (console["LeaveBlackQQ"])
			{
				strNote += "（已退群）";
				grp.leave("发现黑名单用户" + printQQ(beingOperateQQ) + "入群,将预防性退群");
			}
			console.log(strNote, 0b10, strNow);
		}
	}
	else
	{
		return eve_GroupAdd(grp.set("未进"));
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease)
{
	if (fromQQ == 0)return 0; // 考虑Mirai在机器人自行退群时也会调用一次这个函数
	Chat& grp = chat(fromGroup);
	if (beingOperateQQ == console.DiceMaid)
	{
		grp.set("已退");
		if (!console || grp.isset("忽略"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printQQ(fromQQ) + "将" + printQQ(beingOperateQQ) + "移出了" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (!console["ListenGroupKick"] || trustedQQ(fromQQ) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGroup)) return 0;
		DDBlackMarkFactory mark{fromQQ, fromGroup};
		mark.sign().type("kick").time(strNow).note(strNow + " " + strNote).comment(getMsg("strSelfCall") + "原生记录");
		if (grp.inviter && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";入群邀请者：" + printQQ(grp.inviter);
			if (console["KickedBanInviter"])mark.inviterQQ(grp.inviter).note(strNow + " " + strNote);
		}
		grp.reset("许可使用").reset("免清");
		blacklist->create(mark.product());
	}
	else if (mDiceList.count(beingOperateQQ) && subType == 2 && console["ListenGroupKick"])
	{
		if (!console || grp.isset("忽略"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printQQ(fromQQ) + "将" + printQQ(beingOperateQQ) + "移出了" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (trustedQQ(fromQQ) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGroup)) return 0;
		DDBlackMarkFactory mark{fromQQ, fromGroup};
		mark.type("kick").time(strNow).note(strNow + " " + strNote).DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printQQ(console.DiceMaid) + "目击");
		grp.reset("许可使用").reset("免清");
		blacklist->create(mark.product());
	}
	return 0;
}

EVE_System_GroupBan(eventGroupBan)
{
	Chat& grp = chat(fromGroup);
	if (grp.isset("忽略") || (beingOperateQQ != console.DiceMaid && !mDiceList.count(beingOperateQQ)) || !console[
		"ListenGroupBan"])return 0;
	if (subType == 1)
	{
		if (beingOperateQQ == console.DiceMaid)
		{
			console.log(GlobalMsg["strSelfName"] + "在" + printGroup(fromGroup) + "中被解除禁言", 0b10, printSTNow());
			return 1;
		}
	}
	else
	{
		string strNow = printSTNow();
		long long llOwner = 0;
		string strNote = "在" + printGroup(fromGroup) + "中," + printQQ(beingOperateQQ) + "被" + printQQ(fromQQ) + "禁言" + printDuringTime(duration);
		if (!console["ListenGroupBan"] || trustedQQ(fromQQ) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGroup)) 
		{
			console.log(strNote, 0b10, strNow);
			return 1;
		}
		DDBlackMarkFactory mark{fromQQ, fromGroup};
		mark.type("ban").time(strNow).note(strNow + " " + strNote);
		if (mDiceList.count(fromQQ))mark.fromQQ(0);
		if (beingOperateQQ == console.DiceMaid)
		{
			if (!console)return 0;
			mark.sign().comment(getMsg("strSelfCall") + "原生记录");
		}
		else
		{
			mark.DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printQQ(console.DiceMaid) + "目击");
		}
		//统计群内管理
		int intAuthCnt = 0;
		string strAuthList;
		vector<GroupMemberInfo> list = getGroupMemberList(fromGroup);
		for (auto& member : list)
		{
			if (member.permissions == 3)
			{
				llOwner = member.QQID;
			}
			else if (member.permissions == 2)
			{
				strAuthList += '\n' + member.Nick + "(" + to_string(member.QQID) + ")";
				intAuthCnt++;
			}
		}
		strAuthList = "；群主" + printQQ(llOwner) + ",另有管理员" + to_string(intAuthCnt) + "名" + strAuthList;
		mark.note(strNow + " " + strNote);
		if (grp.inviter && beingOperateQQ == console.DiceMaid && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";入群邀请者：" + printQQ(grp.inviter);
			if (console["BannedBanInviter"])mark.inviterQQ(grp.inviter);
			mark.note(strNow + " " + strNote);
		}
		grp.reset("免清").reset("许可使用").leave();
		console.log(strNote + strAuthList, 0b1000, strNow);
		blacklist->create(mark.product());
		return 1;
	}
	return 0;
}

EVE_Request_AddGroup(eventGroupInvited)
{
	if (!console["ListenGroupRequest"])return 0;
	if (subType == 2 && groupset(fromGroup, "忽略") < 1)
	{
		this_thread::sleep_for(3s);
		const string strNow = printSTNow();
		string strMsg = "群添加请求，来自：" + printQQ(fromQQ) + ",群:" +
			to_string(fromGroup) + "。";
		if (blacklist->get_group_danger(fromGroup)) 
		{
			strMsg += "\n已拒绝（群在黑名单中）";
			console.log(strMsg, 0b10, strNow);
			setGroupAddRequest(responseFlag, 2, 2, "");
		}
		else if (blacklist->get_qq_danger(fromQQ))
		{
			strMsg += "\n已拒绝（用户在黑名单中）";
			console.log(strMsg, 0b10, strNow);
			setGroupAddRequest(responseFlag, 2, 2, "");
		}
		else if (Chat& grp = chat(fromGroup).group(); grp.isset("许可使用")) {
			grp.set("未进");
			grp.inviter = fromQQ;
			strMsg += "\n已同意（群已许可使用）";
			console.log(strMsg, 1, strNow);
			setGroupAddRequest(responseFlag, 2, 1, "");
		}
		else if (trustedQQ(fromQQ))
		{
			grp.set("许可使用").set("未进");
			grp.inviter = fromQQ;
			strMsg += "\n已同意（受信任用户）";
			console.log(strMsg, 1, strNow);
			setGroupAddRequest(responseFlag, 2, 1, "");
		}
		else if (console && console["Private"])
		{
			sendPrivateMsg(fromQQ, getMsg("strPreserve"));
			strMsg += "\n已拒绝（当前在私用模式）";
			console.log(strMsg, 1, strNow);
			setGroupAddRequest(responseFlag, 2, 2, "");
		}
		else
		{
			grp.set("未进");
			grp.inviter = fromQQ;
			strMsg += "已同意";
			this_thread::sleep_for(2s);
			console.log(strMsg, 1, strNow);
			setGroupAddRequest(responseFlag, 2, 1, "");
		}
		return 1;
	}
	return 0;
}

EVE_Menu(eventMasterMode)
{
	if (console)
	{
		console.isMasterMode = false;
		console.killMaster();
		MessageBoxA(nullptr, "Master模式已关闭√\nmaster已清除", "Master模式切换", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		console.isMasterMode = true;
		console.save();
		MessageBoxA(nullptr, "Master模式已开启√\n认主请对骰娘发送.master public/private", "Master模式切换", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

EVE_Menu(eventGUI)
{
	return GUIMain();
}

void global_exit() {
	Enabled = false;
	threads.exit();
	threads = {};
	dataBackUp();
	sch.end();
	fmt.reset();
	gm.reset();
	PList.clear();
	ChatList.clear();
	UserList.clear();
	console.reset();
	EditedMsg.clear();
	blacklist.reset();
	Aws::ShutdownAPI(options);
}

EVE_Disable(eventDisable)
{
	global_exit();
	return 0;
}

EVE_Exit(eventExit)
{
	if (!Enabled)
		return 0;
	global_exit();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
