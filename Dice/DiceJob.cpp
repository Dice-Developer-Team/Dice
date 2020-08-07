#pragma once
#include "DiceJob.h"
#include "DiceConsole.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include "StrExtern.hpp"
#include "CQAPI.h"
#include "ManagerSystem.h"
#include "DiceCloud.h"
#include "BlackListManager.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "S3PutObject.h"
#pragma warning(disable:28159)

using namespace std;
using namespace CQ;

int sendSelf(const string msg) {
	static long long selfQQ = CQ::getLoginQQ();
	return CQ::sendPrivateMsg(selfQQ, msg);
}

void cq_exit(DiceJob& job) {
	int pid = _getpid();
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		job.note("重启失败：进程快照创建失败！", 1);
	}
	BOOL bResult = Process32First(hProcessSnap, &pe32);
	int ppid(0);
	while (bResult) {
		if (pe32.th32ProcessID == pid) {
			ppid = pe32.th32ParentProcessID;
			break;
		}
		bResult = Process32Next(hProcessSnap, &pe32);
	}
	if (!ppid) {
		job.note("重启失败：未找到进程！", 1);
	}
	string strCMD("taskkill /f /pid " + to_string(ppid));
	job.note("已命令" + getMsg("self") + "在5秒后自杀", 1);
	std::this_thread::sleep_for(5s);
	job.echo(strCMD);
	Enabled = false;
	dataBackUp();
	system(strCMD.c_str());
}

inline PROCESSENTRY32 getProcess(int pid) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	Process32First(hParentProcess, &pe32);
	return pe32;
}
void cq_restart(DiceJob& job) {
	//string strSelfPath;
	string strSelfName;
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		job.note("重启失败：进程快照创建失败！", 1);
	}
	BOOL bResult = Process32First(hProcessSnap, &pe32);
	int ppid(0);
	if (Mirai) {
		char buffer[MAX_PATH];
		const DWORD length = GetModuleFileNameA(nullptr, buffer, sizeof buffer);
		std::string pathSelf(buffer, length);
		pathSelf = pathSelf.substr(0, pathSelf.find("jre\\bin\\java.exe")) + "MiraiOK.exe";
		char pathFull[MAX_PATH];
		while (bResult) {
			if (strcmp(pe32.szExeFile, "MiraiOK.exe") == 0) {
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
				GetModuleFileNameEx(hProcess, NULL, pathFull, sizeof(pathFull));
				if (pathSelf != pathFull)continue;
				ppid = pe32.th32ProcessID;
				job.echo("确认进程" + pathSelf + "\n进程id:" + to_string(ppid));
				break;
			}
			bResult = Process32Next(hProcessSnap, &pe32);
		}
	}
	else {
		int pid = _getpid();
		while (bResult) {
			if (pe32.th32ProcessID == pid) {
				ppid = pe32.th32ParentProcessID;
				PROCESSENTRY32 pp32 = getProcess(ppid);
				strSelfName = pp32.szExeFile;
				job.echo("确认进程" + strSelfName + "\n本进程id:" + to_string(pe32.th32ProcessID) + "\n父进程id:" + to_string(ppid));
				break;
			}
			bResult = Process32Next(hProcessSnap, &pe32);
		}
	}
	if (!ppid) {
		job.note("重启失败：未找到进程！", 1);
		return;
	}
	string command = "taskkill /f /pid " + to_string(ppid) + "\nstart .\\" + strSelfName + " /account " + to_string(console.DiceMaid);
	if (Mirai) command = "taskkill /f /pid " + to_string(ppid) + " /t\nstart " + dirExe + "MiraiOK.exe";
	ofstream fout("reload.bat");
	fout << command << std::endl;
	fout.close();
	job.note(command, 0);
	std::this_thread::sleep_for(3s);
	Enabled = false;
	dataBackUp();
	std::this_thread::sleep_for(3s);
	switch (UINT res = -1; res = WinExec(".\\reload.bat", SW_SHOW)) {
	case 0:
		job.note("重启失败：内存或资源已耗尽！", 1);
		break;
	case ERROR_FILE_NOT_FOUND:
		job.note("重启失败：指定的文件未找到！", 1);
		break;
	case ERROR_PATH_NOT_FOUND:
		job.note("重启失败：指定的路径未找到！", 1);
		break;
	default:
		if (res > 31)job.note("重启成功√", 0);
		else job.note("重启失败：未知错误" + to_string(res), 0);
		break;
	}
}

void mirai_reload(DiceJob& job){
	using cq_reload_type = int(__stdcall*)(int32_t);
	HMODULE hModule = LoadLibraryA("CQP.dll");
	cq_reload_type cq_reload = (cq_reload_type)GetProcAddress(hModule, "CQ_reload");
	if (!cq_reload) {
		job.note("重载MiraiNative失败×\n使用了过旧或不适配的CQP.dll\n请保证更新适配版本的MiraiNative并删除旧CQP.dll", 0b10);
		return;
	}
	cq_reload(getAuthCode());
	FreeLibrary(hModule);
}

void auto_save(DiceJob& job) {
	if (sch.is_job_cold("autosave"))return;
	dataBackUp();
	console.log(GlobalMsg["strSelfName"] + "已自动保存", 0, printSTNow());
	if (console["AutoSaveInterval"] > 0) {
		sch.refresh_cold("autosave", time(NULL) + console["AutoSaveInterval"]);
		sch.add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	}
}

void check_system(DiceJob& job) {
	static int perRAM(0), perLastRAM(0);
	static double  perLastCPU(0), perLastDisk(0),
		 perCPU(0), perDisk(0);
	static bool isAlarmRAM(false), isAlarmCPU(false), isAlarmDisk(false);
	static double mbFreeBytes = 0, mbTotalBytes = 0;
	//内存检测
	if (console["SystemAlarmRAM"] > 0) {
		perRAM = getRamPort();
		if (perRAM > console["SystemAlarmRAM"] && perRAM > perLastRAM) {
			console.log("警告：" + GlobalMsg["strSelfName"] + "所在系统内存占用达" + to_string(perRAM) + "%", 0b1000, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = true;
		}
		else if (perLastRAM > console["SystemAlarmRAM"] && perRAM < console["SystemAlarmRAM"]) {
			console.log("提醒：" + GlobalMsg["strSelfName"] + "所在系统内存占用降至" + to_string(perRAM) + "%", 0b10, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = false;
		}
	}
	//CPU检测
	if (console["SystemAlarmCPU"] > 0) {
		perCPU = getWinCpuUsage() / 10.0;
		if (perCPU > console["SystemAlarmCPU"] && (!isAlarmCPU || perCPU > perLastCPU + 1)) {
			console.log("警告：" + GlobalMsg["strSelfName"] + "所在系统CPU占用达" + toString(perCPU) + "%", 0b1000, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = true;
		}
		else if (perLastCPU > console["SystemAlarmCPU"] && perCPU < console["SystemAlarmCPU"]) {
			console.log("提醒：" + GlobalMsg["strSelfName"] + "所在系统CPU占用降至" + toString(perCPU) + "%", 0b10, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = false;
		}
	}
	//硬盘检测
	if (console["SystemAlarmRAM"] > 0) {
		perDisk = getDiskUsage(mbFreeBytes, mbTotalBytes) / 10.0;
		if (perDisk > console["SystemAlarmDisk"] && (!isAlarmDisk || perDisk > perLastDisk + 1)) {
			console.log("警告：" + GlobalMsg["strSelfName"] + "所在系统硬盘占用达" + toString(perDisk) + "%", 0b1000, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = true;
		}
		else if (perLastDisk > console["SystemAlarmDisk"] && perDisk < console["SystemAlarmDisk"]) {
			console.log("提醒：" + GlobalMsg["strSelfName"] + "所在系统硬盘占用降至" + toString(perDisk) + "%", 0b10, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = false;
		}
	}
	if (isAlarmRAM || isAlarmCPU || isAlarmDisk) {
		sch.add_job_for(5 * 60, job);
	}
	else {
		sch.add_job_for(30 * 60, job);
	}
}

//被引用的图片列表
void clear_image(DiceJob& job) {
	if (!job.fromQQ) {
		if (sch.is_job_cold("clrimage"))return;
		if (console["AutoClearImage"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
	}
	scanImage(GlobalMsg, sReferencedImage);
	scanImage(HelpDoc, sReferencedImage);
	scanImage(CardDeck::mPublicDeck, sReferencedImage);
	scanImage(CardDeck::mReplyDeck, sReferencedImage);
	scanImage(CardDeck::mGroupDeck, sReferencedImage);
	scanImage(CardDeck::mPrivateDeck, sReferencedImage);
	for (auto it : ChatList) {
		scanImage(it.second.strConf, sReferencedImage);
	}
	job.note("整理" + GlobalMsg["strSelfName"] + "被引用图片" + to_string(sReferencedImage.size()) + "项", 0b0);
	int cnt = clrDir("data\\image\\", sReferencedImage);
	job.note("已清理image文件"+ to_string(cnt) + "项", 1);
	if (console["AutoClearImage"] > 0) {
		sch.refresh_cold("clrimage", time(NULL) + console["AutoClearImage"]);
		sch.add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	}
}

void clear_group(DiceJob& job) {
	int intCnt = 0;
	ResList res;
	std::map<string, string>strVar;
	if (job.strVar["clear_mode"] == "unpower") {
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.DiceMaid).permissions == 1) {
				res << printGroup(id);
				grp.leave(getMsg("strLeaveNoPower"));
				intCnt++;
				this_thread::sleep_for(3s);
			}
		}
		console.log(GlobalMsg["strSelfName"] + "筛除无群权限群聊" + to_string(intCnt) + "个:" + res.show(), 0b10, printSTNow());
	}
	else if (isdigit(static_cast<unsigned char>(job.strVar["clear_mode"][0]))) {
		int intDayLim = stoi(job.strVar["clear_mode"]);
		string strDayLim = to_string(intDayLim);
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清"))continue;
			time_t tLast = grp.tLastMsg;
			if (int tLMT; grp.isGroup && (tLMT = getGroupMemberInfo(id, console.DiceMaid).LastMsgTime) > 0 && tLMT > tLast)tLast = tLMT;
			if (!tLast)continue;
			int intDay = (int)(tNow - tLast) / 86400;
			if (intDay > intDayLim) {
				strVar["day"] = to_string(intDay);
				res << printGroup(id) + ":" + to_string(intDay) + "天\n";
				grp.leave(getMsg("strLeaveUnused", strVar));
				intCnt++;
				this_thread::sleep_for(2s);
			}
		}
		console.log(GlobalMsg["strSelfName"] + "已筛除潜水" + strDayLim + "天群聊" + to_string(intCnt) + "个√" + res.show(), 0b10, printSTNow());
	}
	else if (job.strVar["clear_mode"] == "black") {
		try {
			for (auto& [id, grp_name] : getGroupList()) {
				Chat& grp = chat(id).group().name(grp_name);
				if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清") || grp.isset("免黑"))continue;
				if (blacklist->get_group_danger(id)) {
					res << printGroup(id) + "：" + "黑名单群";
					if (console["LeaveBlackGroup"])grp.leave(getMsg("strBlackGroup"));
				}
				vector<GroupMemberInfo> MemberList = getGroupMemberList(id);
				for (auto eachQQ : MemberList) {
					if (blacklist->get_qq_danger(eachQQ.QQID) > 1) {
						if (eachQQ.permissions < getGroupMemberInfo(id, getLoginQQ()).permissions) {
							continue;
						}
						else if (eachQQ.permissions > getGroupMemberInfo(id, getLoginQQ()).permissions) {
							res << printChat(grp) + "：" + printQQ(eachQQ.QQID) + "对方群权限较高";
							grp.leave("发现黑名单管理员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
							intCnt++;
							break;
						}
						else if (console["LeaveBlackQQ"]) {
							res << printChat(grp) + "：" + printQQ(eachQQ.QQID);
							grp.leave("发现黑名单成员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
							intCnt++;
							break;
						}
					}
				}
			}
		}
		catch (...) {
			console.log("提醒：" + GlobalMsg["strSelfName"] + "清查黑名单群聊时出错！", 0b10, printSTNow());
		}
		if (intCnt) {
			job.note("已按" + getMsg("strSelfName") + "黑名单清查群聊" + to_string(intCnt) + "个：" + res.show(), 0b10);
		}
		else if (job.fromQQ) {
			job.echo(getMsg("strSelfName") + "按黑名单未发现待清查群聊");
		}
	}
	else if (job["clear_mode"] == "preserve") {
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("使用许可") || grp.isset("免清"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.master()).permissions) {
				grp.set("使用许可");
				continue;
			}
			res << printChat(grp);
			grp.leave(getMsg("strPreserve"));
			intCnt++;
			this_thread::sleep_for(3s);
		}
		console.log(GlobalMsg["strSelfName"] + "筛除无许可群聊" + to_string(intCnt) + "个：" + res.show(), 1, printSTNow());
	}
	else
		job.echo("无法识别筛选参数×");
}

//心跳检测
void cloud_beat(DiceJob& job) {
	Cloud::update();
	sch.add_job_for(5 * 60, job);
}

void dice_update(DiceJob& job) {
	job.note("开始更新Dice\n版本:" + job.strVar["ver"], 1);
	if (Mirai) {
		mkDir("plugins/MiraiNative/pluginsnew");
		char pathDll[] = "plugins/MiraiNative/pluginsnew/com.w4123.dice.dll";
		char pathJson[] = "plugins/MiraiNative/pluginsnew/com.w4123.dice.json";
		string urlDll("https://shiki.stringempty.xyz/DiceVer/" + job.strVar["ver"] + "/com.w4123.dice.dll?" + to_string(job.fromTime));
		string urlJson("https://shiki.stringempty.xyz/DiceVer/" + job.strVar["ver"] + "/com.w4123.dice.json?" + to_string(job.fromTime));
		switch (Cloud::DownloadFile(urlDll.c_str(), pathDll)) {
		case -1:
			job.echo("更新失败:" + urlDll);
			break;
		case -2:
			job.note("更新Dice失败!dll文件未下载到指定位置", 0b1);
			break;
		case 0:
		default:
			switch (Cloud::DownloadFile(urlJson.c_str(), pathJson)) {
			case -1:
				job.echo("更新失败:" + urlJson);
				break;
			case -2:
				job.note("更新Dice失败!json文件未下载到指定位置", 0b1);
				break;
			case 0:
			default:
				job.note("更新Dice!" + job.strVar["ver"] + "版成功√", 1);
			}
		}
	}
	else {
		char** path = new char* ();
		_get_pgmptr(path);
		string strAppPath(*path);
		delete path;
		strAppPath = strAppPath.substr(0, strAppPath.find_last_of("\\")) + "\\app\\com.w4123.dice.cpk";
		string strURL("https://shiki.stringempty.xyz/DiceVer/" + job.strVar["ver"] + "?" + to_string(job.fromTime));
		switch (Cloud::DownloadFile(strURL.c_str(), strAppPath.c_str())) {
		case -1:
			job.echo("更新失败:" + strURL);
			break;
		case -2:
			job.note("更新Dice失败!文件未找到:" + strAppPath, 0b10);
			break;
		case 0:
			job.note("更新Dice!" + job.strVar["ver"] + "版成功√\n可用.system reload 重启应用更新", 1);
		}
	}
}

//获取云不良记录
void dice_cloudblack(DiceJob& job) {
	job.echo("开始获取云端记录"); 
	string strURL("https://shiki.stringempty.xyz/blacklist/checked.json?" + to_string(job.fromTime));
	switch (Cloud::DownloadFile(strURL.c_str(), (DiceDir + "/conf/CloudBlackList.json").c_str())) {
	case -1:
		job.echo("同步云不良记录同步失败:" + strURL);
		break;
	case -2:
		job.echo("同步云不良记录同步失败!文件未找到");
		break;
	case 0:
		job.note("同步云不良记录成功，" + getMsg("self") + "开始读取", 1);
		blacklist->loadJson(DiceDir + "/conf/CloudBlackList.json", true);
	}
}

void log_put(DiceJob& job) {
	job["ret"] = put_s3_object("dicelogger",
							   job.strVar["log_file"],
							   job.strVar["log_path"],
							   "ap-southeast-1");
	if (job["ret"] == "SUCCESS") {
		job.echo(getMsg("strLogUpSuccess", job.strVar));
	}
	else if (job.cntExec++ > 1) {
		job.echo(getMsg("strLogUpFailureEnd",job.strVar));
	}
	else {
		job["retry"] = to_string(job.cntExec);
		job.echo(getMsg("strLogUpFailure", job.strVar));
		sch.add_job_for(2 * 60, job);
	}
}

string print_master() {
	if (!console.master())return "（无主）";
	return printQQ(console.master());
}

string list_deck() {
	return listKey(CardDeck::mPublicDeck);
}
string list_extern_deck() {
	return listKey(CardDeck::mExternPublicDeck);
}
