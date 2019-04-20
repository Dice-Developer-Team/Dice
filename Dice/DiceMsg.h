#pragma once
#include <string>
#include "DiceMsgType.h"
namespace Dice
{
	struct DiceMsg
	{
		// 消息内容
		std::string msg;
		// 群ID, 私聊时请设置成QQ号
		long long group_id;
		// QQ号
		long long qq_id;
		// 消息类型
		Dice::MsgType msg_type;


		// 构造回复DiceMsg类
		DiceMsg FormatReplyMsg(std::string reply_msg);

		// 回复消息
		void Reply(std::string reply_msg);

		// 带有内容的构造函数, string可以move的时候记得move
		DiceMsg(std::string msg, long long group_id, long long qq_id, Dice::MsgType msg_type);

		// 默认空构造函数
		DiceMsg() = default;
	};
}


