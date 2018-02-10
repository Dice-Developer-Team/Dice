#pragma once
#include "CQEVE.h"//不能删除此行...
#include "CQEVEBasic.h"

/*
好友事件-好友已添加(Type=201)

subtype 子类型，目前固定为1
msgId 消息ID
fromQQ 来源QQ

本子程序会在酷Q【线程】中被调用，请注意使用对象等需要初始化(CoInitialize,CoUninitialize)
名字如果使用下划线开头需要改成双下划线
返回非零值,消息将被拦截,最高优先不可拦截
*/
#define EVE_Friend_Add_EX(Name) \
	void Name(CQ::EVERequestAddFriend & eve);\
	EVE_Friend_Add(Name)\
	{\
		CQ::EVEFriendAdd tep(subType, msgId, fromGroup, fromQQ, msg, responseFlag);\
		EVETry Name(tep); EVETryEnd(Name,发生了一个错误)										\
		return tep._EVEret;\
	}\
	void Name(CQ::EVEFriendAdd & eve)\

namespace CQ
{
	struct EVEFriendAdd:public EVE{

	};
}

