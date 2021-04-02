#include <mutex>
#include <ctime>
#include <condition_variable>
#include <deque>
#include "DDAPI.h"
#include "GlobalVar.h"
#include "DiceJob.h" 
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceSchedule.h"
#include "DiceNetwork.h"
#include "DiceMod.h"
#include "RandomGenerator.h"
#include <condition_variable>
#include <chrono>

unordered_map<string, cmd> mCommand = {
	{"syscheck",check_system},
	{"autosave",auto_save},
	{"clrimage",clear_image},
	{"clrgroup",clear_group},
	{"lsgroup",list_group},
	{"reload",frame_reload},
	{"remake",frame_restart},
	{"die",cq_exit},
	{"heartbeat",cloud_beat},
	{"update",dice_update},
	{"cloudblack",dice_cloudblack},
	{"uplog",log_put}
};

DiceJobDetail::DiceJobDetail(const char* cmd, bool isFromSelf, unordered_map<string, string> vars) :cmd_key(cmd), strVar(vars) {
	if (isFromSelf)fromQQ = console.DiceMaid;
}

void DiceJob::exec() {
	if (auto it = mCommand.find(cmd_key); it != mCommand.end()) {
		it->second(*this);
	}
	else return;
}
void DiceJob::echo(const std::string& msg) {
	if (!fromChat.first)return;
	switch (fromChat.second) {
	case msgtype::Private:
		DD::sendPrivateMsg(fromQQ, msg);
		break;
	case msgtype::Group:
		DD::sendGroupMsg(fromChat.first, msg);
		break;
	case msgtype::Discuss:
		DD::sendDiscussMsg(fromChat.first, msg);
		break;
	}
}
void DiceJob::reply(const std::string& msg) {
	AddMsgToQueue(format(msg, GlobalMsg, strVar), fromChat);
}
void DiceJob::note(const std::string& strMsg, int note_lv = 0b1) {
	ofstream fout(DiceDir / "audit" / "log" / (to_string(console.DiceMaid) + "_" + printDate() + ".txt"), ios::out | ios::app);
	fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	echo(strMsg);
	string note = fromQQ ? getName(fromQQ) + strMsg : strMsg;
	for (const auto& [ct, level] : console.NoticeList) {
		if (!(level & note_lv) || pair(fromQQ, msgtype::Private) == ct || ct == fromChat)continue;
		AddMsgToQueue(note, ct);
	}
}

// 待处理任务队列
std::queue<DiceJobDetail> queueJob;
std::mutex mtQueueJob;
//std::condition_variable cvJob;
//std::condition_variable cvJobWaited;
//延时任务队列
using waited_job = pair<time_t, DiceJobDetail>;
std::priority_queue<waited_job, std::deque<waited_job>,std::greater<waited_job>> queueJobWaited;
std::mutex mtJobWaited;

void jobHandle() {
	while (Enabled) {
		//监听作业队列
		if (std::unique_lock<std::mutex> lock_queue(mtQueueJob); !queueJob.empty()) {
			DiceJob job(queueJob.front());
			queueJob.pop();
			lock_queue.unlock();
			job.exec();
			//cvJobWaited.notify_one();
		}
		else{
			//cvJob.wait_for(lock_queue, 2s, []() {return !Enabled || !queueJob.empty(); });
			std::this_thread::sleep_for(1s); 
		}
	}
}
void jobWait() {
	while (Enabled) {
		//检查定时作业
		if (std::unique_lock<std::mutex> lock_queue(mtJobWaited); !queueJobWaited.empty() && queueJobWaited.top().first <= time(NULL)) {
			sch.push_job(queueJobWaited.top().second);
			queueJobWaited.pop();
		}
		else {
			//cvJobWaited.wait_for(lock_queue, 1s);
			std::this_thread::sleep_for(1s); 
		}
		today->daily_clear();
	}
}

//将任务加入执行队列
void DiceScheduler::push_job(const DiceJobDetail& job) {
	if (!Enabled)return;
	{
		std::unique_lock<std::mutex> lock_queue(mtQueueJob);
		queueJob.push(job); 
	}
	//cvJob.notify_one();
}
void DiceScheduler::push_job(const char* job_name, bool isSelf, unordered_map<string,string>vars) {
	if (!Enabled)return; 
	{
		std::unique_lock<std::mutex> lock_queue(mtQueueJob);
		queueJob.emplace(job_name, isSelf, vars);
	}
	//cvJob.notify_one();
}
//将任务加入等待队列
void DiceScheduler::add_job_for(unsigned int waited, const DiceJobDetail& job) {
	if (!Enabled)return;
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(time(nullptr) + waited, job);
}
void DiceScheduler::add_job_for(unsigned int waited, const char* job_name) {
	if (!Enabled)return;
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(time(nullptr) + waited, job_name);
}

void DiceScheduler::add_job_until(time_t cloc, const DiceJobDetail& job) {
	if (!Enabled)return;
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(cloc, job);
}
void DiceScheduler::add_job_until(time_t cloc, const char* job_name) {
	if (!Enabled)return;
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(cloc, job_name);
}

bool DiceScheduler::is_job_cold(const char* cmd) {
	return untilJobs[cmd] > time(NULL);
}
void DiceScheduler::refresh_cold(const char* cmd, time_t until) {
	untilJobs[cmd] = until;
}

void DiceScheduler::start() {
	threads(jobHandle);
	threads(jobWait);
	push_job("heartbeat");
	add_job_for(60, "syscheck");
	if (console["AutoSaveInterval"] > 0)add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	if (console["AutoFrameRemake"] > 0)
		add_job_for(console["AutoFrameRemake"] * 60 * 60, "remake");
	else add_job_for(60 * 60, "remake");
}
void DiceScheduler::end() {
}

int DiceToday::getJrrp(long long qq) {
	if (cntUser.count(qq) && cntUser[qq].count("jrrp"))
		return cntUser[qq]["jrrp"];
	string frmdata = "QQ=" + to_string(console.DiceMaid) + "&v=20190114" + "&QueryQQ=" + to_string(qq);
	string res;
	if (Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata.data(), res)) {
		return cntUser[qq]["jrrp"] = stoi(res);
	}
	else {
		if (!cntUser[qq].count("jrrp_local"))
			cntUser[qq]["jrrp_local"] = RandomGenerator::Randint(1, 100);
		console.log(getMsg("strJrrpErr",
						   { {"res", res} }
		), 1);
		return cntUser[qq]["jrrp_local"];
	}
}

void DiceToday::daily_clear() {
	time_t tt = time(nullptr);
#ifdef _MSC_VER
	localtime_s(&stNow, &tt);
#else
	localtime_r(&tt, &stNow);
#endif
	if (stToday.tm_mday != stNow.tm_mday) {
		stToday.tm_mday = stNow.tm_mday;
		cntGlobal.clear();
		cntUser.clear();
	}
}

void DiceToday::save() {
	json jFile;
	try {
		jFile["date"] = { stToday.tm_year + 1900,stToday.tm_mon + 1,stToday.tm_mday };
		jFile["global"] = GBKtoUTF8(cntGlobal);
		jFile["user_cnt"] = GBKtoUTF8(cntUser);
		fwriteJson(pathFile, jFile);
	} catch (...) {
		console.log("每日记录保存失败:json错误!", 0b10);
	}
}
void DiceToday::load() {
	json jFile = freadJson(pathFile);
	if (jFile.is_null()) {
		time_t tt = time(nullptr);
#ifdef _MSC_VER
		localtime_s(&stToday, &tt);
#else
		localtime_r(&tt, &stToday);
#endif
		return;
	}
	if (jFile.count("date")) {
		jFile["date"][0].get_to(stToday.tm_year);
		stToday.tm_year -= 1900;
		jFile["date"][1].get_to(stToday.tm_mon);
		stToday.tm_mon -= 1;
		jFile["date"][2].get_to(stToday.tm_mday);
	}
	if (jFile.count("global")) { 
		jFile["global"].get_to(cntGlobal); 
		cntGlobal = UTF8toGBK(cntGlobal);
	}
	if (jFile.count("user_cnt")) { 
		jFile["user_cnt"].get_to(cntUser);
		cntUser = UTF8toGBK(cntUser);
	}
}

string printTTime(time_t tt) {
	char tm_buffer[20];
	tm t{};
	if(!tt) return "1970-00-00 00:00:00"; 
#ifdef _MSC_VER
	auto ret = localtime_s(&t, &tt);
	if(ret) return "1970-00-00 00:00:00";
#else
	auto ret = localtime_r(&tt, &t);
	if(!ret) return "1970-00-00 00:00:00";
#endif
	strftime(tm_buffer, 20, "%Y-%m-%d %H:%M:%S", &t);
	return tm_buffer;
}

//简易计时器
tm stTmp{};
void ConsoleTimer() 	{
	Console::Clock clockNow{ stNow.tm_hour,stNow.tm_min };
	while (Enabled) 		{
		time_t tt = time(nullptr);
#ifdef _MSC_VER
		localtime_s(&stNow, &tt);
#else
		localtime_r(&tt, &stNow);
#endif
		//分钟时点变动
		if (stTmp.tm_min != stNow.tm_min) 			{
			stTmp = stNow;
			clockNow = { stNow.tm_hour, stNow.tm_min };
			for (const auto& [clock, eve_type] : multi_range(console.mWorkClock, clockNow)){
				switch (Console::mClockEvent[eve_type]) 					{
				case 1:
					if (console["DisabledGlobal"]) 						{
						console.set("DisabledGlobal", 0);
						console.log(getMsg("strClockToWork"), 0b10000, "");
					}
					break;
				case 0:
					if (!console["DisabledGlobal"]) 						{
						console.set("DisabledGlobal", 1);
						console.log(getMsg("strClockOffWork"), 0b10000, "");
					}
					break;
				case 2:
					dataBackUp();
					console.log(GlobalMsg["strSelfName"] + "定时保存完成√", 1, printSTime(stTmp));
					break;
				case 3:
					sch.push_job("clrgroup", true, {
						{"clear_mode","black"}
								 });
					break;
				default:
					fmt->call_task(eve_type);
					break;
				}
			}
		}
		std::this_thread::sleep_for(100ms);
	}
}