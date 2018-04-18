
/*此文件是下面三个头文件的实现*/
#include "..\CQSDK\CQAPI_EX.h"
#include "..\CQSDK\bufstream.h"
#include "..\CQSDK\CQLogger.h"
#include "..\CQSDK\CQMsgSend.h"

using namespace CQ;
using namespace std;

CQ::logger::logger(std::string title) : title(title) {}

void CQ::logger::setTitle(std::string title) { this->title = title; }

void CQ::logger::Debug(std::string & msg) const { Debug(msg.c_str()); }

void CQ::logger::Info(std::string & msg) const { Info(msg.c_str()); }

void CQ::logger::InfoSuccess(std::string & msg) const { InfoSuccess(msg.c_str()); }

void CQ::logger::InfoRecv(std::string & msg) const { InfoRecv(msg.c_str()); }

void CQ::logger::InfoSend(std::string & msg) const { InfoSend(msg.c_str()); }

void CQ::logger::Warning(std::string & msg) const { Warning(msg.c_str()); }

void CQ::logger::Error(std::string & msg) const { Error(msg.c_str()); }

void CQ::logger::Fatal(std::string & msg) const { Fatal(msg.c_str()); }

void CQ::logger::Debug(const char * msg) const { addLog(Log_Debug, title.c_str(), msg); }

void CQ::logger::Info(const char * msg) const { addLog(Log_Info, title.c_str(), msg); }

void CQ::logger::InfoSuccess(const char * msg) const { addLog(Log_InfoSuccess, title.c_str(), msg); }

void CQ::logger::InfoRecv(const char * msg) const { addLog(Log_InfoRecv, title.c_str(), msg); }

void CQ::logger::InfoSend(const char * msg) const { addLog(Log_InfoSend, title.c_str(), msg); }

void CQ::logger::Warning(const char * msg) const { addLog(Log_Warning, title.c_str(), msg); }

void CQ::logger::Error(const char * msg) const { addLog(Log_Error, title.c_str(), msg); }

void CQ::logger::Fatal(const char * msg) const { addLog(Log_Fatal, title.c_str(), msg); }

logstream CQ::logger::Debug() const { return logstream(title, Log_Debug); }

logstream CQ::logger::Info() const { return logstream(title, Log_Info); }

logstream CQ::logger::InfoSuccess() const { return logstream(title, Log_InfoSuccess); }

logstream CQ::logger::InfoRecv() const { return logstream(title, Log_InfoRecv); }

logstream CQ::logger::InfoSend() const { return logstream(title, Log_InfoSend); }

logstream CQ::logger::Warning() const { return logstream(title, Log_Warning); }

logstream CQ::logger::Error() const { return logstream(title, Log_Error); }

logstream CQ::logger::Fatal() const { return logstream(title, Log_Fatal); }

void CQ::send(CQstream & log) { log.send(); log.clear(); }
void CQ::flush(CQstream & log) { log.flush(); }
void CQ::endl(CQstream & log) { log << "\r\n"; }

void CQ::CQstream::clear() { buf.clear(); }

CQstream & CQ::CQstream::append(const string & s) { buf += s; return *this; }

CQstream & CQ::CQstream::operator<<(const string & s) { return (*this).append(s); }

CQstream & CQ::CQstream::append(const int & i) { buf += to_string(i); return *this; }

CQstream & CQ::CQstream::operator<<(const int & i) { return (*this).append(i); }

CQstream & CQ::CQstream::append(const size_t & i) { buf += to_string(i); return *this; }

CQstream & CQ::CQstream::operator<<(const size_t & i) { return (*this).append(i); }

CQstream & CQ::CQstream::append(const long long & l) { buf += to_string(l); return *this; }

CQstream & CQ::CQstream::operator<<(const long long & l) { return (*this).append(l); }

CQstream & CQ::CQstream::append(const char * c) { buf += c; return *this; }

CQstream & CQ::CQstream::operator<<(const char * c) { return (*this).append(c); }

CQstream & CQ::CQstream::operator<<(void(*control)(CQstream &)) { control(*this); return *this; }

void CQ::CQstream::flush() { send();  }

inline CQ::CQstream::~CQstream() {}

inline CQ::logstream::logstream(std::string title, int Log_flag) : flag(Log_flag), title(title) {}

void CQ::logstream::send() {if (buf.size() <= 0)return; addLog(flag, title.c_str(), buf.c_str()); }

CQ::msg::msg(long long ID, msgtype Type) : ID(ID), subType(Type) {}

CQ::msg::msg(long long ID, int Type) : ID(ID), subType(Type) {}

void CQ::msg::send() {
	if (buf.size() <= 0)return;
	switch (subType)
	{
	case msgtype::好友://好友
		sendPrivateMsg(ID, buf);
		break;
	case msgtype::群://群
		sendGroupMsg(ID, buf);
		break;
	case msgtype::讨论组://讨论组
		sendDiscussMsg(ID, buf);
		break;
	default:
		static logger log("异常报告");
		log.Warning()
			<< "消息发送异常"
			<< ",类别:" << ID
			<< ",原文: " << buf
			<< CQ::send;
		break;
	}
}
