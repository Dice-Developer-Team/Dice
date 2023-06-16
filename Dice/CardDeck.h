#pragma once

/*
 * 牌堆抽卡功能
 * 因为发展方向与Dice！插件并不相合，所以更新会止步在某种程度
 * Copyright (C) 2019 String.Empty
 */

#ifndef CARD_DECK
#define CARD_DECK
#include <string>
#include <vector>
#include "STLExtern.hpp"

namespace CardDeck
{
	extern fifo_dict_ci<std::vector<std::string>> mPublicDeck;
	extern fifo_dict_ci<std::vector<std::string>> mExternPublicDeck;
	int findDeck(std::string strDeckName);
	std::string drawCard(std::vector<std::string>& TempDeck, bool boolBack = false);
	std::string drawOne(const std::vector<std::string>& TempDeck);
	std::string draw(std::string strDeckName);
};
#endif /*CARD_DECK*/
