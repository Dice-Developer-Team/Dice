#include "DiceJob.h"
#include "DiceConsole.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#else
#include <curl/curl.h>
#endif
#include "StrExtern.hpp"
#include "OneBotAPI.h"
#include "CQTools.h"
#include "ManagerSystem.h"
#include "DiceCloud.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "DiceMod.h"
#include "DiceZip.h"
#include "DiceNetwork.h"
#include "DiceSession.h"
#include "DiceEvent.h"
#pragma warning(disable:28159)

using namespace std;

#ifdef _WIN32
inline PROCESSENTRY32 getProcess(int pid) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	Process32First(hParentProcess, &pe32);
	return pe32;
}
#endif

void frame_restart(AttrObject& job) {
#ifdef _WIN32
	if (!job.get_ll("uid")) {
		if (console["AutoFrameRemake"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
		else if (int tWait{ console["AutoFrameRemake"] * 60 * 60 - int(time(nullptr) - llStartTime) }; tWait > 0) {
			sch.add_job_for(tWait, job);
			return;
		}
	}
	Enabled = false;
	dataBackUp();
	std::this_thread::sleep_for(3s);
#endif
}

void frame_reload(AttrObject& job) {
}

void check_system(AttrObject& job) {
#ifdef _WIN32
	static int perRAM(0), perLastRAM(0);
	static double  perLastCPU(0), perLastDisk(0),
		perCPU(0), perDisk(0);
	static bool isAlarmRAM(false), isAlarmCPU(false), isAlarmDisk(false);
	static double mbFreeBytes = 0, mbTotalBytes = 0;
	//内存检测
	if (console["SystemAlarmRAM"] > 0) {
		perRAM = getRamPort();
		if (perRAM > console["SystemAlarmRAM"] && perRAM > perLastRAM) {
			console.log("警告：" + getMsg("strSelfName") + "所在系统内存占用达" + to_string(perRAM) + "%", 0b1000, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = true;
		}
		else if (perLastRAM > console["SystemAlarmRAM"] && perRAM < console["SystemAlarmRAM"]) {
			console.log("提醒：" + getMsg("strSelfName") + "所在系统内存占用降至" + to_string(perRAM) + "%", 0b10, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = false;
		}
	}
	//CPU检测
	if (console["SystemAlarmCPU"] > 0) {
		perCPU = getWinCpuUsage() / 10.0;
		if (perCPU > 99.9) {
			this_thread::sleep_for(30s);
			perCPU = getWinCpuUsage() / 10.0;
		}
		if (perCPU > console["SystemAlarmCPU"] && (!isAlarmCPU || perCPU > perLastCPU + 1)) {
			console.log("警告：" + getMsg("strSelfName") + "所在系统CPU占用达" + toString(perCPU) + "%", 0b1000, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = true;
		}
		else if (perLastCPU > console["SystemAlarmCPU"] && perCPU < console["SystemAlarmCPU"]) {
			console.log("提醒：" + getMsg("strSelfName") + "所在系统CPU占用降至" + toString(perCPU) + "%", 0b10, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = false;
		}
	}
	//硬盘检测
	if (console["SystemAlarmRAM"] > 0) {
		perDisk = getDiskUsage(mbFreeBytes, mbTotalBytes) / 10.0;
		if (perDisk > console["SystemAlarmDisk"] && (!isAlarmDisk || perDisk > perLastDisk + 1)) {
			console.log("警告：" + getMsg("strSelfName") + "所在系统硬盘占用达" + toString(perDisk) + "%", 0b1000, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = true;
		}
		else if (perLastDisk > console["SystemAlarmDisk"] && perDisk < console["SystemAlarmDisk"]) {
			console.log("提醒：" + getMsg("strSelfName") + "所在系统硬盘占用降至" + toString(perDisk) + "%", 0b10, printSTime(stNow));
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
#endif
}

void auto_save(AttrObject& job) {
	if (sch.is_job_cold("autosave"))return;
	api::printLog(printSTNow() + " 自动保存");
	dataBackUp();
	//console.log(getMsg("strSelfName") + "已自动保存", 0, printSTNow());
	if (console["AutoSaveInterval"] > 0) {
		sch.refresh_cold("autosave", time(NULL) + console["AutoSaveInterval"] * (time_t)60);
		sch.add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	}
}

//被引用的图片列表
void clear_image(AttrObject& job) {
	return;
	if (!job.has("uid")) {
		if (sch.is_job_cold("clrimage"))return;
		if (console["AutoClearImage"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
	}
	//MsgNote(job, "已清理image文件" + to_string(cnt) + "项", 1);
	if (console["AutoClearImage"] > 0) {
		sch.refresh_cold("clrimage", time(NULL) + console["AutoClearImage"]);
		sch.add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	}
}

void check_update(AttrObject& job) {
	string ret;
	if (!Network::GET("http://shiki.stringempty.xyz/DiceVer/update", ret)) {
		console.log("获取版本信息时出错: \n" + ret, 0);
		return;
	}
	string ver = isDev ? "dev" : "release";
	try {
		fifo_json jInfo(fifo_json::parse(ret));
		if (unsigned short nBuild{ jInfo[ver]["build"] }; nBuild > Dice_Build) {
			MsgNote(job, "发现Dice!的{ver}版本更新:" + jInfo[ver]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n更新说明：" +
				jInfo[ver]["changelog"].get<string>(), 1);
		}
	}
	catch (std::exception& e) {
		console.log(string("获取更新失败:") + e.what(), 0);
	}
	sch.add_job_for(72 * 60 * 60, "check_update");
}
void dice_update(AttrObject& job) {
	string ret;
	if (!Network::GET("http://shiki.stringempty.xyz/DiceVer/update", ret)) {
		reply(job, "{self}获取版本信息时出错: \n" + ret);
		return;
	}
	string ver{ job.get_str("ver")};
	if (ver.empty())ver = isDev ? "dev" : "release";
	try {
		fifo_json jInfo(fifo_json::parse(ret));
		if (unsigned short nBuild{ jInfo[ver]["build"] }; nBuild > Dice_Build) {
			MsgNote(job, "发现Dice!的{ver}版本更新:" + jInfo[ver]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n更新说明：" +
				jInfo[ver]["changelog"].get<string>(), 1);
			//if (DD::updateDice(ver, ret)) {
			//	MsgNote(job, "更新Dice!{ver}版本成功，将在重载后应用√", 1);
			//	return;
			//}
			if(jInfo[ver].count("pkg")) {
				string pkg{ jInfo[ver]["pkg"] };
				if (Network::GET(pkg, ret)) {
					std::error_code ec1;
					Zip::extractZip(ret, dirExe / "Diceki");
					MsgNote(job, "更新Dice!{ver}版本成功，将在重载后应用√", 1);
					return;
				}
			}
			reply(job, "{self}更新失败×" + ret);
		}
		else {
			reply(job, "{self}未发现更新{ver}版本");
		}
	} catch (std::exception& e) {
		job.set("err", e.what());
		reply(job, "{self}获取更新失败！{err}");
	}
}

string print_master() {
	if (!console)return "（无主）";
	return printUser(console);
}

string list_deck() {
	return showKey(CardDeck::mPublicDeck);
}
string list_extern_deck() {
	return showKey(CardDeck::mExternPublicDeck);
}