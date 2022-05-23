/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123���
 * Copyright (C) 2019-2022 String.Empty
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
#define UNICODE
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include <exception>
#include <stdexcept>
#include "filesystem.hpp"

#include "DiceFile.hpp"
#include "Jsonio.h"
#include "QQEvent.h"
#include "DDAPI.h"
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
#include "DiceCensor.h"
#include "EncodingConvert.h"
#include "DiceManager.h"

#ifndef _WIN32
#include <curl/curl.h>
#endif

#pragma warning(disable:4996)
#pragma warning(disable:6031)

using namespace std;

unordered_map<long long, User> UserList{};
unordered_map<long long, long long> TinyList{};
ThreadFactory threads;
std::filesystem::path fpFileLoc;
std::unique_ptr<CivetServer> ManagerServer;
IndexHandler h_index;
BasicInfoApiHandler h_basicinfoapi;
CustomMsgApiHandler h_msgapi;
AdminConfigHandler h_config;
MasterHandler h_master;
CustomReplyApiHandler h_customreply;
WebUIPasswordHandler h_webuipassword;
AuthHandler auth_handler;

constexpr auto msgInit{ R"(��ӭʹ��Dice!���������ˣ�
�뷢��.system gui��������ĺ�̨���
����Masterģʽͨ�������󼴿ɳ�Ϊ�ҵ�����~
�ɷ���.help�鿴����
�ο��ĵ��ο�.help����)" };

//��������
void loadData()
{
	ResList logList;
	try
	{
		std::error_code ec;
		std::filesystem::create_directory(DiceDir, ec);	
		loadDir(loadXML<CardTemp>, DiceDir / "CardTemp", mCardTemplet, logList, true);
		loadDir(loadJMap, DiceDir / "PublicDeck", CardDeck::mExternPublicDeck, logList);
		map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
		//��ȡ�����ĵ�
		fmt->clear();
		fmt->load(logList);
		//��ȡ���дʿ�
		loadDir(load_words, DiceDir / "conf" / "censor", censor, logList, true);
		loadJMap(DiceDir / "conf" / "CustomCensor.json", censor.CustomWords);
		censor.build();
		if (!logList.empty())
		{
			logList << "��չ���ö�ȡ��ϡ�";
			console.log(logList.show(), 1, printSTNow());
		}
	}
	catch (const std::exception& e)
	{
		logList << "��ȡ����ʱ����������󣬳�������޷��������С��볢��������ú����ԡ�" << e.what();
		console.log(logList.show(), 1, printSTNow());
	}
}

//��ʼ���û�����
void readUserData()
{
	std::error_code ec;
	fs::path dir{ DiceDir / "user" };
	ResList log;
	//��ȡ�û���¼
	if (int cnt{ loadBFile(dir / "UserConf.dat", UserList) };cnt > 0) {
		fs::copy(dir / "UserConf.dat", dir / "UserConf.bak",
			fs::copy_options::overwrite_existing, ec);
		log << "��ȡ�û���¼" + to_string(cnt) + "��";
	}
	else if (fs::exists(dir / "UserConf.bak")) {
		cnt = loadBFile(dir / "UserConf.bak", UserList);
		if (cnt > 0)log << "�ָ��û���¼" + to_string(cnt) + "��";
	}
	else {
		cnt = loadBFile<long long, User, &User::old_readb>(dir / "UserConf.RDconf", UserList);
		loadFile(dir / "UserList.txt", UserList);
		if (cnt > 0)log << "Ǩ���û���¼" + to_string(cnt) + "��";
	}
	//for QQ Channel
	if (User& self{ UserList[console.DiceMaid] }; !self.confs.count("tinyID") || self.confs["tinyID"] == 0) {
		if (long long tiny{ DD::getTinyID() }) {
			DD::debugMsg("��ȡ����ID:" + to_string(tiny));
			self.setConf("tinyID", tiny);
		}
	}
	//��ȡ��ɫ��¼
	if (int cnt{ loadBFile(dir / "PlayerCards.RDconf", PList) }; cnt > 0) {
		fs::copy(dir / "PlayerCards.RDconf", dir / "PlayerCards.bak",
			fs::copy_options::overwrite_existing, ec);
		log << "��ȡ��Ҽ�¼" + to_string(cnt) + "��";
	}
	else if (fs::exists(dir / "PlayerCards.bak")) {
		cnt = loadBFile(dir / "PlayerCards.bak", PList);
		if (cnt > 0)log << "�ָ���Ҽ�¼" + to_string(cnt) + "��";
	}
	for (const auto& pl : PList)
	{
		if (!UserList.count(pl.first))getUser(pl.first);
	}
	//��ȡȺ�ļ�¼
	if (int cnt{ loadBFile(dir / "ChatConf.dat", ChatList) };cnt > 0) {
		fs::copy(dir / "ChatConf.dat", dir / "ChatConf.bak",
			fs::copy_options::overwrite_existing, ec);
		log << "��ȡȺ�ļ�¼" + to_string(cnt) + "��";
	}
	else if (fs::exists(dir / "ChatConf.bak")) {
		cnt = loadBFile(dir / "ChatConf.bak", ChatList);
		if (cnt > 0)log << "�ָ�Ⱥ�ļ�¼" + to_string(cnt) + "��";
	}
	else {
		cnt = loadBFile(dir / "ChatConf.RDconf", ChatList);
		loadFile(dir / "ChatList.txt", ChatList);
		if (cnt > 0)log << "Ǩ��Ⱥ�ļ�¼" + to_string(cnt) + "��";
	}
	//��ȡ�����¼
	gm->load();
	//��ȡ��������
	today = make_unique<DiceToday>();
	if (!log.empty()) {
		log << "�û����ݶ�ȡ���";
		console.log(log.show(), 0b1, printSTNow());
	}
}

//��������
void dataBackUp()
{
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	//�����б�
	saveBFile(DiceDir / "user" / "UserConf.dat", UserList);
	saveBFile(DiceDir / "user" / "PlayerCards.RDconf", PList);
	saveBFile(DiceDir / "user" / "ChatConf.dat", ChatList);
}

atomic_flag isIniting{ ATOMIC_FLAG_INIT };
EVE_Enable(eventEnable){
	if (isIniting.test_and_set())return;
	if (Enabled)return;
	llStartTime = time(nullptr);
	#ifndef _WIN32
	CURLcode err;
	err = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (err != CURLE_OK)
	{
		console.log("����: ����libcurlʧ�ܣ�", 1);
	}
#endif
	std::string RootDir = DD::getRootDir();
	if (RootDir.empty()) {	
#ifdef _WIN32
		char path[MAX_PATH];
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		dirExe = std::filesystem::path(path).parent_path();
#endif
	}
	else 
	{
		dirExe = RootDir;
	}
	Dice_Full_Ver_On = Dice_Full_Ver + " on\n" + DD::getDriVer();
	DD::debugLog(Dice_Full_Ver_On);


	mCardTemplet = {
		{
			"COC7", {
				"COC7", SkillNameReplace, BasicCOC7, InfoCOC7, AutoFillCOC7, mVariableCOC7, ExpressionCOC7,
				SkillDefaultVal, {
					{"_default", CardBuild({BuildCOC7},  {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"}, {"����", "7D6+8"}, {"ְҵ", "{����Աְҵ}"}, {"��������", "{��������}"},
											{"��Ҫ֮��", "{��Ҫ֮��}"}, {"˼������", "{˼������}"}, {"����Ƿ�֮��", "{����Ƿ�֮��}"},
											{"����֮��", "{����֮��}"}, {"����", "{����Ա�ص�}"}
										}, {"{�������}"}, {})
					}
				}
			}
		},
		{"BRP", {
				"BRP", {}, {}, {}, {}, {}, {}, {
					{"__DefaultDice",100}
				}, {
					{"_default", CardBuild({},  {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"}, {"����", "7D6+8"}, {"ְҵ", "{����Աְҵ}"}, {"��������", "{��������}"},
											{"��Ҫ֮��", "{��Ҫ֮��}"}, {"˼������", "{˼������}"}, {"����Ƿ�֮��", "{����Ƿ�֮��}"},
											{"����֮��", "{����֮��}"}, {"����", "{����Ա�ص�}"}
										}, {"{�������}"}, {})
					}
				}
		}},
		{"DND", {
				"DND", {}, {}, {}, {}, {}, {}, {
					{"__DefaultDice",20}
				}, {
					{"_default", CardBuild({}, {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"},
										},  {"{�������}"}, {})
					}
				}
		}},
	};
	if (console.DiceMaid = DD::getLoginID())
	{
		DiceDir = dirExe / ("Dice" + to_string(console.DiceMaid));
		if (!exists(DiceDir)) {
			filesystem::path DiceDirOld(dirExe / "DiceData");
			if (exists(DiceDirOld))rename(DiceDirOld, DiceDir);
			else filesystem::create_directory(DiceDir);
		}
	}
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	std::filesystem::create_directory(DiceDir / "mod", ec);
	std::filesystem::create_directory(DiceDir / "plugin", ec);
	console.setPath(DiceDir / "conf" / "Console.xml");
	fpFileLoc = DiceDir / "com.w4123.dice";
	{
		std::unique_lock lock(GlobalMsgMutex);
		string& strSelfName{ GlobalMsg["strSelfName"] = DD::getLoginNick() };
		if (strSelfName.empty())
		{
			strSelfName = "����[" + toString(console.DiceMaid % 10000, 4) + "]";
		}
	}
	fmt = make_unique<DiceModManager>();

	ExtensionManagerInstance = std::make_unique<ExtensionManager>();
	if (!console.load())
	{
		DD::sendPrivateMsg(console.DiceMaid, msgInit);
		std::map<string, int> boolConsole;
		loadJMap(fpFileLoc / "boolConsole.json", boolConsole);
		for (auto& [key, val] : boolConsole)
		{
			console.set(key, val);
		}
		console.setClock({ 11, 45 }, "clear");
		console.loadNotice();
		console.save();
	}
	//��ʼ��������
	blacklist = make_unique<DDBlackManager>();
	if (blacklist->loadJson(DiceDir / "conf" / "BlackList.json") < 0)
	{
		blacklist->loadJson(fpFileLoc / "BlackMarks.json");
		int cnt = blacklist->loadHistory(fpFileLoc);
		if (cnt) {
			blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
			console.log("��ʼ��������¼" + to_string(cnt) + "��", 1);
		}
	}
	else {
		blacklist->loadJson(DiceDir / "conf" / "BlackListEx.json", true);
	}
	{
		std::unique_lock lock(GlobalMsgMutex);
		if (loadJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg) < 0)loadJMap(fpFileLoc / "CustomMsg.json", EditedMsg);
		{	
			merge(GlobalMsg, EditedMsg);
		}
	}
	loadData();
	//��ȡ�û�����
	gm = make_unique<DiceTableMaster>();
	readUserData();
	set<long long> grps{ DD::getGroupIDList() };
	for (auto gid : grps)
	{
		chat(gid).group().reset("δ��").reset("����").set("����Ⱥ");
	}
	// ȷ���߳�ִ�н���
	while (msgSendThreadRunning)this_thread::sleep_for(10ms);
	Aws::InitAPI(options);

	DD::debugLog("Dice.webUIInit");
	WebUIPasswordPath = DiceDir / "conf" / "WebUIPassword.digest";
	if (!std::filesystem::exists(WebUIPasswordPath, ec) || std::filesystem::is_empty(WebUIPasswordPath, ec))
	{		
		setPassword("password");
	}

	// ��ʼ��������
	mg_init_library(0);

	// ��ȡ��������
	bool AllowInternetAccess = console["WebUIAllowInternetAccess"];
	int Port = console["WebUIPort"];

	const char* envAllowInternetAccess = std::getenv("DICE_WEBUI_ALLOW_INTERNET_ACCESS");
	std::string envStrAllowInternetAccess = envAllowInternetAccess ? envAllowInternetAccess : "";

	if (envStrAllowInternetAccess == "1")
	{
		AllowInternetAccess = true;
	}
	else if (envStrAllowInternetAccess == "0")
	{
		AllowInternetAccess = false;
	}

	const char* alternateVariable = std::getenv("DICE_WEBUI_PORT_USE_VARIABLE");
	std::string variable = alternateVariable ? alternateVariable : "DICE_WEBUI_PORT";
	
	try
	{
		const char* envPort = std::getenv(variable.c_str());
		Port = std::stoi(envPort ? envPort : "");
	}
	catch(const std::exception& e)
	{
		;
	}

	if (console["EnableWebUI"])
	{
		try {
			const std::string port_option = std::string(AllowInternetAccess ? "0.0.0.0" : "127.0.0.1") + ":" + std::to_string(Port);

			std::vector<std::string> mg_options = {"listening_ports", port_option.c_str()};

			ManagerServer = std::make_unique<CivetServer>(mg_options);

			ManagerServer->addHandler("/", h_index);
			ManagerServer->addHandler("/api/basicinfo", h_basicinfoapi);
			ManagerServer->addHandler("/api/custommsg", h_msgapi);
			ManagerServer->addHandler("/api/adminconfig", h_config);
			ManagerServer->addHandler("/api/master", h_master);
			ManagerServer->addHandler("/api/customreply", h_customreply);
			ManagerServer->addHandler("/api/webuipassword", h_webuipassword);
			ManagerServer->addAuthHandler("/", auth_handler);
			auto ports = ManagerServer->getListeningPorts();

			if (ports.empty())
			{
				DD::debugLog("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�");
				console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
			}
			else
			{
				DD::debugLog("Dice! WebUI ���ڶ˿�" + std::to_string(ports[0]) + "����");
				console.log("Dice! WebUI ���ڶ˿�" + std::to_string(ports[0]) 
					+ "���У����ؿ�ͨ�����������localhost:" + std::to_string(ports[0])
					+ "\nĬ���û���Ϊadmin����Ϊpassword����ϸ�̳���鿴 https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming", 0b1);
			}
		} 
		catch(const CivetException& e)
		{
			DD::debugLog("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�");
			console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
		}
	}
	//��������
	getDiceList();
	getExceptGroup();
	DD::debugLog("Dice.threadInit");
	Enabled = true;
	threads(SendMsg);
	threads(ConsoleTimer);
	threads(warningHandler);
	threads(frqHandler);
	sch.start();

	console.log(getMsg("strSelfName") + "��ʼ����ɣ���ʱ" + to_string(time(nullptr) - llStartTime) + "��", 0b1,
		printSTNow());
	llStartTime = time(nullptr);

	DD::debugLog("Dice.extensionManagerInit");
	try
	{
		ExtensionManagerInstance->refreshIndex();
		console.log("�ѳɹ�ˢ����������棬" + to_string(ExtensionManagerInstance->getIndexCount()) + "����չ���ã�"
			+ to_string(ExtensionManagerInstance->getUpgradableCount()) + "��������", 0b1);
	}
	catch (const std::exception& e)
	{
		console.log(std::string("ˢ�����������ʧ�ܣ�") + e.what(), 0);
	}
	isIniting.clear();
}

mutex GroupAddMutex;

bool eve_GroupAdd(Chat& grp)
{
	{
		unique_lock<std::mutex> lock_queue(GroupAddMutex);
		if (!grp.lastmsg(time(nullptr)).isset("����Ⱥ"))grp.set("����Ⱥ").reset("δ��").reset("����");
		else return false;
		if (ChatList.size() == 1 && !console.isMasterMode)DD::sendGroupMsg(grp.ID, msgInit);
	}
	long long fromGID = grp.ID;
	if (grp.Name.empty())
		grp.Name = DD::getGroupName(fromGID);
	GroupSize_t gsize(DD::getGroupSize(fromGID));
	if (console["GroupInvalidSize"] > 0 && grp.confs.empty() && gsize.currSize > (size_t)console["GroupInvalidSize"]) {
		grp.set("Э����Ч");
	}
	if (!console["ListenGroupAdd"] || grp.isset("����"))return 0;
	string strNow = printSTNow();
	string strMsg(getMsg("strSelfName"));
	try 
	{
		strMsg += "�¼���:" + DD::printGroupInfo(grp.ID);
		if (blacklist->get_group_danger(fromGID)) 
		{
			grp.leave(blacklist->list_group_warning(fromGID));
			strMsg += "Ϊ������Ⱥ������Ⱥ";
			console.log(strMsg, 0b10, printSTNow());
			return true;
		}
		if (grp.isset("���ʹ��"))strMsg += "���ѻ�ʹ����ɣ�";
		else if(grp.isset("Э����Ч"))strMsg += "��Ĭ��Э����Ч��";
		if (grp.inviter) {
			strMsg += ",������" + printUser(chat(fromGID).inviter);
		}
		int max_trust = 0;
		float ave_trust(0);
		//int max_danger = 0;
		long long ownerQQ = 0;
		ResList blacks;
		std::set<long long> list = DD::getGroupMemberList(fromGID);
		if (list.empty()){
			strMsg += "��ȺԱ����δ���أ�";
		}
		else 
		{
			size_t cntUser{ 0 }, cntMember{ 0 }, cntDiceMaid{ 0 };
			for (auto& each : list) 
			{
				if (each == console.DiceMaid)continue;
				cntMember++;
				if (UserList.count(each)) 
				{
					cntUser++;
					ave_trust += getUser(each).nTrust;
				}
				if (DD::isGroupAdmin(fromGID, each, false))
				{
					max_trust |= (1 << trustedQQ(each));
					if (blacklist->get_qq_danger(each) > 1)
					{
						strMsg += ",���ֺ���������Ա" + printUser(each);
						if (grp.isset("���"))
						{
							strMsg += "��Ⱥ��ڣ�";
						}
						else
						{
							DD::sendGroupMsg(fromGID, blacklist->list_qq_warning(each));
							grp.leave("���ֺ���������Ա" + printUser(each) + "��Ԥ������Ⱥ");
							strMsg += "������Ⱥ";
							console.log(strMsg, 0b10, strNow);
							return true;
						}
					}
					if (DD::isGroupOwner(fromGID, each, false))
					{
						ownerQQ = each;
						ave_trust += (gsize.currSize - 1) * trustedQQ(each);
						strMsg += "��Ⱥ��" + printUser(each) + "��";
					}
					else
					{
						ave_trust += (gsize.currSize - 10) * trustedQQ(each) / 10;
					}
				}
				else if (blacklist->get_qq_danger(each) > 1)
				{
					//max_trust |= 1;
					blacks << printUser(each);
					if (blacklist->get_qq_danger(each)) 
					{
						AddMsgToQueue(blacklist->list_self_qq_warning(each), { 0,fromGID });
					}
				}
				if (DD::isDiceMaid(each)) {
					++cntDiceMaid;
				}
			}
			if (!chat(fromGID).inviter && list.size() == 2 && ownerQQ)
			{
				chat(fromGID).inviter = ownerQQ;
				strMsg += "������" + printUser(ownerQQ);
			}
			if (!cntMember) 
			{
				strMsg += "��ȺԱ����δ���أ�";
			}
			else 
			{
				ave_trust /= cntMember;
				strMsg += "\n�û�Ũ��" + to_string(cntUser * 100 / cntMember) + "% (" + to_string(cntUser) + "/" + to_string(cntMember) + "), ���ζ�" + toString(ave_trust);
			}
			if (cntDiceMaid) 			{
				strMsg += "\n��ʶ��ͬϵDice!" + to_string(cntDiceMaid) + "λ";
			}
		}
		if (!blacks.empty())
		{
			string strNote = "\n���ֺ�����ȺԱ" + blacks.show();
			strMsg += strNote;
		}
		if (console["Private"] && !grp.isset("���ʹ��"))
		{	
			//����СȺ�ƹ�����û���ϰ�����
			if (max_trust > 1 || ave_trust > 0.5)
			{
				grp.set("���ʹ��");
				strMsg += "\n���Զ�׷��ʹ�����";
			}
			else if(!grp.isset("Э����Ч"))
			{
				strMsg += "\n�����ʹ�ã�����Ⱥ";
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
		console.log(strMsg + "\nȺ" + to_string(fromGID) + "��Ϣ��ȡʧ�ܣ�", 0b1, printSTNow());
		return true;
	}
	if (grp.isset("Э����Ч")) return 0;
	if (!getMsg("strAddGroup").empty())
	{
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strAddGroup"), { 0,fromGID, 0 });
	}
	if (console["CheckGroupLicense"] && !grp.isset("���ʹ��"))
	{
		grp.set("δ���");
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strGroupLicenseDeny"), { 0,fromGID, 0 });
	}
	return false;
}

//��������ָ��

EVE_PrivateMsg(eventPrivateMsg)
{
	if (!Enabled)return 0;
	if (fromUID == console.DiceMaid && !console["ListenSelfEcho"])return 0;
	else if (console["DisableStrangerChat"] && !DD::isFriend(fromUID, true))return 0;
	shared_ptr<FromMsg> Msg(make_shared<FromMsg>(
		AttrVars({ { "Event", "Message" },
			{ "fromMsg", message },
			{ "uid", fromUID },
			}), chatInfo{ fromUID,0,0 }));
	return Msg->DiceFilter();
}

EVE_GroupMsg(eventGroupMsg)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID).group().lastmsg(time(nullptr));
	if (fromUID == console.DiceMaid && !console["ListenGroupEcho"])return 0;
	if (!grp.isset("����Ⱥ") && (grp.isset("δ��") || grp.isset("����")))eve_GroupAdd(grp);
	if (!grp.isset("����"))
	{
		shared_ptr<FromMsg> Msg(make_shared<FromMsg>(
			AttrVars({ { "Event", "Message" },
				{ "fromMsg", message },
				{ "msgid",msgId },
				{ "uid",fromUID },
				{ "gid",fromGID }
				}), chatInfo{ fromUID,fromGID,0 }));
		return Msg->DiceFilter();
	}
	return grp.isset("������Ϣ");
}
EVE_ChannelMsg(eventChannelMsg)
{
	if (!Enabled)return 0;
	//Chat& grp = chat(fromGID).group().lastmsg(time(nullptr));
	if (fromUID == console.DiceMaid && !console["ListenChannelEcho"])return 0;
	//if (!grp.isset("����"))
	{
		shared_ptr<FromMsg> Msg(make_shared<FromMsg>(
			AttrVars({ { "Event", AttrVar("Message")},
				{ "fromMsg", message },
				{ "msgid", msgId },
				{ "uid", fromUID },
				{ "gid", fromGID },
				{ "chid", fromChID }
				}), chatInfo{ fromUID,fromGID,fromChID }));
		return Msg->DiceFilter();
	}
	//return grp.isset("������Ϣ");
	return false;
}

EVE_DiscussMsg(eventDiscussMsg)
{
	if (!Enabled)return 0;
	//time_t tNow = time(NULL);
	if (console["LeaveDiscuss"])
	{
		DD::sendDiscussMsg(fromDiscuss, getMsg("strLeaveDiscuss"));
		this_thread::sleep_for(1000ms);
		DD::setDiscussLeave(fromDiscuss);
		return 1;
	}
	Chat& grp = chat(fromDiscuss).discuss().lastmsg(time(nullptr));
	if (blacklist->get_qq_danger(fromUID) && console["AutoClearBlack"])
	{
		const string strMsg = "���ֺ������û�" + printUser(fromUID) + "���Զ�ִ����Ⱥ";
		console.log(printChat(grp) + strMsg, 0b10, printSTNow());
		grp.leave(strMsg);
		return 1;
	}
	shared_ptr<FromMsg> Msg(make_shared<FromMsg>(
		AttrVars({ { "Event", "Message"},
			{ "fromMsg", message },
			{ "uid", fromUID },
			{ "gid", fromDiscuss }
			}), chatInfo{ fromUID,fromDiscuss,0 }));
	return Msg->DiceFilter() || grp.isset("������Ϣ");
}

EVE_GroupMemberIncrease(eventGroupMemberAdd)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID);
	if (grp.isset("����"))return 0;
	if (fromUID != console.DiceMaid)
	{
		if (chat(fromGID).confs.count("��Ⱥ��ӭ"))
		{
			string strReply = chat(fromGID).confs["��Ⱥ��ӭ"].to_str();
			while (strReply.find("{at}") != string::npos)
			{
				strReply.replace(strReply.find("{at}"), 4, "[CQ:at,id=" + to_string(fromUID) + "]");
			}
			while (strReply.find("{@}") != string::npos)
			{
				strReply.replace(strReply.find("{@}"), 3, "[CQ:at,id=" + to_string(fromUID) + "]");
			}
			while (strReply.find("{nick}") != string::npos)
			{
				strReply.replace(strReply.find("{nick}"), 6, DD::getQQNick(fromUID));
			}
			while (strReply.find("{qq}") != string::npos)
			{
				strReply.replace(strReply.find("{qq}"), 4, to_string(fromUID));
			}
			grp.update(time(nullptr));
			AddMsgToQueue(strip(strReply), { 0,fromGID });
		}
		if (blacklist->get_qq_danger(fromUID))
		{
			const string strNow = printSTNow();
			string strNote = printGroup(fromGID) + "����" + getMsg("strSelfName") + "�ĺ������û�" + printUser(
				fromUID) + "��Ⱥ";
			AddMsgToQueue(blacklist->list_self_qq_warning(fromUID), { 0,fromGID });
			if (grp.isset("����"))strNote += "��Ⱥ���壩";
			else if (grp.isset("���"))strNote += "��Ⱥ��ڣ�";
			else if (grp.isset("Э����Ч"))strNote += "��ȺЭ����Ч��";
			else if (DD::isGroupAdmin(fromGID, console.DiceMaid, false))strNote += "��Ⱥ����Ȩ�ޣ�";
			else if (console["LeaveBlackQQ"])
			{
				strNote += "������Ⱥ��";
				grp.leave("���ֺ������û�" + printUser(fromUID) + "��Ⱥ,��Ԥ������Ⱥ");
			}
			console.log(strNote, 0b10, strNow);
		}
	}
	else{
		if (!grp.inviter)grp.inviter = operatorQQ;
		{
			unique_lock<std::mutex> lock_queue(GroupAddMutex);
			if (grp.isset("����Ⱥ"))return 0;
		}
		return eve_GroupAdd(grp);
	}
	return 0;
}

EVE_GroupMemberKicked(eventGroupMemberKicked){
	if (!Enabled) return 0;
	if (fromUID == 0)return 0; // ����Mirai�ڻ�����������ȺʱҲ�����һ���������
	Chat& grp = chat(fromGID);
	if (beingOperateQQ == console.DiceMaid)
	{
		grp.set("����").reset("����Ⱥ");
		if (!console || grp.isset("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "��" + printUser(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (!console["ListenGroupKick"] || trustedQQ(fromUID) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGID)) return 0;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.sign().type("kick").time(strNow).note(strNow + " " + strNote).comment(getMsg("strSelfCall") + "ԭ����¼");
		if (grp.inviter && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";��Ⱥ�����ߣ�" + printUser(grp.inviter);
			if (console["KickedBanInviter"])mark.inviterQQ(grp.inviter).note(strNow + " " + strNote);
		}
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	else if (mDiceList.count(beingOperateQQ) && console["ListenGroupKick"])
	{
		if (!console || grp.isset("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "��" + printUser(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (trustedQQ(fromUID) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGID)) return 0;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.type("kick").time(strNow).note(strNow + " " + strNote).DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "Ŀ��");
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	return 0;
}

EVE_GroupBan(eventGroupBan)
{
	if (!Enabled) return 0;
	Chat& grp = chat(fromGID);
	if (grp.isset("����") || (beingOperateQQ != console.DiceMaid && !mDiceList.count(beingOperateQQ)) || !console[
		"ListenGroupBan"])return 0;
	if (!duration || !duration[0])
	{
		if (beingOperateQQ == console.DiceMaid)
		{
			console.log(getMsg("strSelfName") + "��" + printGroup(fromGID) + "�б��������", 0b10, printSTNow());
			return 1;
		}
	}
	else
	{
		string strNow = printSTNow();
		long long llOwner = 0;
		string strNote = "��" + printGroup(fromGID) + "��," + printUser(beingOperateQQ) + "��" + printUser(operatorQQ) + "����" + duration;
		if (!console["ListenGroupBan"] || trustedQQ(operatorQQ) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGID)) 
		{
			console.log(strNote, 0b10, strNow);
			return 1;
		}
		DDBlackMarkFactory mark{operatorQQ, fromGID};
		mark.type("ban").time(strNow).note(strNow + " " + strNote);
		if (mDiceList.count(operatorQQ))mark.fromUID(0);
		if (beingOperateQQ == console.DiceMaid)
		{
			if (!console)return 0;
			mark.sign().comment(getMsg("strSelfCall") + "ԭ����¼");
		}
		else
		{
			mark.DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "Ŀ��");
		}
		//ͳ��Ⱥ�ڹ���
		int intAuthCnt = 0;
		string strAuthList;
		for (auto admin : DD::getGroupAdminList(fromGID))
		{
			if (DD::isGroupOwner(fromGID, admin, false)) {
				llOwner = admin;
			}
			else 
			{
				strAuthList += '\n' + printUser(admin);
				intAuthCnt++;
			}
		}
		strAuthList = "��Ⱥ��" + printUser(llOwner) + ",���й���Ա" + to_string(intAuthCnt) + "��" + strAuthList;
		mark.note(strNow + " " + strNote);
		if (grp.inviter && beingOperateQQ == console.DiceMaid && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";��Ⱥ�����ߣ�" + printUser(grp.inviter);
			if (console["BannedBanInviter"])mark.inviterQQ(grp.inviter);
			mark.note(strNow + " " + strNote);
		}
		grp.reset("����").reset("���ʹ��").leave();
		console.log(strNote + strAuthList, 0b1000, strNow);
		blacklist->create(mark.product());
		return 1;
	}
	return 0;
}

EVE_GroupInvited(eventGroupInvited)
{
	if (!Enabled) return 0;
	if (!console["ListenGroupRequest"])return 0;
	if (groupset(fromGID, "����") < 1)
	{
		this_thread::sleep_for(3s);
		const string strNow = printSTNow();
		string strMsg = "Ⱥ����������ԣ�" + printUser(fromUID) + ",Ⱥ:" +
			DD::printGroupInfo(fromGID);
		if (ExceptGroups.count(fromGID)) {
			strMsg += "\n�Ѻ��ԣ�Ĭ��Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (blacklist->get_group_danger(fromGID))
		{
			strMsg += "\n�Ѿܾ���Ⱥ�ں������У�";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (blacklist->get_qq_danger(fromUID))
		{
			strMsg += "\n�Ѿܾ����û��ں������У�";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (Chat& grp = chat(fromGID).group(); grp.isset("���ʹ��")) {
			grp.set("δ��");
			grp.inviter = fromUID;
			strMsg += "\n��ͬ�⣨Ⱥ�����ʹ�ã�";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (trustedQQ(fromUID))
		{
			grp.set("���ʹ��").set("δ��").reset("δ���").reset("Э����Ч");
			grp.inviter = fromUID;
			strMsg += "\n��ͬ�⣨�������û���";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (grp.isset("Э����Ч")) {
			grp.set("δ��");
			strMsg += "\n�Ѻ��ԣ�Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console["GroupInvalidSize"] > 0 && DD::getGroupSize(grp.ID).currSize > (size_t)console["GroupInvalidSize"]) {
			grp.set("Э����Ч");
			strMsg += "\n�Ѻ��ԣ���ȺĬ��Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console && console["Private"])
		{
			AddMsgToQueue(getMsg("strPreserve"), fromUID);
			strMsg += "\n�Ѿܾ�����ǰ��˽��ģʽ��";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else
		{
			grp.set("δ��");
			grp.inviter = fromUID;
			strMsg += "��ͬ��";
			this_thread::sleep_for(2s);
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, true);
		}
		return 1;
	}
	return 0;
}
EVE_FriendRequest(eventFriendRequest) {
	if (!Enabled) return 0;
	if (!console["ListenFriendRequest"])return 0;
	string strMsg = "��������������� " + printUser(fromUID) + ":" + message;
	this_thread::sleep_for(3s);
	if (blacklist->get_qq_danger(fromUID)) {
		strMsg += "\n�Ѿܾ����û��ں������У�";
		DD::answerFriendRequest(fromUID, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromUID)) {
		strMsg += "\n��ͬ�⣨�������û���";
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriendWhiteQQ"));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 2 && !UserList.count(fromUID)) {
		strMsg += "\n�Ѿܾ������û���¼��";
		DD::answerFriendRequest(fromUID, 2, getMsg("strFriendDenyNotUser"));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 1) {
		strMsg += "\n�Ѿܾ����������û���";
		DD::answerFriendRequest(fromUID, 2, getMsg("strFriendDenyNoTrust"));
		console.log(strMsg, 1, printSTNow());
	}
	else {
		strMsg += "\n��ͬ��";
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriend"));
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}
EVE_FriendAdded(eventFriendAdd) {
	if (!Enabled) return 0;
	if (!console["ListenFriendAdd"])return 0;
	this_thread::sleep_for(3s);
	(trustedQQ(fromUID) > 0 && !getMsg("strAddFriendWhiteQQ").empty())
		? AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromUID)
		: AddMsgToQueue(getMsg("strAddFriend"), fromUID);
	return 0;
}

EVE_Menu(eventMasterMode)
{
	if (!Enabled) return 0;
	if (console)
	{
		console.isMasterMode = false;
		console.killMaster();
#ifdef _WIN32
		MessageBoxA(nullptr, "Masterģʽ�ѹرա�\nmaster�����", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else
	{
		console.isMasterMode = true;
		console.save();
#ifdef _WIN32
		MessageBoxA(nullptr, "Masterģʽ�ѿ�����\n����������﷢��.master public/private", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
#endif
	}
	return 0;
}

#ifdef _WIN32
#include <shellapi.h>
EVE_Menu(eventGUI)
{
	if (!Enabled) return 0;
	auto port{ ManagerServer->getListeningPorts()[0] };
	ShellExecute(nullptr, TEXT("open"), (TEXT("http://127.0.0.1:") + std::to_wstring(port)).c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
	return 1;
}
#endif

void global_exit() {
	Enabled = false;
	mg_exit_library();
	ManagerServer = nullptr;
	ExtensionManagerInstance = nullptr;
	dataBackUp();
	sch.end();
	censor = {};
	fmt.reset();
	gm.reset();
	PList.clear();
	ChatList.clear();
	UserList.clear();
	console.reset();
	EditedMsg.clear();
	blacklist.reset();
	Aws::ShutdownAPI(options);
#ifndef _WIN32
	curl_global_cleanup();
#endif
	threads.exit();
}

EVE_Disable(eventDisable)
{
	global_exit();
}

EVE_Exit(eventExit)
{
	if (Enabled)global_exit();
}

EVE_Menu(eventGlobalSwitch) {
	if (!Enabled) return 0;
	if (console["DisabledGlobal"]) {
		console.set("DisabledGlobal", 0);
#ifdef _WIN32
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else {
		console.set("DisabledGlobal", 1);
#ifdef _WIN32
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
#endif
	}

	return 0;
}