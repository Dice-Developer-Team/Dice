#pragma once
#include <vector>
#include <string>
#include <random>
#include "CharacterCard.h"

class DiceL5R
{
private:
	enum diceType { white, black };
	enum diceStatus { normal, additional, dropped, kept, rolled };
	typedef struct l5rD
	{
		diceType type;
		diceStatus status;
		int result = 0;
		l5rD(diceType a, diceStatus b) :type(a), status(b) {};
		l5rD(diceType a, diceStatus b, int r) :type(a), status(b), result(r) {};
	}l5rD;
	using l5rDList = std::vector<l5rD>;
	const static std::string symbolB[7];
	const static std::string symbolW[13];
private:
	l5rDList  diceList;
	int Randint(int lowest, int highest)
	{
		std::mt19937 gen(static_cast<unsigned int>(RandomGenerator::GetCycleCount()));
		std::uniform_int_distribution<int> dis(lowest, highest);
		return dis(gen);
	}
public:
	std::string roll();
	void insertW(int, bool isAdd = false);
	void insertB(int, bool isAdd = false);
	void reRoll(int);
	void drop(int);
	std::string encodeToDB();
	void decode(std::string&);
	int getDiceCnt();
	std::string getResult();

};
