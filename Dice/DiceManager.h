#pragma once
#include <regex>
#include <CivetServer.h>
#include <json.hpp>
#include "Jsonio.h"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include "DiceConsole.h"
#include "DiceSchedule.h"
#include "DiceMod.h"
#include "MsgMonitor.h"
#include "CQTools.h"
#include "CardDeck.h"

// init in EventEnable
inline std::filesystem::path WebUIPasswordPath;

// ÉèÖÃWebUIÃÜÂë
bool setPassword(const std::string& password);

class AuthHandler: public CivetAuthHandler
{
    bool authorize(CivetServer *server, struct mg_connection *conn)
    {
        if (mg_check_digest_access_authentication(conn, "DiceWebUI", WebUIPasswordPath.u8string().c_str()) == 0)
        {
            mg_send_digest_access_authentication_request(conn, "DiceWebUI");
            return false;
        }
        return true;
    }
};

class BasicInfoApiHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
			j["data"] = nlohmann::json::object();
            j["data"]["version"] = GBKtoUTF8(Dice_Full_Ver);
            j["data"]["qq"] = console.DiceMaid;
            j["data"]["nick"] = GBKtoUTF8(getMsg("strSelfName"));
            j["data"]["running_time"] = GBKtoUTF8(printDuringTime(time(nullptr) - llStartTime));
            j["data"]["cmd_count"] = std::to_string(FrqMonitor::sumFrqTotal.load());
            j["data"]["cmd_count_today"] = today->get("frq").to_str();
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;    
    }
};

class CustomMsgApiHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try
        {
            std::shared_lock lock(GlobalMsgMutex);
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
            j["count"] = GlobalMsg.size();
			j["data"] = nlohmann::json::array();
            for (const auto& [key,val] : GlobalMsg)
            {
                j["data"].push_back({{"name", GBKtoUTF8(key)}, {"value", GBKtoUTF8(val)}, {"remark", GBKtoUTF8(getComment(key))}});
            }
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try 
        {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"].get<std::string>() == "set")
            {
                std::unique_lock lock(GlobalMsgMutex);
                for(const auto& item: j["data"]){
                    GlobalMsg[UTF8toGBK(item["name"].get<std::string>())] = UTF8toGBK(item["value"].get<std::string>());
                    EditedMsg[UTF8toGBK(item["name"].get<std::string>())] = UTF8toGBK(item["value"].get<std::string>());
                }
                saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
            }
            else if (j["action"].get<std::string>() == "reset") {
                fmt->msg_reset(UTF8toGBK(j["data"]["name"].get<std::string>()));
            }
            else
            {
                throw std::runtime_error("Invalid Action");
            }
            nlohmann::json j2 = nlohmann::json::object();   
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class CustomReplyApiHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
            j["count"] = fmt->custom_reply.size();
			j["data"] = nlohmann::json::array();
            for (const auto& [key,val] : fmt->custom_reply)
            {
                j["data"].push_back({ {"name", GBKtoUTF8(key)} ,
                    {"keyword", GBKtoUTF8(val->keyMatch[0] ? listItem(*val->keyMatch[0]) :
                        val->keyMatch[1] ? listItem(*val->keyMatch[1]) :
                        val->keyMatch[2] ? listItem(*val->keyMatch[2]) :
                        val->keyMatch[3] ? listItem(*val->keyMatch[3]) : "")},
                    {"type", GBKtoUTF8(DiceMsgReply::sType[(int)val->type])},
                    {"mode", val->keyMatch[0] ? "Match" :
                        val->keyMatch[1] ? "Prefix" :
                        val->keyMatch[2] ? "Search" : "Regex"},
                    {"limit", GBKtoUTF8(val->limit.print())},
                    {"echo", GBKtoUTF8(DiceMsgReply::sEcho[(int)val->echo])},
                    {"answer", GBKtoUTF8(val->echo == DiceMsgReply::Echo::Deck ? listItem(val->deck)
                        : val->echo == DiceMsgReply::Echo::Lua ? val->text.to_dict()->at("script").to_str()
                        : val->text.to_str())} });
            }
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try 
        {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"] == "set")
            {
                for(const auto& item: j["data"])
                {   
                    ptr<DiceMsgReply> trigger{ std::make_shared<DiceMsgReply>() };
                    trigger->title = UTF8toGBK(item["name"].get<std::string>());
                    trigger->readJson(item);
                    fmt->set_reply(trigger->title, trigger);
                }
            } 
            else if (j["action"] == "delete")
            {
                for(const auto& item: j["data"])
                {
                    fmt->del_reply(UTF8toGBK(item["name"].get<std::string>()));
                }
            }
            else
            {
                throw std::runtime_error("Invalid Action");
            }
            nlohmann::json j2 = nlohmann::json::object();   
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class ModListApiHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer* server, struct mg_connection* conn)
    {
        std::string ret;
        try {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
            j["count"] = fmt->modOrder.size();
            j["data"] = nlohmann::json::array();
            for (const auto& mod : fmt->modOrder) {
                j["data"].push_back({ {"name", GBKtoUTF8(mod->name)} ,
                    {"title", GBKtoUTF8(mod->title.empty() ? mod->name : mod->title)},
                    {"ver", GBKtoUTF8(mod->ver.exp)},
                    {"author", GBKtoUTF8(mod->author)},
                    {"brief", GBKtoUTF8(mod->brief)},
                    {"active", mod->active},
                    });
            }
            ret = j.dump();
        }
        catch (const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn)
    {
        std::string ret;
        try {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"] == "switch") {
                fmt->turn_over(j["data"]);
            }
            else if(j["action"] == "delete") {
                for (const auto& item : j["data"]) {
                    fmt->uninstall(UTF8toGBK(item["name"].get<std::string>()));
                }
            }
            else if (j["action"] == "reorder") {
                size_t oldIdx{ j["data"]["oldIndex"] }, newIndex{ j["data"]["newIndex"]};
                if(!fmt->reorder(oldIdx,newIndex))throw std::runtime_error("Invalid Index");
            }
            else if (j["action"] == "install") {
                string name{ UTF8toGBK(j["data"]["name"].get<std::string>()) };
#ifndef __ANDROID__
                if (j["data"].count("repo") && fmt->mod_clone(name, j["data"]["repo"]))goto Success;
#endif
                if (string res; j["data"].count("pkg") && fmt->mod_dlpkg(name, j["data"]["pkg"], res))goto Success;
                else throw std::runtime_error(res);
                throw std::runtime_error("No Available Url");
            }
            else {
                throw std::runtime_error("Invalid Action");
            }
Success:
            nlohmann::json j2 = nlohmann::json::object();
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch (const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class AdminConfigHandler : public CivetHandler 
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
			j["data"] = nlohmann::json::array();
            for (const auto& item : console.intDefault)
            {
                const int value = console[item.first.c_str()];
                j["data"].push_back({
                    {"name", GBKtoUTF8(item.first)},
                    {"value", value},
                    {"remark", GBKtoUTF8(console.confComment.find(item.first)->second)},
                    });
            }
            j["count"] = j["data"].size();
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try 
        {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"].get<std::string>() == "set")
            {
                for(const auto& item: j["data"])
                {
                    console.set(UTF8toGBK(item["name"].get<std::string>()), item["value"].get<int>());
                }
            } 
            else
            {
                throw std::runtime_error("Invalid Action");
            }
            nlohmann::json j2 = nlohmann::json::object();   
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class MasterHandler : public CivetHandler 
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = 0;
            j["msg"] = "ok";
			j["data"] = nlohmann::json::object();
            j["data"]["master"] = (long long)console;
            j["data"]["mode"] = console["Private"] ? "private" : "public";
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try 
        {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"].get<std::string>() == "set")
            {
                const long long master = j["data"]["master"];
                if (master == 0) {
                    if (console){
                        console.killMaster();
                    }
                }
                else if (console != master){
                    console.set("private", j["data"]["mode"] == "private" ? 1 : 0);
                    console.newMaster(master);
                }
            } 
            else
            {
                throw std::runtime_error("Invalid Action");
            }
            nlohmann::json j2 = nlohmann::json::object();   
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class UrlApiHandler : public CivetHandler {
public:
    bool handleGet(CivetServer* server, struct mg_connection* conn)
    {
        std::string ret;
        try {
            nlohmann::json j = nlohmann::json::object();
            if (string url; server->getParam(conn, "url", url)) {
                if (!Network::GET(url, ret)) {
                    j["code"] = -1;
                    j["msg"] = GBKtoUTF8(ret);
                    ret = j.dump();
                }
            }
            else {
                j["code"] = -1;
                j["msg"] = "unknown url";
                ret = j.dump();
            }
        }
        catch (const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn)
    {
        std::string ret;
        try {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (!Network::POST(j["url"], j["content"], j.count("header") ? j["header"] : "Content-Type: application/json", ret)) {
                nlohmann::json j2 = nlohmann::json::object();
                j2["code"] = -1;
                j2["msg"] = GBKtoUTF8(ret);
                ret = j2.dump();
            }
        }
        catch (const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};

class WebUIPasswordHandler : public CivetHandler 
{
public:
   
    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        std::string ret;
        try 
        {
            auto data = server->getPostData(conn);
            nlohmann::json j = nlohmann::json::parse(data);
            if (j["action"].get<std::string>() == "set")
            {
                if (!setPassword(j["data"]["password"].get<std::string>()))
                {
                    throw std::runtime_error("Set Password Failed");
                }
            } 
            else
            {
                throw std::runtime_error("Invalid Action");
            }
            nlohmann::json j2 = nlohmann::json::object();   
            j2["code"] = 0;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["code"] = -1;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};