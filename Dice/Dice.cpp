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
 * Copyright (C) 2019-2023 String.Empty
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
#include <algorithm>
#include <exception>
#include <stdexcept>

#include "DiceFile.hpp"
#include "Jsonio.h"
#include "OneBotAPI.h"
#include "ManagerSystem.h"
#include "DiceMod.h"
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
#include "DiceJS.h"
#include "DicePython.h"

#ifdef _WIN32
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
std::unique_ptr<CivetServer> ManagerServer;
BasicInfoApiHandler h_basicinfoapi;
CustomMsgApiHandler h_msgapi;
AdminConfigHandler h_config;
MasterHandler h_master;
CustomReplyApiHandler h_customreply;
ModListApiHandler h_modlist;
WebUIPasswordHandler h_webuipassword;
UrlApiHandler h_url;
AuthHandler auth_handler;

string msgInit;

//加载数据
void loadData(){
	ResList logList;
	try	{
		std::error_code ec;
		std::filesystem::create_directory(DiceDir, ec);
		loadDir(loadJMap, DiceDir / "PublicDeck", CardDeck::mExternPublicDeck, logList);
		map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
		//读取帮助文档
		fmt->clear();
		fmt->load(logList);
		//读取敏感词库
		loadDir(load_words, DiceDir / "conf" / "censor", censor, logList, true);
		loadJMap(DiceDir / "conf" / "CustomCensor.json", censor.CustomWords);
		censor.build();
		//selfdata
		if (const auto dirSelfData{ DiceDir / "selfdata" }; std::filesystem::exists(dirSelfData)) {
			std::error_code err;
			for (const auto& file : std::filesystem::directory_iterator(dirSelfData, err)) {
				if (file.is_regular_file()) {
					const auto p{ file.path() };
					auto& data{ selfdata_byFile[getNativePathString(p.filename())]
						= make_shared<SelfData>(p) };
					if (string file{ p.stem().u8string() }; !selfdata_byStem.count(file)) {
						selfdata_byStem[file] = data;
					}
				}
			}
			api::printLog("预加载selfdata" + to_string(selfdata_byStem.size()) + "份");
		}
		if (!logList.empty())
		{
			logList << "扩展配置读取完毕√";
			console.log(logList.show(), 1, printSTNow());
		}
	}
	catch (const std::exception& e)
	{
		logList << "读取数据时遇到意外错误，程序可能无法正常运行。请排除异常文件后重试。" << e.what();
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
			if (long long tiny{ api::getTinyID() }) {
				api::printLog("获取分身ID:" + to_string(tiny));
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
void dice_init() {
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
#endif
	Dice_Full_Ver_On = Dice_Full_Ver + " for\n" + api::getApiVer();
	api::printLog(Dice_Full_Ver_On);

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
	console.load();
	if (!console) {
		msgInit = R"(欢迎使用Dice!掷骰机器人！
可发送.help查看帮助
发送.system gui开启DiceMaid后台面板
参考文档参看.help链接)";
		api::printLog(msgInit);
	}
	api::printLog("DiceConsole.load");
	fmt = make_unique<DiceModManager>();
	try {
		std::unique_lock lock(GlobalMsgMutex);
		if (loadJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg) >= 0) {
			map_merge(GlobalMsg, EditedMsg);
		}
	}
	catch (const std::exception& e) {
		console.log(string("读取/conf/CustomMsg.json失败!") + e.what(), 1, printSTNow());
	}
	js_global_init();
#ifdef DICE_PYTHON
	if (console["EnablePython"])py.reset(new PyGlobal());
#endif //DICE_PYTHON
	loadData();
	//初始化黑名单
	//读取用户数据
	readUserData();
	//读取当日数据
	today = make_unique<DiceToday>();
	// 确保线程执行结束
	while (msgSendThreadRunning)this_thread::sleep_for(10ms);

	api::printLog("Dice.webUIInit");
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

	api::printLog("Dice.threadInit");
	Enabled = true;
	threads(SendMsg);
	threads(ConsoleTimer);
	threads(frqHandler);
	sch.start();

	console.log(getMsg("strSelfName") + "初始化完成，用时" + to_string(clock() - clockStart) + "毫秒", 0b1,
		printSTNow());
	llStartTime = time(nullptr);
	api::printLog("Dice.WebInit");
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
				fs::create_directories(DiceDir / "webui");
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
			ManagerServer->addHandler("/api/url", h_url);
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
	isIniting.clear();
	fmt->call_hook_event(AttrVars{ {{"Event","StartUp"}} });
}

void global_exit() {
	Enabled = false;
	mg_exit_library();
	ManagerServer = nullptr;
	dataBackUp();
	sch.end();
	censor = {};
	fmt.reset();
	ruleset.reset();
	js_global_end();
#ifdef DICE_PYTHON
	if (py)py.reset();
#endif
	sessions.clear();
	PList.clear();
	ChatList.clear();
	UserList.clear();
	console.reset();
	EditedMsg.clear();
#ifndef _WIN32
	curl_global_cleanup();
#endif
	threads.exit();
}