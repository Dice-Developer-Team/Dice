/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
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
#include "DiceCensor.h"
#include "EncodingConvert.h"
#include "DiceManager.h"
#include "DiceSelfData.h"

#ifdef _WIN32
#include "S3PutObject.h"
#include "resource.h"
#else
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#pragma warning(disable:4996)
#pragma warning(disable:6031)

using namespace std;

unordered_map<long long, User> UserList{};
unordered_map<long long, long long> TinyList{}; 
unordered_map<long long, Chat> ChatList;
ThreadFactory threads;
std::filesystem::path fpFileLoc;
std::unique_ptr<CivetServer> ManagerServer;
BasicInfoApiHandler h_basicinfoapi;
CustomMsgApiHandler h_msgapi;
AdminConfigHandler h_config;
MasterHandler h_master;
CustomReplyApiHandler h_customreply;
ModListApiHandler h_modlist;
WebUIPasswordHandler h_webuipassword;
AuthHandler auth_handler;

string msgInit;

//加载数据
void loadData(){
	ResList logList;
	try	{
		std::error_code ec;
		std::filesystem::create_directory(DiceDir, ec);
		loadDir(loadXML<CardTemp, fifo_cmpr_ci>, DiceDir / "CardTemp", CharaCard::mCardTemplet, logList, true);
		loadDir(loadJMap, DiceDir / "PublicDeck", CardDeck::mExternPublicDeck, logList);
		map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
		//读取帮助文档
		fmt->clear();
		fmt->load(logList);
		//读取敏感词库
		loadDir(load_words, DiceDir / "conf" / "censor", censor, logList, true);
		loadJMap(DiceDir / "conf" / "CustomCensor.json", censor.CustomWords);
		censor.build();
		if (!logList.empty())
		{
			logList << "扩展配置读取完毕√";
			console.log(logList.show(), 1, printSTNow());
		}
	}
	catch (const std::exception& e)
	{
		logList << "读取数据时遇到意外错误，程序可能无法正常运行。请尝试清空配置后重试。" << e.what();
		console.log(logList.show(), 1, printSTNow());
	}
}

//初始化用户数据
void readUserData(){
	std::error_code ec;
	fs::path dir{ DiceDir / "user" };
	ResList log;
	try {
		//读取用户记录
		if (int cnt{ loadBFile(dir / "UserConf.dat", UserList) }; cnt > 0) {
			fs::copy(dir / "UserConf.dat", dir / "UserConf.bak",
				fs::copy_options::overwrite_existing, ec);
			log << "读取用户记录" + to_string(cnt) + "条";
		}
		else if (fs::exists(dir / "UserConf.bak")) {
			cnt = loadBFile(dir / "UserConf.bak", UserList);
			if (cnt > 0)log << "恢复用户记录" + to_string(cnt) + "条";
		}
		else {
			cnt = loadBFile<long long, User, &User::old_readb>(dir / "UserConf.RDconf", UserList);
			loadFile(dir / "UserList.txt", UserList);
			if (cnt > 0)log << "迁移用户记录" + to_string(cnt) + "条";
		}
		//for QQ Channel
		if (User& self{ getUser(console.DiceMaid) }; !self.confs.get_ll("tinyID")) {
			if (long long tiny{ DD::getTinyID() }) {
				DD::debugMsg("获取分身ID:" + to_string(tiny));
				self.setConf("tinyID", tiny);
			}
		}
	}
	catch (const std::exception& e) {
		console.log(string("读取用户记录时遇到意外错误，请尝试删除UserConf.dat启用备份.bak文件!")
			+ e.what(), 0b1000, printSTNow());
	}
	try {
		//读取角色记录
		if (int cnt{ loadBFile(dir / "PlayerCards.RDconf", PList) }; cnt > 0) {
			fs::copy(dir / "PlayerCards.RDconf", dir / "PlayerCards.bak",
				fs::copy_options::overwrite_existing, ec);
			log << "读取玩家记录" + to_string(cnt) + "条";
		}
		else if (fs::exists(dir / "PlayerCards.bak")) {
			cnt = loadBFile(dir / "PlayerCards.bak", PList);
			if (cnt > 0)log << "恢复玩家记录" + to_string(cnt) + "条";
		}
		for (const auto& pl : PList) {
			if (!UserList.count(pl.first))getUser(pl.first);
		}
	}
	catch (const std::exception& e)	{
		console.log("读取玩家记录时遇到意外错误，请尝试删除PlayerCards.RDconf启用备份.bak文件!"
			+ string(e.what()), 0b1000, printSTNow());
	}
	try {
		//读取群聊记录
		if (int cnt{ loadBFile(dir / "ChatConf.dat", ChatList) }; cnt > 0) {
			fs::copy(dir / "ChatConf.dat", dir / "ChatConf.bak",
				fs::copy_options::overwrite_existing, ec);
			log << "读取群聊记录" + to_string(cnt) + "条";
		}
		else if (fs::exists(dir / "ChatConf.bak")) {
			cnt = loadBFile(dir / "ChatConf.bak", ChatList);
			if (cnt > 0)log << "恢复群聊记录" + to_string(cnt) + "条";
		}
		else {
			cnt = loadBFile(dir / "ChatConf.RDconf", ChatList);
			loadFile(dir / "ChatList.txt", ChatList);
			if (cnt > 0)log << "迁移群聊记录" + to_string(cnt) + "条";
		}
	}
	catch (const std::exception& e)	{
		console.log("读取群聊记录时遇到意外错误，请尝试删除ChatConf.dat启用备份.bak文件!"
			+ string(e.what()), 0b1000, printSTNow());
	}
	//读取房间记录
	sessions.load();
	if (!log.empty()) {
		log << "用户数据读取完毕";
		console.log(log.show(), 0b1, printSTNow());
	}
}

//备份数据
void dataBackUp()
{
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	//备份列表
	saveBFile(DiceDir / "user" / "UserConf.dat", UserList);
	saveBFile(DiceDir / "user" / "PlayerCards.RDconf", PList);
	saveBFile(DiceDir / "user" / "ChatConf.dat", ChatList);
}

atomic_flag isIniting = ATOMIC_FLAG_INIT;
EVE_Enable(eventEnable){
	if (isIniting.test_and_set())return;
	if (Enabled)return;
	clock_t clockStart = clock();
#ifndef _WIN32
	CURLcode err;
	err = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (err != CURLE_OK)
	{
		console.log("错误: 加载libcurl失败！", 1);
	}
#else
	Aws::InitAPI(options);
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

	console.DiceMaid = DD::getLoginID();
	DiceDir = dirExe / ("Dice" + to_string(console.DiceMaid));
	if (!exists(DiceDir)) {
		filesystem::path DiceDirOld(dirExe / "DiceData");
		if (exists(DiceDirOld))rename(DiceDirOld, DiceDir);
		else filesystem::create_directory(DiceDir);
	}
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	std::filesystem::create_directory(DiceDir / "mod", ec);
	std::filesystem::create_directory(DiceDir / "plugin", ec);
	fpFileLoc = DiceDir / "com.w4123.dice";
	{
		std::unique_lock lock(GlobalMsgMutex);
		string& strSelfName{ GlobalMsg["strSelfName"] = DD::getLoginNick() };
		if (strSelfName.empty()){
			strSelfName = "骰娘[" + toString(console.DiceMaid % 10000, 4) + "]";
		}
	}
	if (!console.load()){
		console.setClock({ 11, 45 }, "clear");
		console.loadNotice();
	}
	if (!console) {
		msgInit = R"(欢迎使用Dice!掷骰机器人！
请发送.master )"
+ (console.authkey_pub = RandomGenerator::genKey(8)) + " //公骰作成 或\n.master "
+ (console.authkey_pri = RandomGenerator::genKey(8)) + 
R"( //私骰作成 即可成为我的主人~
可发送.help查看帮助
发送.system gui开启DiceMaid后台面板
参考文档参看.help链接)";
		DD::debugMsg(msgInit);
	}
	DD::debugLog("DiceConsole.load");
	fmt = make_unique<DiceModManager>();
	try {
		std::unique_lock lock(GlobalMsgMutex);
		if (loadJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg) >= 0
			|| loadJMap(fpFileLoc / "CustomMsg.json", EditedMsg) > 0) {
			map_merge(GlobalMsg, EditedMsg);
		}
	}
	catch (const std::exception& e) {
		console.log(string("读取/conf/CustomMsg.json失败!") + e.what(), 1, printSTNow());
	}
	loadData();
	//初始化黑名单
	blacklist = make_unique<DDBlackManager>();
	if (auto cnt = blacklist->loadJson(DiceDir / "conf" / "BlackList.json");cnt < 0)
	{
		blacklist->loadJson(fpFileLoc / "BlackMarks.json");
		cnt = blacklist->loadHistory(fpFileLoc);
		if (cnt) {
			blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
			console.log("初始化不良记录" + to_string(cnt) + "条", 1);
		}
	}
	else {
		DD::debugLog("读取不良记录" + to_string(cnt) + "条");
		if ((cnt = blacklist->loadJson(DiceDir / "conf" / "BlackListEx.json", true)) > 0)
			DD::debugLog("读取外源不良记录" + to_string(cnt) + "条");
	}
	//selfdata
	if (const auto dirSelfData{ DiceDir / "selfdata" }; std::filesystem::exists(dirSelfData)) {
		std::error_code err;
		for (const auto& file : std::filesystem::directory_iterator(dirSelfData, err)) {
			if (file.is_regular_file()) {
				const auto p{ file.path() };
				auto& data{ selfdata_byFile[getNativePathString(p.filename())]
					= make_shared<SelfData>(p) };
				if (string file{ UTF8toGBK(p.stem().u8string())}; !selfdata_byStem.count(file)) {
					selfdata_byStem[file] = data;
				}
			}
		}
		DD::debugLog("预加载selfdata" + to_string(selfdata_byStem.size()) + "条");
	}
	//读取用户数据
	readUserData();
	//读取当日数据
	today = make_unique<DiceToday>();
	set<long long> grps{ DD::getGroupIDList() };
	for (auto gid : grps){
		if (auto grp{ chat(gid).group().reset("未进").reset("已退").set("已入群") };
			!grp.isset("lastMsg"))grp.setLst(-1);
	}
	// 确保线程执行结束
	while (msgSendThreadRunning)this_thread::sleep_for(10ms);

	DD::debugLog("Dice.webUIInit");
	WebUIPasswordPath = DiceDir / "conf" / "WebUIPassword.digest";
	if (!std::filesystem::exists(WebUIPasswordPath, ec) || std::filesystem::is_empty(WebUIPasswordPath, ec))
	{		
		setPassword("password");
	}

	// 初始化服务器
	mg_init_library(0);

	// 读取环境变量
	bool AllowInternetAccess = console["WebUIAllowInternetAccess"];
	int Port = console["WebUIPort"];

	const char* envAllowInternetAccess = std::getenv("DICE_WEBUI_ALLOW_INTERNET_ACCESS");
	std::string envStrAllowInternetAccess = envAllowInternetAccess ? envAllowInternetAccess : "";

	if (envStrAllowInternetAccess == "1") {
		AllowInternetAccess = true;
	}
	else if (envStrAllowInternetAccess == "0") {
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
	ExtensionManagerInstance = std::make_unique<ExtensionManager>();

	DD::debugLog("Dice.threadInit");
	Enabled = true;
	threads(SendMsg);
	threads(ConsoleTimer);
	threads(warningHandler);
	threads(frqHandler);
	sch.start();

	console.log(getMsg("strSelfName") + "初始化完成，用时" + to_string(clock() - clockStart) + "毫秒", 0b1,
		printSTNow());
	llStartTime = time(nullptr);
	DD::debugLog("Dice.WebInit");
	if (console["EnableWebUI"]) {
		try {
			const std::string port_option = std::string(AllowInternetAccess ? "0.0.0.0" : "127.0.0.1") + ":" + std::to_string(Port);

			std::vector<std::string> mg_options = { "document_root", (DiceDir / "webui").u8string(), "listening_ports", port_option.c_str() };

			ManagerServer = std::make_unique<CivetServer>(mg_options);
#ifdef _WIN32
			if (HRSRC hRsrcInfo = FindResource(hDllModule, MAKEINTRESOURCE(ID_ADMIN_HTML), TEXT("FILE"))) {
				DWORD dwSize = SizeofResource(hDllModule, hRsrcInfo);
				if (HGLOBAL hGlobal = LoadResource(hDllModule, hRsrcInfo)) {
					LPVOID pBuffer = LockResource(hGlobal);  // 锁定资源
					char* pByte = new char[dwSize + 1];
					fs::create_directories(DiceDir / "webui");
					ofstream fweb{ DiceDir / "webui" / "index.html" };
					fweb.write((const char*)pBuffer, dwSize);
					FreeResource(hGlobal);// 释放资源
				}
			}
#else
			if (string html; Network::GET("https://raw.sevencdn.com/Dice-Developer-Team/Dice/newdev/Dice/webui.html", html)) {
				ofstream fweb{ DiceDir / "webui" / "index.html" };
				fweb.write(html.c_str(), html.length());
			}
			else if (!fs::exists(DiceDir / "webui" / "index.html")) {
				console.log("获取webui页面失败!相关功能无法使用!", 0b10);
			}
#endif
			ManagerServer->addHandler("/api/basicinfo", h_basicinfoapi);
			ManagerServer->addHandler("/api/custommsg", h_msgapi);
			ManagerServer->addHandler("/api/adminconfig", h_config);
			ManagerServer->addHandler("/api/master", h_master);
			ManagerServer->addHandler("/api/customreply", h_customreply);
			ManagerServer->addHandler("/api/mod/list", h_modlist);
			ManagerServer->addHandler("/api/webuipassword", h_webuipassword);
			ManagerServer->addAuthHandler("/", auth_handler);
			auto ports = ManagerServer->getListeningPorts();

			if (ports.empty())
			{
				console.log("Dice! WebUI 启动失败！端口已被使用？", 0b1);
			}
			else
			{
				console.log("Dice! WebUI 正于端口" + std::to_string(ports[0])
					+ "运行，本地可通过浏览器访问http://localhost:" + std::to_string(ports[0])
					+ "\n默认用户名为admin密码为password，详细教程请查看 https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming", 0b1);
			}
		}
		catch (const CivetException& e)
		{
			console.log("Dice! WebUI 启动失败！端口已被使用？", 0b1);
		}
	}
	//骰娘网络
	getDiceList();
	getExceptGroup();
	try
	{
		ExtensionManagerInstance->refreshIndex();
		if (auto cnt{ ExtensionManagerInstance->getUpgradableCount() }) {
			console.log("已成功刷新软件包缓存：" + to_string(ExtensionManagerInstance->getIndexCount()) + "个拓展可用，"
				+ to_string(cnt) + "个可升级", 0b1);
		}
		else {
			console.log("已成功刷新软件包缓存：" + to_string(ExtensionManagerInstance->getIndexCount()) + "个拓展可用", 0);
		}
	}
	catch (const std::exception& e)
	{
		console.log(std::string("刷新软件包缓存失败：") + e.what(), 0);
	}
	fmt->call_hook_event(AttrVars{ {{"Event","StartUp"}} });
}

mutex GroupAddMutex;
bool eve_GroupAdd(Chat& grp) {
	{
		unique_lock<std::mutex> lock_queue(GroupAddMutex);
		if (!grp.isset("已入群"))grp.set("已入群").reset("未进").reset("已退");
		else return false;
		if (ChatList.size() == 1 && !console)DD::sendGroupMsg(grp.ID, msgInit);
	}
	long long fromGID = grp.ID;
	if (grp.Name.empty())
		grp.set("Name", grp.Name = DD::getGroupName(fromGID));
	GroupSize_t gsize(DD::getGroupSize(fromGID));
	if (console["GroupInvalidSize"] > 0 && grp.confs.empty() && gsize.currSize > (size_t)console["GroupInvalidSize"]) {
		grp.set("协议无效");
	}
	if (!console["ListenGroupAdd"] || grp.isset("忽略"))return 0;
	string strNow = printSTNow();
	string strMsg(getMsg("strSelfName"));
	AttrObject eve{ {
		{"Event","GroupAdd"},
		{"gid",fromGID},
	} };
	try 
	{
		strMsg += "新加入:" + DD::printGroupInfo(grp.ID);
		if (blacklist->get_group_danger(fromGID)) 
		{
			grp.leave(blacklist->list_group_warning(fromGID));
			strMsg += "为黑名单群，已退群";
			console.log(strMsg, 0b10, printSTNow());
			return true;
		}
		if (grp.isset("许可使用"))strMsg += "（已获使用许可）";
		else if(grp.isset("协议无效"))strMsg += "（已标记协议无效）";
		if (grp.inviter) {
			strMsg += ",邀请者" + printUser(grp.inviter);
		}
		int max_trust = 0;
		float ave_trust(0);
		//int max_danger = 0;
		long long ownerQQ = 0;
		ResList blacks;
		std::set<long long> list = DD::getGroupMemberList(fromGID);
		if (list.empty()){
			strMsg += "，群员名单未加载；";
		}
		else 
		{
			size_t cntUser{ 0 }, cntMember{ 0 }, cntDiceMaid{ 0 };
			for (auto& each : list) 
			{
				if (each == console.DiceMaid)continue;
				cntMember++;
				if (UserList.count(each)) {
					cntUser++;
					ave_trust += getUser(each).nTrust;
				}
				if (DD::isGroupAdmin(fromGID, each, false))
				{
					max_trust |= (1 << trustedQQ(each));
					if (blacklist->get_qq_danger(each) > 1)
					{
						strMsg += ",发现黑名单管理员" + printUser(each);
						if (grp.isset("免黑")) {
							strMsg += "（群免黑）";
						}
						else
						{
							DD::sendGroupMsg(fromGID, blacklist->list_qq_warning(each));
							grp.leave("发现黑名单管理员" + printUser(each) + "将预防性退群");
							strMsg += "，已退群";
							console.log(strMsg, 0b10, strNow);
							return true;
						}
					}
					if (DD::isGroupOwner(fromGID, each, false))
					{
						ownerQQ = each;
						ave_trust += (gsize.currSize - 1) * trustedQQ(each);
						strMsg += "，群主" + printUser(each) + "；";
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
			if (!grp.inviter && list.size() <= 2 && ownerQQ){
				grp.inviter = ownerQQ;
				strMsg += "邀请者" + printUser(ownerQQ);
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
			if (cntDiceMaid) 			{
				strMsg += "\n可识别同系Dice!" + to_string(cntDiceMaid) + "位";
			}
		}
		if (!blacks.empty())
		{
			string strNote = "\n发现黑名单群员" + blacks.show();
			strMsg += strNote;
		}
		if (fmt->call_hook_event(eve)) {
			console.log(strMsg, 1, strNow);
			return 0;
		}
		if (console["Private"] && !grp.isset("许可使用"))
		{	
			//避免小群绕过邀请没加上白名单
			if (max_trust > 1 || ave_trust > 0.5)
			{
				grp.set("许可使用");
				strMsg += "\n已自动追加使用许可";
			}
			else if(!grp.isset("协议无效"))
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
		console.log(strMsg + "\n群" + to_string(fromGID) + "信息获取失败！", 0b1, printSTNow());
		return true;
	}
	if (grp.isset("协议无效"))return 0;
	if (string selfIntro{ getMsg("strAddGroup", AttrVars{{"gid",fromGID}}) };
		!selfIntro.empty()) {
		this_thread::sleep_for(2s);
		AddMsgToQueue(selfIntro, { 0,fromGID, 0 });
	}
	if (console["CheckGroupLicense"] && !grp.isset("许可使用"))	{
		grp.set("未审核");
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strGroupLicenseDeny"), { 0,fromGID, 0 });
	}
	return false;
}

//处理骰子指令

EVE_PrivateMsg(eventPrivateMsg)
{
	if (!Enabled)return 0;
	if (fromUID == console.DiceMaid && !console["ListenSelfEcho"])return 0;
	else if (console["DisableStrangerChat"] && !DD::isFriend(fromUID, true))return 0;
	shared_ptr<DiceEvent> Msg(make_shared<DiceEvent>(
		AttrVars{ { "Event", "Message" },
			{ "fromMsg", message },
			{ "uid", fromUID },
			{ "time",(long long)time(nullptr)},
		}, chatInfo{ fromUID,0,0 }));
	return Msg->DiceFilter() || fmt->call_hook_event(Msg->merge({
		{"hook","WhisperIgnored"},
		}));
}

EVE_GroupMsg(eventGroupMsg)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID).group();
	if (fromUID == console.DiceMaid && !console["ListenGroupEcho"])return 0;
	if (!grp.isset("已入群") && grp.getLst())eve_GroupAdd(grp);
	if (!grp.isset("忽略"))	{
		shared_ptr<DiceEvent> Msg(make_shared<DiceEvent>(
			AttrVars({ { "Event", "Message" },
				{ "fromMsg", message },
				{ "msgid",msgId },
				{ "uid",fromUID },
				{ "gid",fromGID },
				{ "time",(long long)time(nullptr)},
				}), chatInfo{ fromUID,fromGID,0 }));
		return Msg->DiceFilter();
	}
	return grp.setLst(time(nullptr)).isset("拦截消息");
}
EVE_ChannelMsg(eventChannelMsg)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID).channel();
	if (fromUID == console.DiceMaid && !console["ListenChannelEcho"])return 0;
	//if (!grp.isset("忽略"))
	{
		shared_ptr<DiceEvent> Msg(make_shared<DiceEvent>(
			AttrVars({ { "Event","Message"},
				{ "fromMsg", message },
				{ "msgid", msgId },
				{ "uid", fromUID },
				{ "gid", fromGID },
				{ "chid", fromChID },
				{ "time",(long long)time(nullptr)}
				}), chatInfo{ fromUID,fromGID,fromChID }));
		return Msg->DiceFilter();
	}
	//return grp.isset("拦截消息");
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
	Chat& grp = chat(fromDiscuss);
	if (blacklist->get_qq_danger(fromUID) && console["AutoClearBlack"])
	{
		const string strMsg = "发现黑名单用户" + printUser(fromUID) + "，自动执行退群";
		console.log(printChat(grp) + strMsg, 0b10, printSTNow());
		grp.leave(strMsg);
		return 1;
	}
	shared_ptr<DiceEvent> Msg(make_shared<DiceEvent>(
		AttrVars({ { "Event", "Message"},
			{ "fromMsg", message },
			{ "uid", fromUID },
			{ "gid", fromDiscuss }
			}), chatInfo{ fromUID,fromDiscuss,0 }));
	return Msg->DiceFilter() || grp.isset("拦截消息");
}

EVE_GroupMemberIncrease(eventGroupMemberAdd)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID);
	if (grp.isset("忽略"))return 0;
	if (fromUID != console.DiceMaid){
		if (chat(fromGID).confs.has("入群欢迎")){
			AttrObject eve{ {{ "Event","GroupWelcome"},
				{"gid",fromGID},
				{"uid",fromUID},
			} };
			AddMsgToQueue(strip(fmt->format(grp.update().confs.get_str("入群欢迎"), eve, false)),
				{ 0,fromGID });
		}
		if (blacklist->get_qq_danger(fromUID))
		{
			const string strNow = printSTNow();
			string strNote = printGroup(fromGID) + "发现" + getMsg("strSelfName") + "的黑名单用户" + printUser(
				fromUID) + "入群";
			AddMsgToQueue(blacklist->list_self_qq_warning(fromUID), { 0,fromGID });
			if (grp.isset("免清"))strNote += "（群免清）";
			else if (grp.isset("免黑"))strNote += "（群免黑）";
			else if (grp.isset("协议无效"))strNote += "（群协议无效）";
			else if (DD::isGroupAdmin(fromGID, console.DiceMaid, false))strNote += "（群内有权限）";
			else if (console["LeaveBlackQQ"])
			{
				strNote += "（已退群）";
				grp.leave("发现黑名单用户" + printUser(fromUID) + "入群,将预防性退群");
			}
			console.log(strNote, 0b10, strNow);
		}
	}
	else{
		if (!grp.inviter)grp.inviter = operatorQQ;
		{
			unique_lock<std::mutex> lock_queue(GroupAddMutex);
			if (grp.isset("已入群"))return 0;
		}
		return eve_GroupAdd(grp);
	}
	return 0;
}

EVE_GroupMemberKicked(eventGroupMemberKicked){
	if (!Enabled || !fromUID)return 0;
	Chat& grp = chat(fromGID);
	if (beingOperateQQ == console.DiceMaid){
		grp.reset("已入群").rmLst();
		if (!console || grp.isset("忽略"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "将" + printUser(beingOperateQQ) + "移出了" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (!console["ListenGroupKick"] || trustedQQ(fromUID) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGID)) return 0;
		AttrObject eve{ {
			{"Event","GroupKicked"},
			{"uid",fromUID},
			{"gid",fromGID},
		} };
		if (fmt->call_hook_event(eve))return 1;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.sign().type("kick").time(strNow).note(strNow + " " + strNote).comment(getMsg("strSelfCall") + "原生记录");
		if (grp.inviter && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";入群邀请者：" + printUser(grp.inviter);
			if (console["KickedBanInviter"])mark.inviter(grp.inviter).note(strNow + " " + strNote);
		}
		grp.reset("许可使用").reset("免清");
		blacklist->create(mark.product());
	}
	else if (mDiceList.count(beingOperateQQ) && console["ListenGroupKick"])
	{
		if (!console || grp.isset("忽略"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "将" + printUser(beingOperateQQ) + "移出了" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (trustedQQ(fromUID) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGID)) return 0;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.type("kick").time(strNow).note(strNow + " " + strNote).DiceMaid(beingOperateQQ).master(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "目击");
		grp.reset("许可使用").reset("免清");
		blacklist->create(mark.product());
	}
	return 0;
}

EVE_GroupBan(eventGroupBan)
{
	if (!Enabled) return 0;
	Chat& grp = chat(fromGID);
	if (grp.isset("忽略") || (beingOperateQQ != console.DiceMaid && !mDiceList.count(beingOperateQQ)) || !console["ListenGroupBan"])return 0;
	if (!duration || !duration[0])
	{
		if (beingOperateQQ == console.DiceMaid)
		{
			console.log(getMsg("self") + "在" + printGroup(fromGID) + "中被解除禁言", 0b10, printSTNow());
			return 1;
		}
	}
	else
	{
		string strNow = printSTNow();
		long long llOwner = 0;
		string strNote = "在" + printGroup(fromGID) + "中," + printUser(beingOperateQQ) + "被" + printUser(operatorQQ) + "禁言" + duration;
		if (!console["ListenGroupBan"] || trustedQQ(operatorQQ) > 1 || grp.isset("免黑") || grp.isset("协议无效") || ExceptGroups.count(fromGID)) 
		{
			console.log(strNote, 0b10, strNow);
			return 1;
		}
		AttrObject eve{ {
			{"Event","GroupBanned"},
			{"uid",operatorQQ},
			{"gid",fromGID},
			{"duration",duration},
		} };
		if (fmt->call_hook_event(eve))return 1;
		DDBlackMarkFactory mark{operatorQQ, fromGID};
		mark.type("ban").time(strNow).note(strNow + " " + strNote);
		if (mDiceList.count(operatorQQ))mark.fromUID(0);
		if (beingOperateQQ == console.DiceMaid) {
			if (!console)return 0;
			mark.sign().comment(getMsg("strSelfCall") + "原生记录");
		}
		else{
			mark.DiceMaid(beingOperateQQ).master(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "目击");
		}
		//统计群内管理
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
		strAuthList = "；群主" + printUser(llOwner) + ",另有管理员" + to_string(intAuthCnt) + "名" + strAuthList;
		mark.note(strNow + " " + strNote);
		if (grp.inviter && beingOperateQQ == console.DiceMaid && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";入群邀请者：" + printUser(grp.inviter);
			if (console["BannedBanInviter"])mark.inviter(grp.inviter);
			mark.note(strNow + " " + strNote);
		}
		grp.reset("免清").reset("许可使用").leave();
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
	if (groupset(fromGID, "忽略") < 1)
	{
		this_thread::sleep_for(3s);
		AttrObject eve{ {
			{"Event","GroupRequest"},
			{"uid",fromUID},
			{"gid",fromGID},
		} };
		bool isBlocked{ fmt->call_hook_event(eve) };
		if (eve.has("approval")) {
			if (eve.is("approval")) {
				chat(fromGID).inviter = fromUID;
				DD::answerFriendRequest(fromUID, 1);
			}
			else {
				DD::answerFriendRequest(fromUID, 2);
			}
		}
		if (isBlocked)return 1;
		const string strNow = printSTNow();
		string strMsg = "收到" + printUser(fromUID) + "的入群邀请:" +
			DD::printGroupInfo(fromGID);
		if (ExceptGroups.count(fromGID)) {
			strMsg += "\n已忽略（默认协议无效）";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (blacklist->get_group_danger(fromGID)){
			strMsg += "\n已拒绝（黑名单群）";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (blacklist->get_qq_danger(fromUID)){
			strMsg += "\n已拒绝（黑名单用户）";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (Chat& grp = chat(fromGID).group(); grp.isset("许可使用")) {
			grp.setLst(0);
			grp.inviter = fromUID;
			strMsg += "\n已同意（已许可使用）";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (trustedQQ(fromUID))
		{
			grp.set("许可使用").reset("未审核").reset("协议无效");
			grp.inviter = fromUID;
			strMsg += "\n已同意（信任用户）";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (grp.isset("协议无效")) {
			strMsg += "\n已忽略（协议无效）";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console["GroupInvalidSize"] > 0 && DD::getGroupSize(grp.ID).currSize > (size_t)console["GroupInvalidSize"]) {
			grp.set("协议无效");
			console.log(strMsg + "\n已忽略（群规模超标）", 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console && console["Private"])
		{
			AddMsgToQueue(getMsg("strPreserve"), fromUID);
			strMsg += "\n已拒绝（私用模式）";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else {
			grp.setLst(0);
			grp.inviter = fromUID;
			strMsg += "已同意";
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
	this_thread::sleep_for(3s);
	AttrObject eve{{
		{"Event","FriendRequest"},
		{"fromMsg",message},
		{"uid",fromUID},
	} };
	bool isBlocked{ fmt->call_hook_event(eve) };
	if (eve.has("approval")) {
		DD::answerFriendRequest(fromUID, eve.is("approval") ? 1 : 2,
			fmt->format(eve.get_str("msg_reply"), eve));
	}
	if (isBlocked)return 1;
	string strMsg = "好友添加请求，来自 " + printUser(fromUID) + ":" + message;
	if (blacklist->get_qq_danger(fromUID)) {
		strMsg += "\n已拒绝（用户在黑名单中）";
		DD::answerFriendRequest(fromUID, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromUID)) {
		strMsg += "\n已同意（受信任用户）";
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriendWhiteQQ", eve));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 2 && !UserList.count(fromUID)) {
		strMsg += "\n已拒绝（无用户记录）";
		DD::answerFriendRequest(fromUID, 2, getMsg("strFriendDenyNotUser"));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 1) {
		strMsg += "\n已拒绝（非信任用户）";
		DD::answerFriendRequest(fromUID, 2, getMsg("strFriendDenyNoTrust"));
		console.log(strMsg, 1, printSTNow());
	}
	else {
		strMsg += "\n已同意";
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriend",eve));
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}
EVE_FriendAdded(eventFriendAdd) {
	if (!Enabled) return 0;
	if (!console["ListenFriendAdd"])return 0;
	this_thread::sleep_for(3s);
	AttrObject eve{ {
		{"Event","FriendAdd"},
		{"uid",fromUID},
	} };
	if(fmt->call_hook_event(eve))return 1;
	(trustedQQ(fromUID) > 0 && !getMsg("strAddFriendWhiteQQ", eve).empty())
		? AddMsgToQueue(getMsg("strAddFriendWhiteQQ", eve), fromUID)
		: AddMsgToQueue(getMsg("strAddFriend", eve), fromUID);
	return 0;
}
EVE_Extra(eventExtra) {
	if (!Enabled) return 0;
	try {
		AttrObject eve{ AttrObject(fifo_json::parse(jsonData)) };
		if (fmt->call_hook_event(eve))return 1;
	}
	catch (std::exception& e) {
		DD::debugLog("eventExtra处理异常!" + string(e.what()));
	}
	return 0;
}

EVE_Menu(eventMasterMode)
{
	if (!Enabled) return 0;
	if (console) {
		console.killMaster();
#ifdef _WIN32
		MessageBoxA(nullptr, "master已清除", "关闭Master模式", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else {
#ifdef _WIN32
		MessageBoxA(nullptr, "Master模式已不再另设开关，请使用口令或在UI界面直接认主", "Master模式已弃用", MB_OK | MB_ICONINFORMATION);
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
	sessions.clear();
	PList.clear();
	ChatList.clear();
	UserList.clear();
	console.reset();
	EditedMsg.clear();
	blacklist.reset();
#ifndef _WIN32
	curl_global_cleanup();
#else
	Aws::ShutdownAPI(options);
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
		MessageBoxA(nullptr, "骰娘已结束静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else {
		console.set("DisabledGlobal", 1);
#ifdef _WIN32
		MessageBoxA(nullptr, "骰娘已全局静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
#endif
	}

	return 0;
}