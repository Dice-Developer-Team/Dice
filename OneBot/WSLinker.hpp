#pragma once
#include <string>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>
#include "json.hpp"

using std::string;
using std::cout;
using std::endl;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::this_thread;
using nlohmann::json;
constexpr size_t MAX_PAYLOAD_SIZE = 8 * 1024;

class WSLinker {
protected:
    struct lws_context* context;
public:
    lws* wsi{ nullptr };
    bool isBreak{ false };
    string rcvData;
    void write_data(lws* wsi, const string& data) {
        //cout << data << endl;
        char buffer[LWS_PRE + MAX_PAYLOAD_SIZE + 1] = { 0 };
        if (auto len{ data.length() }; len <= MAX_PAYLOAD_SIZE) {
            strcpy(buffer + LWS_PRE, data.c_str());
            lws_write(wsi, (unsigned char*)buffer + LWS_PRE, strlen(buffer + LWS_PRE), LWS_WRITE_TEXT);
        }
        else {
            strncpy(buffer + LWS_PRE, data.c_str(), MAX_PAYLOAD_SIZE);
            lws_write(wsi, (unsigned char*)buffer + LWS_PRE, MAX_PAYLOAD_SIZE, lws_write_protocol(LWS_WRITE_TEXT | LWS_WRITE_NO_FIN));
            size_t pos = MAX_PAYLOAD_SIZE;
            while ((len -= MAX_PAYLOAD_SIZE) > 0) {
                if (len > MAX_PAYLOAD_SIZE) {
                    strncpy(buffer + LWS_PRE, data.c_str() + pos, MAX_PAYLOAD_SIZE);
                    lws_write(wsi, (unsigned char*)buffer + LWS_PRE, MAX_PAYLOAD_SIZE, lws_write_protocol(LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN));
                    pos += MAX_PAYLOAD_SIZE;
                }
                else {
                    strncpy(buffer + LWS_PRE, data.c_str() + pos, len);
                    lws_write(wsi, (unsigned char*)buffer + LWS_PRE, len, LWS_WRITE_CONTINUATION);
                    break;
                }
            }
        }
    }
    virtual void send_data(const string& data) = 0;
    void send(const json& data) {
        send_data(data.dump());
    }
    //virtual json get_data(const json& data) = 0;
    json get_data(const json& data) {
        if (!data.count("echo"))return{};
        string echo_key{ data["echo"] };
        if (auto it{ mEcho.find(echo_key) }; it != mEcho.end()) {
            if (time(nullptr) - it->second.second < 60
                && !it->second.first.is_null())
                return it->second.first;
            else mEcho.erase(it);
        }
        send(data);
        size_t cntTry{ 0 };
        do {
            if (++cntTry > 100)break;
            std::this_thread::sleep_for(50ms);
        } while (!mEcho.count(echo_key));
        return mEcho[echo_key].first;
    }
    void keep() {
        //进入消息循环
        while (!isBreak) {
            lws_service(context, 20);
        }
        lws_context_destroy(context);
        context = nullptr;
    }
    std::unordered_map<std::string, std::pair<json, time_t>> mEcho;
};

//extern std::unique_ptr<WSLinker> linker;
int callback_ws(lws* wsi, enum lws_callback_reasons reasons, void* user, void* in, size_t len);