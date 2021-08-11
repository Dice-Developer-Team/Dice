#pragma once
#include <CivetServer.h>
#include <json.hpp>
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include "DiceConsole.h"

class IndexHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string html = R"(<!DOCTYPE html>
<html>
<head>
<title>Dice! WebUI</title>
</head>
<body>
<h1>Dice WebUI</h1>
<p>点击<a href="#">这里</a>进入Dice WebUI</p>
</body>
</html>)";

        mg_send_http_ok(conn, "text/html", html.length());
        mg_write(conn, html.c_str(), html.length());
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
            nlohmann::json j = nlohmann::json::object();
            j["ret"] = 200;
            j["msg"] = "ok";
            for (auto& [key,val] : GlobalMsg)
            {
                j["data"][GBKtoUTF8(key)] = GBKtoUTF8(val);
            }
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["ret"] = 500;
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
            for(auto& [key,val] : j.items())
            {
                GlobalMsg[UTF8toGBK(key)] = UTF8toGBK(val.get<std::string>());
            }
            nlohmann::json j2 = nlohmann::json::object();
            j2["ret"] = 200;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["ret"] = 500;
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
            j["ret"] = 200;
            j["msg"] = "ok";
            j["data"] = nlohmann::json::object();
            for (const auto& item : console.intDefault)
            {
                const int value = console[item.first.c_str()];
                j["data"][GBKtoUTF8(item.first)] = value;
            }
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["ret"] = 500;
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
            for(auto& [key,val] : j.items())
            {
                console.set(UTF8toGBK(key), val.get<int>());
            }
            nlohmann::json j2 = nlohmann::json::object();
            j2["ret"] = 200;
            j2["msg"] = "ok";
            ret = j2.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = nlohmann::json::object();
            j["ret"] = 500;
            j["msg"] = GBKtoUTF8(e.what());
            ret = j.dump();
        }
        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};
