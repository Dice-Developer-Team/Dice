/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
 * Copyright (C) 2019 String.Empty
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
#include "RDConstant.h"
#include "RandomGenerator.h"

class RD
{
private:

	int_errno RollDice(std::string dice) const
	{
		const bool boolNegative = *(vboolNegative.end() - 1);
		int intDivider = 1;
		while (dice.find_last_of('/') != std::string::npos)
		{
			const int intXPosition = dice.find_last_of('/');
			std::string strRate = dice.substr(intXPosition + 1);
			RD Rate(strRate);
			if (!Rate.Roll() && Rate.intTotal) intDivider *= Rate.intTotal;
			else return Input_Err;
			dice = dice.substr(0, dice.find_last_of('/'));
		}
		int intMultiplier = 1;
		while (dice.find_last_of('X') != std::string::npos)
		{
			const int intXPosition = dice.find_last_of('X');
			std::string strRate = dice.substr(intXPosition + 1);
			RD Rate(strRate);
			if (Rate.Roll() == 0) intMultiplier *= Rate.intTotal;
			else return Input_Err;
			dice = dice.substr(0, dice.find_last_of('X'));
		}
		vintDivider.push_back(intDivider);
		vintMultiplier.push_back(intMultiplier);
		if (dice.find('a') != std::string::npos)
		{
			vBnP.push_back(WW_Dice);
			std::string strDiceCnt = dice.substr(0, dice.find('a'));
			for (auto i : strDiceCnt)
				if (!isdigit(static_cast<unsigned char>(i)))
					return Input_Err;
			if (strDiceCnt.length() > 3)
				return DiceTooBig_Err;
			std::string strAddVal = dice.substr(dice.find('a') + 1);
			for (auto i : strAddVal)
				if (!isdigit(static_cast<unsigned char>(i)))
					return Input_Err;
			if (strAddVal.length() > 2)
				return AddDiceVal_Err;
			int intDiceCnt = stoi(strDiceCnt);
			const int AddDiceVal = stoi(strAddVal);
			if (intDiceCnt == 0)
				return ZeroDice_Err;
			if (AddDiceVal < 5 || AddDiceVal > 11)
				return AddDiceVal_Err;
			std::vector<int> vintTmpRes;
			int intTmpRes = 0;
			while (intDiceCnt != 0)
			{
				vintTmpRes.push_back(intDiceCnt);
				int AddNum = 0;
				int intCnt = intDiceCnt;
				while (intDiceCnt--)
				{
					int intTmpResOnce = RandomGenerator::Randint(1, 10);
					vintTmpRes.push_back(intTmpResOnce);
					if (intTmpResOnce >= 8)
						intTmpRes++;
					if (intTmpResOnce >= AddDiceVal)
						AddNum++;
				}
				if (intCnt > 10)sort(vintTmpRes.end() - intCnt, vintTmpRes.end());
				intDiceCnt = AddNum;
			}
			if (boolNegative)
				intTotal -= intTmpRes;
			else
				intTotal += intTmpRes;
			vvintRes.push_back(vintTmpRes);
			vintRes.push_back(intTmpRes);
			return 0;
		}
		if (dice[dice.length() - 1] == 'F')
		{
			vBnP.push_back(Fudge_Dice);
			std::string strDiceNum;
			if (dice[dice.length() - 2] == 'D')
				strDiceNum = dice.substr(0, dice.length() - 2);
			else
				strDiceNum = dice.substr(0, dice.length() - 1);
			for (auto Element : strDiceNum)
			{
				if (!isdigit(static_cast<unsigned char>(Element)))
				{
					return Value_Err;
				}
			}
			if (strDiceNum.length() > 2)
				return DiceTooBig_Err;
			int intDiceNum = stoi(strDiceNum);
			if (intDiceNum == 0)
				return ZeroDice_Err;
			std::vector<int> vintTmpRes;
			int intSum = 0;
			while (intDiceNum--)
			{
				int intTmpSum = RandomGenerator::Randint(0, 2) - 1;
				vintTmpRes.push_back(intTmpSum);
				intSum += intTmpSum;
			}
			vvintRes.push_back(vintTmpRes);
			vintRes.push_back(intSum);
			if (boolNegative)
				intTotal -= intSum;
			else
				intTotal += intSum;
			return 0;
		}
		if (dice[0] == 'P')
		{
			vBnP.push_back(P_Dice);
			if (dice.length() > 2)
				return DiceTooBig_Err;
			for (size_t i = 1; i != dice.length(); i++)
				if (!isdigit(static_cast<unsigned char>(dice[i])))
					return Input_Err;
			int intPNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
			if (dice.length() == 1)
				intPNum = 1;
			if (intPNum == 0)
				return Value_Err;
			std::vector<int> vintTmpRes;
			vintTmpRes.push_back(RandomGenerator::Randint(1, 100));
			while (intPNum--)
			{
				int intTmpRollRes = RandomGenerator::Randint(1, 10);
				if (vintTmpRes[0] % 10 == 0)
					vintTmpRes.push_back(intTmpRollRes);
				else
					vintTmpRes.push_back(intTmpRollRes - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (size_t i = 1; i != vintTmpRes.size(); i++)
			{
				if (vintTmpRes[i] > intTmpD100 / 10)
					intTmpD100 = vintTmpRes[i] * 10 + intTmpD100 % 10;
			}
			if (boolNegative)
				intTotal -= intTmpD100;
			else
				intTotal += intTmpD100;
			vintRes.push_back(intTmpD100);
			vvintRes.push_back(vintTmpRes);
			return 0;
		}
		if (dice[0] == 'B')
		{
			vBnP.push_back(B_Dice);
			if (dice.length() > 2)
				return DiceTooBig_Err;
			for (size_t i = 1; i != dice.length(); i++)
				if (!isdigit(static_cast<unsigned char>(dice[i])))
					return Input_Err;
			int intBNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
			if (dice.length() == 1)
				intBNum = 1;
			if (intBNum == 0)
				return Value_Err;
			std::vector<int> vintTmpRes;
			vintTmpRes.push_back(RandomGenerator::Randint(1, 100));
			while (intBNum--)
			{
				int intTmpRollRes = RandomGenerator::Randint(1, 10);
				if (vintTmpRes[0] % 10 == 0)
					vintTmpRes.push_back(intTmpRollRes);
				else
					vintTmpRes.push_back(intTmpRollRes - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (size_t i = 1; i != vintTmpRes.size(); i++)
			{
				if (vintTmpRes[i] < intTmpD100 / 10)
					intTmpD100 = vintTmpRes[i] * 10 + intTmpD100 % 10;
			}
			if (boolNegative)
				intTotal -= intTmpD100;
			else
				intTotal += intTmpD100;
			vintRes.push_back(intTmpD100);
			vvintRes.push_back(vintTmpRes);
			return 0;
		}
		vBnP.push_back(Normal_Dice);
		if (dice[0] == 'X')
		{
			return Input_Err;
		}
		bool boolContainD = false;
		bool boolContainK = false;
		for (auto& i : dice)
		{
			i = toupper(static_cast<unsigned char>(i));
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				if (i == 'D')
				{
					if (boolContainD)
						return Input_Err;
					boolContainD = true;
				}
				else if (i == 'K')
				{
					if (!boolContainD || boolContainK)
						return Input_Err;
					boolContainK = true;
				}
				else
					return Input_Err;
			}
		}

		if (!boolContainD)
		{
			if (dice.length() > 5 || dice.length() == 0)
				return Value_Err;
			const int intTmpRes = stoi(dice);
			if (boolNegative)
				intTotal -= intTmpRes * intMultiplier / intDivider;
			else
				intTotal += intTmpRes * intMultiplier / intDivider;
			vintRes.push_back(intTmpRes * intMultiplier / intDivider);
			vvintRes.push_back(std::vector<int>{intTmpRes});
			return 0;
		}
		if (!boolContainK)
		{
			if (dice.substr(0, dice.find('D')).length() > 3 || (dice.substr(0, dice.find('D')).length() == 3 && dice.
				substr(0, dice.find('D')) != "100"))
				return DiceTooBig_Err;
			if (dice.substr(dice.find('D') + 1).length() > 4 || (dice.substr(dice.find('D') + 1).length() == 4 && dice.
				substr(dice.find('D') + 1) != "1000"))
				return TypeTooBig_Err;
			int intDiceCnt = dice.substr(0, dice.find('D')).length() == 0 ? 1 : stoi(dice.substr(0, dice.find('D')));
			const int intDiceType = stoi(dice.substr(dice.find('D') + 1));
			if (intDiceCnt == 0)
				return ZeroDice_Err;
			if (intDiceType == 0)
				return ZeroType_Err;
			std::vector<int> vintTmpRes;
			int intTmpRes = 0;
			while (intDiceCnt--)
			{
				int intTmpResOnce = RandomGenerator::Randint(1, intDiceType);
				vintTmpRes.push_back(intTmpResOnce);
				intTmpRes += intTmpResOnce;
			}
			if (boolNegative)
				intTotal -= intTmpRes * intMultiplier / intDivider;
			else
				intTotal += intTmpRes * intMultiplier / intDivider;
			if (vintTmpRes.size() > 20)sort(vintTmpRes.begin(), vintTmpRes.end());
			vvintRes.push_back(vintTmpRes);
			vintRes.push_back(intTmpRes * intMultiplier / intDivider);
			return 0;
		}
		if (dice.substr(dice.find('K') + 1).length() > 3)
			return Value_Err;
		const int intKNum = stoi(dice.substr(dice.find('K') + 1));
		dice = dice.substr(0, dice.find('K'));
		if (dice.substr(0, dice.find('D')).length() > 3 || (dice.substr(0, dice.find('D')).length() == 3 && dice.
			substr(0, dice.find('D')) != "100"))
			return DiceTooBig_Err;
		if (dice.substr(dice.find('D') + 1).length() > 4)
			return TypeTooBig_Err;
		int intDiceCnt = dice.substr(0, dice.find('D')).length() == 0 ? 1 : stoi(dice.substr(0, dice.find('D')));
		const int intDiceType = stoi(dice.substr(dice.find('D') + 1));
		if (intKNum <= 0 || intDiceCnt == 0)
			return ZeroDice_Err;
		if (intKNum > intDiceCnt)
			return Value_Err;
		if (intDiceType == 0)
			return ZeroType_Err;
		std::vector<int> vintTmpRes;
		while (intDiceCnt--)
		{
			int intTmpResOnce = RandomGenerator::Randint(1, intDiceType);
			if (vintTmpRes.size() != static_cast<size_t>(intKNum))
				vintTmpRes.push_back(intTmpResOnce);
			else if (intTmpResOnce > *std::min_element(vintTmpRes.begin(), vintTmpRes.end()))
				vintTmpRes[std::distance(vintTmpRes.begin(), std::min_element(vintTmpRes.begin(), vintTmpRes.end()))] =
					intTmpResOnce;
		}
		int intTmpRes = std::accumulate(vintTmpRes.begin(), vintTmpRes.end(), 0);
		if (boolNegative)
			intTotal -= intTmpRes * intMultiplier / intDivider;
		else
			intTotal += intTmpRes * intMultiplier / intDivider;
		vintRes.push_back(intTmpRes);
		if (vintTmpRes.size() > 20)sort(vintTmpRes.begin(), vintTmpRes.end());
		vvintRes.push_back(vintTmpRes);
		return 0;
	}

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

	RD(std::string dice, const int defaultDice = 100)
	{
		char ch = '\0';
		const std::string strOpera("+-X/");
		for (auto& i : dice)
		{
			if (i == '*' || i == 'x' || i == 'X')
			{
				if (strOpera.find(ch) != std::string::npos)
				{
					strDice.erase(--strDice.end());
					break;
				}
				i = 'X';
			}
			if (i == '/')
			{
				if (strOpera.find(ch) != std::string::npos)
				{
					strDice.erase(--strDice.end());
					break;
				}
			}
			if (i == '+')
			{
				if (strOpera.find(ch) != std::string::npos)continue;
			}
			if (i == '-')
			{
				if (strOpera.find(ch) != std::string::npos)
				{
					if (ch == '+')
					{
						strDice[strDice.length() - 1] = '-';
					}
					else if (ch == '-')
					{
						strDice[strDice.length() - 1] = '+';
					}
					else if (ch == 'X' || ch == '/')
					{
						strDice.erase(--strDice.end());
						break;
					}
					continue;
				}
			}
			else if (i == 'a' || i == 'A')
			{
				i = tolower(static_cast<unsigned char>(i));
			}
			else
			{
				i = toupper(static_cast<unsigned char>(i));
			}
			ch = i;
			strDice += i;
		}
		if (strDice.empty())
			strDice.append("D" + (std::to_string(defaultDice)));
		if (strDice[0] == 'a')strDice.insert(0, "10");
		if (strDice[0] == 'D' && strDice[1] == 'F')
			strDice.insert(0, "4");
		if (strDice[0] == 'F')
			strDice.insert(0, "4D");
		while (strDice.find("XX") != std::string::npos) strDice.replace(strDice.find("XX"), 2, "X");
		while (strDice.find("//") != std::string::npos) strDice.replace(strDice.find("//"), 2, "/");
		for (size_t ReadCnt = 1; ReadCnt != strDice.length(); ReadCnt++)
			if (strDice[ReadCnt] == 'F' && (isdigit(strDice[ReadCnt - 1]) || strDice[ReadCnt - 1] == '+' || strDice[
				ReadCnt - 1] == '-'))
				strDice.insert(ReadCnt, "D");
		while (strDice.find("+DF") != std::string::npos)
			strDice.insert(strDice.find("+DF") + 1, "4");
		while (strDice.find("-DF") != std::string::npos)
			strDice.insert(strDice.find("-DF") + 1, "4");
		while (strDice.find("D+") != std::string::npos)
			strDice.insert(strDice.find("D+") + 1,
			               std::to_string(defaultDice));
		while (strDice.find("D-") != std::string::npos)
			strDice.insert(strDice.find("D-") + 1,
			               std::to_string(defaultDice));
		while (strDice.find("DX") != std::string::npos)
			strDice.insert(strDice.find("DX") + 1,
						   std::to_string(defaultDice));
		while (strDice.find("D/") != std::string::npos)
			strDice.insert(strDice.find("D/") + 1,
						   std::to_string(defaultDice));
		while (strDice.find("DK") != std::string::npos)
			strDice.insert(strDice.find("DK") + 1,
			               std::to_string(defaultDice));
		while (strDice.find("K+") != std::string::npos)
			strDice.insert(strDice.find("K+") + 1, "1");
		while (strDice.find("K-") != std::string::npos)
			strDice.insert(strDice.find("K-") + 1, "1");
		while (strDice.find("a-") != std::string::npos)
			strDice.insert(strDice.find("a-") + 1, "10");
		while (strDice.find("a+") != std::string::npos)
			strDice.insert(strDice.find("a+") + 1, "10");
		while (strDice.find("-a") != std::string::npos)
			strDice.insert(strDice.find("-a") + 1, "10");
		while (strDice.find("+a") != std::string::npos)
			strDice.insert(strDice.find("+a") + 1, "10");
		if (*(strDice.end() - 1) == 'D')
			strDice.append(std::to_string(defaultDice));
		if (*(strDice.end() - 1) == 'K')
			strDice.append("1");
		if (*strDice.begin() == '+')
			strDice.erase(strDice.begin());
		if (strDice[strDice.length() - 1] == 'a')strDice.append("10");
	}

	mutable std::vector<std::vector<int>> vvintRes{};
	mutable std::vector<int> vintRes{};
	mutable std::vector<bool> vboolNegative{};
	mutable std::vector<int> vintMultiplier{};
	mutable std::vector<int> vintDivider{};

	//0-Normal, 1-B, 2-P
	mutable std::vector<int> vBnP{};
	mutable int intTotal = 0;

	int_errno Roll() const
	{
		intTotal = 0;
		vvintRes.clear();
		vboolNegative.clear();
		vintMultiplier.clear();
		vintRes.clear();
		vBnP.clear();
		std::string dice = strDice;
		int intReadDiceLoc = 0;
		if (dice[0] == '-')
		{
			vboolNegative.push_back(true);
			intReadDiceLoc = 1;
		}
		else
			vboolNegative.push_back(false);
		if (dice[dice.length() - 1] == '+' || dice[dice.length() - 1] == '-' || dice[dice.length() - 1] == 'X' || dice[
			dice.length() - 1] == '/')
			return Input_Err;
		while (dice.find('+', intReadDiceLoc) != std::string::npos || dice.find('-', intReadDiceLoc) != std::string::
			npos)
		{
			const int intSymbolPosition = dice.find('+', intReadDiceLoc) < dice.find('-', intReadDiceLoc)
				                              ? dice.find('+', intReadDiceLoc)
				                              : dice.find('-', intReadDiceLoc);
			const int intRDRes = RollDice(dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc));
			if (intRDRes != 0)
				return intRDRes;
			intReadDiceLoc = intSymbolPosition + 1;
			if (dice[intSymbolPosition] == '+')
				vboolNegative.push_back(false);
			else
				vboolNegative.push_back(true);
		}
		const int intFinalRDRes = RollDice(dice.substr(intReadDiceLoc));
		if (intFinalRDRes != 0)
			return intFinalRDRes;
		return 0;
	}

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

	std::string FormStringSeparate() const
	{
		std::string strReturnString;
		for (auto i = vvintRes.begin(); i != vvintRes.end(); ++i)
		{
			strReturnString.append(vboolNegative[distance(vvintRes.begin(), i)]
				                       ? "-"
				                       : (i == vvintRes.begin() ? "" : "+"));
			if (vBnP[distance(vvintRes.begin(), i)] == Normal_Dice)
			{
				if (i->size() != 1 && (vvintRes.size() != 1 || vintMultiplier[distance(vvintRes.begin(), i)] != 1 ||
					vintDivider[distance(vvintRes.begin(), i)] != 1 || vboolNegative[distance(vvintRes.begin(), i)]))
					strReturnString.append("(");
				for (auto j = i->begin(); j != i->end(); ++j)
				{
					if (j != i->begin())
						strReturnString.append("+");
					strReturnString.append(std::to_string(*j));
				}
				if (i->size() != 1 && (vvintRes.size() != 1 || vintMultiplier[distance(vvintRes.begin(), i)] != 1 ||
					vintDivider[distance(vvintRes.begin(), i)] != 1 || vboolNegative[distance(vvintRes.begin(), i)]))
					strReturnString.append(")");
				if (vintMultiplier[distance(vvintRes.begin(), i)] != 1)
				{
					strReturnString += "¡Á" + std::to_string(vintMultiplier[distance(vvintRes.begin(), i)]);
				}
				if (vintDivider[distance(vvintRes.begin(), i)] != 1)
				{
					strReturnString += "/" + std::to_string(vintDivider[distance(vvintRes.begin(), i)]);
				}
			}
			else if (vBnP[distance(vvintRes.begin(), i)] == Fudge_Dice)
			{
				strReturnString.append("[");
				for (auto j = i->begin(); j != i->end(); ++j)
				{
					strReturnString.append(*j == 1 ? "+" : *j == 0 ? "0" : "-");
					if (j != i->end() - 1)
						strReturnString.append(" ");
				}
				strReturnString.append("]");
			}
			else if (vBnP[distance(vvintRes.begin(), i)] == WW_Dice)
			{
				bool isCnt = false;
				if (i->size() >= 100)isCnt = true;
				if (vvintRes.size() != 1 && i->size() != (*i)[0] + 1)
					strReturnString.append("{ ");
				int intWWPos = 0;
				while (true)
				{
					if (intWWPos)
					{
						strReturnString += isCnt ? "\n¼Ó÷»" + std::to_string((*i)[intWWPos]) + "£º" : "+";
					}
					strReturnString.append("{");
					if (isCnt)
					{
						int Cnt[11] = {0};
						for (int a = intWWPos + 1; a <= intWWPos + (*i)[intWWPos]; a++)
						{
							Cnt[(*i)[a]]++;
						}
						for (int i2 = 1, c = 0; i2 <= 10; i2++)
						{
							if (Cnt[i2])
							{
								strReturnString.append("(" + std::to_string(i2) + ":" + std::to_string(Cnt[i2]) + "),");
								c++;
								if (c % 3 == 1)strReturnString += '\n';
							}
						}
						if (strReturnString[strReturnString.length() - 1] == '\n')strReturnString.pop_back();
						if (strReturnString[strReturnString.length() - 1] == ',')strReturnString.pop_back();
					}
					else
						for (int a = intWWPos + 1; a <= intWWPos + (*i)[intWWPos]; a++)
						{
							strReturnString.append(std::to_string((*i)[a]));
							if (a != intWWPos + (*i)[intWWPos])
								strReturnString.append(",");
						}
					strReturnString.append("}");
					intWWPos = intWWPos + (*i)[intWWPos] + 1;
					if (intWWPos == i->size())
					{
						break;
					}
				}
				if (vvintRes.size() != 1 && i->size() != (*i)[0] + 1)
					strReturnString.append(" }");
			}
			else
			{
				strReturnString.append(std::to_string((*i)[0]));
				strReturnString.append(vBnP[distance(vvintRes.begin(), i)] == B_Dice ? "[½±Àø÷»:" : "[³Í·£÷»:");
				for (auto it = i->begin() + 1; it != i->end(); ++it)
				{
					strReturnString.append(std::to_string(*it) + ((it == i->end() - 1) ? "" : " "));
				}
				strReturnString.append("]");
			}
		}
		return strReturnString;
	}

	std::string FormStringCombined() const
	{
		std::string strReturnString;
		for (auto i = vintRes.begin(); i != vintRes.end(); ++i)
		{
			strReturnString.append(
				vboolNegative[distance(vintRes.begin(), i)] ? "-" : (i == vintRes.begin() ? "" : "+"));
			if (*i < 0 && i != vintRes.begin())
				strReturnString.append("(");
			strReturnString.append(std::to_string(*i));
			if (*i < 0 && i != vintRes.begin())
				strReturnString.append(")");
		}
		return strReturnString;
	}

	std::string FormCompleteString() const
	{
		std::string strReturnString = strDice;
		if (strDice == FormStringSeparate() && vintRes.size() == 1)
		{
			return strDice;
		}
		if (FormStringSeparate().length() > 256)
		{
			return FormShortString();
		}
		if (strDice != FormStringSeparate())
		{
			strReturnString.append("=");
			strReturnString.append(FormStringSeparate());
		}
		if (FormStringSeparate() != FormStringCombined())
		{
			strReturnString.append("=");
			strReturnString.append(FormStringCombined());
		}
		if (FormStringCombined() != std::to_string(intTotal))
		{
			strReturnString.append("=");
			strReturnString.append(std::to_string(intTotal));
		}
		return strReturnString;
	}

	std::string FormShortString() const
	{
		std::string strReturnString = strDice;
		std::string stringCombined{ FormStringCombined() };
		if (strDice != stringCombined) {
			strReturnString.append("=");
			strReturnString.append(stringCombined);
		}
		if (std::string strTotal{ std::to_string(intTotal) };stringCombined != strTotal)
		{
			strReturnString.append("=");
			strReturnString.append(strTotal);
		}
		return strReturnString;
	}
};

void init(std::string&);
void init2(std::string&);
void COC6D(std::string&);
void COC6(std::string&, int);
void COC7D(std::string&);
void COC7(std::string&, int);
void DND(std::string&, int);
void LongInsane(std::string&);
void TempInsane(std::string&);
int RollSuccessLevel(int, int, int);
#endif /*DICE_RD*/
