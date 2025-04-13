/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef DICE_RD
#define DICE_RD
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <numeric>
#include <map>
#include <unordered_map>
#include "RDConstant.h"
#include "RandomGenerator.h"
#include "DiceSession.h"

std::string to_circled(int num, int c = 8);
class DiceSession;
class RD
{
private:

	int_errno RollDice(std::string dice, const ptr<DiceSession>& game = {}) const;

	int_errno MaxDice(const std::string& dice) const
	{
		const bool boolNegative = *(vboolNegative.end() - 1);
		int intSum;
		if (dice.find('D') != std::string::npos)
		{
			std::string strDiceCnt = dice.substr(dice.find('D') + 1);
			for (auto& i : strDiceCnt)
			{
				if (!isdigit(static_cast<unsigned char>(i))) return Input_Err;
			}
			if (strDiceCnt.length() > 3) return DiceTooBig_Err;
			const int intDiceCnt = stoi(strDiceCnt);
			strDiceCnt = dice.substr(0, dice.find('D'));
			for (auto& i : strDiceCnt)
			{
				if (!isdigit(static_cast<unsigned char>(i))) return Input_Err;
			}
			if (strDiceCnt.length() > 3) return DiceTooBig_Err;
			intSum = stoi(strDiceCnt.empty() ? "1" : strDiceCnt) * intDiceCnt;
		}
		else
		{
			for (auto i : dice)
			{
				if (!isdigit(static_cast<unsigned char>(i))) return Input_Err;
			}
			if (dice.length() > 3) return DiceTooBig_Err;
			intSum = stoi(dice);
		}
		if (boolNegative)
			intTotal -= intSum;
		else
			intTotal += intSum;
		return 0;
	}

	int_errno MinDice(const std::string& dice) const
	{
		const bool boolNegative = *(vboolNegative.end() - 1);
		int intSum;
		if (dice.find('D') != std::string::npos)
		{
			std::string strDiceCnt = dice.substr(0, dice.find('D'));
			for (auto& i : strDiceCnt)
			{
				if (!isdigit(static_cast<unsigned char>(i))) return Input_Err;
			}
			if (strDiceCnt.length() > 3) return DiceTooBig_Err;
			intSum = stoi(strDiceCnt.empty() ? "1" : strDiceCnt);
		}
		else
		{
			for (auto i : dice)
			{
				if (!isdigit(static_cast<unsigned char>(i))) return Input_Err;
			}
			if (dice.length() > 3) return DiceTooBig_Err;
			intSum = stoi(dice);
		}
		if (boolNegative)
			intTotal -= intSum;
		else
			intTotal += intSum;
		return 0;
	}


public:
	std::string strDice;

	RD(std::string dice, const int defaultDice = 100);

	mutable std::vector<std::vector<int>> vvintRes{};
	mutable std::vector<int> vintRes{};
	mutable std::vector<bool> vboolNegative{};
	mutable std::vector<int> vintMultiplier{};
	mutable std::vector<int> vintDivider{};

	//0-Normal, 1-B, 2-P
	mutable std::vector<int> vBnP{};
	mutable int intTotal = 0;

	int_errno Roll(ptr<DiceSession> game = {}) const;

	int_errno Max() const
	{
		vboolNegative.clear();
		intTotal = 0;
		int intRDRes, intReadDiceLoc = 0;
		std::string strtemp, dice = strDice;
		if (dice[0] == '-')
		{
			vboolNegative.push_back(true);
			intReadDiceLoc = 1;
		}
		else
			vboolNegative.push_back(false);
		if (dice[dice.length() - 1] == '+' || dice[dice.length() - 1] == '-')
			return Input_Err;
		while (dice.find('+', intReadDiceLoc) != std::string::npos || dice.find('-', intReadDiceLoc) != std::string::
			npos)
		{
			const int intSymbolPosition = dice.find('+', intReadDiceLoc) < dice.find('-', intReadDiceLoc)
				                              ? dice.find('+', intReadDiceLoc)
				                              : dice.find('-', intReadDiceLoc);
			strtemp = dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc);
			if (*(vboolNegative.end() - 1)) intRDRes = MinDice(strtemp);
			else intRDRes = MaxDice(strtemp);
			intReadDiceLoc = intSymbolPosition + 1;
			if (dice[intSymbolPosition] == '+') vboolNegative.push_back(false);
			else vboolNegative.push_back(true);
			if (intRDRes != 0) return intRDRes;
		}
		strtemp = dice.substr(intReadDiceLoc);
		if (*(vboolNegative.end() - 1)) intRDRes = MinDice(strtemp);
		else intRDRes = MaxDice(strtemp);
		if (intRDRes != 0) return intRDRes;
		return 0;
	}

	int_errno Min() const
	{
		vboolNegative.clear();
		intTotal = 0;
		int intRDRes, intReadDiceLoc = 0;
		std::string strtemp, dice = strDice;
		if (dice[0] == '-')
		{
			vboolNegative.push_back(true);
			intReadDiceLoc = 1;
		}
		else
			vboolNegative.push_back(false);
		if (dice[dice.length() - 1] == '+' || dice[dice.length() - 1] == '-')
			return Input_Err;
		while (dice.find('+', intReadDiceLoc) != std::string::npos || dice.find('-', intReadDiceLoc) != std::string::
			npos)
		{
			const int intSymbolPosition = dice.find('+', intReadDiceLoc) < dice.find('-', intReadDiceLoc)
				                              ? dice.find('+', intReadDiceLoc)
				                              : dice.find('-', intReadDiceLoc);
			strtemp = dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc);
			if (!*(vboolNegative.end() - 1)) intRDRes = MinDice(strtemp);
			else intRDRes = MaxDice(strtemp);
			intReadDiceLoc = intSymbolPosition + 1;
			if (dice[intSymbolPosition] == '+') vboolNegative.push_back(false);
			else vboolNegative.push_back(true);
			if (intRDRes != 0) return intRDRes;
		}
		strtemp = dice.substr(intReadDiceLoc);
		if (!*(vboolNegative.end() - 1)) intRDRes = MinDice(strtemp);
		else intRDRes = MaxDice(strtemp);
		if (intRDRes != 0) return intRDRes;
		return 0;
	}

	virtual std::string FormStringSeparate() const;

	std::string FormStringCombined() const;

	std::string FormCompleteString() const;

	std::string FormShortString() const;
};

class DicePool: public RD {
	mutable int nDiceCnt{ 0 };
	mutable int nDiceAdd{ 10 };
	int nTarget = 8;
	int_errno err = 0;
	int_errno cntDice(std::string& dice);
public:
	mutable int nSuccess = 0;
	DicePool(const std::string& expr, const int target = 8);
	int_errno roll(ptr<DiceSession> game = {}) const;
	std::string FormStringSeparate() const override;
};

void init(std::string&);
void init2(std::string&);
std::string COC6D();
std::string COC6(int);
std::string COC7D();
std::string COC7(int);
std::string DND(int);
class AnysTable;
void LongInsane(AnysTable&);
void TempInsane(AnysTable&);
int RollSuccessLevel(int, int, int);
#endif /*DICE_RD*/
