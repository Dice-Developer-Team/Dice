#pragma once
#include "CQEVEBasic.h"

#include <string>
#include <map>

namespace CQ
{
	// 字体
	struct Font final
	{
		const char* Name{};
		int Size{},
		    Color{},
		    /*粗体：1 斜体：2 下划线：4*/
		    Style{},
		    Bubble{};

		explicit Font(int);
	};

	//正则消息
	class regexMsg final
	{
		//消息
		std::map<std::string, std::string> regexMap{};
	public:
		regexMsg(const std::string& msg);
		std::string get(const std::string&);
		std::string operator [](const std::string&);
	};

	class msg;

	//消息事件基类
	struct EVEMsg : EVE
	{
		//子类型
		int subType;
		//消息ID
		int msgId;
		//来源QQ
		long long fromQQ;
		//消息
		std::string message;
		//字体
		Font font;

		EVEMsg(int subType, int msgId, long long fromQQ, std::string message, int font);

		//真实用户
		bool isUser() const;
		//是否是系统用户
		bool isSystem() const;

		virtual int sendMsg(const char*) const = 0;
		virtual int sendMsg(const std::string&) const = 0;
		virtual msg sendMsg() const = 0;
	};
}
