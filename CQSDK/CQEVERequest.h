#pragma once
#include <string>
#include "CQEVEBasic.h"
namespace CQ
{
	struct EVERequest :public EVE
	{
		int sendTime; // 发送时间(时间戳)
		long long fromQQ; // 来源QQ
		const char* msg; // 附言
		const char* responseFlag;// 反馈标识(处理请求用)

		EVERequest(int sendTime, long long fromQQ, const char* msg, const char* responseFlag);
		virtual void pass(std::string msg) = 0;//通过此请求
		virtual	void fail(std::string msg) = 0;//拒绝此请求
	};
}