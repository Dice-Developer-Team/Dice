/*
 *Dice! QQ dice robot for TRPG game
 *Copyright (C) 2018 w4123ËÝä§
 *
 *This program is free software: you can redistribute it and/or modify it under the terms
 *of the GNU Affero General Public License as published by the Free Software Foundation,
 *either version 3 of the License, or (at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU Affero General Public License for more details.
 *
 *You should have received a copy of the GNU Affero General Public License along with this
 *program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef _RD_
#define _RD_
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include "RDConstant.h"
#include "CQTools.h"
extern std::map<long long, int> DefaultDice;

//This function is used to generate random integer
inline int Randint(int lowest, int highest)
{
	std::mt19937 gen(static_cast<unsigned int> (GetCycleCount()));
	std::uniform_int_distribution<int> dis(lowest, highest);
	return dis(gen);
}
class RD
{
private:

	int_errno RollDice(std::string dice)
	{
		bool boolNegative = *(vboolNegative.end() - 1);
		if (dice.find('a') != std::string::npos)
		{
			vBnP.push_back(WW_Dice);
			std::string strDiceCnt = dice.substr(0, dice.find("a"));
			for (auto i : strDiceCnt)
				if (!isdigit(i))
					return Input_Err;
			if (strDiceCnt.length() > 3 || (strDiceCnt.length() == 3 && strDiceCnt != "100"))
				return DiceTooBig_Err;
			std::string strAddVal = dice.substr(dice.find("a") + 1);
			for (auto i : strAddVal)
				if (!isdigit(i))
					return Input_Err;
			if (strAddVal.length() > 2)
				return AddDiceVal_Err;
			int intDiceCnt = stoi(strDiceCnt);
			int AddDiceVal = stoi(strAddVal);
			if (intDiceCnt == 0)
				return ZeroDice_Err;
			if (AddDiceVal < 5 || AddDiceVal > 10)
				return AddDiceVal_Err;
			std::vector<int> vintTmpRes;
			int intTmpRes = 0;
			while (intDiceCnt != 0)
			{
				vintTmpRes.push_back(intDiceCnt);
				int AddNum = 0;
				while (intDiceCnt--)
				{
					int intTmpResOnce = Randint(1, 10);
					vintTmpRes.push_back(intTmpResOnce);
					if (intTmpResOnce >= 8)
						intTmpRes++;
					if (intTmpResOnce >= AddDiceVal)
						AddNum++;
				}
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
		else if (dice[dice.length() - 1] == 'F')
		{
			vBnP.push_back(Fudge_Dice);
			std::string strDiceNum;
			if (dice[dice.length() - 2] == 'D')
				strDiceNum = dice.substr(0, dice.length() - 2);
			else
				strDiceNum = dice.substr(0, dice.length() - 1);
			for (auto Element : strDiceNum)
			{
				if (!isdigit(Element))
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
				int intTmpSum = Randint(0, 2) - 1;
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
		else if (dice[0] == 'P')
		{
			vBnP.push_back(P_Dice);
			if (dice.length() > 2)
				return DiceTooBig_Err;
			for (int i = 1; i != dice.length(); i++)
				if (!isdigit(dice[i]))
					return Input_Err;
			int intPNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
			if (dice.length() == 1)
				intPNum = 1;
			if (intPNum == 0)
				return Value_Err;
			std::vector<int> vintTmpRes;
			vintTmpRes.push_back(Randint(1, 100));
			while (intPNum--)
			{
				int intTmpRollRes = Randint(1, 10);
				if (vintTmpRes[0] % 10 == 0)
					vintTmpRes.push_back(intTmpRollRes);
				else
					vintTmpRes.push_back(intTmpRollRes - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (int i = 1; i != vintTmpRes.size(); i++)
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
		else if (dice[0] == 'B')
		{
			vBnP.push_back(B_Dice);
			if (dice.length() > 2)
				return DiceTooBig_Err;
			for (int i = 1; i != dice.length(); i++)
				if (!isdigit(dice[i]))
					return Input_Err;
			int intBNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
			if (dice.length() == 1)
				intBNum = 1;
			if (intBNum == 0)
				return Value_Err;
			std::vector<int> vintTmpRes;
			vintTmpRes.push_back(Randint(1, 100));
			while (intBNum--)
			{
				int intTmpRollRes = Randint(1, 10);
				if (vintTmpRes[0] % 10 == 0)
					vintTmpRes.push_back(intTmpRollRes);
				else
					vintTmpRes.push_back(intTmpRollRes - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (int i = 1; i != vintTmpRes.size(); i++)
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
		bool boolContainD = false;
		bool boolContainK = false;
		for (auto &i : dice)
		{
			i = toupper(i);
			if (!isdigit(i))
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
			int intTmpRes = stoi(dice);
			if (boolNegative)
				intTotal -= intTmpRes;
			else
				intTotal += intTmpRes;
			vintRes.push_back(intTmpRes);
			vvintRes.push_back(std::vector<int> {intTmpRes});
			return 0;
		}
		else if (!boolContainK)
		{
			if (dice.substr(0, dice.find("D")).length() > 3 || (dice.substr(0, dice.find("D")).length() == 3 && dice.substr(0, dice.find("D")) != "100"))
				return DiceTooBig_Err;
			if (dice.substr(dice.find("D") + 1).length() > 4 || (dice.substr(dice.find("D") + 1).length() == 4 && dice.substr(dice.find("D") + 1) != "1000"))
				return TypeTooBig_Err;
			int intDiceCnt = dice.substr(0, dice.find("D")).length() == 0 ? 1 : stoi(dice.substr(0, dice.find("D")));
			int intDiceType = stoi(dice.substr(dice.find("D") + 1));
			if (intDiceCnt == 0)
				return ZeroDice_Err;
			if (intDiceType == 0)
				return ZeroType_Err;
			std::vector<int> vintTmpRes;
			int intTmpRes = 0;
			while (intDiceCnt--)
			{
				int intTmpResOnce = Randint(1, intDiceType);
				vintTmpRes.push_back(intTmpResOnce);
				intTmpRes += intTmpResOnce;
			}
			if (boolNegative)
				intTotal -= intTmpRes;
			else
				intTotal += intTmpRes;
			vvintRes.push_back(vintTmpRes);
			vintRes.push_back(intTmpRes);
			return 0;
		}
		else
		{
			if (dice.substr(dice.find("K") + 1).length() > 3)
				return Value_Err;
			int intKNum = stoi(dice.substr(dice.find("K") + 1));
			dice = dice.substr(0, dice.find("K"));
			if (dice.substr(0, dice.find("D")).length() > 3 || (dice.substr(0, dice.find("D")).length() == 3 && dice.substr(0, dice.find("D")) != "100"))
				return DiceTooBig_Err;
			if (dice.substr(dice.find("D") + 1).length() > 4 || (dice.substr(dice.find("D") + 1).length() == 4 && dice.substr(dice.find("D") + 1) != "1000"))
				return TypeTooBig_Err;
			int intDiceCnt = dice.substr(0, dice.find("D")).length() == 0 ? 1 : stoi(dice.substr(0, dice.find("D")));
			int intDiceType = stoi(dice.substr(dice.find("D") + 1));
			if (intKNum <= 0 || intDiceCnt == 0)
				return ZeroDice_Err;
			if (intKNum > intDiceCnt)
				return Value_Err;
			if (intDiceType == 0)
				return ZeroType_Err;
			std::vector<int> vintTmpRes;
			while (intDiceCnt--)
			{
				int intTmpResOnce = Randint(1, intDiceType);
				if (vintTmpRes.size() != intKNum)
					vintTmpRes.push_back(intTmpResOnce);
				else if (intTmpResOnce > *(std::min_element(vintTmpRes.begin(), vintTmpRes.end())))
					vintTmpRes[std::distance(vintTmpRes.begin(), std::min_element(vintTmpRes.begin(), vintTmpRes.end()))] = intTmpResOnce;
			}
			int intTmpRes = 0;
			for (const auto intElement : vintTmpRes)
				intTmpRes += intElement;
			if (boolNegative)
				intTotal -= intTmpRes;
			else
				intTotal += intTmpRes;
			vintRes.push_back(intTmpRes);
			vvintRes.push_back(vintTmpRes);
			return 0;
		}
	}


public:
	std::string strDice;
	RD(std::string dice) :strDice(dice)
	{
		for (auto &i : strDice)
		{
			if (i != 'a'&&i != 'A')
			{
				i = toupper(i);
			}
			else
			{
				i = tolower(i);
			}
		}
		if (strDice.empty())
			strDice = "D100";
		if (strDice[0] == 'a')strDice.insert(0, "10");
		if (strDice[0] == 'D'&&strDice[1] == 'F')
			strDice.insert(0, "4");
		if (strDice[0] == 'F')
			strDice.insert(0, "4D");
		for (int ReadCnt = 1; ReadCnt != strDice.length(); ReadCnt++)
			if (strDice[ReadCnt] == 'F' && (isdigit(strDice[ReadCnt - 1]) || strDice[ReadCnt - 1] == '+' || strDice[ReadCnt - 1] == '-'))
				strDice.insert(ReadCnt, "D");
		while (strDice.find("+DF") != std::string::npos)
			strDice.insert(strDice.find("+DF") + 1, "4");
		while (strDice.find("-DF") != std::string::npos)
			strDice.insert(strDice.find("-DF") + 1, "4");
		while (strDice.find("D+") != std::string::npos)
			strDice.insert(strDice.find("D+") + 1, "100");
		while (strDice.find("D-") != std::string::npos)
			strDice.insert(strDice.find("D-") + 1, "100");
		while (strDice.find("DK") != std::string::npos)
			strDice.insert(strDice.find("DK") + 1, "100");
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
			strDice.append("100");
		if (*(strDice.end() - 1) == 'K')
			strDice.append("1");
		if (*strDice.begin() == '+')
			strDice.erase(strDice.begin());
		if (strDice[strDice.length() - 1] == 'a')strDice.append("10");
	}
	RD(std::string dice, long long QQNumber) : strDice(dice)
	{
		for (auto &i : strDice)
		{
			if (i != 'a'&&i != 'A')
			{
				i = toupper(i);
			}
			else
			{
				i = tolower(i);
			}
		}
		if (strDice.empty())
			strDice.append("D" + (DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100"));
		if (strDice[0] == 'a')strDice.insert(0, "10");
		if (strDice[0] == 'D'&&strDice[1] == 'F')
			strDice.insert(0, "4");
		if (strDice[0] == 'F')
			strDice.insert(0, "4D");
		for (int ReadCnt = 1; ReadCnt != strDice.length(); ReadCnt++)
			if (strDice[ReadCnt] == 'F' && (isdigit(strDice[ReadCnt - 1]) || strDice[ReadCnt - 1] == '+' || strDice[ReadCnt - 1] == '-'))
				strDice.insert(ReadCnt, "D");
		while (strDice.find("+DF") != std::string::npos)
			strDice.insert(strDice.find("+DF") + 1, "4");
		while (strDice.find("-DF") != std::string::npos)
			strDice.insert(strDice.find("-DF") + 1, "4");
		while (strDice.find("D+") != std::string::npos)
			strDice.insert(strDice.find("D+") + 1, DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100");
		while (strDice.find("D-") != std::string::npos)
			strDice.insert(strDice.find("D-") + 1, DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100");
		while (strDice.find("DK") != std::string::npos)
			strDice.insert(strDice.find("DK") + 1, DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100");
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
			strDice.append(DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100");
		if (*(strDice.end() - 1) == 'K')
			strDice.append("1");
		if (*strDice.begin() == '+')
			strDice.erase(strDice.begin());
		if (strDice[strDice.length() - 1] == 'a')strDice.append("10");
	}
	mutable std::vector<std::vector<int>> vvintRes;
	mutable std::vector<int> vintRes;
	mutable std::vector<bool> vboolNegative;

	//0-Normal, 1-B, 2-P
	mutable std::vector<int> vBnP;
	mutable int intTotal = 0;
	int_errno Roll()
	{
		intTotal = 0;
		vvintRes.clear();
		vboolNegative.clear();
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
		if (dice[dice.length() - 1] == '+' || dice[dice.length() - 1] == '-')
			return Input_Err;
		while (dice.find("+", intReadDiceLoc) != std::string::npos || dice.find("-", intReadDiceLoc) != std::string::npos)
		{
			int intSymbolPosition = dice.find("+", intReadDiceLoc) < dice.find("-", intReadDiceLoc) ? dice.find("+", intReadDiceLoc) : dice.find("-", intReadDiceLoc);
			int intRDRes = RollDice(dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc));
			if (intRDRes != 0)
				return intRDRes;
			intReadDiceLoc = intSymbolPosition + 1;
			if (dice[intSymbolPosition] == '+')
				vboolNegative.push_back(false);
			else
				vboolNegative.push_back(true);
		}
		int intFinalRDRes = RollDice(dice.substr(intReadDiceLoc));
		if (intFinalRDRes != 0)
			return intFinalRDRes;
		return 0;
	}
	std::string FormStringSeparate()
	{
		std::string strReturnString;
		for (auto i = vvintRes.begin(); i != vvintRes.end(); i++)
		{
			strReturnString.append(vboolNegative[distance(vvintRes.begin(), i)] ? "-" : (i == vvintRes.begin() ? "" : "+"));
			if (vBnP[distance(vvintRes.begin(), i)] == Normal_Dice)
			{
				if (i->size() != 1 && (vvintRes.size() != 1 || vboolNegative[distance(vvintRes.begin(), i)]))
					strReturnString.append("(");
				for (auto j = i->begin(); j != i->end(); j++)
				{
					if (j != i->begin())
						strReturnString.append("+");
					strReturnString.append(std::to_string(*j));
				}
				if (i->size() != 1 && (vvintRes.size() != 1 || vboolNegative[distance(vvintRes.begin(), i)]))
					strReturnString.append(")");
			}
			else if (vBnP[distance(vvintRes.begin(), i)] == Fudge_Dice)
			{
				strReturnString.append("[");
				for (auto j = i->begin(); j != i->end(); j++)
				{
					strReturnString.append(*j == 1 ? "+" : *j == 0 ? "0" : "-");
					if (j != i->end() - 1)
						strReturnString.append(" ");
				}
				strReturnString.append("]");
			}
			else if (vBnP[distance(vvintRes.begin(), i)] == WW_Dice)
			{
				if (vvintRes.size() != 1 && i->size() != (*i)[0] + 1)
					strReturnString.append("{ ");
				int intWWPos = 0;
				while (1)
				{
					strReturnString.append("(");
					for (int a = intWWPos + 1; a <= intWWPos + (*i)[intWWPos]; a++)
					{
						strReturnString.append(std::to_string((*i)[a]));
						if (a != intWWPos + (*i)[intWWPos])
							strReturnString.append(",");
					}
					strReturnString.append(")");
					intWWPos = intWWPos + (*i)[intWWPos] + 1;
					if (intWWPos != i->size())
					{
						strReturnString.append("+");
					}
					else
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
				for (auto it = i->begin() + 1; it != i->end(); it++)
				{
					strReturnString.append(std::to_string(*it) + ((it == i->end() - 1) ? "" : " "));
				}
				strReturnString.append("]");
			}
		}
		return strReturnString;
	}
	std::string FormStringCombined()
	{
		std::string strReturnString;
		for (auto i = vintRes.begin(); i != vintRes.end(); i++)
		{
			strReturnString.append(vboolNegative[distance(vintRes.begin(), i)] ? "-" : (i == vintRes.begin() ? "" : "+"));
			if (*i < 0 && i != vintRes.begin())
				strReturnString.append("(");
			strReturnString.append(std::to_string(*i));
			if (*i < 0 && i != vintRes.begin())
				strReturnString.append(")");
		}
		return strReturnString;
	}
	std::string FormCompleteString()
	{
		std::string strReturnString = strDice;
		strReturnString.append("=");
		if(FormStringSeparate().length() > 30)
		{
			return FormShortString();
		}
		strReturnString.append(FormStringSeparate());
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
	std::string FormShortString()
	{
		std::string strReturnString = strDice;
		strReturnString.append("=");
		strReturnString.append(FormStringCombined());
		if (FormStringCombined() != std::to_string(intTotal))
		{
			strReturnString.append("=");
			strReturnString.append(std::to_string(intTotal));
		}
		return strReturnString;
	}
};
extern inline void init(std::string&);
extern inline void init2(std::string&);
extern inline void COC6D(std::string&);
extern inline void COC6(std::string&, int);
extern inline void COC7D(std::string&);
extern inline void COC7(std::string&, int);
extern inline void DND(std::string&, int);
extern inline void LongInsane(std::string&);
extern inline void TempInsane(std::string&);
#endif /*_RD_*/
