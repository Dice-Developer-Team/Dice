#pragma once

namespace Dice
{
	/*
     * 消息类型枚举类
     * 用于发送消息
     */
	enum class MsgType : int
	{
		Private = 0,
		Group = 1,
		Discuss = 2
	};
}
