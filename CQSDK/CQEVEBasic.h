#pragma once

#include "CQconstant.h"

namespace CQ
{
	// 事件基类
	struct EVE
	{
		//不对消息做任何动作
		//如果之前拦截了消息,这里将重新放行本条消息
		void message_ignore() noexcept;
		//拦截本条消息
		void message_block() noexcept;

		int _EVEret = Msg_Ignored;

		virtual ~EVE()
		{
		}
	};


	inline void EVE::message_ignore() noexcept { _EVEret = Msg_Ignored; }
	inline void EVE::message_block() noexcept { _EVEret = Msg_Blocked; }

}
