/*
 * 牌堆抽卡功能
 * 因为发展方向与Dice！插件并不相合，所以更新会止步在某种程度
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#ifndef CARD_DECK
#define CARD_DECK
#include <string>
#include <vector>
#include <map>
#include "STLExtern.hpp"

namespace CardDeck
{
	extern std::map<std::string, std::vector<std::string>, less_ci> mPublicDeck;
	extern std::map<std::string, std::vector<std::string>, less_ci> mExternPublicDeck;
	extern std::map<std::string, std::vector<std::string>, less_ci> mReplyDeck;
	//群聊牌堆
	extern std::map<long long, std::vector<std::string>> mGroupDeck;
	//群聊临时牌堆
	extern std::map<long long, std::vector<std::string>> mGroupDeckTmp;
	//私人牌堆
	extern std::map<long long, std::vector<std::string>> mPrivateDeck;
	//私人临时牌堆
	extern std::map<long long, std::vector<std::string>> mPrivateDeckTmp;
	extern std::map<std::string, std::string> PublicComplexDeck;
	int findDeck(std::string strDeckName);
	std::string drawCard(std::vector<std::string>& TempDeck, bool boolBack = false);
	std::string draw(std::string strDeckName);
};
#endif /*CARD_DECK*/
