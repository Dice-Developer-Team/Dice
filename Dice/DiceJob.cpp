#include "DiceJob.h"
#include "DiceConsole.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#endif

#include "StrExtern.hpp"
#include "DDAPI.h"
#include "ManagerSystem.h"
#include "DiceCloud.h"
#include "BlackListManager.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "DiceMod.h"
#include "DiceNetwork.h"
#include "S3PutObject.h"
#pragma warning(disable:28159)

using namespace std;

int sendSelf(const string& msg) {
	static long long selfQQ = DD::getLoginQQ();
	DD::sendPrivateMsg(selfQQ, msg);
	return 0;
}

void cq_exit(DiceJob& job) {
#ifdef _WIN32
	job.note("已令" + getMsg("self") + "在5秒后自杀", 1);
	std::this_thread::sleep_for(5s);
	dataBackUp();
	DD::killme();
#endif
}

#ifdef _WIN32
inline PROCESSENTRY32 getProcess(int pid) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	Process32First(hParentProcess, &pe32);
	return pe32;
}
#endif

void frame_restart(DiceJob& job) {
#ifdef _WIN32
	if (!job.fromQQ) {
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
	DD::remake();
#endif
}

void frame_reload(DiceJob& job){
	if(DD::reload())
		job.note("重载" + getMsg("self") + "完成√", 1);
	else
		job.note("重载" + getMsg("self") + "失败×", 0b10);
}

void check_system(DiceJob& job) {
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
		if (perCPU > 99.9) {
			this_thread::sleep_for(10s);
			perCPU = getWinCpuUsage() / 10.0;
		}
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
	#endif
}



void auto_save(DiceJob& job) {
	if (sch.is_job_cold("autosave"))return;
	dataBackUp();
	//console.log(GlobalMsg["strSelfName"] + "已自动保存", 0, printSTNow());
	if (console["AutoSaveInterval"] > 0) {
		sch.refresh_cold("autosave", time(NULL) + console["AutoSaveInterval"]);
		sch.add_job_for(console["AutoSaveInterval"] * 60, "autosave");
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
	for (auto it : ChatList) {
		scanImage(it.second.strConf, sReferencedImage);
	}
	job.note("整理" + GlobalMsg["strSelfName"] + "被引用图片" + to_string(sReferencedImage.size()) + "项", 0b0);
	int cnt = clrDir("data/image/", sReferencedImage);
	job.note("已清理image文件"+ to_string(cnt) + "项", 1);
	if (console["AutoClearImage"] > 0) {
		sch.refresh_cold("clrimage", time(NULL) + console["AutoClearImage"]);
		sch.add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	}
}

void clear_group(DiceJob& job) {
	int intCnt = 0;
	ResList res;
	if (job.strVar["clear_mode"] == "unpower") {
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清") || grp.isset("协议无效"))continue;
			if (grp.isGroup && !DD::isGroupAdmin(id, console.DiceMaid, true)) {
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
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清") || grp.isset("协议无效"))continue;
			time_t tLast = grp.tUpdated;
			if (long long tLMT; grp.isGroup && ((tLMT = DD::getGroupLastMsg(id, console.DiceMaid)) > 0 && tLMT > tLast))tLast = tLMT;
			if (!tLast)continue;
			int intDay = (int)(tNow - tLast) / 86400;
			if (intDay > intDayLim) {
				job["day"] = to_string(intDay);
				res << printGroup(id) + ":" + to_string(intDay) + "天\n";
				grp.leave(getMsg("strLeaveUnused", job.strVar));
				intCnt++;
				this_thread::sleep_for(2s);
			}
		}
		job.note(GlobalMsg["strSelfName"] + "已筛除潜水" + strDayLim + "天群聊" + to_string(intCnt) + "个√" + res.show(), 0b10);
	}
	else if (job.strVar["clear_mode"] == "black") {
		try {
			for (auto id : DD::getGroupIDList()) {
				Chat& grp = chat(id).group().name(DD::getGroupName(id));
				if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清") || grp.isset("免黑") || grp.isset("协议无效"))continue;
				if (blacklist->get_group_danger(id)) {
					res << printGroup(id) + "：" + "黑名单群";
					if (console["LeaveBlackGroup"])grp.leave(getMsg("strBlackGroup"));
				}
				set<long long> MemberList{ DD::getGroupMemberList(id) };
				int authSelf{ DD::getGroupAuth(id, console.DiceMaid, 1) };
				for (auto eachQQ : MemberList) {
					if (blacklist->get_qq_danger(eachQQ) > 1) {
						if (auto authBlack{DD::getGroupAuth(id, eachQQ, 1)};authBlack < authSelf) {
							continue;
						}
						else if (authBlack > authSelf) {
							res << printChat(grp) + "：" + printQQ(eachQQ) + "对方群权限较高";
							grp.leave("发现黑名单管理员" + printQQ(eachQQ) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
							intCnt++;
							break;
						}
						else if (console["LeaveBlackQQ"]) {
							res << printChat(grp) + "：" + printQQ(eachQQ);
							grp.leave("发现黑名单成员" + printQQ(eachQQ) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
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
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("使用许可") || grp.isset("免清") || grp.isset("协议无效"))continue;
			if (grp.isGroup && DD::isGroupAdmin(id, console.master(), false)) {
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
void list_group(DiceJob& job) {
	if (job["list_mode"].empty()) {
		job.reply(fmt->get_help("groups_list"));
	}
	if (mChatConf.count(job["list_mode"])) {
		ResList res;
		for (auto& [id, grp] : ChatList) {
			if (grp.isset(job["list_mode"])) {
				res << printChat(grp);
			}
		}
		job.reply("{self}含词条" + job["list_mode"] + "群记录" + to_string(res.size()) + "条" + res.head(":").show());
	}
	else if (set<long long> grps(DD::getGroupIDList()); job["list_mode"] == "idle") {
		std::priority_queue<std::pair<time_t, string>> qDiver;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isGroup && !grps.empty() && !grps.count(id))grp.set("已退");
			if (grp.isset("已退") || grp.isset("未进"))continue;
			time_t tLast = grp.tUpdated;
			if (long long tLMT; grp.isGroup && (tLMT = DD::getGroupLastMsg(grp.ID, console.DiceMaid)) > 0 && tLMT > tLast)tLast = tLMT;
			if (!tLast)continue;
			int intDay = (int)(tNow - tLast) / 86400;
			qDiver.emplace(intDay, printGroup(id));
		}
		if (qDiver.empty()) {
			job.reply("{self}无群聊或群信息加载失败！");
		}
		size_t intCnt(0);
		ResList res;
		while (!qDiver.empty()) {
			res << qDiver.top().second + to_string(qDiver.top().first) + "天";
			qDiver.pop();
			if (++intCnt > 32 || qDiver.top().first < 7)break;
		}
		job.reply("{self}所在闲置群列表:" + res.show(1));
	}
	else if (job["list_mode"] == "size") {
		std::priority_queue<std::pair<time_t, string>> qSize;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isGroup && !grps.empty() && !grps.count(id))grp.set("已退");
			if (grp.isset("已退") || grp.isset("未进") || !grp.isGroup)continue;
			Size size(DD::getGroupSize(id));
			if (!size.siz)continue;
			qSize.emplace(size.siz, DD::printGroupInfo(id));
		}
		if (qSize.empty()) {
			job.reply("{self}无群聊或群信息加载失败！");
		}
		size_t intCnt(0);
		ResList res;
		while (!qSize.empty()) {
			res << qSize.top().second;
			qSize.pop();
			if (++intCnt > 32 || qSize.top().first < 7)break;
		}
		job.reply("{self}所在大群列表:" + res.show(1));
	}
}

//心跳检测
void cloud_beat(DiceJob& job) {
	Cloud::heartbeat();
	sch.add_job_for(5 * 60, job);
}

void dice_update(DiceJob& job) {
	job.note("开始更新Dice\n版本:" + job.strVar["ver"], 1);
	string ret;
	if (DD::updateDice(job.strVar["ver"], ret)) {
		job.note("更新Dice!" + job.strVar["ver"] + "版成功√", 1);
	}
	else {
		job.echo("更新失败:" + ret);
	}
}

//获取云不良记录
void dice_cloudblack(DiceJob& job) {
	bool isSuccess(false);
	job.echo("开始获取云端记录"); 
	string strURL("https://shiki.stringempty.xyz/blacklist/checked.json?" + to_string(job.fromTime));
	switch (Cloud::DownloadFile(strURL.c_str(), DiceDir / "conf" / "CloudBlackList.json")) {
	case -1: {
		string des;
		if (Network::GET("shiki.stringempty.xyz", "/blacklist/checked.json", 80, des)) {
			ofstream fout(DiceDir / "conf" / "CloudBlackList.json");
			fout << des << endl;
			isSuccess = true;
		}
		else
			job.echo("同步云不良记录同步失败:" + des);
	}
		break;
	case -2:
		job.echo("同步云不良记录同步失败!文件未找到");
		break;
	case 0:
	default:
		break;
	}
	if (isSuccess) {
		if (job.fromQQ)job.note("同步云不良记录成功，" + getMsg("self") + "开始读取", 1);
		blacklist->loadJson(DiceDir / "conf" / "CloudBlackList.json", true);
	}
	if (console["CloudBlackShare"])
		sch.add_job_for(24 * 60 * 60, "cloudblack");
}

void log_put(DiceJob& job) {
	job["ret"] = put_s3_object("dicelogger",
							   job.strVar["log_file"].c_str(),
							   job.strVar["log_path"].c_str(),
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
string list_order_ex() {
	return fmt->list_order();
}
string list_dice_sister() {
	std::set<long long>list{ DD::getDiceSisters() };
	if (list.size() <= 1)return {};
	else {
		list.erase(console.DiceMaid);
		ResList li;
		li << printQQ(console.DiceMaid) + "的姐妹骰:";
		for (auto dice : list) {
			li << printQQ(dice);
		}
		return li.show();
	}
}