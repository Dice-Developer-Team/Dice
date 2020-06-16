/*此文件是下面三个头文件的实现*/
#include <cassert>
#include <utility>
#include "CQAPI_EX.h"
#include "bufstream.h"
#include "CQLogger.h"
#include "CQMsgSend.h"
#include "CQconstant.h"

using namespace CQ;
using namespace std;

logger::logger(std::string title) noexcept : title(std::move(title))
{
}

void logger::setTitle(std::string title) noexcept { this->title = std::move(title); }

void logger::Debug(const std::string& msg) const noexcept { Debug(msg.c_str()); }

void logger::Info(const std::string& msg) const noexcept { Info(msg.c_str()); }

void logger::InfoSuccess(const std::string& msg) const noexcept { InfoSuccess(msg.c_str()); }

void logger::InfoRecv(const std::string& msg) const noexcept { InfoRecv(msg.c_str()); }

void logger::InfoSend(const std::string& msg) const noexcept { InfoSend(msg.c_str()); }

void logger::Warning(const std::string& msg) const noexcept { Warning(msg.c_str()); }

void logger::Error(const std::string& msg) const noexcept { Error(msg.c_str()); }

void logger::Fatal(const std::string& msg) const noexcept { Fatal(msg.c_str()); }

void logger::Debug(const char* const msg) const noexcept { addLog(Log_Debug, title.c_str(), msg); }

void logger::Info(const char* const msg) const noexcept { addLog(Log_Info, title.c_str(), msg); }

void logger::InfoSuccess(const char* const msg) const noexcept { addLog(Log_InfoSuccess, title.c_str(), msg); }

void logger::InfoRecv(const char* const msg) const noexcept { addLog(Log_InfoRecv, title.c_str(), msg); }

void logger::InfoSend(const char* const msg) const noexcept { addLog(Log_InfoSend, title.c_str(), msg); }

void logger::Warning(const char* const msg) const noexcept { addLog(Log_Warning, title.c_str(), msg); }

void logger::Error(const char* const msg) const noexcept { addLog(Log_Error, title.c_str(), msg); }

void logger::Fatal(const char* const msg) const noexcept { addLog(Log_Fatal, title.c_str(), msg); }

logstream logger::Debug() const noexcept { return logstream(title, Log_Debug); }

logstream logger::Info() const noexcept { return logstream(title, Log_Info); }

logstream logger::InfoSuccess() const noexcept { return logstream(title, Log_InfoSuccess); }

logstream logger::InfoRecv() const noexcept { return logstream(title, Log_InfoRecv); }

logstream logger::InfoSend() const noexcept { return logstream(title, Log_InfoSend); }

logstream logger::Warning() const noexcept { return logstream(title, Log_Warning); }

logstream logger::Error() const noexcept { return logstream(title, Log_Error); }

logstream logger::Fatal() const noexcept { return logstream(title, Log_Fatal); }

void CQ::send(CQstream& log) noexcept
{
	log.send();
	log.clear();
}

void CQ::flush(CQstream& log) noexcept { log.flush(); }
void CQ::endl(CQstream& log) noexcept { log << "\r\n"; }

void CQstream::clear() noexcept { buf.clear(); }

CQstream& CQstream::append(const string& s) noexcept
{
	buf += s;
	return *this;
}

CQstream& CQstream::operator<<(const string& s) noexcept { return append(s); }

CQstream& CQstream::append(const int& i) noexcept
{
	buf += to_string(i);
	return *this;
}

CQstream& CQstream::operator<<(const int& i) noexcept { return append(i); }

CQstream& CQstream::append(const size_t& i) noexcept
{
	buf += to_string(i);
	return *this;
}

CQstream& CQstream::operator<<(const size_t& i) noexcept { return append(i); }

CQstream& CQstream::append(const long long& l) noexcept
{
	buf += to_string(l);
	return *this;
}

CQstream& CQstream::operator<<(const long long& l) noexcept { return append(l); }

CQstream& CQstream::append(const char* const c) noexcept
{
	buf += c;
	return *this;
}

CQstream& CQstream::operator<<(const char* const c) noexcept { return append(c); }

CQstream& CQstream::operator<<(void (*control)(CQstream&))
{
	control(*this);
	return *this;
}

void CQstream::flush() noexcept { send(); }

inline CQstream::~CQstream() noexcept = default;

inline logstream::logstream(std::string title, const int Log_flag) noexcept : flag(Log_flag), title(std::move(title))
{
}

void logstream::send() noexcept
{
	if (buf.empty())return;
	addLog(flag, title.c_str(), buf.c_str());
}

msg::msg(const long long GroupID_Or_QQID, const msgtype Type) noexcept : ID(GroupID_Or_QQID),
                                                                         subType(static_cast<int>(Type))
{
}

msg::msg(const long long GroupID_Or_QQID, const int Type) noexcept : ID(GroupID_Or_QQID), subType(Type)
{
}

void msg::send() noexcept
{
	if (buf.empty())return;
	switch (subType)
	{
	case static_cast<int>(msgtype::Private): //好友
		sendPrivateMsg(ID, buf);
		break;
	case static_cast<int>(msgtype::Group): //群
		sendGroupMsg(ID, buf);
		break;
	case static_cast<int>(msgtype::Discuss): //讨论组
		sendDiscussMsg(ID, buf);
		break;
	default:
		assert(false);
		break;
	}
}
