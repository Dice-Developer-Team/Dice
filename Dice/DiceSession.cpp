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

int DiceSession::table_add(string key, int prior, string item) {
    mTable[key].emplace(item,prior);
    update();
    return 0;
}
string DiceSession::table_prior_show(string key)const {
    return PriorList(mTable.find(key)->second).show();
}
bool DiceSession::table_clr(string key) {
    if (auto it = mTable.find(key); it != mTable.end()) {
        mTable.erase(it);
        update();
        return true;
    }
    else {
        return false;
    }
}

int DiceSession::ob_enter(FromMsg* msg) {
    if (sOB.count(msg->fromQQ)) {
        msg->reply(GlobalMsg["strObEnterAlready"]);
    }
    else {
        sOB.insert(msg->fromQQ);
        msg->reply(GlobalMsg["strObEnter"]);
    }
    update();
    return 0;
}
int DiceSession::ob_exit(FromMsg* msg) {
    if (sOB.count(msg->fromQQ)) {
        sOB.erase(msg->fromQQ);
        msg->reply(GlobalMsg["strObExit"]);
    }
    else {
        msg->reply(GlobalMsg["strObExitAlready"]);
    }
    update();
    return 0;
}
int DiceSession::ob_list(FromMsg* msg)const {
    if (sOB.empty())msg->reply(GlobalMsg["strObListEmpty"]);
    else {
        ResList res;
        for (auto qq : sOB) {
            res << printQQ(qq);
        }
        msg->reply(GlobalMsg["strObList"] + res.linebreak().show());
    }
    return 0;
}
int DiceSession::ob_clr(FromMsg* msg) {
    if (sOB.empty())msg->reply(GlobalMsg["strObListEmpty"]);
    else {
        sOB.clear();
        msg->reply(GlobalMsg["strObListClr"]);
    }
    update();
    return 0;
}

void DiceSession::save() const {
    mkDir(DiceDir + "\\user\\session");
    ofstream fout(DiceDir + "\\user\\session\\" + to_string(room) + ".json");
    if (!fout) {
        console.log("群开团信息保存失败:" + DiceDir + "\\user\\session\\" + to_string(room), 1);
        return;
    }
    nlohmann::json jData;
    jData["type"] = "Simple";
    jData["room"] = room;
    jData["create_time"] = tCreate;
    jData["update_time"] = tUpdate;
    if(!sOB.empty())jData["observer"] = sOB;
    if(!mTable.empty())
        for (auto& [key, table] : mTable) {
            string strTable = GBKtoUTF8(key);
            for (auto& [item, val] : table) {
                jData["tables"][strTable][GBKtoUTF8(item)] = val;
            }
        }
    fout << jData.dump(1);
}

Session& DiceTableMaster::session(long long group) {
    if (!mSession.count(group)) {
        std::unique_lock<std::shared_mutex> lock(sessionMutex);
        mSession.emplace(group, std::make_shared<Session>(group));
    }
    return *mSession[group];
}

void DiceTableMaster::session_end(long long group) {
    std::unique_lock<std::shared_mutex> lock(sessionMutex);
    remove((DiceDir + "\\user\\session\\" + to_string(group)).c_str());
    mSession.erase(group);
}

const enumap<string> mSMTag{ "type", "room", "gm", "player", "observer", "tables" };

void DiceTableMaster::save() {
    mkDir(DiceDir + "\\user\\session");
    std::shared_lock<std::shared_mutex> lock(sessionMutex);
    for (auto [grp, pSession] : mSession) {
        pSession->save();
    }
}

int DiceTableMaster::load() {
    string strLog;
    std::unique_lock<std::shared_mutex> lock(sessionMutex);
    vector<std::filesystem::path> sFile;
    int cnt = listDir(DiceDir + "\\user\\session\\", sFile);
    if (cnt <= 0)return cnt;
    for (auto& filename : sFile) {
        nlohmann::json j = freadJson(filename);
        if (j.is_null()) {
            cnt--;
            continue;
        }
        if (j["type"] == "simple") {
            auto pSession(std::make_shared<Session>(j["room"]));
            pSession->create(j["create_time"]).update(j["update_time"]);
            if (j.count("observer")) pSession->sOB = j["observer"].get<set<long long>>();
            if (j.count("tables")) {
                for (nlohmann::json::iterator itTable = j["tables"].begin(); itTable != j["tables"].end(); ++itTable){
                    string strTable = UTF8toGBK(itTable.key());
                    for (nlohmann::json::iterator itItem = itTable.value().begin(); itItem != j.end(); ++itItem) {
                        pSession->mTable[strTable].emplace(UTF8toGBK(itItem.key()), itItem.value());
                    }
                }
            }
        }
    }
    return cnt;
}
