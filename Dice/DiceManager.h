#pragma once
#include <CivetServer.h>
#include <json.hpp>
#include "GlobalVar.h"
#include "EncodingConvert.h"

class IndexHandler : public CivetHandler
{
public:
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        std::string html = R"(<!DOCTYPE html>
<html>
<head>
<title>Dice! Manager</title>
</head>
<body>
<p>Hello World</p>
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
            j["result"] = "ok";
            for (auto& [key,val] : GlobalMsg)
            {
                j["data"][GBKtoUTF8(key)] = GBKtoUTF8(val);
            }
            ret = j.dump();
        }
        catch(const std::exception& e)
        {
            nlohmann::json j = { {"result", "error"}, {"msg" , e.what()}};
            ret = j.dump();
        }

        mg_send_http_ok(conn, "application/json", ret.length());
        mg_write(conn, ret.c_str(), ret.length());
        return true;
    }
};