/*
 * 玩家人物卡
 * Copyright (C) 2019-2020 String.Empty
 */
#include "CharacterCard.h"

map<string, CardTemp>& getmCardTemplet()
{
	static map<string, CardTemp> mCardTemplet = {
		{
			"COC7", {
				"COC7", SkillNameReplace, BasicCOC7, InfoCOC7, AutoFillCOC7, mVariableCOC7, ExpressionCOC7,
				SkillDefaultVal, {
					{"", CardBuild({BuildCOC7}, CardDeck::mPublicDeck["随机姓名"], {})},
					{
						"bg", CardBuild({
							                {"性别", "{性别}"}, {"年龄", "7D6+8"}, {"职业", "{调查员职业}"}, {"个人描述", "{个人描述}"},
							                {"重要之人", "{重要之人}"}, {"思想信念", "{思想信念}"}, {"意义非凡之地", "{意义非凡之地}"},
							                {"宝贵之物", "{宝贵之物}"}, {"特质", "{调查员特点}"}
						                }, CardDeck::mPublicDeck["随机姓名"], {})
					}
				}
			}
		},
		{"BRP", {}}
	};
	return mCardTemplet;
}

Player& getPlayer(long long qq)
{
	if (!PList.count(qq))PList[qq] = {};
	return PList[qq];
}

void getPCName(FromMsg& msg)
{
	msg["pc"] = (PList.count(msg.fromQQ) && PList[msg.fromQQ][msg.fromGroup].Name != "角色卡")
		? PList[msg.fromQQ][msg.fromGroup].Name
		: msg["nick"];
}
