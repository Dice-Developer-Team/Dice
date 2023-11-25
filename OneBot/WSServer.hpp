#pragma once
#include <queue>
#define LWS_ROLE_H1
#define LWS_WITH_NETWORK
#include "libwebsockets.h"
#include "WSLinker.hpp"
using std::queue;

extern lws* client_wsi;
class EventServer: public WSLinker {
public:
    queue<string> qSending;
	EventServer(std::string url) {
        bool ssl{ false }; //确认是否进行SSL加密
        string addr; //目标主机
        string path{ "/" }; //目标主机服务PATH
        int port{ CONTEXT_PORT_NO_LISTEN }; //端口号

        size_t pslash{ url.find("//", 0) };
        if (pslash != string::npos) {
            ssl = 0 == url.find("wss");
            url = url.substr(pslash + 2);
        }
        if ((pslash = url.find('/')) == string::npos) {   //没有path
            if ((pslash = url.find(':')) == string::npos) { //只有端口号
                if (strspn(url.c_str(), "0123456789") == url.length()) {
                    addr = "localhost";
                    port = stoi(url);
                }
                else addr = url;    //没有端口号
            }
            else {   //有端口号
                addr = url.substr(0, pslash);
                port = stoi(url.substr(pslash + 1));
            }
        }
        else {  //有path
            path = url.substr(pslash + 1);
            addr = url.substr(0, pslash);
            if ((pslash = url.find(':')) != string::npos) {  //有端口号
                addr = addr.substr(0, pslash);
                port = stoi(addr.substr(pslash + 1));
            }
        }
        /*cout << "ws init:" << url << endl
            << "addr " << addr << endl
            << "path " << path << endl
            << "port " << port << endl;*/
        struct lws_protocols lwsprotocol[]{
            //{ "http-only", callback_http, 0, 0 },
            {
                //协议名称，协议回调，接收缓冲区大小
                "ws-protocol", callback_ws, sizeof(EventServer), MAX_PAYLOAD_SIZE,
            },
            {
                NULL, NULL,   0 // 最后一个元素固定为此格式
            }
        };
        //lws初始化阶段
        lws_set_log_level(0xFF, NULL);
        struct lws_context_creation_info info { 0 }; //websocket 配置参数

        info.protocols = lwsprotocol;       //设置处理协议
        info.port = port; 
        //ws和wss的初始化配置不同
        if (ssl) {
            info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT | LWS_SERVER_OPTION_DISABLE_IPV6 | LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED | LWS_SERVER_OPTION_IGNORE_MISSING_CERT;
            //info.ssl_cert_filepath = getenv(X509_get_default_cert_file_env()); 
            //info.ssl_private_key_filepath = SSL_PRIVATE_KEY_PATH;
            info.ssl_ca_filepath = "./ssl/ca-cert.pem";
            info.ssl_cert_filepath = "./ssl/client-cert.pem";
            std::cout << std::filesystem::current_path() / "ssl" / "client-cert.pem" << std::endl;
            //info.ssl_cert_filepath = (std::filesystem::current_path() / "ssl" / "client-cert.pem").string().c_str();
            info.ssl_private_key_filepath = "./ssl/client-key.pem";
        }
        //info.options = ssl ? LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT : 0; //如果是wss，需要做全局SSL初始化
        info.gid = -1;
        info.uid = -1;

        // 创建WebSocket语境
        context = lws_create_context(&info); //websocket 连接上下文
        while (context == NULL) {
            std::cout << "websocket连接上下文创建失败" << std::endl;
        }
    }
    void send_data(const string& data) {
        if (wsi)write_data(wsi, data);
        else if (client_wsi)write_data(client_wsi, data);
        else std::cout << "lws不存在，无法发送" << std::endl;
    }
};