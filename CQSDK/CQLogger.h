#pragma once
#include "bufstream.h"
#include <string>

#define DEBUGINFO "文件:" << __FILE__ << ",行数:" << __LINE__ << ",输出:"

namespace CQ
{
	class logstream : public CQstream
	{
		int flag;
		std::string title;
	public:
		logstream(std::string title, int Log_flag);

		// 通过 CQstream 继承
		void send() override;
	};

	class logger
	{
		std::string title;
	public:
		logger(std::string title);
		void setTitle(std::string title);

		void Debug(const std::string& msg) const;
		void Info(const std::string& msg) const;
		void InfoSuccess(const std::string& msg) const;
		void InfoRecv(const std::string& msg) const;
		void InfoSend(const std::string& msg) const;
		void Warning(const std::string& msg) const;
		void Error(const std::string& msg) const;
		void Fatal(const std::string& msg) const;

		void Debug(const char* msg) const;
		void Info(const char* msg) const;
		void InfoSuccess(const char* msg) const;
		void InfoRecv(const char* msg) const;
		void InfoSend(const char* msg) const;
		void Warning(const char* msg) const;
		void Error(const char* msg) const;
		void Fatal(const char* msg) const;

		logstream Debug() const;
		logstream Info() const;
		logstream InfoSuccess() const;
		logstream InfoRecv() const;
		logstream InfoSend() const;
		logstream Warning() const;
		logstream Error() const;
		logstream Fatal() const;
	};
}
