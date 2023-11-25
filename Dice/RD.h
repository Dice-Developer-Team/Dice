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
 * Copyright (C) 2019-2023 String.Empty
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

	mutable int intWWCnt{ 0 };
	mutable int intWWAdd{ 10 };
	int_errno RollWW() const
	{
		std::vector<int> vintTmpRes;
		int intTmpRes = 0;
		bool boolNegative{ false };
		if (intWWCnt < 0) {
			boolNegative = true;
			intWWCnt = -intWWCnt;
		}
		while (intWWCnt != 0)
		{
			vintTmpRes.push_back(intWWCnt);
			int AddNum = 0;
			int intCnt = intWWCnt;
			while (intCnt--)
			{
				int intTmpResOnce = RandomGenerator::Randint(1, 10);
				vintTmpRes.push_back(intTmpResOnce);
				if (intTmpResOnce >= 8)
					intTmpRes++;
				if (intTmpResOnce >= intWWAdd)
					AddNum++;
			}
			if (intWWCnt > 10)sort(vintTmpRes.end() - intWWCnt, vintTmpRes.end());
			intWWCnt = AddNum;
		}
		if (boolNegative)
			intTotal -= intTmpRes;
		else
			intTotal += intTmpRes;
		vBnP.insert(vBnP.begin(), WW_Dice);
		vboolNegative.insert(vboolNegative.begin(), boolNegative);
		vvintRes.insert(vvintRes.begin(),vintTmpRes);
		vintRes.insert(vintRes.begin(), intTmpRes);
		vintMultiplier.insert(vintMultiplier.begin(), 1);
		vintDivider.insert(vintDivider.begin(), 1);
		return 0;
	}

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
		if (strDice[0] == 'a')strDice.insert(0, "1");
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
		while (strDice.find("-a") != std::string::npos)
			strDice.insert(strDice.find("-a") + 1, "1");
		while (strDice.find("+a") != std::string::npos)
			strDice.insert(strDice.find("+a") + 1, "1");
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
		if (*(strDice.end() - 1) == 'D')
			strDice.append(std::to_string(defaultDice));
		if (*(strDice.end() - 1) == 'K')
			strDice.append("1");
		if (*strDice.begin() == '+')
			strDice.erase(strDice.begin());
	}

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

	std::string FormStringSeparate() const
	{
		std::string strReturnString;
		unsigned int idx{ 0 };
		for (auto i = vvintRes.begin(); i != vvintRes.end(); ++i)
		{
			strReturnString.append(vboolNegative[idx]
				                       ? "-"
				                       : (idx ? "+" : ""));
			if (vBnP[idx] == Normal_Dice)
			{
				if (i->size() != 1 && (vvintRes.size() != 1 || vintMultiplier[idx] != 1 ||
					vintDivider[idx] != 1 || vboolNegative[idx]))
					strReturnString.append("(");
				for (auto j = i->begin(); j != i->end(); ++j)
				{
					if (j != i->begin())
						strReturnString.append("+");
					strReturnString.append(std::to_string(*j));
				}
				if (i->size() != 1 && (vvintRes.size() != 1 || vintMultiplier[idx] != 1 ||
					vintDivider[idx] != 1 || vboolNegative[idx]))
					strReturnString.append(")");
				if (vintMultiplier[idx] != 1)
				{
					strReturnString += "×" + std::to_string(vintMultiplier[idx]);
				}
				if (vintDivider[idx] != 1)
				{
					strReturnString += "/" + std::to_string(vintDivider[idx]);
				}
			}
			else if (vBnP[idx] == Fudge_Dice)
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
			else if (vBnP[idx] == WW_Dice)
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
						strReturnString += isCnt ? "\n加骰" + std::to_string((*i)[intWWPos]) + "：" : "+";
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
								strReturnString.append("(" + to_circled(i2) + ":" + std::to_string(Cnt[i2]) + "),");
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
							strReturnString.append(to_circled((*i)[a]));
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
				strReturnString.append(vBnP[idx] == B_Dice ? "[奖励骰:" : "[惩罚骰:");
				for (auto it = i->begin() + 1; it != i->end(); ++it)
				{
					strReturnString.append(std::to_string(*it) + ((it == i->end() - 1) ? "" : " "));
				}
				strReturnString.append("]");
			}
			++idx;
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
std::string COC6D();
std::string COC6(int);
std::string COC7D();
std::string COC7(int);
std::string DND(int);
class AttrObject;
void LongInsane(AttrObject&);
void TempInsane(AttrObject&);
int RollSuccessLevel(int, int, int);
#endif /*DICE_RD*/
