#pragma once
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include "RDConstant.h"
extern std::map<long long, int> DefaultDice;
//This funtion template is used to convert a type into another type
//Param:origin->Original Data
//Usage:Convert<Converted Type> (Original Data)

template<typename typeTo, typename typeFrom>
typeTo Convert(typeFrom origin) {
	std::stringstream ConvertStream;
	typeTo converted;
	ConvertStream << origin;
	ConvertStream >> converted;
	return converted;
}
class RD {
private:
	//This function is used to get the CPU Cycle Count, which is used as the seed of the random function
	//Notice: The funtion uses something not in the C++ standard, so please do not use "Microsoft's All Rules" when compiling, use "Microsoft's Recommand Rules" instead
	inline unsigned long long GetCycleCount()
	{
		__asm _emit 0x0F
		__asm _emit 0x31
	}

	//This function is used to generate random integer
	int Randint(int lowest, int highest) {
		std::mt19937 gen(static_cast<unsigned int> (GetCycleCount()));
		std::uniform_int_distribution<int> dis(lowest, highest);
		return dis(gen);
	}
	int_errno RollDice(std::string dice) {
		bool boolNegative = *(vboolNegative.end() - 1);
		if (dice[0] == 'P') {
			vBnP.push_back(P_Dice);
			if (dice.length() > 4)return DiceTooBig_Err;
			for (int i = 1; i != dice.length(); i++)
				if (!isdigit(dice[i]))return Input_Err;
			int intPNum = Convert<int>(dice.substr(1));
			if (dice.length() == 1)intPNum = 1;
			if (intPNum == 0)return Value_Err;
			std::vector<int> vintTmpRes;
			RD rdD100("D100");
			rdD100.Roll();
			vintTmpRes.push_back(rdD100.intTotal);
			RD rdD10("D10");
			while (intPNum--) {
				rdD10.Roll();
				if (vintTmpRes[0] % 10 == 0)vintTmpRes.push_back(rdD10.intTotal);
				else vintTmpRes.push_back(rdD10.intTotal - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (int i = 1; i != vintTmpRes.size(); i++) {
				if (vintTmpRes[i] > intTmpD100 / 10)intTmpD100 = vintTmpRes[i] * 10 + intTmpD100 % 10;
			}
			if (boolNegative)intTotal -= intTmpD100; else intTotal += intTmpD100;
			vintRes.push_back(intTmpD100);
			vvintRes.push_back(vintTmpRes);
			return 0;
		}
		else if (dice[0] == 'B') {
			vBnP.push_back(B_Dice);
			if (dice.length() > 4)return DiceTooBig_Err;
			for (int i = 1; i != dice.length(); i++)
				if (!isdigit(dice[i]))return Input_Err;
			int intBNum = Convert<int>(dice.substr(1));
			if (dice.length() == 1)intBNum = 1;
			if (intBNum == 0)return Value_Err;
			std::vector<int> vintTmpRes;
			RD rdD100("D100");
			rdD100.Roll();
			vintTmpRes.push_back(rdD100.intTotal);
			RD rdD10("D10");
			while (intBNum--) {
				rdD10.Roll();
				if (vintTmpRes[0] % 10 == 0)vintTmpRes.push_back(rdD10.intTotal);
				else vintTmpRes.push_back(rdD10.intTotal - 1);
			}
			int intTmpD100 = vintTmpRes[0];
			for (int i = 1; i != vintTmpRes.size(); i++) {
				if (vintTmpRes[i] < intTmpD100 / 10)intTmpD100 = vintTmpRes[i] * 10 + intTmpD100 % 10;
			}
			if (boolNegative)intTotal -= intTmpD100; else intTotal += intTmpD100;
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
			if (!isdigit(i)) {
				if (i == 'D') {
					if (boolContainD)return Input_Err;
					boolContainD = true;
				}
				else if (i == 'K') {
					if (!boolContainD || boolContainK)return Input_Err;
					boolContainK = true;
				}
				else return Input_Err;
			}
		}

		if (!boolContainD) {
			if (dice.length() > 5 || dice.length() == 0)
				return Value_Err;
			int intTmpRes = Convert<int>(dice);
			if (boolNegative) intTotal -= intTmpRes;
			else intTotal += intTmpRes;
			vintRes.push_back(intTmpRes);
			vvintRes.push_back(std::vector<int>{intTmpRes});
			return 0;
		}
		else if (!boolContainK) {
			if (dice.substr(0, dice.find("D")).length() > 3 )
				return DiceTooBig_Err;
			if (dice.substr(dice.find("D") + 1).length() > 5)return TypeTooBig_Err;
			int intDiceCnt = dice.substr(0, dice.find("D")).length() == 0 ? 1 : Convert<int>(dice.substr(0, dice.find("D")));
			int intDiceType = Convert<int>(dice.substr(dice.find("D") + 1));
			if (intDiceCnt == 0 )return ZeroDice_Err;
			if (intDiceType == 0)return ZeroType_Err;
			std::vector<int> vintTmpRes;
			int intTmpRes = 0;
			while (intDiceCnt--) {
				int intTmpResOnce = Randint(1, intDiceType);
				vintTmpRes.push_back(intTmpResOnce);
				intTmpRes += intTmpResOnce;
			}
			if (boolNegative) intTotal -= intTmpRes;
			else intTotal += intTmpRes;
			vvintRes.push_back(vintTmpRes);
			vintRes.push_back(intTmpRes);
			return 0;
		}
		else {
			if (dice.substr(dice.find("K") + 1).length() > 3)return Value_Err;
			int intKNum = Convert<int>(dice.substr(dice.find("K") + 1));
			dice = dice.substr(0, dice.find("K"));
			if (dice.substr(0, dice.find("D")).length() > 3 || dice.substr(dice.find("D") + 1).length() > 5)return Value_Err;
			int intDiceCnt = dice.substr(0, dice.find("D")).length() == 0 ? 1 : Convert<int>(dice.substr(0, dice.find("D")));
			int intDiceType = Convert<int>(dice.substr(dice.find("D") + 1));
			if (intKNum <= 0  || intDiceCnt == 0)return ZeroDice_Err;
			if(intKNum > intDiceCnt)return Value_Err;
			if (intDiceType == 0)return ZeroType_Err;
			std::vector<int> vintTmpRes;
			while (intDiceCnt--) {
				int intTmpResOnce = Randint(1, intDiceType);
				if (vintTmpRes.size() != intKNum)vintTmpRes.push_back(intTmpResOnce);
				else if (intTmpResOnce > *(std::min_element(vintTmpRes.begin(), vintTmpRes.end())))
					vintTmpRes.at(std::distance(vintTmpRes.begin(), std::min_element(vintTmpRes.begin(), vintTmpRes.end()))) = intTmpResOnce;
			}
			int intTmpRes = 0;
			for (const auto intElement : vintTmpRes)intTmpRes += intElement;
			if (boolNegative) intTotal -= intTmpRes;
			else intTotal += intTmpRes;
			vintRes.push_back(intTmpRes);
			vvintRes.push_back(vintTmpRes);
			return 0;
		}
	}


public:
	std::string strDice;
	RD(std::string dice) :strDice(dice) {
		for (auto &i : strDice)i = toupper(i);
		if (strDice.empty())strDice = "D100";
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
		if (*(strDice.end() - 1) == 'D')strDice.append("100");
		if (*(strDice.end() - 1) == 'K')strDice.append("1");
		if (*strDice.begin() == '+')strDice.erase(strDice.begin());
	}
	RD(std::string dice,long long QQNumber) : strDice(dice) {
		for (auto &i : strDice)i = toupper(i);
		if (strDice.empty()) 
			strDice.append("D" + (DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100"));
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
		if (*(strDice.end() - 1) == 'D')strDice.append(DefaultDice.count(QQNumber) ? std::to_string(DefaultDice[QQNumber]) : "100");
		if (*(strDice.end() - 1) == 'K')strDice.append("1");
		if (*strDice.begin() == '+')strDice.erase(strDice.begin());
	}
	mutable std::vector<std::vector<int>> vvintRes;
	mutable std::vector<int> vintRes;
	mutable std::vector<bool> vboolNegative;

	//0-Normal, 1-B, 2-P
	mutable std::vector<int> vBnP;
	mutable int intTotal = 0;
	int_errno Roll() {
		intTotal = 0;
		vvintRes.clear();
		vboolNegative.clear();
		vintRes.clear();
		vBnP.clear();
		std::string dice = strDice;
		int intReadDiceLoc = 0;
		if (dice[0] == '-') {
			vboolNegative.push_back(true);
			intReadDiceLoc = 1;
		}
		else vboolNegative.push_back(false);
		if (dice[dice.length() - 1] == '+' || dice[dice.length() - 1] == '-')return Input_Err;
		while (dice.find("+", intReadDiceLoc) != std::string::npos || dice.find("-", intReadDiceLoc) != std::string::npos) {
			int intSymbolPosition = dice.find("+", intReadDiceLoc) < dice.find("-", intReadDiceLoc) ? dice.find("+", intReadDiceLoc) : dice.find("-", intReadDiceLoc);
			int intRDRes = RollDice(dice.substr(intReadDiceLoc, intSymbolPosition - intReadDiceLoc));
			if (intRDRes != 0)return intRDRes;
			intReadDiceLoc = intSymbolPosition + 1;
			if (dice[intSymbolPosition] == '+')vboolNegative.push_back(false); else vboolNegative.push_back(true);
		}
		int intFinalRDRes = RollDice(dice.substr(intReadDiceLoc));
		if (intFinalRDRes != 0)return intFinalRDRes;
		return 0;
	}
	std::string FormStringSeparate() {
		std::string strReturnString;
		for (auto i = vvintRes.begin(); i != vvintRes.end(); i++) {
			strReturnString.append(vboolNegative.at(distance(vvintRes.begin(), i)) ? "-" : (i == vvintRes.begin() ? "" : "+"));
			if (vBnP[distance(vvintRes.begin(), i)] == Normal_Dice) {
				if (i->size() != 1 && vvintRes.size() != 1)strReturnString.append("(");
				for (auto j = i->begin(); j != i->end(); j++) {
					if (j != i->begin())strReturnString.append("+");
					strReturnString.append(std::to_string(*j));
				}
				if (i->size() != 1 && vvintRes.size() != 1)strReturnString.append(")");
			}
			else {
				strReturnString.append(std::to_string((*i)[0]));
				strReturnString.append(vBnP[distance(vvintRes.begin(), i)] == B_Dice ? "[½±Àø÷»:" : "[³Í·£÷»:");
				for (auto it = i->begin() + 1; it != i->end(); it++) {
					strReturnString.append(std::to_string(*it) + ((it == i->end() - 1) ? "" : " "));
				}
				strReturnString.append("]");
			}
		}
		return strReturnString;
	}
	std::string FormStringCombined() {
		std::string strReturnString;
		for (auto i = vintRes.begin(); i != vintRes.end(); i++) {
			strReturnString.append(vboolNegative.at(distance(vintRes.begin(), i)) ? "-" : (i == vintRes.begin() ? "" : "+"));
			strReturnString.append(std::to_string(*i));
		}
		return strReturnString;
	}
	std::string FormCompleteString() {
		std::string strReturnString = strDice;
		strReturnString.append("=");
		strReturnString.append(FormStringSeparate());
		if (FormStringSeparate() != FormStringCombined()) {
			strReturnString.append("=");
			strReturnString.append(FormStringCombined());
		}
		if (FormStringCombined() != std::to_string(intTotal)) {
			strReturnString.append("=");
			strReturnString.append(std::to_string(intTotal));
		}
		return strReturnString;
	}
};
extern inline void init(std::string&);
extern inline void COC6D(std::string&);
extern inline void COC6(std::string&, int);
extern inline void COC7D(std::string&);
extern inline void COC7(std::string&, int);
extern inline void DND(std::string&, int);
extern inline void LongInsane(std::string&);
extern inline void TempInsane(std::string&);