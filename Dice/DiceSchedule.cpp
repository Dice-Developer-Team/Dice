#include <mutex>
#include <condition_variable>
#include <deque>
#include "GlobalVar.h"
#include "DiceJob.h" 
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceSchedule.h"

unordered_map<string, cmd> mCommand = {
	{"syscheck",check_system},
	{"autosave",auto_save},
	{"clrimage",clear_image},
	{"reload",mirai_reload},
	{"remake",cq_restart},
	{"die",cq_exit},
	{"heartbeat",cloud_beat},
	{"update",dice_update},
	{"cloudblack",dice_cloudblack},
	{"uplog",log_put}
};


void DiceJob::exec() {
	if (auto it = mCommand.find(cmd_key); it != mCommand.end()) {
		it->second(*this);
	}
	else return;
}
void DiceJob::echo(const std::string& msg) {
	switch (fromChat.second) {
	case CQ::msgtype::Private:
		CQ::sendPrivateMsg(fromQQ, msg);
		break;
	case CQ::msgtype::Group:
		CQ::sendGroupMsg(fromChat.first, msg);
		break;
	case CQ::msgtype::Discuss:
		CQ::sendDiscussMsg(fromChat.first, msg);
		break;
	}
}
void DiceJob::note(const std::string& strMsg, int note_lv = 0b1) {
	ofstream fout(DiceDir + "/audit/log" + to_string(console.DiceMaid) + "_" + printDate() + ".txt", ios::out | ios::app);
	fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	echo(strMsg);
	string note = fromQQ ? getName(fromQQ) + strMsg : strMsg;
	for (const auto& [ct, level] : console.NoticeList) {
		if (!(level & note_lv) || pair(fromQQ, CQ::msgtype::Private) == ct || ct == fromChat)continue;
		AddMsgToQueue(note, ct);
	}
}

// 待处理任务队列
std::queue<DiceJobDetail> queueJob;
std::mutex mtQueueJob;
std::condition_variable cvJob;
std::condition_variable cvJobWaited;
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
			cvJobWaited.notify_one();
		}
		else{
			cvJob.wait_for(lock_queue, 2s, []() {return !queueJob.empty(); });
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
			cvJobWaited.wait_for(lock_queue, 1s);
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
	cvJob.notify_one();
}
void DiceScheduler::push_job(const char* job_name) {
	if (!Enabled)return; 
	{
		std::unique_lock<std::mutex> lock_queue(mtQueueJob);
		queueJob.emplace(job_name);
	}
	cvJob.notify_one();
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
	push_job("syscheck");
	if (console["AutoSaveInterval"] > 0)add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	if (console["AutoClearImage"] > 0)add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	else add_job_for(60 * 60, "clrimage");
}
void DiceScheduler::end() {
}

void DiceToday::daily_clear() {
	GetLocalTime(&stNow);
	if (stToday.wDay != stNow.wDay) {
		stToday = stNow;
		cntGlobal.clear();
	}
}

void DiceToday::save() {
	json jFile;
	jFile["date"] = { stToday.wYear,stToday.wMonth,stToday.wDay };
	jFile["global"] = cntGlobal;
	jFile["user_cnt"] = cntUser;
	fwriteJson(pathFile, jFile);
}
void DiceToday::load() {
	json jFile = freadJson(pathFile);
	if (jFile.is_null()) {
		GetLocalTime(&stToday);
		return;
	}
	if (jFile.count("date")) {
		jFile["date"][0].get_to(stToday.wYear);
		jFile["date"][1].get_to(stToday.wMonth);
		jFile["date"][2].get_to(stToday.wDay);
	}
	if (jFile.count("global")) { jFile["global"].get_to(cntGlobal); }
	if (jFile.count("user_cnt")) { jFile["user_cnt"].get_to(cntUser); }
}

string printTTime(time_t tt) {
	char tm_buffer[20];
	tm t{};
	if (!tt || localtime_s(&t, &tt))return "1970-00-00 00:00:00"; 
	strftime(tm_buffer, 20, "%Y-%m-%d %H:%M:%S", &t);
	return tm_buffer;
}