/**
 * 黑名单明细
 * 更数据库式的管理
 * Copyright (C) 2019-2020 String.Empty
 */
#include <shared_mutex>
#include "Jsonio.h"
#include "DiceSession.h"
#include "MsgFormat.h"
#include "EncodingConvert.h"
#include "DiceEvent.h"

std::shared_mutex sessionMutex;

int DiceSession::table_add(string key, int prior, string item)
{
	mTable[key].emplace(item, prior);
	update();
	return 0;
}

string DiceSession::table_prior_show(string key) const
{
	return PriorList(mTable.find(key)->second).show();
}

bool DiceSession::table_clr(string key)
{
	if (const auto it = mTable.find(key); it != mTable.end())
	{
		mTable.erase(it);
		update();
		return true;
	}
	return false;
}

void DiceSession::ob_enter(FromMsg* msg)
{
	if (sOB.count(msg->fromQQ))
	{
		msg->reply(GlobalMsg["strObEnterAlready"]);
	}
	else
	{
		sOB.insert(msg->fromQQ);
		msg->reply(GlobalMsg["strObEnter"]);
	}
	update();
}

void DiceSession::ob_exit(FromMsg* msg)
{
	if (sOB.count(msg->fromQQ))
	{
		sOB.erase(msg->fromQQ);
		msg->reply(GlobalMsg["strObExit"]);
	}
	else
	{
		msg->reply(GlobalMsg["strObExitAlready"]);
	}
	update();
}

void DiceSession::ob_list(FromMsg* msg) const
{
	if (sOB.empty())msg->reply(GlobalMsg["strObListEmpty"]);
	else
	{
		ResList res;
		for (auto qq : sOB)
		{
			res << printQQ(qq);
		}
		msg->reply(GlobalMsg["strObList"] + res.linebreak().show());
	}
}

void DiceSession::ob_clr(FromMsg* msg)
{
	if (sOB.empty())msg->reply(GlobalMsg["strObListEmpty"]);
	else
	{
		sOB.clear();
		msg->reply(GlobalMsg["strObListClr"]);
	}
	update();
}

void DiceSession::log_new(FromMsg* msg) {
	mkDir(DiceDir + logger.dirLog);
	logger.tStart = time(nullptr);
	logger.isLogging = true;
	logger.fileLog = "group_" + to_string(msg->fromGroup) + "_" + to_string(time(nullptr)) + ".txt";
	//先发消息后插入
	msg->reply(GlobalMsg["strLogNew"]);
	LogList.insert(msg->fromGroup);
	update();
}
void DiceSession::log_on(FromMsg* msg) {
	if (!logger.tStart) {
		log_new(msg);
		return;
	}
	if (logger.isLogging) {
		msg->reply(GlobalMsg["strLogOnAlready"]);
		return;
	}
	logger.isLogging = true;
	msg->reply(GlobalMsg["strLogOn"]);
	LogList.insert(msg->fromGroup);
	update();
}
void DiceSession::log_off(FromMsg* msg) {
	if (!logger.tStart) {
		msg->reply(GlobalMsg["strLogNullErr"]);
		return;
	}
	if (!logger.isLogging) {
		msg->reply(GlobalMsg["strLogOffAlready"]);
		return;
	}
	logger.isLogging = false;
	//先擦除后发消息
	LogList.erase(msg->fromGroup);
	msg->reply(GlobalMsg["strLogOff"]);
	update();
}
void DiceSession::log_end(FromMsg* msg) {
	if (!logger.tStart) {
		msg->reply(GlobalMsg["strLogNullErr"]);
		return;
	}
	logger.isLogging = false;
	logger.tStart = time(0);
	msg->strVar["log_file"] = logger.fileLog;
	LogList.erase(msg->fromGroup);
	msg->reply(GlobalMsg["strLogEnd"]);
	update();
	msg->cmd_key = "uplog"; 
	msg->strVar["log_path"] = log_path();
}
string DiceSession::log_path() {
	return DiceDir + LogInfo::dirLog + "\\" + logger.fileLog; 
}

void DiceSession::save() const
{
	mkDir(DiceDir + "\\user\\session");
	ofstream fout(DiceDir + R"(\user\session\)" + to_string(room) + ".json");
	if (!fout)
	{
		console.log("群开团信息保存失败:" + DiceDir + R"(\user\session\)" + to_string(room), 1);
		return;
	}
	nlohmann::json jData;
	jData["type"] = "Simple";
	jData["room"] = room;
	jData["create_time"] = tCreate;
	jData["update_time"] = tUpdate;
	if (!sOB.empty())jData["observer"] = sOB;
	if (!mTable.empty())
		for (auto& [key, table] : mTable)
		{
			string strTable = GBKtoUTF8(key);
			for (auto& [item, val] : table)
			{
				jData["tables"][strTable][GBKtoUTF8(item)] = val;
			}
		}
	if (logger.tStart) {
		json& jLog = jData["log"];
		jLog["start"] = logger.tStart;
		jLog["lastMsg"] = logger.tLastMsg;
		jLog["file"] = logger.fileLog;
		jLog["logging"] = logger.isLogging;
	}
	fout << jData.dump(1);
}

Session& DiceTableMaster::session(long long group)
{
	if (!mSession.count(group))
	{
		std::unique_lock<std::shared_mutex> lock(sessionMutex);
		mSession.emplace(group, std::make_shared<Session>(group));
	}
	return *mSession[group];
}

void DiceTableMaster::session_end(long long group)
{
	std::unique_lock<std::shared_mutex> lock(sessionMutex);
	remove((DiceDir + R"(\user\session\)" + to_string(group)).c_str());
	mSession.erase(group);
}

const enumap<string> mSMTag{"type", "room", "gm", "log", "player", "observer", "tables"};

void DiceTableMaster::save()
{
	mkDir(DiceDir + "\\user\\session");
	std::shared_lock<std::shared_mutex> lock(sessionMutex);
	for (auto [grp, pSession] : mSession)
	{
		pSession->save();
	}
}

int DiceTableMaster::load() 
{
    string strLog;
    std::unique_lock<std::shared_mutex> lock(sessionMutex);
    vector<std::filesystem::path> sFile;
    int cnt = listDir(DiceDir + "\\user\\session\\", sFile);
    if (cnt <= 0)return cnt;
    for (auto& filename : sFile)
	{
        nlohmann::json j = freadJson(filename);
        if (j.is_null())
		{
            cnt--;
            continue;
        }
        if (j["type"] == "simple")
		{
            auto pSession(std::make_shared<Session>(j["room"]));
            pSession->create(j["create_time"]).update(j["update_time"]);
            if (j.count("observer")) pSession->sOB = j["observer"].get<set<long long>>();
            if (j.count("tables"))
			{
                for (nlohmann::json::iterator itTable = j["tables"].begin(); itTable != j["tables"].end(); ++itTable)
				{
                    string strTable = UTF8toGBK(itTable.key());
                    for (nlohmann::json::iterator itItem = itTable.value().begin(); itItem != j.end(); ++itItem)
					{
                        pSession->mTable[strTable].emplace(UTF8toGBK(itItem.key()), itItem.value());
                    }
                }
            }
			if (j.count("log")) {
				json& jLog = j["log"];
				jLog["start"].get_to(pSession->logger.tStart);
				jLog["lastMsg"].get_to(pSession->logger.tLastMsg);
				jLog["file"].get_to(pSession->logger.fileLog);
				jLog["logging"].get_to(pSession->logger.isLogging);
			}
        }
    }
    return cnt;
}
