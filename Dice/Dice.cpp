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
 * Copyright (C) 2019-2024 String.Empty
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

#include "DiceFile.hpp"
#include "Jsonio.h"
#include "QQEvent.h"
#include "DDAPI.h"
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

unordered_map<long long, ptr<User>> UserList{};
unordered_map<long long, long long> TinyList{}; 
unordered_map<long long, ptr<Chat>> ChatList;
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
UrlApiHandler h_url;
AuthHandler auth_handler;

string msgInit;

//��������
void loadData(){
	ResList logList;
	try	{
		std::error_code ec;
		std::filesystem::create_directory(DiceDir, ec);
		loadDir(loadJMap, DiceDir / "PublicDeck", CardDeck::mExternPublicDeck, logList);
		map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
		//ModManger
		fmt->clear();
		fmt->load(logList);
		//Censor word
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
					if (string file{ UTF8toGBK(p.stem().u8string()) }; !selfdata_byStem.count(file)) {
						selfdata_byStem[file] = data;
					}
				}
			}
			DD::debugLog("Ԥ����selfdata" + to_string(selfdata_byStem.size()) + "��");
		}
		if (!logList.empty())
		{
			logList << "��չ���ö�ȡ��ϡ�";
			Enabled ? console.log(logList.show(), 1, printSTNow())
				: DD::debugLog(logList.show());
		}
	}
	catch (const std::exception& e)
	{
		logList << "��ȡ����ʱ����������󣬳�������޷��������С����ų��쳣�ļ������ԡ�" << e.what();
		console.log(logList.show(), 1, printSTNow());
	}
}

//��ʼ���û�����
void readUserData(){
	std::error_code ec;
	fs::path dir{ DiceDir / "user" };
	ResList log;
	try {
		//��ȡ�û���¼
		if (int cnt{ loadBFile(dir / "UserConf.dat", UserList) }; cnt > 0) {
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
			if (cnt > 0)log << "Ǩ���û���¼" + to_string(cnt) + "��";
		}
		//for QQ Channel
		if (User& self{ getUser(console.DiceMaid) }; !self.get_ll("tinyID")) {
			if (long long tiny{ DD::getTinyID() }) {
				DD::debugMsg("��ȡ����ID:" + to_string(tiny));
				self.setConf("tinyID", tiny);
			}
		}
	}
	catch (const std::exception& e) {
		console.log(string("��ȡ�û���¼ʱ������������볢��ɾ��UserConf.dat���ñ���.bak�ļ�!")
			+ e.what(), 0b1000, printSTNow());
	}
	try {
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
		for (const auto& pl : PList) {
			if (!UserList.count(pl.first))getUser(pl.first);
		}
	}
	catch (const std::exception& e)	{
		console.log("��ȡ��Ҽ�¼ʱ������������볢��ɾ��PlayerCards.RDconf���ñ���.bak�ļ�!"
			+ string(e.what()), 0b1000, printSTNow());
	}
	try {
		//��ȡȺ�ļ�¼
		if (int cnt{ loadBFile(dir / "ChatConf.dat", ChatList) }; cnt > 0) {
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
			if (cnt > 0)log << "Ǩ��Ⱥ�ļ�¼" + to_string(cnt) + "��";
		}
	}
	catch (const std::exception& e)	{
		console.log("��ȡȺ�ļ�¼ʱ������������볢��ɾ��ChatConf.dat���ñ���.bak�ļ�!"
			+ string(e.what()), 0b1000, printSTNow());
	}
	//��ȡ�����¼
	sessions.load();
	if (!log.empty()) {
		log << "�û����ݶ�ȡ���";
		DD::debugLog(printSTNow() + log.show());
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
		console.log("����: ����libcurlʧ�ܣ�", 1);
	}
#else
	aws_init();
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
			strSelfName = "����[" + toString(console.DiceMaid % 10000, 4) + "]";
		}
	}
	if (!console.load()){
		console.setClock({ 11, 45 }, "clear");
		console.loadNotice();
	}
	if (!console) {
		msgInit = R"(��ӭʹ��Dice!���������ˣ�
�뷢��.master )"
+ (console.authkey_pub = RandomGenerator::genKey(8)) + " //�������� ��\n.master "
+ (console.authkey_pri = RandomGenerator::genKey(8)) + 
R"( //˽������ ���ɳ�Ϊ�ҵ�����~
�ɷ���.help�鿴����
����.system gui����DiceMaid��̨���
�ο��ĵ��ο�.help����)";
		DD::debugMsg(msgInit);
	}
	try {
		js_global_init();
#ifdef DICE_PYTHON
		if (console["EnablePython"])py = make_unique<PyGlobal>();
#endif //DICE_PYTHON
	}
	catch (const std::exception& e) {
		console.log(string("��ʼ��js/python����ʧ��!") + e.what(), 1, printSTNow());
	}
	fmt = make_unique<DiceModManager>();
	try {
		std::unique_lock lock(GlobalMsgMutex);
		if (loadJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg) >= 0
			|| loadJMap(fpFileLoc / "CustomMsg.json", EditedMsg) > 0) {
			map_merge(GlobalMsg, EditedMsg);
		}
	}
	catch (const std::exception& e) {
		console.log(string("��ȡ/conf/CustomMsg.jsonʧ��!") + e.what(), 1, printSTNow());
	}
	loadData();
	//��ʼ��������
	blacklist = make_unique<DDBlackManager>();
	if (auto cnt = blacklist->loadJson(DiceDir / "conf" / "BlackList.json");cnt < 0)
	{
		blacklist->loadJson(fpFileLoc / "BlackMarks.json");
		cnt = blacklist->loadHistory(fpFileLoc);
		if (cnt) {
			blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
			console.log("��ʼ��������¼" + to_string(cnt) + "��", 1);
		}
	}
	else {
		DD::debugLog("��ȡ������¼" + to_string(cnt) + "��");
		if ((cnt = blacklist->loadJson(DiceDir / "conf" / "BlackListEx.json", true)) > 0)
			DD::debugLog("�ϲ���Դ������¼" + to_string(cnt) + "��");
	}
	//��ȡ�û�����
	readUserData();
	//��ȡ��������
	today = make_unique<DiceToday>();
	set<long long> grps{ DD::getGroupIDList() };
	for (auto gid : grps){
		if (auto grp{ chat(gid).reset("δ��").reset("����").set("����Ⱥ") };
			!grp.is("lastMsg"))grp.setLst(-1);
	}
	// ȷ���߳�ִ�н���
	while (msgSendThreadRunning)this_thread::sleep_for(10ms);

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

	console.log(getMsg("strSelfName") + "��ʼ����ɣ���ʱ" + to_string(clock() - clockStart) + "����", 0b1,
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
					LPVOID pBuffer = LockResource(hGlobal);  // ������Դ
					char* pByte = new char[dwSize + 1];
					fs::create_directories(DiceDir / "webui");
					ofstream fweb{ DiceDir / "webui" / "index.html" };
					fweb.write((const char*)pBuffer, dwSize);
					FreeResource(hGlobal);// �ͷ���Դ
				}
			}
#else
			if (string html; Network::GET("https://raw.sevencdn.com/Dice-Developer-Team/Dice/newdev/Dice/webui.html", html)) {
				fs::create_directories(DiceDir / "webui");
				ofstream fweb{ DiceDir / "webui" / "index.html" };
				fweb.write(html.c_str(), html.length());
			}
			else if (!fs::exists(DiceDir / "webui" / "index.html")) {
				console.log("��ȡwebuiҳ��ʧ��!��ع����޷�ʹ��!", 0b10);
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
				console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
			}
			else {
				string note{ "Dice! WebUI ���ڶ˿�" + std::to_string(ports[0])
					+ "���У����ؿ�ͨ�����������http://localhost:" + std::to_string(ports[0])
					+ "\nĬ���û���Ϊadmin����Ϊpassword����ϸ�̳���鿴 https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming" };
				console ? DD::debugLog(note) : console.log(note, 0b1);
			}
		}
		catch (const CivetException& e)
		{
			console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
		}
	}
	//��������
	getDiceList();
	getExceptGroup();
	isIniting.clear();
	fmt->call_hook_event(AnysTable{ AttrVars {{"Event","StartUp"}} });
}

mutex GroupAddMutex;
bool eve_GroupAdd(Chat& grp) {
	{
		unique_lock<std::mutex> lock_queue(GroupAddMutex);
		if (!grp.is("����Ⱥ"))grp.set("����Ⱥ").reset("δ��").reset("����");
		else return false;
		if (ChatList.size() == 1 && !console)DD::sendGroupMsg(grp.ID, msgInit);
	}
	long long fromGID = grp.ID;
	if (grp.get_str("name").empty())
		grp.name(DD::getGroupName(fromGID));
	GroupSize_t gsize(DD::getGroupSize(fromGID));
	if (console["GroupInvalidSize"] > 0 && grp.empty() && gsize.currSize > (size_t)console["GroupInvalidSize"]) {
		grp.set("Э����Ч");
	}
	if (!console["ListenGroupAdd"] || grp.is("����"))return 0;
	string strNow = printSTNow();
	string strMsg(getMsg("strSelfName"));
	AttrObject eve{ AnysTable{{
		{"Event","GroupAdd"},
		{"gid",fromGID},
	}} };
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
		if (grp.is("���ʹ��"))strMsg += "���ѻ�ʹ����ɣ�";
		else if(grp.is("Э����Ч"))strMsg += "���ѱ��Э����Ч��";
		if (grp.inviter) {
			strMsg += ",������" + printUser(grp.inviter);
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
				if (UserList.count(each)) {
					cntUser++;
					ave_trust += getUser(each).nTrust;
				}
				if (DD::isGroupAdmin(fromGID, each, false))
				{
					max_trust |= (1 << trustedQQ(each));
					if (blacklist->get_user_danger(each) > 1)
					{
						strMsg += ",���ֺ���������Ա" + printUser(each);
						if (grp.is("���")) {
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
				else if (blacklist->get_user_danger(each) > 1)
				{
					//max_trust |= 1;
					blacks << printUser(each);
					if (blacklist->get_user_danger(each)) 
					{
						AddMsgToQueue(blacklist->list_self_qq_warning(each), { 0,fromGID });
					}
				}
				if (DD::isDiceMaid(each)) {
					++cntDiceMaid;
				}
			}
			if (!grp.inviter && list.size() <= 2 && ownerQQ){
				grp.invited(ownerQQ);
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
		if (fmt->call_hook_event(eve)) {
			console.log(strMsg, 1, strNow);
			return 0;
		}
		if (console["Private"] && !grp.is("���ʹ��"))
		{	
			//����СȺ�ƹ�����û���ϰ�����
			if (max_trust > 1 || ave_trust > 0.5)
			{
				grp.set("���ʹ��");
				strMsg += "\n���Զ�׷��ʹ�����";
			}
			else if(!grp.is("Э����Ч"))
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
	if (grp.is("Э����Ч"))return 0;
	if (string selfIntro{ getMsg("strAddGroup", AnysTable{{"gid",fromGID}}) };
		!selfIntro.empty()) {
		this_thread::sleep_for(2s);
		AddMsgToQueue(selfIntro, { 0,fromGID, 0 });
	}
	if (console["CheckGroupLicense"] && !grp.is("���ʹ��"))	{
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
	Chat& grp = chat(fromGID);
	if (fromUID == console.DiceMaid && !console["ListenGroupEcho"])return 0;
	if (!grp.is("����Ⱥ") && grp.getLst())eve_GroupAdd(grp);
	if (!grp.is("����"))	{
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
	return 0;
}
EVE_ChannelMsg(eventChannelMsg)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID);
	if (fromUID == console.DiceMaid && !console["ListenChannelEcho"])return 0;
	//if (!grp.is("����"))
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
	//return grp.is("������Ϣ");
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
	if (blacklist->get_user_danger(fromUID) && console["AutoClearBlack"])
	{
		const string strMsg = "���ֺ������û�" + printUser(fromUID) + "���Զ�ִ����Ⱥ";
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
	return Msg->DiceFilter() || grp.is("������Ϣ");
}

EVE_GroupMemberIncrease(eventGroupMemberAdd)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGID);
	if (grp.is("����"))return 0;
	if (fromUID != console.DiceMaid){
		if (chat(fromGID).has("��Ⱥ��ӭ")){
			AttrObject eve{ AnysTable{{
				{ "Event","GroupWelcome"},
				{"gid",fromGID},
				{"uid",fromUID},
			}} };
			AddMsgToQueue(strip(fmt->format(grp.update().get_str("��Ⱥ��ӭ"), eve, false)),
				{ 0,fromGID });
		}
		if (blacklist->get_user_danger(fromUID))
		{
			const string strNow = printSTNow();
			string strNote = printGroup(fromGID) + "����" + getMsg("strSelfName") + "�ĺ������û�" + printUser(
				fromUID) + "��Ⱥ";
			AddMsgToQueue(blacklist->list_self_qq_warning(fromUID), { 0,fromGID });
			if (grp.is("����"))strNote += "��Ⱥ���壩";
			else if (grp.is("���"))strNote += "��Ⱥ��ڣ�";
			else if (grp.is("Э����Ч"))strNote += "��ȺЭ����Ч��";
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
		if (!grp.inviter)grp.invited(operatorQQ);
		{
			unique_lock<std::mutex> lock_queue(GroupAddMutex);
			if (grp.is("����Ⱥ"))return 0;
		}
		return eve_GroupAdd(grp);
	}
	return 0;
}

EVE_GroupMemberKicked(eventGroupMemberKicked){
	if (!Enabled || !fromUID)return 0;
	Chat& grp = chat(fromGID);
	if (beingOperateQQ == console.DiceMaid){
		grp.reset("����Ⱥ").rmLst();
		if (!console || grp.is("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "��" + printUser(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (!console["ListenGroupKick"] || trustedQQ(fromUID) > 1 || grp.is("���") || grp.is("Э����Ч") || ExceptGroups.count(fromGID)) return 0;
		AttrObject eve{ AnysTable{{
			{"Event","GroupKicked"},
			{"uid",fromUID},
			{"gid",fromGID},
		}} };
		if (fmt->call_hook_event(eve))return 1;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.sign().type("kick").time(strNow).note(strNow + " " + strNote).comment(getMsg("strSelfCall") + "ԭ����¼");
		if (grp.inviter && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";��Ⱥ�����ߣ�" + printUser(grp.inviter);
			if (console["KickedBanInviter"])mark.inviter(grp.inviter).note(strNow + " " + strNote);
		}
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	else if (mDiceList.count(beingOperateQQ) && console["ListenGroupKick"])
	{
		if (!console || grp.is("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printUser(fromUID) + "��" + printUser(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (trustedQQ(fromUID) > 1 || grp.is("���") || grp.is("Э����Ч") || ExceptGroups.count(fromGID)) return 0;
		DDBlackMarkFactory mark{fromUID, fromGID};
		mark.type("kick").time(strNow).note(strNow + " " + strNote).DiceMaid(beingOperateQQ).master(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "Ŀ��");
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	return 0;
}

EVE_GroupBan(eventGroupBan)
{
	if (!Enabled) return 0;
	Chat& grp = chat(fromGID);
	if (grp.is("����") || (beingOperateQQ != console.DiceMaid && !mDiceList.count(beingOperateQQ)) || !console["ListenGroupBan"])return 0;
	if (!duration || !duration[0])
	{
		if (beingOperateQQ == console.DiceMaid)
		{
			console.log(getMsg("self") + "��" + printGroup(fromGID) + "�б��������", 0b10, printSTNow());
			return 1;
		}
	}
	else
	{
		string strNow = printSTNow();
		long long llOwner = 0;
		string strNote = "��" + printGroup(fromGID) + "��," + printUser(beingOperateQQ) + "��" + printUser(operatorQQ) + "����" + duration;
		if (!console["ListenGroupBan"] || trustedQQ(operatorQQ) > 1 || grp.is("���") || grp.is("Э����Ч") || ExceptGroups.count(fromGID)) 
		{
			console.log(strNote, 0b10, strNow);
			return 1;
		}
		AttrObject eve{ AnysTable{{
			{"Event","GroupBanned"},
			{"uid",operatorQQ},
			{"gid",fromGID},
			{"duration",duration},
		}} };
		if (fmt->call_hook_event(eve))return 1;
		DDBlackMarkFactory mark{operatorQQ, fromGID};
		mark.type("ban").time(strNow).note(strNow + " " + strNote);
		if (mDiceList.count(operatorQQ))mark.fromUID(0);
		if (beingOperateQQ == console.DiceMaid) {
			if (!console)return 0;
			mark.sign().comment(getMsg("strSelfCall") + "ԭ����¼");
		}
		else{
			mark.DiceMaid(beingOperateQQ).master(mDiceList[beingOperateQQ]).comment(strNow + " " + printUser(console.DiceMaid) + "Ŀ��");
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
			if (console["BannedBanInviter"])mark.inviter(grp.inviter);
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
		AttrObject eve{ AnysTable{ {
			{"Event","GroupRequest"},
			{"uid",fromUID},
			{"gid",fromGID},
		}} };
		bool isBlocked{ fmt->call_hook_event(eve) };
		if (eve->has("approval")) {
			if (eve->is("approval")) {
				chat(fromGID).invited(fromUID);
				DD::answerFriendRequest(fromUID, 1);
			}
			else {
				DD::answerFriendRequest(fromUID, 2);
			}
		}
		if (isBlocked)return 1;
		const string strNow = printSTNow();
		string strMsg = "�յ�" + printUser(fromUID) + "����Ⱥ����:" + DD::printGroupInfo(fromGID);
		if (ExceptGroups.count(fromGID)) {
			console.log(strMsg + "\n�Ѻ��ԣ�Ĭ��Э����Ч��", 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (blacklist->get_group_danger(fromGID)){
			strMsg += "\n��������Ⱥ����";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (blacklist->get_user_danger(fromUID)){
			strMsg += "\n�Ѿܾ����������û���";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else if (Chat& grp = chat(fromGID); grp.is("���ʹ��")) {
			grp.setLst(0);
			grp.invited(fromUID);
			strMsg += "\n��ͬ�⣨�����ʹ�ã�";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (trustedQQ(fromUID))
		{
			grp.set("���ʹ��").reset("δ���").reset("Э����Ч");
			grp.invited(fromUID);
			strMsg += "\n��ͬ�⣨�����û���";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 1);
		}
		else if (grp.is("Э����Ч")) {
			strMsg += "\n�Ѻ��ԣ�Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console["GroupInvalidSize"] > 0 && DD::getGroupSize(grp.ID).currSize > (size_t)console["GroupInvalidSize"]) {
			grp.set("Э����Ч");
			console.log(strMsg + "\n�Ѻ��ԣ�Ⱥ��ģ���꣩", 0b10, strNow);
			DD::answerGroupInvited(fromGID, 3);
		}
		else if (console && console["Private"])
		{
			AddMsgToQueue(getMsg("strPreserve"), fromUID);
			strMsg += "\n�Ѿܾ���˽��ģʽ��";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGID, 2);
		}
		else {
			grp.setLst(0);
			grp.invited(fromUID);
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
	this_thread::sleep_for(3s);
	AttrObject eve{ AnysTable{{
		{"Event","FriendRequest"},
		{"fromMsg",message},
		{"uid",fromUID},
	}} };
	bool isBlocked{ fmt->call_hook_event(eve) };
	if (eve->has("approval")) {
		DD::answerFriendRequest(fromUID, eve->is("approval") ? 1 : 2,
			fmt->format(eve->get_str("msg_reply"), eve));
	}
	if (isBlocked)return 1;
	string strMsg = "��������������� " + printUser(fromUID) + ":" + message;
	if (blacklist->get_user_danger(fromUID)) {
		strMsg += "\n�Ѿܾ����û��ں������У�";
		DD::answerFriendRequest(fromUID, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromUID)) {
		strMsg += "\n��ͬ�⣨�������û���";
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriendWhiteQQ", eve));
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
		DD::answerFriendRequest(fromUID, 1, getMsg("strAddFriend",eve));
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}
EVE_FriendAdded(eventFriendAdd) {
	if (!Enabled) return 0;
	if (!console["ListenFriendAdd"])return 0;
	this_thread::sleep_for(3s);
	AttrObject eve{ AnysTable{{
		{"Event","FriendAdd"},
		{"uid",fromUID},
	}} };
	if(fmt->call_hook_event(eve))return 1;
	(trustedQQ(fromUID) > 0 && !getMsg("strAddFriendWhiteQQ", eve).empty())
		? AddMsgToQueue(getMsg("strAddFriendWhiteQQ", eve), fromUID)
		: AddMsgToQueue(getMsg("strAddFriend", eve), fromUID);
	return 0;
}
EVE_Extra(eventExtra) {
	if (!Enabled) return 0;
	try {
		AttrObject eve{ AnysTable(fifo_json::parse(jsonData)) };
		if (fmt->call_hook_event(eve))return 1;
	}
	catch (std::exception& e) {
		DD::debugLog("eventExtra�����쳣!" + string(e.what()));
	}
	return 0;
}

EVE_Menu(eventMasterMode)
{
	if (!Enabled) return 0;
	if (console) {
		console.killMaster();
#ifdef _WIN32
		MessageBoxA(nullptr, "master�����", "�ر�Masterģʽ", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else {
#ifdef _WIN32
		MessageBoxA(nullptr, "Masterģʽ�Ѳ������迪�أ���ʹ�ÿ������UI����ֱ������", "Masterģʽ������", MB_OK | MB_ICONINFORMATION);
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
	blacklist.reset();
#ifndef _WIN32
	curl_global_cleanup();
#else
	aws_shutdown();
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