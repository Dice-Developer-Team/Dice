#pragma once
/*
 * Copyright (C) 2019-2024 String.Empty
 * ����ʱ�¼�
 * �����ܼ�ʱ��ɵ�ָ��
 * 2022/1/13: ��ȴ��ʱ
 * 2022/2/05: ��������������չ
 * 2022/8/08: ������
 */

#include <mutex>
#include <optional>
#include "DiceMsgSend.h"
#include "json.hpp"
#include "DiceEvent.h"
#include "STLExtern.hpp"

using std::shared_ptr;

extern AttrGetters MsgIndexs;

/*
class DiceJob : public DiceJobDetail {
	enum class Renum { NIL, Retry_For, Retry_Until };
public:
	DiceJob(DiceJobDetail detail) :DiceJobDetail(detail) {}
	Renum ren = Renum::NIL;
	void exec();
	void echo(const std::string&);
	void reply(const std::string&);
	void note(const std::string&, int);
};
*/

struct CDQuest {
	chatInfo chat;
	string key;
	time_t cd;
	CDQuest(const chatInfo& ct, const string& k, time_t t) :chat(ct), key(k), cd(t) {}
};
enum class CDType :size_t { Chat, User, Global };
struct CDConfig {
	CDType type;
	string key;
	time_t cd;
	CDConfig(const CDType& ct, const string& k, time_t t) :type(ct), key(k), cd(t) {}
	static enumap<string> eType;
};

class DiceScheduler {
	//�¼���ȴ��
	unordered_map<string, time_t> untilJobs;
	unordered_map<chatInfo, unordered_map<string, time_t>> cd_timer;
public:
	unordered_map<chatInfo, unordered_map<string, std::mutex>> locker;
	void start();
	void end();
	void push_job(const AttrObject&);
	void push_job(const char*, bool = false, const AttrVars& = {});
	void add_job_for(unsigned int, const AttrObject&);
	void add_job_for(unsigned int, const char*);
	void add_job_until(time_t, const AttrObject&);
	void add_job_until(time_t, const char*);
	bool is_job_cold(const char*);
	void refresh_cold(const char*, time_t);
	//0:success -1:colding -2:limited
	int cnt_cd(const vector<CDQuest>&, const vector<CDQuest>&);
};
inline DiceScheduler sch;

typedef void (*cmd)(AttrObject&);

//���ռ�¼
class DiceToday {
	tm stToday;
	unordered_map<long long, AttrObject>UserInfo;
	std::filesystem::path pathFile;
public:
	unordered_map<chatInfo, unordered_map<string, int>> counter;
	DiceToday() {
		load();
	}
	void load();
	void save();
	void set(long long qq, const string& key, const AttrVar& val);
	void inc(const string& key);
	//void inc(long long qq, const string& key, int cnt = 1) { cntUser[qq][key] += cnt; save(); }
	unordered_map<long long, AttrObject>& getUserInfo() { return UserInfo; }
	AttrVar& get(const string& key) { return get(0)->at(key); }
	AttrObject& get(long long uid) {
		if (!UserInfo.count(uid))UserInfo.emplace(uid, AnysTable());
		return UserInfo[uid];
	}
	//AttrVar& get(long long uid, const string& key) { return UserInfo[uid].to_dict()[key]; }
	std::optional<AttrVar> get_if(long long qq, const string& key) {
		if (UserInfo.count(qq) && UserInfo[qq]->has(key))
			return UserInfo[qq]->at(key);
		else return std::nullopt;
	}
	AttrVar getJrrp(long long);
	size_t cntUser() { return UserInfo.size(); }
	void daily_clear();
};
inline std::unique_ptr<DiceToday> today;

string printTTime(time_t tt);