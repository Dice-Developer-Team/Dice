#include <string>
#include "fifo_json.hpp"
using std::string;
namespace api {
	bool init();
	void printLog(const string& info);
	string getApiVer();
	long long getTinyID();
	string getUserNick(long long uid);
	int getGroupAuth(long long gid, long long uid, int def);
	bool isGroupAdmin(long long gid, long long uid, bool def);
	void sendPrivateMsg(long long uid, const string& msg);
	void sendGroupMsg(long long gid, const string& msg);
	void sendChannelMsg(long long gid, long long chid, const string& msg);
	void pushExtra(const fifo_json& para);
	bool getExtra(const fifo_json& para, string& ret);
}