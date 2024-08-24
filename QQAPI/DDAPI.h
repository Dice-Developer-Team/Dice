#pragma once
#include <string>
#include <set>
#include <unordered_map>

inline long long loginID{ 0 };

struct GroupSize_t
{
	unsigned int currSize = 0;
	unsigned int maxSize = 0;
	std::string tostring() const
	{
		return std::to_string(currSize) + "/" + std::to_string(maxSize);
	}
};

using api_list = std::unordered_map<std::string, void*>;
//DiceDriver Interface
namespace DD {
	const std::string& getDriVer();
	bool reload();
	bool remake();
	void killme();
	bool updateDice(const std::string&, std::string&);
	inline api_list ApiList;
	const std::string& getRootDir();
	inline long long getLoginID() { return loginID; }
	long long getTinyID();
	std::string getQQNick(long long);
	inline std::string getLoginNick() { return getQQNick(loginID); }
	void debugLog(const std::string&);
	void debugMsg(const std::string&);
	bool isDiceMaid(long long);
	std::set<long long> getDiceSisters();
	void heartbeat(const std::string&);
	int uploadBlack(long long DiceMaid, long long fromQQ, long long fromGroup, const std::string& type, std::string& info);
	void sendPrivateMsg(long long, const std::string& msg);
	void sendGroupMsg(long long, const std::string& msg);
	void sendChannelMsg(long long, long long, const std::string& msg);
	void sendDiscussMsg(long long, const std::string& msg);
	bool isFriend(long long, bool);
	std::set<long long> getFriendQQList();
	std::set<long long> getGroupIDList();
	std::set<long long> getGroupMemberList(long long);
	std::set<long long> getGroupAdminList(long long);
	int getGroupAuth(long long llgroup, long long llQQ, int iDefault);
	bool isGroupAdmin(long long, long long, bool);
	bool isGroupOwner(long long, long long, bool);
	bool isGroupMember(long long, long long, bool);
	void answerFriendRequest(long long fromQQ, int respon, const std::string& msg = {});
	void answerGroupInvited(long long fromGroup, int respon);
	GroupSize_t getGroupSize(long long);
	std::string getGroupName(long long);
	std::string getGroupNick(long long, long long);
	long long getGroupLastMsg(long long, long long);
	std::string printGroupInfo(long long);
	void setGroupKick(long long, long long);
	void setGroupBan(long long, long long, int);
	void setGroupAdmin(long long, long long, bool = true);
	void setGroupCard(long long, long long, const std::string&);
	void setGroupTitle(long long, long long, const std::string&);
	void setGroupWholeBan(long long, int);
	void setGroupLeave(long long);
	void setDiscussLeave(long long);
	bool uploadGroupFile(long long, const std::string&);
	bool sendFriendFile(long long, const std::string&);
	bool getExtra(const std::string&, std::string&);
}
//enum class msgtype : int { Private = 0, Group = 1, Discuss = 2 };