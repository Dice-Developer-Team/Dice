/*此文件是下面三个头文件的实现*/
#include <utility>
#include "CQAPI_EX.h"
#include "bufstream.h"
#include "CQLogger.h"
#include "CQMsgSend.h"
#include "CQconstant.h"

using namespace CQ;
using namespace std;

logger::logger(std::string title) : title(std::move(title))
{
}

void logger::setTitle(std::string title) { this->title = std::move(title); }

void logger::Debug(const std::string& msg) const { Debug(msg.c_str()); }

void logger::Info(const std::string& msg) const { Info(msg.c_str()); }

void logger::InfoSuccess(const std::string& msg) const { InfoSuccess(msg.c_str()); }

void logger::InfoRecv(const std::string& msg) const { InfoRecv(msg.c_str()); }

void logger::InfoSend(const std::string& msg) const { InfoSend(msg.c_str()); }

void logger::Warning(const std::string& msg) const { Warning(msg.c_str()); }

void logger::Error(const std::string& msg) const { Error(msg.c_str()); }

void logger::Fatal(const std::string& msg) const { Fatal(msg.c_str()); }

void logger::Debug(const char* const msg) const { addLog(Log_Debug, title.c_str(), msg); }

void logger::Info(const char* const msg) const { addLog(Log_Info, title.c_str(), msg); }

void logger::InfoSuccess(const char* const msg) const { addLog(Log_InfoSuccess, title.c_str(), msg); }

void logger::InfoRecv(const char* const msg) const { addLog(Log_InfoRecv, title.c_str(), msg); }

void logger::InfoSend(const char* const msg) const { addLog(Log_InfoSend, title.c_str(), msg); }

void logger::Warning(const char* const msg) const { addLog(Log_Warning, title.c_str(), msg); }

void logger::Error(const char* const msg) const { addLog(Log_Error, title.c_str(), msg); }

void logger::Fatal(const char* const msg) const { addLog(Log_Fatal, title.c_str(), msg); }

logstream logger::Debug() const { return logstream(title, Log_Debug); }

logstream logger::Info() const { return logstream(title, Log_Info); }

logstream logger::InfoSuccess() const { return logstream(title, Log_InfoSuccess); }

logstream logger::InfoRecv() const { return logstream(title, Log_InfoRecv); }

logstream logger::InfoSend() const { return logstream(title, Log_InfoSend); }

logstream logger::Warning() const { return logstream(title, Log_Warning); }

logstream logger::Error() const { return logstream(title, Log_Error); }

logstream logger::Fatal() const { return logstream(title, Log_Fatal); }

void CQ::send(CQstream& log)
{
	log.send();
	log.clear();
}

void CQ::flush(CQstream& log) { log.flush(); }
void CQ::endl(CQstream& log) { log << "\r\n"; }

void CQstream::clear() { buf.clear(); }

CQstream& CQstream::append(const string& s)
{
	buf += s;
	return *this;
}

CQstream& CQstream::operator<<(const string& s) { return append(s); }

CQstream& CQstream::append(const int& i)
{
	buf += to_string(i);
	return *this;
}

CQstream& CQstream::operator<<(const int& i) { return append(i); }

CQstream& CQstream::append(const size_t& i)
{
	buf += to_string(i);
	return *this;
}

CQstream& CQstream::operator<<(const size_t& i) { return append(i); }

CQstream& CQstream::append(const long long& l)
{
	buf += to_string(l);
	return *this;
}

CQstream& CQstream::operator<<(const long long& l) { return append(l); }

CQstream& CQstream::append(const char* const c)
{
	buf += c;
	return *this;
}

CQstream& CQstream::operator<<(const char* const c) { return append(c); }

CQstream& CQstream::operator<<(void (*control)(CQstream&))
{
	control(*this);
	return *this;
}

void CQstream::flush() { send(); }

inline CQstream::~CQstream() = default;

inline logstream::logstream(std::string title, const int Log_flag) : flag(Log_flag), title(std::move(title))
{
}

void logstream::send()
{
	if (buf.empty())return;
	addLog(flag, title.c_str(), buf.c_str());
}

msg::msg(const long long GroupID_Or_QQID, const msgtype Type) : ID(GroupID_Or_QQID), subType(Type)
{
}

msg::msg(const long long GroupID_Or_QQID, const int Type) : ID(GroupID_Or_QQID), subType(Type)
{
}

void msg::send()
{
	if (buf.empty())return;
	switch (subType)
	{
	case Friend: //好友
		sendPrivateMsg(ID, buf);
		break;
	case Group: //群
		sendGroupMsg(ID, buf);
		break;
	case Discuss: //讨论组
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
