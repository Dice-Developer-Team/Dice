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
#include "CQTools.h"
#include <cctype>
#include "RD.h"
using namespace std;

string to_circled(int num, int c) {
	if (num < 1 || num > 10)return "?";
	if (num < c)return to_string(num);
	return string{ char(0xe2),char(0x91),char(0x9f + num) };
}

RD::RD(std::string dice, const int defaultDice)
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
int_errno RD::RollDice(std::string dice, const ptr<DiceSession>& game) const
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
				return DiceCnt_Err;
		int intPNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
		if (dice.length() == 1)
			intPNum = 1;
		if (intPNum == 0)
			return Value_Err;
		std::vector<int> vintTmpRes;
		vintTmpRes.push_back(game ? game->roll(100) : RandomGenerator::Randint(1, 100));
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
				return DiceCnt_Err;
		int intBNum = stoi(dice.substr(1).empty() ? "1" : dice.substr(1));
		if (dice.length() == 1)
			intBNum = 1;
		if (intBNum == 0)
			return Value_Err;
		std::vector<int> vintTmpRes;
		vintTmpRes.push_back(game ? game->roll(100) : RandomGenerator::Randint(1, 100));
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
		if (dice.substr(0, dice.find('D')).length() > 3)
			return DiceTooBig_Err;
		if (dice.substr(dice.find('D') + 1).length() > 4)
			return TypeTooBig_Err;
		int intDiceCnt = dice.substr(0, dice.find('D')).length() == 0 ? 1 : stoi(dice.substr(0, dice.find('D')));
		const int intDiceType = stoi(dice.substr(dice.find('D') + 1));
		if (intDiceCnt == 0)
			return ZeroDice_Err;
		if (intDiceType == 0)
			return ZeroType_Err;
		std::vector<int> vintTmpRes;
		int intTmpRes = 0;
		if (game && intDiceCnt < 10 && game->roulette.count(intDiceType)) {
			auto& rou{ game->roulette[intDiceType] };
			while (intDiceCnt--) {
				int intTmpResOnce = rou.roll();
				vintTmpRes.push_back(intTmpResOnce);
				intTmpRes += intTmpResOnce;
			}
			game->update();
		}
		else while (intDiceCnt--)
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
int_errno RD::Roll(ptr<DiceSession> game) const
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
		dice.length() - 1] == '/') return Input_Err;
	while (dice.find('+', intReadDiceLoc) != std::string::npos
		|| dice.find('-', intReadDiceLoc) != std::string::npos)	{
		const int intSymbolPosition = dice.find('+', intReadDiceLoc) < dice.find('-', intReadDiceLoc)
			? dice.find('+', intReadDiceLoc)
			: dice.find('-', intReadDiceLoc);
		if (const int intRDRes = RollDice(dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc), game); intRDRes != 0)
			return intRDRes;
		intReadDiceLoc = intSymbolPosition + 1;
		if (dice[intSymbolPosition] == '+')
			vboolNegative.push_back(false);
		else
			vboolNegative.push_back(true);
	}
	if (const int intFinalRDRes = RollDice(dice.substr(intReadDiceLoc), game))
		return intFinalRDRes;
	return 0;
}

std::string RD::FormStringSeparate() const
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
				strReturnString += "\\{";
			int intWWPos = 0;
			while (true)
			{
				if (intWWPos)
				{
					strReturnString += isCnt ? "\n加骰" + std::to_string((*i)[intWWPos]) + "：" : "+";
				}
				strReturnString += "\\{";
				if (isCnt)
				{
					int Cnt[11] = { 0 };
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
std::string RD::FormStringCombined() const
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
std::string RD::FormCompleteString() const
{
	std::string strReturnString = strDice;
	string strSeparated{ FormStringSeparate() };
	if (strDice == strSeparated && vintRes.size() == 1)
	{
		return strDice;
	}
	if (strSeparated.length() > 256)
	{
		return FormShortString();
	}
	if (strDice != strSeparated)
	{
		strReturnString.append("=");
		strReturnString.append(strSeparated);
	}
	string strCombined{ FormStringCombined() };
	if (strSeparated != strCombined)
	{
		strReturnString.append("=");
		strReturnString.append(strCombined);
	}
	if (strCombined != std::to_string(intTotal))
	{
		strReturnString.append("=");
		strReturnString.append(std::to_string(intTotal));
	}
	return strReturnString;
}
std::string RD::FormShortString() const
{
	std::string strReturnString = strDice;
	std::string stringCombined{ FormStringCombined() };
	if (strDice != stringCombined) {
		strReturnString.append("=");
		strReturnString.append(stringCombined);
	}
	if (std::string strTotal{ std::to_string(intTotal) }; stringCombined != strTotal)
	{
		strReturnString.append("=");
		strReturnString.append(strTotal);
	}
	return strReturnString;
}

DicePool::DicePool(const std::string& expr, const int target) :RD(expr, 10), nTarget(target) {
	int intReadDiceLoc = 0;
	if (strDice[0] == '-')
	{
		vboolNegative.push_back(true);
		intReadDiceLoc = 1;
	}
	else
		vboolNegative.push_back(false);
	while (strDice.find('+', intReadDiceLoc) != std::string::npos
		|| strDice.find('-', intReadDiceLoc) != std::string::npos) {
		const int intSymbolPosition = strDice.find('+', intReadDiceLoc) < strDice.find('-', intReadDiceLoc)
			? strDice.find('+', intReadDiceLoc)
			: strDice.find('-', intReadDiceLoc);
		if (const int intRDRes = cntDice(strDice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc)); intRDRes != 0) {
			err = intRDRes;
			break;
		}
		intReadDiceLoc = intSymbolPosition + 1;
		if (strDice[intSymbolPosition] == '+')
			vboolNegative.push_back(false);
		else
			vboolNegative.push_back(true);
	}
	if (const int intRDRes = cntDice(strDice.substr(intReadDiceLoc)))
		err = intRDRes;
}
int_errno DicePool::cntDice(std::string& dice) {
	std::string strDiceCnt = dice.substr(0, dice.find('a'));
	for (auto i : strDiceCnt)
		if (!isdigit(static_cast<unsigned char>(i)))
			return DiceCnt_Err;
	if (strDiceCnt.length() > 4)
		return DiceTooBig_Err;
	int intDiceCnt = stoi(strDiceCnt);
	if (intDiceCnt == 0)
		return ZeroDice_Err;
	if (*(vboolNegative.end() - 1))nDiceCnt -= intDiceCnt;
	else nDiceCnt += intDiceCnt;
	//AddVal
	std::string strAddVal = dice.substr(dice.find('a') + 1);
	if (strAddVal.length() > 2)
		return AddDiceVal_Err;
	for (auto i : strAddVal)
		if (!isdigit(static_cast<unsigned char>(i)))
			return Input_Err;
	if (!strAddVal.empty()) {
		nDiceAdd = stoi(strAddVal);
		if (nDiceAdd < 5 || nDiceAdd > 11)
			return AddDiceVal_Err;
	}
	vboolNegative.erase(vboolNegative.end() - 1);
	return 0;
}
int_errno DicePool::roll(ptr<DiceSession> game) const
{
	if (err) return err;
	if (!nDiceCnt)return ZeroDice_Err;
	intTotal = 0;
	while (nDiceCnt != 0){
		std::vector<int> vintTmpRes;
		int intTmpRes = 0;
		vintTmpRes.push_back(nDiceCnt);
		int AddNum = 0;
		int intCnt = nDiceCnt;
		while (intCnt--)
		{
			int intTmpResOnce = RandomGenerator::Randint(1, 10);
			vintTmpRes.push_back(intTmpResOnce);
			if (intTmpResOnce >= nTarget)
				intTmpRes++;
			if (intTmpResOnce >= nDiceAdd)
				AddNum++;
		}
		if (nDiceCnt > 10)sort(vintTmpRes.end() - nDiceCnt, vintTmpRes.end());
		nDiceCnt = AddNum;
		vvintRes.emplace_back(vintTmpRes);
		vintRes.emplace_back(intTmpRes);
		intTotal += intTmpRes;
	}
	//if (boolNegative)
	//	nSuccess -= intTmpRes;
	//else
	//	nSuccess += intTmpRes;
	//vBnP.insert(vBnP.begin(), WW_Dice);
	//vboolNegative.insert(vboolNegative.begin(), boolNegative);
	return 0;
}
std::string DicePool::FormStringSeparate() const
{
	std::string strReturnString;
	unsigned int idx{ 0 };
	for (auto i = vvintRes.begin(); i != vvintRes.end(); ++i)
	{
		if (idx)strReturnString.append("+");
		bool isStatic{ i->size() >= 100 };
		if (vvintRes.size() != 1 && i->size() != (*i)[0] + 1)
			strReturnString += "\\{";
		int intWWPos = 0;
		while (true)
		{
			if (intWWPos)
			{
				strReturnString += isStatic ? "\n加骰" + std::to_string((*i)[intWWPos]) + "：" : "+";
			}
			strReturnString += "\\{";
			if (isStatic)
			{
				int Cnt[11] = { 0 };
				for (int a = intWWPos + 1; a <= intWWPos + (*i)[intWWPos]; a++)
				{
					Cnt[(*i)[a]]++;
				}
				for (int i2 = 1, c = 0; i2 <= 10; i2++)
				{
					if (Cnt[i2])
					{
						strReturnString.append("(" + to_circled(i2, nTarget) + ":" + std::to_string(Cnt[i2]) + "),");
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
					strReturnString.append(to_circled((*i)[a], nTarget));
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
		++idx;
	}
	return strReturnString;
}
void init(string& msg)
{
	msg_decode(msg);
}

void init2(string& msg)
{
	while (isspace(static_cast<unsigned char>(msg[0])))
		msg.erase(msg.begin());
	while (!msg.empty() && isspace(static_cast<unsigned char>(msg[msg.length() - 1])))
		msg.erase(msg.end() - 1);
	if (auto header{ msg.substr(0, 3) }; header == "。" || header == "！" || header == "．")
	{
		msg.replace(0, 3, ".");
	}
	if (msg[0] == '!')
		msg[0] = '.';
	int posFF(0);
	while ((posFF = msg.find('\f'))!= string::npos) {
		msg[posFF] = '\v';
	}
}

string COC7D()
{
	string strMAns;
	RD rd3D6("3D6");
	RD rd2D6p6("2D6+6");
	strMAns += '\n';
	strMAns += "力量STR=3D6*5=";
	rd3D6.Roll();
	const int STR = rd3D6.intTotal * 5;

	strMAns += to_string(STR) + "/" + to_string(STR / 2) + "/" + to_string(STR / 5);
	strMAns += '\n';
	strMAns += "体质CON=3D6*5=";
	rd3D6.Roll();
	const int CON = rd3D6.intTotal * 5;

	strMAns += to_string(CON) + "/" + to_string(CON / 2) + "/" + to_string(CON / 5);
	strMAns += '\n';
	strMAns += "体型SIZ=(2D6+6)*5=";
	rd2D6p6.Roll();
	const int SIZ = rd2D6p6.intTotal * 5;

	strMAns += to_string(SIZ) + "/" + to_string(SIZ / 2) + "/" + to_string(SIZ / 5);
	strMAns += '\n';
	strMAns += "敏捷DEX=3D6*5=";
	rd3D6.Roll();
	const int DEX = rd3D6.intTotal * 5;

	strMAns += to_string(DEX) + "/" + to_string(DEX / 2) + "/" + to_string(DEX / 5);
	strMAns += '\n';
	strMAns += "外貌APP=3D6*5=";
	rd3D6.Roll();
	const int APP = rd3D6.intTotal * 5;

	strMAns += to_string(APP) + "/" + to_string(APP / 2) + "/" + to_string(APP / 5);
	strMAns += '\n';
	strMAns += "智力INT=(2D6+6)*5=";
	rd2D6p6.Roll();
	const int INT = rd2D6p6.intTotal * 5;

	strMAns += to_string(INT) + "/" + to_string(INT / 2) + "/" + to_string(INT / 5);
	strMAns += '\n';
	strMAns += "意志POW=3D6*5=";
	rd3D6.Roll();
	const int POW = rd3D6.intTotal * 5;

	strMAns += to_string(POW) + "/" + to_string(POW / 2) + "/" + to_string(POW / 5);
	strMAns += '\n';
	strMAns += "教育EDU=(2D6+6)*5=";
	rd2D6p6.Roll();
	const int EDU = rd2D6p6.intTotal * 5;

	strMAns += to_string(EDU) + "/" + to_string(EDU / 2) + "/" + to_string(EDU / 5);
	strMAns += '\n';
	strMAns += "幸运LUCK=3D6*5=";
	rd3D6.Roll();
	const int LUCK = rd3D6.intTotal * 5;
	strMAns += to_string(LUCK);

	strMAns += '\n';
	strMAns += "共计:" + to_string(STR + CON + SIZ + APP + POW + EDU + DEX + INT) + "/" + to_string(
		STR + CON + SIZ + APP + POW + EDU + DEX + INT + LUCK);

	strMAns += "\n理智SAN=POW=";
	const int SAN = POW;
	strMAns += to_string(SAN);
	strMAns += "\n生命值HP=(SIZ+CON)/10=";
	const int HP = (SIZ + CON) / 10;
	strMAns += to_string(HP);
	strMAns += "\n魔法值MP=POW/5=";
	const int MP = POW / 5;
	strMAns += to_string(MP);
	string DB;
	int build;
	if (STR + SIZ >= 2 && STR + SIZ <= 64)
	{
		DB = "-2";
		build = -2;
	}
	else if (STR + SIZ >= 65 && STR + SIZ <= 84)
	{
		DB = "-1";
		build = -1;
	}
	else if (STR + SIZ >= 85 && STR + SIZ <= 124)
	{
		DB = "0";
		build = 0;
	}
	else if (STR + SIZ >= 125 && STR + SIZ <= 164)
	{
		DB = "1D4";
		build = 1;
	}
	else if (STR + SIZ >= 165 && STR + SIZ <= 204)
	{
		DB = "1d6";
		build = 2;
	}
	else
	{
		DB = "计算错误!";
		build = -10;
	}
	strMAns += "\n伤害奖励DB=" + DB + "\n体格=" + (build == -10 ? "计算错误" : to_string(build));
	int MOV;
	if (DEX < SIZ && STR < SIZ)
		MOV = 7;
	else if (DEX > SIZ && STR > SIZ)
		MOV = 9;
	else
		MOV = 8;
	strMAns += "\n移动力MOV=" + to_string(MOV);
	return strMAns;
}

string COC6D()
{
	string strMAns;
	RD rd3D6("3D6");
	RD rd2D6p6("2D6+6");
	RD rd3D6p3("3D6+3");
	RD rd1D10("1D10");
	strMAns += '\n';
	strMAns += "力量STR=3D6=";
	rd3D6.Roll();
	const int STR = rd3D6.intTotal;
	strMAns += to_string(STR);
	strMAns += '\n';

	strMAns += "体质CON=3D6=";
	rd3D6.Roll();
	const int CON = rd3D6.intTotal;
	strMAns += to_string(CON);
	strMAns += '\n';

	strMAns += "体型SIZ=2D6+6=";
	rd2D6p6.Roll();
	const int SIZ = rd2D6p6.intTotal;
	strMAns += to_string(SIZ);
	strMAns += '\n';

	strMAns += "敏捷DEX=3D6=";
	rd3D6.Roll();
	const int DEX = rd3D6.intTotal;
	strMAns += to_string(DEX);
	strMAns += '\n';

	strMAns += "外貌APP=3D6=";
	rd3D6.Roll();
	const int APP = rd3D6.intTotal;
	strMAns += to_string(APP);
	strMAns += '\n';

	strMAns += "智力INT=2D6+6=";
	rd2D6p6.Roll();
	const int INT = rd2D6p6.intTotal;
	strMAns += to_string(INT);
	strMAns += '\n';

	strMAns += "意志POW=3D6=";
	rd3D6.Roll();
	const int POW = rd3D6.intTotal;
	strMAns += to_string(POW);
	strMAns += '\n';

	strMAns += "教育EDU=3D6+3=";
	rd3D6p3.Roll();
	const int EDU = rd3D6p3.intTotal;
	strMAns += to_string(EDU);
	strMAns += '\n';
	strMAns += "共计:" + to_string(STR + CON + SIZ + APP + POW + EDU + DEX + INT);
	const int SAN = POW * 5;
	const int IDEA = INT * 5;
	const int LUCK = POW * 5;
	const int KNOW = EDU * 5;
	const int HP = static_cast<int>(ceil((static_cast<double>(CON) + SIZ) / 2.0));
	const int MP = POW;
	strMAns += "\n理智SAN=POW*5=" + to_string(SAN) + "\n灵感IDEA=INT*5=" + to_string(IDEA) + "\n幸运LUCK=POW*5=" +
		to_string(LUCK) + "\n知识KNOW=EDU*5=" + to_string(KNOW);
	strMAns += "\n生命值HP=(CON+SIZ)/2=" + to_string(HP) + "\n魔法值MP=POW=" + to_string(MP);
	strMAns += "\n伤害奖励DB=";
	string DB;
	if (STR + SIZ >= 2 && STR + SIZ <= 12)
	{
		DB = "-1D6";
	}
	else if (STR + SIZ >= 13 && STR + SIZ <= 16)
	{
		DB = "-1D4";
	}
	else if (STR + SIZ >= 17 && STR + SIZ <= 24)
	{
		DB = "0";
	}
	else if (STR + SIZ >= 25 && STR + SIZ <= 32)
	{
		DB = "1D4";
	}
	else if (STR + SIZ >= 33 && STR + SIZ <= 40)
	{
		DB = "1D6";
	}
	else
	{
		DB = "计算错误!";
	}
	strMAns += DB;
	strMAns += "\n";
	strMAns += "资产=1D10=";
	rd1D10.Roll();
	const int PRO = rd1D10.intTotal;
	strMAns += to_string(PRO);
	return strMAns;
}

string COC7(int intNum)
{
	string strMAns;
	string strProperty[] = {"力量", "体质", "体型", "敏捷", "外貌", "智力", "意志", "教育", "幸运"};
	string strRoll[] = {"3D6", "3D6", "2D6+6", "3D6", "3D6", "2D6+6", "3D6", "2D6+6", "3D6"};
	int intAllTotal = 0;
	int intLuc = 0;
	while (intNum--)
	{
		strMAns += '\n';
		for (int i = 0; i != 9; i++)
		{
			RD rdCOC(strRoll[i]);
			rdCOC.Roll();
			strMAns += strProperty[i] + ":" + to_string(rdCOC.intTotal * 5) + " ";
			intAllTotal += rdCOC.intTotal * 5;
			intLuc = rdCOC.intTotal * 5;
		}
		strMAns += "共计:" + to_string(intAllTotal - intLuc) + "/" + to_string(intAllTotal);
		intAllTotal = 0;
	}
	return strMAns;
}

string COC6(int intNum)
{
	string strMAns;
	string strProperty[] = {"力量", "体质", "体型", "敏捷", "外貌", "智力", "意志", "教育", "资产"};
	string strRoll[] = {"3D6", "3D6", "2D6+6", "3D6", "3D6", "2D6+6", "3D6", "3D6+3", "1D10"};
	const bool boolAddSpace = intNum != 1;
	int intAllTotal = 0;
	while (intNum--)
	{
		strMAns += '\n';
		for (int i = 0; i != 8; i++)
		{
			RD rdCOC(strRoll[i]);
			rdCOC.Roll();
			strMAns += strProperty[i] + ":" + to_string(rdCOC.intTotal) + " ";
			if (boolAddSpace && rdCOC.intTotal < 10)
				strMAns += "  ";
			intAllTotal += rdCOC.intTotal;
		}
		strMAns += "共计:" + to_string(intAllTotal);
		intAllTotal = 0;
		RD rdCOC(strRoll[8]);
		rdCOC.Roll();
		strMAns += " " + strProperty[8] + ":" + to_string(rdCOC.intTotal);
	}
	return strMAns;
}

string DND(int intNum)
{
	string strOutput;
	const RD rdDND("4D6K3");
	int intAllTotal = 0;
	std::priority_queue<int> pool;
	while (intNum--){
		strOutput += "\n";
		for (int i = 0; i <= 5; i++){
			rdDND.Roll();
			pool.emplace(rdDND.intTotal);
			intAllTotal += rdDND.intTotal;
		}
		while (!pool.empty()) {
			if (pool.top() < 10)strOutput += ' ';
			strOutput += to_string(pool.top()) + " /";
			pool.pop();
		}
		strOutput += "总计" + to_string(intAllTotal);
		intAllTotal = 0;
	}
	return strOutput;
}

void TempInsane(AnysTable& vars) {
	const int intSymRes = RandomGenerator::Randint(1, 10);
	vars["res"] = "1D10=" + to_string(intSymRes) + "\n症状: " + TempInsanity[intSymRes];
	vars["dur"] = "1D10=" + to_string(RandomGenerator::Randint(1, 10));
	if (intSymRes == 9) {
		const int intDetailSymRes = RandomGenerator::Randint(1, 100);
		vars["detail_roll"] = "1D100=" + to_string(intDetailSymRes);
		vars["detail"] = strFear[intDetailSymRes];
	}
	else if (intSymRes == 10) {
		const int intDetailSymRes = RandomGenerator::Randint(1, 100);
		vars["detail_roll"] = "1D100=" + to_string(intDetailSymRes);
		vars["detail"] = strPanic[intDetailSymRes];
	}
}

void LongInsane(AnysTable& vars) {
	const int intSymRes = RandomGenerator::Randint(1, 10);
	vars["res"] = "1D10=" + to_string(intSymRes) + "\n症状: " + LongInsanity[intSymRes];
	vars["dur"] = "1D10=" + to_string(RandomGenerator::Randint(1, 10));
	if (intSymRes == 9) {
		const int intDetailSymRes = RandomGenerator::Randint(1, 100);
		vars["detail_roll"] = "1D100=" + to_string(intDetailSymRes);
		vars["detail"] = strFear[intDetailSymRes];
	}
	else if (intSymRes == 10) {
		const int intDetailSymRes = RandomGenerator::Randint(1, 100);
		vars["detail_roll"] = "1D100=" + to_string(intDetailSymRes);
		vars["detail"] = strPanic[intDetailSymRes];
	}
}

//成功等级
//0-大失败，1-失败，2-成功，3-困难成功，4-极难成功，5-大成功
int RollSuccessLevel(int res, int rate, int rule)
{
	switch (rule)
	{
	case 0:
		if (res == 100)return 0;
		if (res == 1)return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		if (rate >= 50 || res < 96)return 1;
		return 0;
		break;
	case 1:
		if (res == 100)return 0;
		if (res == 1 || (res <= 5 && rate >= 50))return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		if (rate >= 50 || res < 96)return 1;
		return 0;
		break;
	case 2:
		if (res == 100)return 0;
		if (res <= 5 && res <= rate)return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		if (res < 96)return 1;
		return 0;
		break;
	case 3:
		if (res >= 96)return 0;
		if (res <= 5)return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		return 1;
		break;
	case 4:
		if (res == 100)return 0;
		if (res <= 5 && res <= rate / 10)return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		if (rate >= 50 || res < 96 + rate / 10)return 1;
		return 0;
		break;
	case 5:
		if (res >= 99)return 0;
		if (res <= 2 && res < rate / 10)return 5;
		if (res <= rate / 5)return 4;
		if (res <= rate / 2)return 3;
		if (res <= rate)return 2;
		if (rate >= 50 || res < 96)return 1;
		return 0;
		break;
	case 6:
		if (res > rate) {
			if (res == 100 || res % 11 == 0) {
				return 0;
			}
			return 1;
		} else {
			if (res == 1 || res % 11 == 0) {
				return 5;
			}
			return 2;
		}
		break;
	default: return -1;
	}
}
