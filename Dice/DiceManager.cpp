#include <CivetServer.h>
#include <fstream>
#include "filesystem.hpp"
#include "DiceManager.h"
#include "DiceJS.h"
#include "DicePython.h"
#include "DiceLua.h"


bool setPassword(const std::string& password)
{
	// mg_modify_passwords_file似乎只接受ascii路径而不是其他函数的utf-8路径
	// anyway, try both
	if (!mg_modify_passwords_file(WebUIPasswordPath.u8string().c_str(), "DiceWebUI", "admin", password.c_str()))
	{
		std::error_code ec;
		if (!std::filesystem::exists(WebUIPasswordPath, ec))
		{
			ofstream(WebUIPasswordPath).close();
		}
		return mg_modify_passwords_file(getNativePathString(WebUIPasswordPath).c_str(), "DiceWebUI", "admin", password.c_str());
	}
	return true; 
}

bool CommandRunner::handlePost(CivetServer* server, struct mg_connection* conn) {
	std::string ret;
	try
	{
		auto data = server->getPostData(conn);
		fifo_json j = fifo_json::parse(data);
		string mode{ j["mode"] };
		if (mode == "chat") { //虚拟聊天消息，message 形如 { fromMsg, uid }
			const auto& message{ j["message"] };
			AnysTable eve;
			if (message.is_object()) {
				eve.from_json(message);
			}
			else { //兼容纯文本
				eve.set("fromMsg", message.get<std::string>());
			}
			// 发送者暂时固定为骰娘本人，弃用前端传来的 uid
			if (console.DiceMaid <= 0) {
				throw std::runtime_error("骰娘账号未就绪（servant_id 为空），无法构造虚拟私聊事件");
			}
			eve.set("uid", AttrVar(console.DiceMaid));
			shared_ptr<DiceEvent> msg{ std::make_shared<DiceEvent>(eve) };
			msg->virtualCall();
			nlohmann::json j2 = nlohmann::json::object();
			j2["code"] = 0;
			j2["msg"] = msg->strReply.empty() ? "消息已处理√" : msg->strReply;
			ret = j2.dump();
		}
		else if (string cmd{ j["message"] }; mode == "js") {
			auto res{ js_simple_eval(cmd) };
			nlohmann::json j2 = nlohmann::json::object();
			j2["code"] = 0;
			j2["msg"] = res ? res.to_str() : "js脚本已运行√";
			ret = j2.dump();
		}
		else if (mode == "py") {
			if (py) {
				auto res{ py->evalString(cmd) };
				nlohmann::json j2 = nlohmann::json::object();
				j2["code"] = 0;
				j2["msg"] = res ? res.to_str() : "python脚本已运行√";
				ret = j2.dump();
			}
			throw std::runtime_error("Python 未启用（EnablePython 为假或初始化失败）");
		}
		else if (mode == "lua") {
			static LuaState lua;
			auto res{ lua.dostring(cmd) };
			nlohmann::json j2 = nlohmann::json::object();
			j2["code"] = 0;
			j2["msg"] = res ? res.to_str() : "lua脚本已运行√";
			ret = j2.dump();
		}
		else
		{
			throw std::runtime_error("Invalid Command");
		}
	}
	catch (const std::exception& e)
	{
		nlohmann::json j = nlohmann::json::object();
		j["code"] = -1;
		j["msg"] = e.what();
		ret = j.dump();
	}
	mg_send_http_ok(conn, "application/json", ret.length());
	mg_write(conn, ret.c_str(), ret.length());
	return true;
}