#pragma once
#include <memory>
#include <filesystem>
#include "openssl/ssl.h"
#include "openssl/rsa.h"
#define LWS_ROLE_H1
#define LWS_WITH_NETWORK
#include "libwebsockets.h"
#include "WSLinker.hpp"

/*struct session_data {
    EventClient* client;
	unsigned char buf[LWS_PRE + MAX_PAYLOAD_SIZE];
	int len;
};*/
/**
 * 某个协议下的连接发生事件时，执行的回调函数
 * wsi：指向WebSocket实例的指针
 * reason：导致回调的事件
 * user 库为每个WebSocket会话分配的内存空间
 * in 指向与连接关联的SSL结构的指针
 * len 某些事件使用此参数，说明传入数据的长度
 */
//int callback_http(lws* wsi, enum lws_callback_reasons reasons, void* user, void* in, size_t len);
class WSClient: public WSLinker {
protected:
	bool isBuilt{ false };
	//bool isStoping{ false };
	int recvSum{ 0 };
	time_t lastGet{ 0 };
    //friend int callback_http(lws*, enum lws_callback_reasons, void*, void*, size_t);
    friend int callback_ws(lws*, enum lws_callback_reasons, void*, void*, size_t);
public:
    WSClient(string url) {
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
                "ws-protocol", callback_ws, sizeof(void*), MAX_PAYLOAD_SIZE,
            },
            {
                NULL, NULL,   0 // 最后一个元素固定为此格式
            }
        };
        //lws初始化阶段
        lws_set_log_level(0xFF, NULL);
        struct lws_context_creation_info info { 0 }; //websocket 配置参数

        info.protocols = lwsprotocol;       //设置处理协议
        info.port = CONTEXT_PORT_NO_LISTEN; //作为ws客户端，无需绑定端口
        //ws和wss的初始化配置不同
        if (ssl) {
            info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT | LWS_SERVER_OPTION_DISABLE_IPV6 | LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED | LWS_SERVER_OPTION_IGNORE_MISSING_CERT;
            //info.ssl_cert_filepath = getenv(X509_get_default_cert_file_env()); 
            //info.ssl_private_key_filepath = SSL_PRIVATE_KEY_PATH;
            info.ssl_ca_filepath = "./ssl/ca-cert.pem";
            //info.ssl_cert_filepath = SSL_CERT_FILE_PATH;
            cout << std::filesystem::current_path() / "ssl" / "client-cert.pem";
            //info.ssl_cert_filepath = (std::filesystem::current_path() / "ssl" / "client-cert.pem").string().c_str();
            info.ssl_private_key_filepath = "./ssl/client-key.pem";
        }
        //info.options = ssl ? LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT : 0; //如果是wss，需要做全局SSL初始化
        info.gid = -1;
        info.uid = -1; 

        // 创建WebSocket语境
        context = lws_create_context(&info); //websocket 连接上下文
        while (context == NULL) {
            cout << "websocket连接上下文创建失败" << endl;
        }

        //初始化连接信息
        struct lws_client_connect_info conn_info { 0 };     //websocket 连接信息
        conn_info.context = context;      //设置上下文
        conn_info.address = addr.c_str(); //设置目标主机IP
        conn_info.port = port;            //设置目标主机服务端口
        conn_info.path = path.c_str();    //设置目标主机服务PATH
        conn_info.host = lws_canonical_hostname(context);      //设置目标主机header
        conn_info.origin = "origin";    //设置目标主机IP
        //ci.pwsi = &wsi;            //设置wsi句柄
        conn_info.userdata = this;        //userdata 指针会传递给callback的user参数，一般用作自定义变量传入
        conn_info.protocol = lwsprotocol[0].name;

        //ws/wss需要不同的配置
        if (ssl) {
            conn_info.ssl_connection = LCCSCF_USE_SSL
                | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_INSECURE;
        }

        do {//使连接信息生效
            if (wsi = lws_client_connect_via_info(&conn_info))break;
            cout << "Client Connect Waiting: " << addr << ":" << port << "/" << path << endl;
            sleep_for(3s);
        } while (!wsi);
    }
    ~WSClient() {
        if (context) {
            lws_context_destroy(context);
        }
    }
    void send_data(const string& data) {
        if(wsi)write_data(wsi, data);
    }
    static string access_token;
	void shut() {
        isBreak = true;
    }
};