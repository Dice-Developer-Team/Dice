/**
 * 黑名单明细
 * 更数据库式的管理
 * Copyright (C) 2019-2024 String.Empty
 */
#include <mutex>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include "BlackListManager.h"
#include "Jsonio.h"
#include "STLExtern.hpp"
#include "DDAPI.h"
#include "DiceEvent.h"
#include "DiceConsole.h"
#include "DiceNetwork.h"

using namespace std;

using Mark = DDBlackMark;
using Manager = DDBlackManager;
using Factory = DDBlackMarkFactory;

//判断信任
static int isReliable(long long uid)
{
	if (!uid)return 0;
	if (uid == console)return 255;
	if (trustedQQ(uid) > 2)return trustedQQ(uid) - 1;
	if (mDiceList.count(uid))
	{
		if (trustedQQ(mDiceList[uid]) > 2)return trustedQQ(mDiceList[uid]) - 1;
		if (console["BelieveDiceList"] || mDiceList[uid] == uid)return 1;
	}
	if (blacklist->get_user_danger(uid))return -1;
	return 0;
}

//拉黑用户后搜查群
void checkGroupWithBlackQQ(const DDBlackMark& mark, long long llQQ)
{
	set<unsigned int> sQQwarning;
	ResList list;
	string strNotice;
	for (auto& [id, grp] : ChatList)
	{
		int authSelf;
		if (!grp->getLst() || grp->is("忽略")
			|| !(authSelf = DD::getGroupAuth(id, console.DiceMaid, 0)))continue;
		if (DD::isGroupMember(grp->ID, llQQ, false))	{
			strNotice = printGroup(id);
			if (grp->is("协议无效")) {
				strNotice += "群协议无效";
			}
			else if (grp->is("免黑")) {
				if (mark.isSource(console.DiceMaid) && !mark.isType("local"))DD::sendGroupMsg(id, mark.warning());
				strNotice += "群免黑";
			}
			else if (int authBlack{ DD::getGroupAuth(id,llQQ,0) }; authBlack < 1 || authSelf < 1) {
				strNotice += "群权限获取失败";
			}
			else if (authBlack < authSelf) {
				if (mark.isSource(console.DiceMaid) && !mark.isType("local"))AddMsgToQueue(
					mark.warning(), { 0,id });
				strNotice += "对方群权限较低";
			}
			else if (authSelf > authBlack)
			{
				DD::sendGroupMsg(id, mark.warning());
				grp->leave("发现新增黑名单管理员" + printUser(llQQ) + "\n" + getMsg("strSelfName") + "将预防性退群");
				strNotice += "对方群权限较高，已退群";
				this_thread::sleep_for(1s);
			}
			else if (grp->is("免清"))
			{
				if (mark.isSource(console.DiceMaid) && !mark.isType("local"))AddMsgToQueue(
					mark.warning(), { 0, id });
				strNotice += "群免清";
			}
			else if (console["LeaveBlackQQ"])
			{
				DD::sendGroupMsg(id, mark.warning());
				grp->leave("发现新增黑名单成员" + printUser(llQQ) + "（同等群权限）\n" + getMsg("strSelfName") + "将预防性退群");
				strNotice += "已退群";
				this_thread::sleep_for(1s);
			}
			else if (mark.isSource(console.DiceMaid) && !mark.isType("local"))AddMsgToQueue(
				mark.warning(), { 0, id });
			list << strNotice;
		}
	}
	if (!list.empty())
	{
		strNotice = "已清查与" + printUser(llQQ) + "共同群聊" + to_string(list.size()) + "个：" + list.show();
		console.log(strNotice, 0b100, printSTNow());
	}
}

// warning处理队列
std::queue<fromMsg> warningQueue;
// 消息发送队列锁
mutex warningMutex;

bool isLoadingExtern = false;

void AddWarning(const string& msg, long long DiceQQ, long long fromGID)
{
	lock_guard<std::mutex> lock_queue(warningMutex);
	warningQueue.emplace(msg, DiceQQ, fromGID);
}

void warningHandler()
{
	fromMsg warning;
	while (Enabled)
	{
		if (!warningQueue.empty())
		{
			{
				lock_guard<std::mutex> lock_queue(warningMutex);
				warning = warningQueue.front();
				warningQueue.pop();
			}
			if (warning.strMsg.empty())continue;
			if (warning.fromUID)console.log("接收来自" + printUser(warning.fromUID) + "的warning:" + warning.strMsg, 0,
			                               printSTNow());
			try
			{
				fifo_json j = fifo_json::parse(GBKtoUTF8(warning.strMsg));
				if (j.is_array())
				{
					for (auto& it : j)
					{
						blacklist->verify(it, warning.fromUID);
					}
				}
				else
				{
					blacklist->verify(j, warning.fromUID);
				}
				std::this_thread::sleep_for(100ms);
			}
			catch (...)
			{
				console.log("warning解析失败×", 0);
			}
		}
		else std::this_thread::sleep_for(200ms);
	}
}

int getCloudBlackMark(int wid, string& res)
{
	string temp;
	const bool reqRes = Network::GET("http://shiki.stringempty.xyz/blacklist/warning.php?wid=" + to_string(wid), temp);
	if (!reqRes)return -1;
	if (temp == "null")return -2;
	res = temp;
	return 1;
}

std::unique_ptr<DDBlackManager> blacklist;
std::mutex blacklistMutex;

map<string, int> credit_limit{
	{"null", 6},
	{"kick", 2}, {"ban", 2}, {"spam", 2}, {"ruler", 2}, {"extern", 2},
	{"other", 3}, {"local", 3}, {"invite", 3}
};

DDBlackMark::DDBlackMark(long long uid, long long group) : type("local"), danger(2), comment("本地黑名单记录")
{
	if (uid)fromUID = { uid, true};
	if (group)fromGID = {group, true};
}

DDBlackMark::DDBlackMark(const fifo_json& j){
	try
	{
		bool isAdd = true;
		if (j.count("type"))
		{
			if (j["type"] == "erase")
			{
				isAdd = false;
				isClear = true;
			}
			else if (!credit_limit.count(j["type"].get<string>())) { return; }
			else type = j["type"].get<string>();
		}
		if (j.count("wid")) {
			j["wid"].get_to(wid);
			if (j.count("isErased")) {
				isClear = j["isErased"].get<int>();
				isAdd = !isClear;
			}
		}
		if (j.count("fromGID"))fromGID = { j["fromGID"], isAdd };
		else if (j.count("fromGroup"))fromGID = { j["fromGroup"], isAdd };
		if (j.count("fromUID"))fromUID = { j["fromUID"], isAdd };
		else if (j.count("fromQQ"))fromUID = { j["fromQQ"], isAdd };
		if (j.count("inviterQQ"))inviterQQ = {j["inviterQQ"], isAdd};
		else if (j.count("inviter"))inviterQQ = { j["inviter"], isAdd };
		if (j.count("inviter"))inviterQQ = { j["inviter"], isAdd };
		if (j.count("ownerQQ"))ownerQQ = {j["ownerQQ"], isAdd};

		if (j.count("DiceMaid"))DiceMaid = j["DiceMaid"];
		if (j.count("masterQQ"))masterID = j["masterQQ"];
		else if (j.count("master"))masterID = j["master"];

		if (j.count("danger"))
		{
			danger = j["danger"].get<int>();
			if (danger < 1)return;
		}
		else {
			danger = (type == "ruler") ? 3 : 2;
		}

		if (j.count("time"))time = UTF8toGBK(j["time"].get<string>());
		if (j.count("note"))note = UTF8toGBK(j["note"].get<string>());
		if (j.count("comment"))comment = UTF8toGBK(j["comment"].get<string>());
		if (j.count("erase"))
		{
			isClear = true;
			for (auto& key : j["erase"])
			{
				if (key.get<string>() == "fromGID"|| key.get<string>() == "fromGroup")fromGID.second = false;
				else if (key.get<string>() == "fromUID"|| key.get<string>() == "fromQQ")fromUID.second = false;
				else if (key.get<string>() == "inviter"|| key.get<string>() == "inviterQQ")inviterQQ.second = false;
				else if (key.get<string>() == "ownerQQ")ownerQQ.second = false;
			}
			if (fromGID.second || fromUID.second || inviterQQ.second || ownerQQ.second)isClear = false;
		}
		isValid = true;
		return;
	}
	catch (...)
	{
		console.log("解析黑名单json失败！", 0, printSTNow());
		return;
	}
}

DDBlackMark::DDBlackMark(const string& strWarning)
{
	try
	{
		fifo_json j = fifo_json::parse(strWarning);
		new(this)DDBlackMark(j);
	}
	catch (...)
	{
		console.log("解析黑名单json失败！", 0, printSTNow());
		return;
	}
}

string DDBlackMark::printJson(int tab = 0) const
{
	fifo_json j;
	if (wid)j["wid"] = wid;
	j["type"] = type;
	j["danger"] = danger;
	vector<string> eraseID;
	if (fromGID.first)
	{
		j["fromGID"] = fromGID.first;
		if (!fromGID.second)eraseID.push_back("fromGID");
	}
	if (fromUID.first)
	{
		j["fromUID"] = fromUID.first;
		if (!fromUID.second)eraseID.push_back("fromUID");
	}
	if (inviterQQ.first)
	{
		j["inviterQQ"] = inviterQQ.first;
		if (!inviterQQ.second)eraseID.push_back("inviterQQ");
	}
	if (ownerQQ.first)
	{
		j["ownerQQ"] = ownerQQ.first;
		if (!ownerQQ.second)eraseID.push_back("ownerQQ");
	}
	if (!eraseID.empty())j["erase"] = eraseID;
	if (!time.empty())j["time"] = GBKtoUTF8(time);
	if (!note.empty())j["note"] = GBKtoUTF8(note);
	if (DiceMaid)j["DiceMaid"] = DiceMaid;
	if (masterID)j["masterQQ"] = masterID;
	if (!comment.empty())j["comment"] = GBKtoUTF8(comment);
	if (tab)return UTF8toGBK(j.dump(tab));
	return UTF8toGBK(j.dump());
}

string DDBlackMark::warning() const
{
	fifo_json j;
	if (wid)j["wid"] = wid;
	j["type"] = type;
	j["danger"] = danger;
	if (fromGID.second)j["fromGID"] = fromGID.first;
	if (fromUID.second)j["fromUID"] = fromUID.first;
	if (inviterQQ.second)j["inviterQQ"] = inviterQQ.first;
	if (ownerQQ.second)j["ownerQQ"] = ownerQQ.first;
	if (!time.empty())j["time"] = GBKtoUTF8(time);
	if (!note.empty())j["note"] = GBKtoUTF8(note);
	if (DiceMaid)j["DiceMaid"] = DiceMaid;
	if (masterID)j["masterQQ"] = masterID;
	if (!comment.empty())j["comment"] = GBKtoUTF8(comment);
	return "!warning" + UTF8toGBK(j.dump(2));
}

std::string DDBlackMark::getData() const
{
	std::string data = "fromChat.uid=" + std::to_string(fromUID.first) + "&fromGID=" + std::to_string(fromGID.first) +
		"&DiceMaid=" + std::to_string(DiceMaid) + "&masterQQ=" + std::to_string(masterID) + "&time=" + time;
	return data;
}

void DDBlackMark::fill_note()
{
	if (!note.empty())return;
	if (type == "kick")
	{
		note = time + " " + (fromUID.first ? printUser(fromUID.first) : "") + "将" + printUser(DiceMaid) + "移出了群" + (
			fromGID.first ? to_string(fromGID.first) : "");
	}
	else if (type == "ban")
	{
		note = time + " " + (fromGID.first ? "在" + printGroup(fromGID.first) + "中" : "") + (
			fromUID.first ? printUser(fromUID.first) : "") + "将" + printUser(DiceMaid) + "禁言";
	}
	else if (type == "spam")
	{
		note = time + " " + (fromUID.first ? printUser(fromUID.first) : "") + "对" + printUser(DiceMaid) + "高频发送指令";
	}
}

bool DDBlackMark::isType() const
{
	return "null" != type;
}

bool DDBlackMark::isType(const string& strType) const
{
	return strType == type;
}

bool DDBlackMark::isSame(const DDBlackMark& other) const
{
	if (wid && other.wid)return other.wid == wid;
	bool isLike = false;
	if (type != other.type && type != "null" && other.type != "null")return false;
	if (time.length() == other.time.length() && time.length() > 0)
	{
		if (time != other.time)return false;
		isLike = true;
	}
	if (fromGID.first && other.fromGID.first)
	{
		if (fromGID.first != other.fromGID.first)return false;
		isLike = true;
	}
	if (fromUID.first && other.fromUID.first)
	{
		if (fromUID.first != other.fromUID.first)return false;
		isLike = true;
	}
	if (inviterQQ.first && other.inviterQQ.first)
	{
		if (inviterQQ.first != other.inviterQQ.first)return false;
		isLike = true;
	}
	if (ownerQQ.first && other.ownerQQ.first)
	{
		if (ownerQQ.first != other.ownerQQ.first)return false;
		isLike = true;
	}
	if (DiceMaid && other.DiceMaid && DiceMaid != other.DiceMaid)return false;
	if (note == other.note)return true;
	return isLike;
}

bool DDBlackMark::isSource(long long uid) const
{
	return DiceMaid == uid || masterID == uid;
}
void DDBlackMark::check_remit()
{
    if (fromGID.first && groupset(fromGID.first, "免黑") > 0)erase();
    if (fromUID.first && trustedQQ(fromUID.first) > 1)erase();
	if (inviterQQ.first && trustedQQ(inviterQQ.first) > 1)inviterQQ = { 0,false };
    if (ownerQQ.first && trustedQQ(ownerQQ.first) > 1)ownerQQ = { 0,false };
}

void DDBlackMark::erase()
{
	fromGID.second = fromUID.second = inviterQQ.second = ownerQQ.second = false;
	isClear = true;
}

void DDBlackMark::upload()
{
	std::string info = "&masterID=" + std::to_string(masterID) + "&time=" +	time + "&note=" + UrlEncode(GBKtoUTF8(note));
	int res{ DD::uploadBlack(console.DiceMaid, fromUID.first, fromGID.first, type, info) };
	if (!res) {
		erase();
	}
	else if (res < 0) {
		console.log("上传不良记录失败:" + info, 0b1);
	}
	else {
		wid = res;
	}
}

int DDBlackMark::check_cloud()
{
	std::string frmdata = "fromUID=" + std::to_string(fromUID.first) + "&fromGID=" + std::to_string(fromGID.first) +
		"&DiceMaid=" + std::to_string(DiceMaid) + "&masterID=" + std::to_string(masterID) + "&time=" + time;
	string temp;
	const bool reqRes = Network::POST("http://shiki.stringempty.xyz/blacklist/check.php", frmdata, "", temp);
	if (!reqRes)
	{
		console.log("云记录访问失败" + temp, 0);
		return -2;
	}
	if (temp[0] == '?') 
	{
		console.log("匹配到未确认云记录:wid" + temp, 0);
		wid = stoi(temp.substr(1));
		return 1;
	}
	else if (temp[0] == '+')
	{
		console.log("匹配到未注销云记录:wid" + temp, 0);
		wid = stoi(temp.substr(1));
		return 1;
	}
	if (temp[0] == '-')
	{
		console.log("匹配到已注销云记录:wid" + temp, 0);
		wid = stoi(temp.substr(1));
		return 0;
	}
	return -1;
}

int DDBlackManager::find(const DDBlackMark& mark)
{
    std::lock_guard<std::mutex> lock_queue(blacklistMutex);
    unordered_set<unsigned int> sRange;
    if (mark.wid && mCloud.count(mark.wid)) return mCloud[mark.wid];
    if (mark.wid){
        if (mCloud.count(mark.wid)) return mCloud[mark.wid];
        sRange = sIDEmpty;
    }
    if (mark.time.length() == 19) 
	{
        unordered_set<unsigned int> sTimeRange = sTimeEmpty;
        for (auto& [key,id] : multi_range(mTimeIndex, mark.time)) 
		{
            sTimeRange.insert(id);
        }
        if (sRange.empty())
		{
            sRange.swap(sTimeRange);
        }
        else
		{
            unordered_set<unsigned int> sInter;

            for (const auto& ele : sRange)
            {
                if (sTimeRange.count(ele))
                {
                    sInter.insert(ele);
				}
			}
            //std::set_intersection(sRange.begin(), sRange.end(), sTimeRange.begin(), sTimeRange.end(), std::inserter(sInter, sInter.begin()));
            if (sInter.empty())return -1;
            sRange.swap(sInter);
        }
    }
    if (mark.fromGID.first)
	{
        unordered_set<unsigned int> sGroupRange = sGroupEmpty;
        for (auto& [key, id] : multi_range(mGroupIndex, mark.fromGID.first))
		{
            sGroupRange.insert(id);
        }
        if (sGroupRange.empty())return -1;
        if (sRange.empty())
		{
            sRange.swap(sGroupRange);
        }
        else 
		{
            unordered_set<unsigned int> sInter;
            
            // O(n)
            for (const auto& ele : sRange)
            {
                if (sGroupRange.count(ele))
                {
                    sInter.insert(ele);
				}
			}
            //std::set_intersection(sRange.begin(), sRange.end(), sGroupRange.begin(), sGroupRange.end(), std::inserter(sInter, sInter.begin()));
            if (sInter.empty())return -1;
            sRange.swap(sInter);
        }
    }
    if (mark.fromUID.first)
	{
        unordered_set<unsigned int> sQQRange = sQQEmpty;
        for (auto& [key, id] : multi_range(mQQIndex, mark.fromUID.first)) {
            if (Enabled)console.log("匹配不良记录" + to_string(id), 0);
            sQQRange.insert(id);
        }
        if (sQQRange.empty())return -1;
        if (sRange.empty())
		{
            sRange.swap(sQQRange);
        }
        else
		{
            unordered_set<unsigned int> sInter;

            for (const auto& ele : sRange)
            {
                if (sQQRange.count(ele))
                {
                    sInter.insert(ele);
				}
			}
            //std::set_intersection(sRange.begin(), sRange.end(), sQQRange.begin(), sQQRange.end(), std::inserter(sInter, sInter.begin()));
            if (sInter.empty())return -1;
            sRange.swap(sInter);
        }
    }
    for (auto i : sRange) 
	{
        if (vBlackList[i].isSame(mark))return i;
    }
    return -1;
}

DDBlackMark& DDBlackMark::operator<<(const DDBlackMark& mark)
{
    // int delta_danger = mark.danger - danger;
    if (type == "null" && mark.type != "null")type = mark.type;
    if (time.empty() && !mark.time.empty())
	{
        time = mark.time;
    }
    if (note != mark.note &&
        (note.empty() || note.length() < mark.note.length())
        )
	{
        note = mark.note;
    }
    if (mark.fromGID.first)
	{
        fromGID = mark.fromGID;
    }
    if (mark.fromUID.first)
	{
		fromUID = mark.fromUID;
    }
    if (mark.inviterQQ.first)
	{
        inviterQQ = mark.inviterQQ;
    }
    if (mark.ownerQQ.first) 
	{
        ownerQQ = mark.ownerQQ;
    }
    if (!DiceMaid && mark.DiceMaid)DiceMaid = mark.DiceMaid;
    if (!masterID && mark.masterID)masterID = mark.masterID;
    //save comment if the mark changed at this update
    if (!mark.comment.empty())
	{
        comment = mark.comment;
    }
    return *this;
}

bool DDBlackManager::insert(DDBlackMark& ex_mark)
{
    std::lock_guard<std::mutex> lock_queue(blacklistMutex);
    unsigned id = vBlackList.size();
    vBlackList.push_back(ex_mark);
    DDBlackMark& mark(vBlackList[id]);
    if (mark.wid)mCloud[mark.wid] = id;
    else sIDEmpty.insert(id);
    if (mark.time.length() == 19)mTimeIndex.emplace(mark.time, id);
    else sTimeEmpty.insert(id);
    if (mark.fromGID.first)
	{
        mGroupIndex.emplace(mark.fromGID.first, id);
        if (mark.fromGID.second)
		{
            up_group_danger(mark.fromGID.first, mark); 
        }
    }
    else
	{
        sGroupEmpty.insert(id);
    }
    if (mark.fromUID.first) 
	{
        mQQIndex.emplace(mark.fromUID.first, id);
        if (mark.fromUID.second)
		{
            up_qq_danger(mark.fromUID.first, mark); 
        }
    }
    else
	{
        sQQEmpty.insert(id);
    }
    if (mark.inviterQQ.first)
	{
        mQQIndex.emplace(mark.inviterQQ.first, id);
        if (mark.inviterQQ.second)
		{
            up_qq_danger(mark.inviterQQ.first, mark);
        }
    }
    if (mark.ownerQQ.first)
	{
        mQQIndex.emplace(mark.ownerQQ.first, id);
        if (mark.ownerQQ.second)
		{
            up_qq_danger(mark.ownerQQ.first, mark);
        }
    }
    if (Enabled && !isLoadingExtern)blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
	return !mark.isClear;
}

bool DDBlackManager::update(DDBlackMark& mark, unsigned int id, int credit = 5)
{
    std::lock_guard<std::mutex> lock_queue(blacklistMutex);
    DDBlackMark& old_mark = vBlackList[id];
    int delta_danger = mark.danger - old_mark.danger;
    bool isUpdated = false;
    if (delta_danger)
	{
        old_mark.danger = mark.danger;
        isUpdated = true;
    }
    if (mark.wid && !old_mark.wid)
	{
        sIDEmpty.erase(id);
        old_mark.wid = mark.wid;
        mCloud.emplace(old_mark.wid, id);
        isUpdated = true;
    }
    if (old_mark.type == "null" && mark.type != "null")old_mark.type = mark.type;
    if (old_mark.time.length() < mark.time.length() && mark.time.length() <= 19) {
        old_mark.time = mark.time;
        sTimeEmpty.erase(id);
        if (mark.time.length() == 19)mTimeIndex.emplace(old_mark.time, id);
    }
    if (old_mark.note != mark.note &&
        (old_mark.note.empty() || old_mark.note.length()
			< mark.note.length())
        )
	{
        old_mark.note = mark.note;
    }
    if (mark.fromGID.first)
	{
        if (!old_mark.fromGID.first) 
		{
            sGroupEmpty.erase(id);
            mGroupIndex.emplace(mark.fromGID.first, id);
            if (mark.fromGID.second)
			{
                up_group_danger(mark.fromGID.first, mark);
            }
            old_mark.fromGID = mark.fromGID;
            isUpdated = true;
        }
        else if (mark.fromGID.second != old_mark.fromGID.second)
		{
            if (old_mark.fromGID.second)
			{
                old_mark.fromGID.second = false;
                reset_group_danger(mark.fromGID.first);
                isUpdated = true;
            }
            else if (delta_danger > 0 || credit > 2)
			{
                old_mark.fromGID.second = true;
                up_group_danger(mark.fromGID.first, mark);
                isUpdated = true;
            }
        }
    }
    if (mark.fromUID.first) 
	{
        if (!old_mark.fromUID.first) 
		{
            sQQEmpty.erase(id);
            mQQIndex.emplace(mark.fromUID.first, id);
            old_mark.fromUID = mark.fromUID;
            if (mark.fromUID.second)
			{
                up_qq_danger(mark.fromUID.first, mark);
            }
            isUpdated = true;
        }
        else if (mark.fromUID.second != old_mark.fromUID.second)
		{
            if (old_mark.fromUID.second)
			{
                old_mark.fromUID.second = false;
                reset_qq_danger(mark.fromUID.first);
                isUpdated = true;
            }
            else if (delta_danger > 0 || credit > 2)
			{
                old_mark.fromUID.second = true;
                up_qq_danger(mark.fromUID.first, mark);
                isUpdated = true;
            }
        }
    }
    if (mark.inviterQQ.first)
	{
        if (!old_mark.inviterQQ.first)
		{
            mQQIndex.emplace(mark.inviterQQ.first, id);
            old_mark.inviterQQ = mark.inviterQQ;
            if (mark.inviterQQ.second)
			{
                up_qq_danger(mark.inviterQQ.first, mark);
            }
            isUpdated = true;
        }
        else if (mark.inviterQQ.second != old_mark.inviterQQ.second)
		{
            if (old_mark.inviterQQ.second) 
			{
                old_mark.inviterQQ.second = false;
                reset_qq_danger(mark.inviterQQ.first);
                isUpdated = true;
            }
            else if (delta_danger > 0 || credit > 2)
			{
                old_mark.inviterQQ.second = true;
                up_qq_danger(mark.inviterQQ.first, mark);
                isUpdated = true;
            }
        }
    }
    if (mark.ownerQQ.first)
	{
        if (!old_mark.ownerQQ.first)
		{
            sQQEmpty.erase(id);
            mQQIndex.emplace(mark.ownerQQ.first, id);
            old_mark.ownerQQ = mark.ownerQQ;
            if (mark.ownerQQ.second)
			{
                up_qq_danger(mark.ownerQQ.first, mark);
            }
            isUpdated = true;
        }
        else if (mark.ownerQQ.second != old_mark.ownerQQ.second) 
		{
            if (old_mark.ownerQQ.second)
			{
                old_mark.ownerQQ.second = false;
                reset_qq_danger(mark.ownerQQ.first);
                isUpdated = true;
            }
            else if (delta_danger > 0 || credit > 2)
			{
                old_mark.ownerQQ.second = true;
                up_qq_danger(mark.ownerQQ.first, mark);
                isUpdated = true;
            }
        }
    }
    if (!old_mark.DiceMaid && mark.DiceMaid)old_mark.DiceMaid = mark.DiceMaid;
    if (!old_mark.masterID && mark.masterID)old_mark.masterID = mark.masterID;
    //save comment if the mark changed at this update
    if (isUpdated)
	{
        if (!mark.comment.empty())old_mark.comment = mark.comment;
		else if (old_mark.comment.empty()) {
			old_mark.comment = printSTNow() + " 更新";
		}
		if (Enabled && !isLoadingExtern)blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
    }
    return isUpdated;
}

void DDBlackManager::reset_group_danger(long long llgroup)
{
	int max_danger = 0;
	for (auto [group, id] : multi_range(mGroupIndex, llgroup))
	{
		if (vBlackList[id].fromGID.second && vBlackList[id].danger > max_danger)max_danger = vBlackList[id].danger;
	}
	if (max_danger == mGroupDanger[llgroup])return;
	if (max_danger)
	{
		mGroupDanger[llgroup] = max_danger;
		console.log("已调整" + printGroup(llgroup) + "的危险等级为" + to_string(max_danger), 0b10, printSTNow());
	}
	else
	{
		mGroupDanger.erase(llgroup);
		if (Enabled && !isLoadingExtern)console.log("已消除" + printGroup(llgroup) + "的危险等级", 0b10, printSTNow());
	}
}

void DDBlackManager::reset_qq_danger(long long llqq)
{
	int max_danger = 0;
	for (auto [uid, id] : multi_range(mQQIndex, llqq))
	{
		if (vBlackList[id].fromUID.first == llqq && vBlackList[id].fromUID.second && vBlackList[id].danger > max_danger)
			max_danger = vBlackList[id].danger;
		if (vBlackList[id].inviterQQ.first == llqq && vBlackList[id].inviterQQ.second && vBlackList[id].danger >
			max_danger)max_danger = vBlackList[id].danger;
		if (vBlackList[id].ownerQQ.first == llqq && vBlackList[id].ownerQQ.second && vBlackList[id].danger > max_danger)
			max_danger = vBlackList[id].danger;
	}
	if (max_danger == mQQDanger[llqq])return;
	if (max_danger)
	{
		mQQDanger[llqq] = max_danger;
		console.log("已调整" + printUser(llqq) + "的危险等级为" + to_string(max_danger), 0b10, printSTNow());
	}
	else
	{
		mQQDanger.erase(llqq);
		if (Enabled)
		{
			if (!isLoadingExtern)console.log("已消除" + printUser(llqq) + "的危险等级", 0b10, printSTNow());
			if (UserList.count(llqq))AddMsgToQueue(getMsg("strBlackQQDelNotice", std::make_shared<AnysTable>(AttrVars{ {"uid",llqq}, {"user_nick", getName(llqq)}})), llqq);
		}
	}
}

bool DDBlackManager::up_group_danger(long long llgroup, DDBlackMark& mark)
{
	if (groupset(llgroup, "免黑") > 0)
	{
		mark.erase();
		return false;
	}
	if (mGroupDanger.count(llgroup) && mGroupDanger[llgroup] >= mark.danger)return false;
	mGroupDanger[llgroup] = mark.danger;
	if (Enabled)
	{
		if (ChatList.count(llgroup) && chat(llgroup).getLst())chat(llgroup).leave(
			mark.warning());
		if(!isLoadingExtern)console.log(getMsg("strSelfName") + "已将" + printGroup(llgroup) + "危险等级提升至" + to_string(mark.danger), 0b10,
		            printSTNow());
	}
	return true;
}

bool DDBlackManager::up_qq_danger(long long llqq, DDBlackMark& mark)
{
	if (trustedQQ(llqq) > 1 || llqq == console || llqq == console.DiceMaid)
	{
		if (mark.fromUID.first == llqq)mark.erase();
		if (mark.inviterQQ.first == llqq)mark.inviterQQ.second = false;
		if (mark.ownerQQ.first == llqq)mark.ownerQQ.second = false;
		return false;
	}
	if (mQQDanger.count(llqq) && mQQDanger[llqq] >= mark.danger)return false;
	mQQDanger[llqq] = mark.danger;
	if (Enabled && mark.danger > 1)
	{
		if (!mQQDanger.count(llqq) && UserList.count(llqq) && mark.danger == 2)
			mark.note.empty()
			? AddMsgToQueue(getMsg("strBlackQQAddNotice", std::make_shared<AnysTable>(AttrVars{ {"uid",llqq}, {"user_nick", getName(llqq)}})), llqq)
				: AddMsgToQueue(getMsg("strBlackQQAddNoticeReason", std::make_shared<AnysTable>(AttrVars{
										   {"uid",llqq}, {"reason", mark.note}, {"user_nick", getName(llqq)}
				                       })), llqq);
		if (!isLoadingExtern) {
			console.log(getMsg("strSelfName") + "已将" + printUser(llqq) + "危险等级提升至" + to_string(mark.danger), 0b10,
						printSTNow());
			checkGroupWithBlackQQ(mark, llqq);
		}
	}
	return true;
}

short DDBlackManager::get_group_danger(long long id) const
{
	if (auto it = mGroupDanger.find(id); it != mGroupDanger.end())return it->second;
	return 0;
}

short DDBlackManager::get_user_danger(long long id) const
{
	if (auto it = mQQDanger.find(id); it != mQQDanger.end())
		return it->second;
	return 0;
}

void DDBlackManager::rm_black_group(long long llgroup, DiceEvent* msg)
{
	std::lock_guard<std::mutex> lock_queue(blacklistMutex);
	if (!mGroupDanger.count(llgroup))
	{
		msg->reply(printGroup(llgroup) + "无未注销记录！");
		return;
	}
	if (mGroupDanger[llgroup] >= msg->trusted && !msg->isFromMaster())
	{
		msg->reply("你注销目标黑名单的权限不足×");
		return;
	}
	for (auto [key,index] : multi_range(mGroupIndex, llgroup))
	{
		vBlackList[index].fromGID.second = false;
	}
	mGroupDanger.erase(llgroup);
	msg->note("已注销" + printGroup(llgroup) + "的黑名单记录√");
	blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
}

void DDBlackManager::rm_black_qq(long long llqq, DiceEvent* msg)
{
	std::lock_guard<std::mutex> lock_queue(blacklistMutex);
	if (!mQQDanger.count(llqq))
	{
		msg->reply(printUser(llqq) + "无未注销记录！");
		return;
	}
	if (mQQDanger[llqq] >= msg->trusted && !msg->isFromMaster())
	{
		msg->reply("你注销目标黑名单的权限不足×");
		return;
	}
	for (auto [key, index] : multi_range(mQQIndex, llqq))
	{
		if (vBlackList[index].fromUID.first == llqq)vBlackList[index].fromUID.second = false;
		if (vBlackList[index].inviterQQ.first == llqq)vBlackList[index].inviterQQ.second = false;
		if (vBlackList[index].ownerQQ.first == llqq)vBlackList[index].ownerQQ.second = false;
	}
	reset_qq_danger(llqq);
	msg->note("已注销" + printUser(llqq) + "的黑名单记录√");
	blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
}

void DDBlackManager::isban(DiceEvent* msg)
{
	if (!msg->get_ll("target"))
	{
		msg->reply("请{nick}给出查询对象的ID×");
		return;
	}
	long long llID{ msg->get_ll("target") };
	bool isGet = false;
	if (mGroupIndex.count(llID))
	{
		set<int> sRec;
		JsonList jary;
		for (auto& [key, index] : multi_range(mGroupIndex, llID))
		{
			jary << vBlackList[index].printJson();
			sRec.insert(index);
		}
		msg->reply(
			string("该群有黑名单记录") + to_string(jary.size()) + "条" + (mGroupDanger[llID] ? "（未注销）:\n!warning" : "（已注销）:\n") +
			(jary.size() == 1 ? vBlackList[*sRec.begin()].printJson(2) : jary.dump()));
		isGet = true;
	}
	if (mQQIndex.count(llID))
	{
		set<int> sRec;
		JsonList jary;
		for (auto& [key, index] : multi_range(mQQIndex, llID))
		{
			jary << vBlackList[index].printJson();
			sRec.insert(index);
		}
		msg->reply(
			string("该用户有黑名单记录") + to_string(jary.size()) + "条" + (mQQDanger[llID] ? "（未注销）:\n!warning" : "（已注销）:\n") + (
				jary.size() == 1 ? vBlackList[*sRec.begin()].printJson(2) : jary.dump()));
		isGet = true;
	}
	if (!isGet)
	{
		msg->reply(msg->get_str("target") + "无黑名单记录");
	}
}

string DDBlackManager::list_group_warning(long long llgroup)
{
	for (auto [group, index] : multi_range(mGroupIndex, llgroup))
	{
		const DDBlackMark& mark = vBlackList[index];
		if (mark.fromGID.second && mark.danger == mGroupDanger[llgroup] && !mark.isType("local"))return mark.
			warning();
	}
	return "";
}

string DDBlackManager::list_qq_warning(long long llqq)
{
	for (auto [uid, index] : multi_range(mQQIndex, llqq))
	{
		const DDBlackMark& mark = vBlackList[index];
		if (mark.danger != mQQDanger[llqq] || mark.isType("local"))continue;
		if ((mark.fromUID.first == llqq && mark.fromUID.second)
			|| (mark.inviterQQ.first == llqq && mark.inviterQQ.second)
			|| (mark.ownerQQ.first == llqq && mark.ownerQQ.second))
			return mark.warning();
	}
	return "";
}

string DDBlackManager::list_self_group_warning(long long llgroup)
{
	for (auto [group, index] : multi_range(mGroupIndex, llgroup))
	{
		const DDBlackMark& mark = vBlackList[index];
		if (mark.isSource(console.DiceMaid) && mark.fromGID.second && mark.danger == mGroupDanger[llgroup] && !mark.
			isType("local"))return mark.warning();
	}
	return "";
}

string DDBlackManager::list_self_qq_warning(long long llqq)
{
	for (auto [uid, index] : multi_range(mQQIndex, llqq))
	{
		const DDBlackMark& mark = vBlackList[index];
		if (mark.danger != mQQDanger[llqq] || mark.isType("local"))continue;
		if (mark.isSource(console.DiceMaid) &&
			((mark.fromUID.first == llqq && mark.fromUID.second)
				|| (mark.inviterQQ.first == llqq && mark.inviterQQ.second)
				|| (mark.ownerQQ.first == llqq && mark.ownerQQ.second)))
			return mark.warning();
	}
	return "";
}

void DDBlackManager::add_black_group(long long llgroup, DiceEvent* msg)
{
	if (groupset(llgroup, "免黑") > 0)
	{
		msg->reply(getMsg("self") + "不能拉黑免黑群！");
		return;
	}
	DDBlackMark mark{0, llgroup};
	mark.danger = 1;
	mark.note = msg->get_str("note");
	if (!mark.note.empty()) {
		mark.danger = 2;
		mark.type = "other";
	}
	if (mark.danger < get_user_danger(llgroup))
	{
		msg->reply(getMsg("strSelfName") + "已拉黑群" + to_string(llgroup) + "！");
		return;
	}
	mark.time = msg->get_str("time");
	mark.DiceMaid = console.DiceMaid;
	mark.masterID = console;
	mark.comment = printSTNow() + " 由" + printUser(msg->get_ll("uid")) + "拉黑";
	insert(mark);
	msg->note("已添加" + printGroup(llgroup) + "的黑名单记录√");
}

void DDBlackManager::add_black_qq(long long llqq, DiceEvent* msg)
{
	if (trustedQQ(llqq) > 1){
		msg->reply(getMsg("strSelfName") + "不能拉黑受信任用户！");
		return;
	}
	DDBlackMark mark{llqq, 0};
	mark.danger = 1;
	mark.note = msg->get_str("note");
	if (!mark.note.empty() && !msg->has("user")) {
		mark.danger = 2;
		mark.type = "other";
	}
	if (mark.danger < get_user_danger(llqq))
	{
		msg->reply(getMsg("strSelfName") + "已拉黑用户" + printUser(llqq) + "！");
		return;
	}
	mark.time = msg->get_str("time");
	mark.DiceMaid = console.DiceMaid;
	mark.masterID = console;
	mark.comment = printSTNow() + " 由" + printUser(msg->fromChat.uid) + "拉黑";
	insert(mark);
	msg->note("已添加" + printUser(llqq) + "的本地黑名单记录√");
}

void DDBlackManager::verify(const fifo_json& pJson, long long operatorQQ)
{
    DDBlackMark mark{pJson};
    if (!mark.isValid)return;
    int credit = isReliable(operatorQQ);
    //数据库是否有记录:-1=不存在;0=已注销;1=未确认;2=已确认;
    int is_cloud = -1;
    if (console["CloudBlackShare"])
	{
        if (mark.wid)
		{
            string strInfo;
            if ((is_cloud = getCloudBlackMark(mark.wid, strInfo)) > -1)
			{
                try
				{
					fifo_json j = fifo_json::parse(strInfo);
                    if (j["isErased"].get<int>())is_cloud = 0;
                    else if (j["isCheck"].get<int>())is_cloud = 2;
                    if (mark.fromUID.first) 
					{
                        if (mark.fromUID.first != j["fromQQ"])return;
                    }
                    else
					{
                        if (!mark.isClear)mark.fromUID = {j["fromQQ"], true};
                    }
                    if (mark.fromGID.first)
					{
                        if (mark.fromGID.first != j["fromGroup"])return;
                    }
                    else
					{
                        if (!mark.isClear)mark.fromGID = {j["fromGroup"], true};
                    }
                    mark.DiceMaid = j["DiceMaid"];
                    mark.masterID = j["masterQQ"];
                    mark.type = j["type"].get<string>();
                    mark.time = j["time"].get<string>();
                    if (mark.note.empty())
					{
                         if (j.count("note") && !j["note"].get<string>().empty())mark.note = UTF8toGBK(
							 j["note"].get<string>());
                         else mark.fill_note();
                    }
                    if (credit < 3 || !mark.danger)mark.danger = 2;
                }
                catch (...)
				{
                    console.log("云端数据同步失败:wid=" + to_string(mark.wid), 0);
                    //mark.wid = 0;
                }
            }
            else 
			{
                console.log("云端核验失败:wid=" + to_string(mark.wid), 0);
                mark.wid = 0;
            }
        }
        else
		{
            is_cloud = mark.check_cloud();
        }
    }
    else
	{
        mark.wid = 0;
    }
    if (credit < 3)
	{
        if ((mark.isType("kick") && !console["ListenGroupKick"]) || (mark.isType("ban") && !console["ListenGroupBan"])
			|| (mark.isType("spam") && !console["ListenSpam"]))return;
        if (mark.type == "local" || mark.type == "other" || mark.isSource(console.DiceMaid)) {
            if (credit > 0)console.log(
				getName(operatorQQ) + "已通知" + getMsg("strSelfName") + "不良记录(未采用):\n!warning" + UTF8toGBK(
					pJson.dump()), 1, printSTNow());
            return;
        }
    }
    else if (credit < 5 && mark.danger > credit) mark.danger = credit;
    if (!mark.danger || credit < 3)mark.danger = (mark.type == "ruler" ? 3 : 2);
    mark.check_remit();
    int index = find(mark);
    //新记录
    if (index < 0)
	{
        //发送者或当事人任一有黑名单
        if (credit < 0 || get_user_danger(mark.DiceMaid) || get_user_danger(mark.masterID))return;
        if (!mark.isType())return;
        //无云端确认记录且不受信任
        if (is_cloud < 0 || is_cloud == 1) 
		{
            if (mark.type == "ruler" || credit == 0)return;
        }
        //云端注销，则同步注销
        else if (is_cloud == 0)
		{
            if (!mark.isClear && credit < 3)mark.erase();
        }
        if (credit < 3)
		{
            if (is_cloud < 1 && mark.type == "extern")return;
        }
        else
		{
            if (mark.type == "local" && credit < 4)return;
        }
        if (mark.fromGID.first && (groupset(mark.fromGID.first, "忽略") > 0 || groupset(mark.fromGID.first, "协议无效") > 0 || ExceptGroups.count(mark.fromGID.first)))return;
        insert(mark);
        console.log(getName(operatorQQ) + "已通知" + getMsg("strSelfName") + "不良记录" + to_string(vBlackList.size() - 1) + ":\n!warning" + UTF8toGBK(pJson.dump()), 1, printSTNow());
    }
    else 
	{ 
		//已有记录
        DDBlackMark& old_mark = vBlackList[index];
        bool isSource = operatorQQ == old_mark.DiceMaid || operatorQQ == old_mark.masterID;
        //低于危险等级无权修改
        if (old_mark.danger > credit && credit < 255) {
            if (old_mark.danger != 2)return;
            //当事人权利
            if (isSource) {
                mark.fromUID.second &= old_mark.fromUID.second;
                mark.fromGID.second &= old_mark.fromGID.second;
				mark.inviterQQ.second &= old_mark.inviterQQ.second;
				mark.ownerQQ.second &= old_mark.ownerQQ.second;
            }
            //同步云端注销
            else if (!is_cloud) {
                mark.erase();
            }
            else return;
			if (!isSource) {
				mark.inviterQQ = { 0,false };
				mark.ownerQQ = { 0,false };
			}
        }
        //无权调整危险等级
        if (mark.danger != old_mark.danger && credit < 3) { 
            mark.danger = old_mark.danger; 
        }
        if(update(mark,index,credit))console.log(getName(operatorQQ) + "已更新" + getMsg("strSelfName") + "不良记录" + to_string(index) + ":\n!warning" + UTF8toGBK(pJson.dump()), 1, printSTNow());
    }
}

void DDBlackManager::create(DDBlackMark& mark) 
{
    if (mark.check_remit(); mark.isClear)return;
    if (console["CloudBlackShare"] && mark.isSource(console.DiceMaid))mark.upload();
    console.log(mark.warning(), 0b100000);
    insert(mark);
}

int DDBlackManager::loadJson(const std::filesystem::path& fpPath, bool isExtern)
{
	fifo_json j = freadJson(fpPath);
	if (j.is_null())return -1;
	if (j.empty())return 0;
	if (j.size() > vBlackList.capacity())vBlackList.reserve(j.size() * 2);
	int cnt(0);
	for (auto& item : j)
	{
		DDBlackMark mark{item};
		if (!mark.isValid)continue;
		if (!mark.danger)mark.danger = 2;
		//新插入或更新
		isLoadingExtern = true;
		if (int res = find(mark); res < 0)
		{
			if(insert(mark))cnt++;
		}
		else
		{
			if (isExtern) {
				if (mark.isSource(console.DiceMaid))continue;	//涉及自身的不处理
				if (mark.danger != vBlackList[res].danger)continue;	//危险等级特别改动的不处理
			}
			if (update(mark, res))cnt++;
		}
		isLoadingExtern = false;
	}
	if (isExtern) {
		filesystem::remove(fpPath);
		if(cnt)console.log("更新外源不良记录条目" + to_string(cnt) + "条", 1, printSTNow());
		blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
	}
	return cnt;
}

int DDBlackManager::loadHistory(const std::filesystem::path& fpLoc)
{
	long long id;
	std::ifstream fgroup(fpLoc / "BlackGroup.RDconf");
	if (fgroup)
	{
		while (fgroup >> id)
		{
			if (mGroupDanger.count(id))continue;
			DDBlackMark mark{0, id};
			insert(mark);
		}
	}
	fgroup.close();
	std::ifstream fqq(fpLoc / "BlackQQ.RDconf");
	if (fqq)
	{
		while (fqq >> id)
		{
			if (mQQDanger.count(id))continue;
			DDBlackMark mark{id, 0};
			insert(mark);
		}
	}
	fqq.close();
	return vBlackList.size();
}

void DDBlackManager::saveJson(const std::filesystem::path& fpPath) const
{
	std::ofstream fout(fpPath);
	JsonList jary;
	for (auto& mark : vBlackList)
	{
		jary << GBKtoUTF8(mark.printJson());
	}
	fout << jary.dump();
}

Factory& DDBlackMarkFactory::sign()
{
	mark.DiceMaid = console.DiceMaid;
	mark.masterID = console;
	return *this;
}
