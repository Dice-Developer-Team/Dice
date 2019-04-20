#include <string>
#include "DiceMsgType.h"
#include "DiceMsgSend.h"
#include "DiceMsg.h"


namespace Dice
{
	DiceMsg::DiceMsg(std::string msg, long long group_id, long long qq_id, Dice::MsgType msg_type) :
		msg(std::move(msg)), group_id(group_id), qq_id(qq_id), msg_type(std::move(msg_type))
	{
	}
	DiceMsg DiceMsg::FormatReplyMsg(std::string reply_msg)
	{
		return DiceMsg(std::move(reply_msg), group_id, qq_id, msg_type);
	}
	void DiceMsg::Reply(std::string reply_msg)
	{
		AddMsgToQueue(FormatReplyMsg(std::move(reply_msg)));
	}
}


