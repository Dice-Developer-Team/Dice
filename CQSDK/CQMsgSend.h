#pragma once
#include "bufstream.h"

namespace CQ
{
	enum msgtype { Friend, Group, Discuss };

	class msg final : public CQstream
	{
		long long ID;
		int subType = 0;

	public:
		/*
		Type:
		0=msgtype::好友
		1=msgtype::群
		2=msgtype::讨论组
		*/
		msg(long long GroupID_Or_QQID, msgtype Type);
		/*
		Type:
		0=好友
		1=群
		2=讨论组
		*/
		msg(long long GroupID_Or_QQID, int Type);

		void send() override;
	};
}
