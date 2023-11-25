#include "OneBotAPI.h"
#include "DiceConsole.h"
#include "DiceEvent.h"
#include "WSClient.hpp"
#include "WSServer.hpp"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
template<typename T>
using ptr = std::shared_ptr<T>;

const string Empty;

string server_url;
bool isReverse{ false };
std::unique_ptr<WSLinker> linker;

//OneBotAPI
namespace api {
	bool init() {
//#ifdef _WIN32
//		char path[MAX_PATH];
//		GetModuleFileNameA(nullptr, path, MAX_PATH);
//		dirExe = std::filesystem::path(path);
//#else
		dirExe = std::filesystem::current_path();
//#endif
		std::filesystem::create_directories(dirExe / "Diceki" / "log");
		if (fs::exists("config.yml")) {
			cout << dirExe / "config.yml" << endl;
			YAML::Node yaml{ YAML::LoadFile("config.yml") };
			if (yaml["settings"]) {
				isReverse = true;
				server_url = yaml["settings"]["ws_address"][0].Scalar();
				return true;
			}
		}
		else if (fs::exists("Diceki/config.toml")) {
			cout << dirExe / "Diceki" / "config.toml" << endl;
			auto cfg{ toml::parse(ifstream("Diceki/config.toml")) };
			server_url = string(*cfg["server"].as_string());
			return true;
		}
		printLog("服务端配置读取失败！请确认同级文件夹有正确配置的config.yml或Diceki/driver.yaml！");
		return false;
	}
	void printLog(const string& log) {
		cout << UTF8toLocal(log) << endl;
		static std::filesystem::path logPath(dirExe / "Diceki" / "log" / ("log" + to_string(time(nullptr)) + ".txt"));
		std::ofstream flog(logPath, std::ios::app);
		if (flog)flog << log << std::endl;
	}
	string getApiVer() {
		return "Gensokyo";
	}
	long long getTinyID() { return 0; }
	void sendPrivateMsg(long long uid, const string& msg) {	}
	void sendGroupMsg(long long gid, const string& msg) {
		if (!msg.empty()) {
			linker->send(json{
				{"action","send_group_msg"},
				{"params",{
					{"group_id",gid},
					{"message",msg},
				}}, }
			);
		}
	}
	void sendChannelMsg(long long gid, long long chid, const string& msg) {
		if (!msg.empty()) {
			linker->send(json{
				{"action","send_guild_channel_msg"},
				{"params",{
					{"guild_id",to_string(gid)},
					{"channel_id",to_string(chid)},
					{"message",msg},
				}}, }
			);
		}
	}
	string getUserNick(long long uid) {
		return {};
	}
	int getGroupAuth(long long gid, long long uid, int def) {
		return def;
	}
	bool isGroupAdmin(long long gid, long long uid, bool def) {
		return true;
	}
	void pushExtra(const fifo_json& para){
		linker->send(para);
	}
	bool getExtra(const fifo_json& para, string& ret) {
		if (auto res{ linker->get_data(para) }) {
		}
		return false;
	}
}
/*
//QQAPI
namespace QQ {
	string Code_At(ptr<User> user) {
		return "[CQ:at,qq=" + string(*user) + "]";
	}
	bool IsEnable() {
		return dd.enabled();
	}
#ifdef _WIN32
	void debugLog(const char* log) {
#else
	void debugLog(const char* gbk) {
		string log{ charcvt(CP_GBK, CP_UTF8, gbk) };
#endif
		driverlog(log);
		std::cout << log << std::endl;
	}
	void debugMsg(ptr<Bot> bot, const string& log) {
		driverlog(log);
		sendDirectMsg(bot, bot, log);
	}
	void sendDirectMsg(ptr<Bot> bot, ptr<User> aim, const string& msg) {
		if (msg.empty())return;
		if (!bot->getFriendQQList()->count(*aim) && bot->lastChat.count(*aim)) {
			linker->send(json{
				{"action","send_private_msg"},
				{"params",{
					{"user_id",*aim},
					{"group_id",bot->lastChat[*aim].gid},
					{"message",msg},
				}}, }
			);
		}
		else linker->send(json{
			{"action","send_private_msg"},
			{"params",{
				{"user_id",*aim},
				{"message",msg},
			}}, }
		);
	}
	void sendGroupMsg(ptr<Bot> bot, ptr<Group> aimGrp, const string& msg) {
		if (msg.empty())return;
		linker->send(json{
			{"action","send_group_msg"},
			{"params",{
				{"group_id",*aimGrp},
				{"message",msg},
			}}, }
		);
	}
	long long GetTinyID(long long qq) {
		linker->send(json{
			{"action","get_guild_service_profile"},
			}
		);
		return 0;
	}
	void answerGroupRequest(ptr<Group> grp, ptr<User> actQQ, int repType, const char* ret) {
		std::this_thread::sleep_for(3s);
		if (!cache.GroupReqs.count(*grp) || !cache.GroupReqs[*grp].count(*actQQ)) {
			if (!cache.GroupReqs.count(*grp))api::printLog("未找到加群" + string(*grp) + "的申请");
			else api::printLog("未找到来自" + string(*actQQ) + "的申请");
			return;
		}
		std::this_thread::sleep_for(1s);
		GroupReq& req = cache.GroupReqs[*grp][*actQQ];
		req.answer(repType, ret);
		if (repType == 1) {
			std::this_thread::sleep_for(3s);
			if (ret && ret[0])sendDirectMsg(req.botQQ, req.applyQQ, ret);
		}
	}
	void answerGroupInvited(ptr<Bot> botQQ, ptr<Group> grp, int repType, const char* ret) {
		if (!cache.GroupInvs.count(*botQQ) || !cache.GroupInvs[*botQQ].count(*grp)) {
			api::printLog("未找到入群邀请(群" + string(*grp) + "受邀bot" + string(*botQQ) + ")");
			return;
		}
		std::this_thread::sleep_for(3s);
		GroupInv& req = cache.GroupInvs[*botQQ][*grp];
		req.answer(repType, ret);
		if (repType == 1) {
			std::this_thread::sleep_for(3s);
			if (ret && ret[0])sendDirectMsg(botQQ, req.applyQQ, ret);
		}
	}
	void answerFriendRequest(ptr<Bot> botQQ, ptr<User> aimQQ, int repType, string ret) {
		FriendReq& req = cache.FriendReqs[*botQQ][*aimQQ];
		req.answer(repType, ret.c_str());
		if (repType == 1) {
			std::this_thread::sleep_for(5s);
			if (!ret.empty())sendDirectMsg(botQQ, req.applyQQ, ret);
		}
	}
	void setGroupKick(ptr<Bot> bot, ptr<Group> grp, ptr<User> user) {
		linker->send(json{
			{"action","set_group_kick"},
			{"params",{
				{"group_id",*grp},
				{"user_id",*user},
			}}, }
		);
	}
	void setGroupBan(ptr<Bot> bot, ptr<Group> grp, ptr<User> user, int secTime) {
		linker->send(json{
			{"action","set_group_ban"},
			{"params",{
				{"group_id",*grp},
				{"user_id",*user},
				{"duration",secTime},
			}}, }
		);
	}
	void setGroupAdmin(ptr<Bot> bot, ptr<Group> grp, ptr<User> user, bool isSet) {
		linker->send(json{
			{"action","set_group_admin"},
			{"params",{
				{"group_id",*grp},
				{"user_id",*user},
				{"enable",isSet},
			}}, }
		);
	}
	void setGroupCard(ptr<Bot> bot, ptr<Group> grp, ptr<User> user, const string& card) {
		linker->send(json{
			{"action","set_group_card"},
			{"params",{
				{"group_id",*grp},
				{"user_id",*user},
				{"card",charcvt(CP_GBK, CP_UTF8, card)},
			}}, }
		);
	}
	void setGroupTitle(ptr<Bot> bot, ptr<Group> grp, ptr<User> user, const string& title) {
		linker->send(json{
			{"action","set_group_special_title"},
			{"params",{
				{"group_id",*grp},
				{"user_id",*user},
				{"special_title",charcvt(CP_GBK, CP_UTF8, title)},
			}}, }
		);
	}
	void setGroupWholeBan(ptr<Bot> bot, ptr<Group> grp, int secTime) {
		linker->send(json{
			{"action","set_group_whole_ban"},
			{"params",{
				{"group_id",*grp},
				{"enable",bool(secTime)},
			}}, }
		);
		if (secTime > 0 && secTime < 600) {
			linker->send(json{
				{"action","set_group_whole_ban"},
				{"params",{
					{"group_id",*grp},
					{"enable",false},
				}}, }
			);
		}
	}
	void setGroupLeave(ptr<Bot> bot, ptr<Group> grp) {
		linker->send(json{
			{"action","set_group_leave"},
			{"params",{
				{"group_id",*grp},
			}}, }
		);
	}
	void setDiscussLeave(ptr<Bot> bot, ptr<Group> grp) {
		linker->send(json{
			{"action","set_group_leave"},
			{"params",{
				{"group_id",*grp},
			}}, }
		);
	}

	void GroupReq::answer(int reType, const char* ret) {
		linker->send(json{
			{"action","set_group_add_request"},
			{"params",{
				{"flag",seq},
				{"type","add"},
				{"approve",reType == 1},
				{"reason",charcvt(CP_GBK, CP_UTF8, ret)},
			}}, }
		);
	}
	void GroupInv::answer(int reType, const char* ret) {
		linker->send(json{
			{"action","set_group_add_request"},
			{"params",{
				{"flag",seq},
				{"type","invite"},
				{"approve",reType == 1},
				{"reason",charcvt(CP_GBK, CP_UTF8, ret)},
			}}, }
		);
	}
	//将CQ格式转换为框架格式
	string decodeCQcode(const string& CQMsg, ptr<Bot> bot, int msgtype, long long chatid) {
		debugLog("<< " + CQMsg);
		string msg{ charcvt(CP_GBK, CP_UTF8, CQMsg) };
		size_t l{ 0 }, r{ 0 };
		while (string::npos != (l = msg.find("[CQ:", r))++ && (r = msg.find(']', l)) != string::npos) {
			if (msg.substr(l, 10) == "CQ:record,") {
				if (msg.substr(l, 14) == "CQ:record,url=") {
					msg.replace(l + 10, 3, "file");
					return msg.substr(l - 1, r - l + 3);
				}
				return msg.substr(l - 1, r - l + 2);
			}
			else if (msg.substr(l, 9) == "CQ:at,id=") {
				msg.replace(l + 6, 2, "qq");
			}
			else if (msg.substr(l, 12) == "CQ:emoji,id=") {
				string strEmoji{ msg.substr(l + 12, r - l - 12) };
				//十进制转16进制
				if (strEmoji.find_first_not_of("0123456789") == string::npos && strEmoji.length() < 8) {
					char16_t idEmoji[2]{ (unsigned short)stoi(strEmoji),0 };	//10进制编码转换为Unicode宽字符
					string strEmoji{ charcvt(CP_UTF8, idEmoji) };	//宽字符转换为UTF8字符串
					msg.replace(l - 1, r - l + 2, strEmoji);
					r = l - 1 + strEmoji.length();
				}
				else {
					msg.replace(l, r - l, "emoji");
					r = l + 5;
				}
			}
			else if (msg.substr(l, 14) == "CQ:image,file=") {
				string strPath{ msg.substr(l + 14, r - l - 14) };
#ifdef _WIN32
				strPath = charcvt(CP_UTF8, CP_GBK, strPath);
#endif
				if (fspath pathImage{ strPath },pathImages{ dd.getDir() / "data" / "images" / strPath };
					!std::filesystem::exists(pathImages) && pathImage.is_relative()
					&& std::filesystem::exists(pathImage = dd.getDir() / "data" / "image" / strPath)) {
						fs::copy_file(pathImage, pathImages);
				}
			}
			else if (msg.substr(l, 13) == "CQ:image,url=") {
				msg.replace(l + 9, 3, "file");
				++r;
			}
			else if (msg.substr(l, 13) == "CQ:file,path=") {
				string strPath{ msg.substr(l + 13, r - l - 13) };
#ifdef _WIN32
				strPath = charcvt( CP_UTF8, CP_GBK, strPath);
#endif
				if (fspath pathFile{ strPath }; !std::filesystem::exists(pathFile) && pathFile.is_relative()) {
					pathFile = dd.getDir() / "data" / "file" / pathFile;
					strPath = pathFile.string();
				}
				if (msgtype == 1)
					sendPrivateFile(*bot, chatid, strPath);
				else
					uploadGroupFile(*bot, chatid, strPath);
				msg.erase(l - 1, r - l + 2);
				r = l - 1;
			}
			else if (msg.substr(l, 11) == "CQ:poke,id=") {
				msg.replace(l + 8, 2, "qq");
				r = l + 6;
			}
		}
		return msg;
	}
	//将框架格式转换为CQ格式
	string encodeCQcode(const string& rawMsg) {
		string msg{ charcvt(CP_UTF8, CP_GBK, rawMsg) };
		size_t l{ 0 }, r{ 0 };
		while (string::npos != (l = msg.find("[CQ:", r))++ && (r = msg.find(']', l)) != string::npos) {
			if (msg.substr(l, 9) == "CQ:at,qq=") {
				msg.replace(l + 6, 2, "id");
			}
			else if (msg.substr(l, 11) == "CQ:poke,qq=") {
				msg.replace(l + 8, 2, "id");
			}
		}
		return msg;
	}

	const string& GetGroupNick(long long botQQ, long long groupID, long long aimQQ) {
		if (!botQQ || !groupID || !aimQQ)return Empty;
		return *getGroup(groupID)->getMember(aimQQ)->getCard(getBot(botQQ));
	}
	const string& Group::getGroupCard(ptr<Bot> bot, ptr<User> user) {
		return *getMember(*user)->updateLastUpdateTime().getCard(bot);
	}
	long long GetGroupLastMsg(long long botQQ, long long groupID, long long aimQQ) {
		if (!botQQ || !groupID || !aimQQ)return 0;
		return getGroup(groupID)->getMember(aimQQ)->getLastMsgTime(getBot(botQQ));
	}
	bool uploadGroupFile(long long botQQ, long long aimID, const std::string& file) {
		fs::path pathFile{ file };
		if (!fs::exists(pathFile)) {
			api::printLog("群文件上传失败：找不到文件" + pathFile.string());
			return false;
		}
		debugLog("<<[文件]" + file);
		linker->send(json{
			{"action","upload_group_file"},
			{"params",{
				{"group_id",aimID},
				{"file",pathFile.u8string()},
				{"name",pathFile.filename().u8string()},
			}}, }
		);
		return true;
	}
	bool sendPrivateFile(long long botQQ, long long aimID, const std::string& file) {
		fs::path pathFile{ file };
		if (!fs::exists(pathFile)) {
			api::printLog("文件发送失败：找不到文件" + pathFile.string());
			return false;
		}
		debugLog("<<[文件]" + file);
		linker->send(json{
			{"action","upload_private_file"},
			{"params",{
				{"user_id",aimID},
				{"file",pathFile.u8string()},
				{"name",pathFile.filename().u8string()},
			}}, }
		);
		return true;
	}
	bool getExtra(json& j, string& ret) {
		return false;
	}
}*/
std::mutex OnlineListMutex;
const std::set<long long>& GetOnlineBots() {
	std::lock_guard lock(OnlineListMutex);
	static std::set<long long>OnlineList{};
	//if (!OnlineList.empty())
		return OnlineList;
}
bool Remake() {
	return false;
}

bool link_server() {
	if (isReverse)linker = std::make_unique<EventServer>(server_url);
	else if(!server_url.empty())linker = std::make_unique<WSClient>(server_url);
	return true;
}

void OneBot_Event(fifo_json& j) {
	//clock_t t{ clock() };
	try {
		if (j.count("post_type")) {
			long long botID{ j["self_id"] };
			if (!console.DiceMaid) {
				console.DiceMaid = botID;
				dice_init();
			}
			if (j["post_type"] == "message") {
				string msg{ j["message"] };
				//频道
				if (j["message_type"] == "guild") {
					ptr<DiceEvent> Msg(std::make_shared<DiceEvent>(
						AttrObject({ { "Event","Message"},
							{ "fromMsg", msg },
							{ "msgid", AttrVar(j["message_id"]) },
							{ "uid", j["user_id"] },
							{ "gid", j["guild_id"] },
							{ "chid", j["channel_id"] },
							{ "time",(long long)time(nullptr)}
							})));
					Msg->DiceFilter();
				}
				else {
					int msgId{ j["message_id"].get<int>() };
					long long uid = j["user_id"].get<long long>();
					//私聊
					if (j["message_type"] == "private") {
						ptr<DiceEvent> Msg(std::make_shared<DiceEvent>(
							AttrObject({ { "Event", "Message" },
								{ "fromMsg", msg },
								{ "msgid",msgId },
								{ "uid",uid },
								{ "time",(long long)time(nullptr)},
								})));
						Msg->DiceFilter();
					}
					//群聊
					else if (j["message_type"] == "group") {
						api::printLog("group msg:" + msg);
						ptr<DiceEvent> Msg(std::make_shared<DiceEvent>(
							AttrObject({ { "Event", "Message" },
								{ "fromMsg", msg },
								{ "msgid",msgId },
								{ "uid",uid },
								{ "gid",j["group_id"] },
								{ "time",(long long)time(nullptr)},
								})));
						Msg->DiceFilter();
					}
				}
				return;
			}
			else if (j["post_type"] == "notice") {
				//新好友添加
				if (j["notice_type"] == "friend_add") {
					return;
				}
				//新成员入群
				else if (j["notice_type"] == "group_increase") {
					return;
				}
				//成员退群
				else if (j["notice_type"] == "group_decrease") {
					return;
				}
				//群消息撤回
				else if (j["notice_type"] == "group_recall") {
					return;
				}
				//私聊消息撤回
				else if (j["notice_type"] == "friend_recall") {
					return;
				}
				else if (j["notice_type"] == "guild_channel_recall") {
					return;
				}
				//其他客户端状态
				//else if (j["notice_type"] == "client_status") {
				//	api::printLog(string(j["client"]["device_name"]) + ": " + (j["online"] ? "online" : "offline"));
				//	return;
				//}
				//群禁言
				else if (j["notice_type"] == "group_ban") {
					return;
				}
				//群名片变动
				else if (j["notice_type"] == "group_card") {
					return;
				}
				//群文件上传
				else if (j["notice_type"] == "group_upload") {
					return;
				}
				//群管理变动
				else if (j["notice_type"] == "group_admin") {
					return;
				}
				else if (j["notice_type"] == "essence") {
					return;
				}
				else if (j["notice_type"] == "message_reactions_updated") {
					return;
				}
				//戳一戳
				else if (j["sub_type"] == "poke") {
					return;
				}
				else if (j["sub_type"] == "honor") {
					return;
				}
				else if (j["sub_type"] == "title") {
					return;
				}
				else if (j["sub_type"] == "lucky_king") {
					return;
				}
			}
			else if (j["post_type"] == "message_sent") {
				return;
			}
			else if (j["post_type"] == "request") {
				//群邀请/申请
				if (j["request_type"] == "group") {
					return;
				}
				else if (j["request_type"] == "friend") {
					return;
				}
			}
		}
		else if (j.count("echo") && !j["echo"].is_null()) {
			linker->mEcho[j["echo"]] = { j["data"],time(nullptr) };
			return;
		}
		else if (j["status"] == "ok")return;
	}
	catch (json::exception& e) {
		api::printLog(string("解析OneBot事件错误:") + e.what());
	}
	catch (std::exception& e) {
		api::printLog(string("事件处理异常:") + e.what());
	}
	api::printLog(j.dump(0));
	return;
}
class Event_Pool {
	std::deque<json> eves;
	std::array<std::thread, 10> pool;
	std::mutex Fetching;
	bool isActive{ false };
public:
	void start() {
		if (!isActive)for (int i = 0; i < 10; ++i) {
			pool[i] = std::thread(&Event_Pool::listenEvent, this);
		}
		isActive = true;
	}
	void push(const string& eve) {
		std::lock_guard<std::mutex> lk{ Fetching };
		try {
			fifo_json j = fifo_json::parse(eve);
			if (!j.count("meta_event_type")) {
				//j["clock"] = clock();
				eves.push_back(j);
			}
		}
		catch (json::exception& e) {
			api::printLog(string("解析接收json错误:") + e.what());
			api::printLog(eve);
		}
	}
	bool fetchEvent(fifo_json& eve) {
		std::lock_guard<std::mutex> lk{ Fetching };
		if (!eves.empty()) {
			eve = eves.front();
			eves.pop_front();
			return true;
		}
		return false;
	}
	void listenEvent() {
		while (isActive) {
			fifo_json eve;
			if (fetchEvent(eve)) {
				OneBot_Event(eve);
				sleep_for(19ms);
			}
			else {
				sleep_for(37ms);
			}
		}
	}
	void end() {
		isActive = false;
		for (auto& th : pool) {
			if (th.joinable())th.join();
		}
		pool = {};
	}
	~Event_Pool() {
		end();
	}
};
Event_Pool eve_pool;
string WSClient::access_token;
lws* client_wsi{ nullptr };
int callback_ws(lws* wsi, enum lws_callback_reasons reasons, void* user, void* _in, size_t _len) {
	WSLinker* linker{ (WSLinker*)user };
	char buffer[LWS_PRE + MAX_PAYLOAD_SIZE + 1] = { 0 };
	memset(buffer, 0, LWS_PRE + MAX_PAYLOAD_SIZE);
	//发送或者接受buffer，建议使用栈区的局部变量，lws会自己释放相关内存
	//如果使用堆区自定义内存空间，可能会导致内存泄漏或者指针越界
	switch (reasons) {
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: //24
		if (!WSClient::access_token.empty()) {
			unsigned char** p = (unsigned char**)_in, * end = (*p) + _len;
			auto res = lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_AUTHORIZATION,
				(unsigned char*)WSClient::access_token.c_str(), WSClient::access_token.length(), p, end);
		}
		break;
	case LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL: //85
	case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: //19
	case LWS_CALLBACK_GET_THREAD_ID: //31
	case LWS_CALLBACK_ADD_POLL_FD: //32
	case LWS_CALLBACK_DEL_POLL_FD: //33
	case LWS_CALLBACK_CHANGE_MODE_POLL_FD: //34
	case LWS_CALLBACK_LOCK_POLL: //35
	case LWS_CALLBACK_UNLOCK_POLL: //36
	case LWS_CALLBACK_PROTOCOL_INIT: //27
	case LWS_CALLBACK_EVENT_WAIT_CANCELLED: //71
	case LWS_CALLBACK_WSI_CREATE: //29
	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP: //44
	case LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL: //76
	case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: //2 拒绝连接的最后机会
	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:	//17
	case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:	//86
	case LWS_CALLBACK_HTTP_BIND_PROTOCOL:	//49
	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:	//20
	case LWS_CALLBACK_ADD_HEADERS:	//53
		break;
	case LWS_CALLBACK_CONNECTING:
		cout << "Websockets CONNECTING" << endl;
		break;
	case LWS_CALLBACK_ESTABLISHED:	//0
		if (wsi)client_wsi = wsi;
		eve_pool.start();
		cout << "Websockets ESTABLISHED" << endl;
		lws_callback_on_writable(wsi);
		break;
	case LWS_CALLBACK_CLIENT_ESTABLISHED:   //3
		eve_pool.start();
		//连接成功时，会触发此reason
		cout << "Websockets client ESTABLISHED" << endl;
		//调用一次lws_callback_on_writeable，会触发一次callback的LWS_CALLBACK_CLIENT_WRITEABLE，之后可进行一次发送数据操作
		lws_callback_on_writable(wsi);
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
		//cout << "Websockets PONG" << endl;
		break;
	case LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL:	//80
		cout << "Websockets Server Drop." << endl;
		break;
	case LWS_CALLBACK_CLIENT_CLOSED: //75
		// 客户端主动断开、服务端断开都会触发此reason
	case LWS_CALLBACK_CLOSED: //4
		linker->isBreak = true; // ws关闭，发出消息，退出消息循环
		cout << "ws closed." << endl;
		break;
	case LWS_CALLBACK_WSI_DESTROY: //30
		linker->isBreak = true; // ws关闭，发出消息，退出消息循环
		linker->wsi = nullptr;
		cout << "ws destroyed." << endl;
		break;
	case LWS_CALLBACK_PROTOCOL_DESTROY:	//28
		cout << "protocol destroyed." << endl;
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:	//1
		//连接失败、异常
		memcpy(buffer, _in, _len);
		cout << "connect error:" << buffer << endl;
		break;
		// 获取到服务端的数据
	case LWS_CALLBACK_CLIENT_RECEIVE:   //8 
		// 获取到客户端的数据
	case LWS_CALLBACK_RECEIVE:	//6
		memcpy(buffer, _in, _len);
		linker->rcvData += buffer;
		if (lws_is_final_fragment(wsi)) {
			//if (_len < MAX_PAYLOAD_SIZE && _len != 4082) {
			static size_t posN{ 0 };
			while ((posN = linker->rcvData.find("\n")) != string::npos) {
				eve_pool.push(linker->rcvData.substr(0, posN));
				linker->rcvData = linker->rcvData.substr(posN + 1);
			}
			if (!linker->rcvData.empty()) {
				eve_pool.push(linker->rcvData);
				linker->rcvData.clear();
			}
		}
		lws_callback_on_writable(wsi);
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE: //10
		break;
		//调用lws_callback_on_writeable，会触发一次此reason
	case LWS_CALLBACK_SERVER_WRITEABLE: //11
		//if (wsi)linker->wsi = wsi;
		break;
	default:
		cout << "CALLBACK REASON: " << reasons << endl;
		break;
	}
	return 0;
}

int main() {
	if (api::init()) {
		while (link_server()) {
			linker->keep();
			sleep_for(30s);
		}
	}
	return 0;
}