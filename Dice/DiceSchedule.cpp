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

enumap<string> CDConfig::eType{ "chat","user","global" };

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

// 待处理任务队列
std::queue<AttrObject> queueJob;
std::mutex mtQueueJob;
//std::condition_variable cvJob;
//std::condition_variable cvJobWaited;
//延时任务队列
using waited_job = pair<time_t, AttrObject>;
std::priority_queue<waited_job, std::deque<waited_job>,std::greater<waited_job>> queueJobWaited;
std::mutex mtJobWaited;

void exec(AttrObject& job) {
	if (job->has("cmd")) {
		if (auto it = mCommand.find(job->get_str("cmd")); it != mCommand.end()) {
			it->second(job);
		}
	}
	else if (job->has("id")) {
		fmt->call_cycle_event(job->get_str("id"));
	}
}
void jobHandle() {
	while (Enabled) {
		//监听作业队列
		{
			std::unique_lock<std::mutex> lock_queue(mtQueueJob);
			while (Enabled && !queueJob.empty()) {
				AttrObject job(queueJob.front());
				queueJob.pop();
				lock_queue.unlock();
				exec(job);
				lock_queue.lock();
				//cvJobWaited.notify_one();
			}
		}
		//cvJob.wait_for(lock_queue, 2s, []() {return !Enabled || !queueJob.empty(); });
		std::this_thread::sleep_for(1s); 
	}
}
void jobWait() {
	while (Enabled) {
		//检查定时作业
		{
			std::unique_lock<std::mutex> lock_queue(mtJobWaited);
			while (Enabled && !queueJobWaited.empty() && queueJobWaited.top().first <= time(NULL)) {
				sch.push_job(queueJobWaited.top().second);
				queueJobWaited.pop();
			}
		}
		today->daily_clear();
		std::this_thread::sleep_for(1s); 
	}
}

//将任务加入执行队列
void DiceScheduler::push_job(const AttrObject& job) {
	if (!Enabled)return;
	{
		std::unique_lock<std::mutex> lock_queue(mtQueueJob);
		queueJob.push(job); 
	}
	//cvJob.notify_one();
}
void DiceScheduler::push_job(const char* job_name, bool isSelf, const AttrVars& vars) {
	if (!Enabled)return; 
	{
		std::unique_lock<std::mutex> lock_queue(mtQueueJob);
		AttrObject obj{ vars };
		obj->at("cmd") = job_name;
		queueJob.emplace(obj);
	}
	//cvJob.notify_one();
}
//将任务加入等待队列
void DiceScheduler::add_job_for(unsigned int waited, const AttrObject& job) {
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(time(nullptr) + waited, job);
}
void DiceScheduler::add_job_for(unsigned int waited, const char* job_name) {
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(time(nullptr) + waited, AttrVars{ { "cmd" , job_name } });
}

void DiceScheduler::add_job_until(time_t cloc, const AttrObject& job) {
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(cloc, job);
}
void DiceScheduler::add_job_until(time_t cloc, const char* job_name) {
	std::unique_lock<std::mutex> lock_queue(mtJobWaited);
	queueJobWaited.emplace(cloc, AttrVars{ { "cmd" , job_name } });
}

bool DiceScheduler::is_job_cold(const char* cmd) {
	return untilJobs[cmd] > time(NULL);
}
void DiceScheduler::refresh_cold(const char* cmd, time_t until) {
	untilJobs[cmd] = until;
}


std::mutex mtCDQuery;
bool DiceScheduler::cnt_cd(const vector<CDQuest>& cd_list, const vector<CDQuest>& cnt_list){
	std::unique_lock<std::mutex> lock_queue(mtCDQuery);
	time_t tNow{ time(nullptr) };
	for (auto& quest : cd_list) {
		if (cd_timer.count(quest.chat)
			&& cd_timer[quest.chat].count(quest.key)
			&& cd_timer[quest.chat][quest.key] > tNow) {
			return false;
		}
	}
	for (auto& quest : cnt_list) {
		if (today->counter.count(quest.chat)
			&& today->counter[quest.chat].count(quest.key)
			&& today->counter[quest.chat][quest.key] >= quest.cd) {
			return false;
		}
	}
	for (auto& quest : cd_list) {
		cd_timer[quest.chat][quest.key] = tNow + quest.cd;
	}
	if (!cnt_list.empty()) {
		for (auto& quest : cnt_list) {
			++today->counter[quest.chat][quest.key];
		}
		today->save();
	}
	return true;
}

void DiceScheduler::start() {
	threads(jobHandle);
	threads(jobWait);
	push_job("heartbeat");
	add_job_for(60, "syscheck");
	if (console["AutoSaveInterval"] > 0)add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	/*if (console["AutoFrameRemake"] > 0)
		add_job_for(console["AutoFrameRemake"] * 60 * 60, "remake");
	else add_job_for(60 * 60, "remake");*/
	add_job_for(120, "check_update");
	if (console["CloudBlackShare"] > 0)
		add_job_for(12 * 60 * 60, "cloudblack");
}
void DiceScheduler::end() {
}

AttrVar DiceToday::getJrrp(long long uid) {
	if (UserInfo.count(uid) && UserInfo[uid]->has("jrrp"))
		return UserInfo[uid]->get("jrrp");
	string frmdata = "QQ=" + to_string(console.DiceMaid) + "&v=20190114" + "&QueryQQ=" + to_string(uid);
	string res;
	if (Network::POST("http://api.kokona.tech:5555/jrrp", frmdata, "", res)) {
		return UserInfo[uid]->at("jrrp") = stoi(res);
	}
	else {
		if (!UserInfo[uid]->has("jrrp_local")) {
			UserInfo[uid]->at("jrrp_local") = RandomGenerator::Randint(1, 100);
			console.log(getMsg("strJrrpErr",
				AttrVars{ {"res", res} }
			), 0);
		}
		return UserInfo[uid]->get("jrrp_local");
	}
}

void DiceToday::daily_clear() {
	if (!Enabled)return;
	time_t tt = time(nullptr) + time_t(console["TimeZoneLag"]) * 3600;
	static tm newDay{};
#ifdef _MSC_VER
	localtime_s(&newDay, &tt);
#else
	localtime_r(&tt, &newDay);
#endif
	if (stToday.tm_mday != newDay.tm_mday) {
		fmt->call_hook_event(AnysTable{ {
			{"Event","DayEnd"},
			{"year",stToday.tm_year + 1900},
			{"month",stToday.tm_mon + 1},
			{"day",stToday.tm_mday},
			} });
		console.log("Today:" + printDate(stToday) + "->" + printDate(), 0);
		stToday.tm_year = newDay.tm_year;
		stToday.tm_mon = newDay.tm_mon;
		stToday.tm_mday = newDay.tm_mday;
		counter.clear();
		UserInfo.clear();
		pathFile = DiceDir / "user" / "daily" /
			("daily_" + printDate() + ".json");
		fmt->call_hook_event(AnysTable{ {
			{"Event","DayNew"},
			{"year",newDay.tm_year + 1900},
			{"month",newDay.tm_mon + 1},
			{"day",newDay.tm_mday},
			} });
	}
}
void DiceToday::set(long long qq, const string& key, const AttrVar& val) {
	if (val)
		get(qq)->set(key, val);
	else if (UserInfo.count(qq) && UserInfo[qq]->has(key)) {
		get(qq)->reset(key);
	}
	else return;
	save();
}
void DiceToday::inc(const string& key) {
	get(0)->inc(key);
	save();
}

std::mutex mtDailySave;
void DiceToday::save() {
	std::unique_lock<std::mutex> lock(mtDailySave);
	fifo_json jFile;
	try {
		jFile["date"] = { stToday.tm_year + 1900,stToday.tm_mon + 1,stToday.tm_mday };
		if (!UserInfo.empty()) {
			fifo_json& jCnt{ jFile["user"] = fifo_json::object() };
			for (auto& [id, user] : UserInfo) {
				jCnt[to_string(id)] = user->to_json();
			}
		}
		if (!counter.empty()) {
			fifo_json& jCnt{ jFile["cnt_list"] = fifo_json::array() };
			for (auto& [chat, cnt_list] : counter) {
				fifo_json j;
				j["chat"] = to_json(chat);
				j["cnt"] = GBKtoUTF8(cnt_list);
				jCnt.push_back(j);
			}
		}
		fwriteJson(pathFile, jFile, 0);
	} catch (std::exception& e) {
		console.log("每日记录保存失败:" + string(e.what()), 0b10);
	}
}
void DiceToday::load() {
	time_t tt{ time(nullptr) + time_t(console["TimeZoneLag"]) * 3600 };
#ifdef _MSC_VER
	localtime_s(&stToday, &tt);
#else
	localtime_r(&tt, &stToday);
#endif
	std::filesystem::create_directories(DiceDir / "user" / "daily");
	pathFile = DiceDir / "user" / "daily" /
		("daily_" + printDate() + ".json");
	try{
		fifo_json jFile;
		if (!std::filesystem::exists(pathFile)) {
			std::filesystem::path fileToday{ DiceDir / "user" / "DiceToday.json" };
			if (!std::filesystem::exists(fileToday) || (jFile = freadJson(fileToday)).is_null()) {
				save();
				return;
			}
			else if (!jFile.count("date")
				|| jFile["date"][2] != stToday.tm_mday
				|| jFile["date"][1] != (stToday.tm_mon + 1)
				|| jFile["date"][0] != (stToday.tm_year + 1900)) {
				std::filesystem::remove(fileToday);
				return;
			}
			else std::filesystem::rename(fileToday, pathFile);
		}
		else if ((jFile = freadJson(pathFile)).is_null()) {
			return;
		}
		if (jFile.count("cnt_list")) {
			for (auto& j : jFile["cnt_list"]) {
				chatInfo chat{ chatInfo::from_json(j["chat"]) };
				if (j.count("cnt"))j["cnt"].get_to(counter[chat]);
				counter[chat] = UTF8toGBK(counter[chat]);
			}
		}
		if (jFile.count("global")) {
			UserInfo[0] = AttrVar(jFile["global"]).to_obj();
		}
		if (jFile.count("user")) {
			for (auto& [key, val] : jFile["user"].items()) {
				UserInfo[std::stoll(key)] = AttrVar(val).to_obj();
			}
		}
	}
	catch (std::exception& e) {
		console.log("解析每日数据错误:" + string(e.what()), 0b10);
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
	Clock clockNow{ stNow.tm_hour,stNow.tm_min };
	while (Enabled) 		{
		time_t tt = time(nullptr);
#ifdef _MSC_VER
		localtime_s(&stNow, &tt);
#else
		localtime_r(&tt, &stNow);
#endif
		//分钟时点变动
		if (stTmp.tm_min != stNow.tm_min) {
			stTmp = stNow;
			clockNow = { stNow.tm_hour, stNow.tm_min };
			for (const auto& [clock, eve_type] : multi_range(console.mWorkClock, clockNow)){
				switch (Console::mClockEvent[eve_type]) {
				case 1:
					if (console["DisabledGlobal"]) {
						console.set("DisabledGlobal", 0);
						console.log(getMsg("strClockToWork"), 0b10000, "");
					}
					break;
				case 0:
					if (!console["DisabledGlobal"]) {
						console.set("DisabledGlobal", 1);
						console.log(getMsg("strClockOffWork"), 0b10000, "");
					}
					break;
				case 2:
					dataBackUp();
					console.log(getMsg("strSelfName") + "定时保存完成√", 1, printSTime(stTmp));
					break;
				case 3:
					if (int cnt{ clearGroup() }) {
						console.log("已清理过期群记录" + to_string(cnt) + "条", 1, printSTime(stTmp));
					}
					if (int cnt{ clearUser() }) {
						console.log("已清理无效或过期用户记录" + to_string(cnt) + "条", 1, printSTime(stTmp));
					}
					break;
				default:
					fmt->call_task(eve_type);
					break;
				}
			}
			for (const auto& [clock, eve] : multi_range(fmt->clock_events, clockNow)) {
				fmt->call_clock_event(eve);
			}
		}
		std::this_thread::sleep_for(100ms);
	}
}