#include "l5rDice.h"
#include<string>
#include<sstream>
#include"CQTools.h"
#include"GlobalVar.h"

using namespace std;

const string l5rDice::symbolB[7] = {"","B","O+St","O","S+St","S","ES+St"};
const string l5rDice::symbolW[13] = {"","B","B","O","O","O","S+St","S+St","S","S","S+O","ES+St","ES"};
l5rDice::l5rDice()
{
	
}


l5rDice::~l5rDice()
{

}

std::string l5rDice::roll()
{
	string result;
	auto iter = diceList.begin();
	int i = 1;
	for(;iter!=diceList.end();iter++)
	{
		if (iter->status == dropped) continue;
		if (iter->status == normal)
		{
			if (iter->type == white)
				iter->result = Randint(1, 12);
			else
				iter->result = Randint(1, 6);

			iter->status = rolled;
		}
		
		result += "【" +to_string(i++) +"】";
		if (iter->type == white)
			result += symbolW[iter->result]+"(w)";
		else
			result += symbolB[iter->result]+"(b)";
		
		if (iter->status == additional)
			result += "(add)";
	}
	return result;
}

void l5rDice::insertW(int cnt,bool isAdd)
{
	while (cnt)
	{
		if (isAdd)
			diceList.push_back(l5rD(white, additional,Randint(1,12)));
		else
			diceList.push_back(l5rD(white, normal));
		cnt--;
	}
}

void l5rDice::insertB(int cnt, bool isAdd)
{
	while (cnt)
	{
		if (isAdd)
			diceList.push_back(l5rD(black, additional, Randint(1, 6)));
		else
			diceList.push_back(l5rD(black, normal));
		cnt--;
	}
	
}

void l5rDice::reRoll(int i)
{
	if (i > diceList.size())
		return;

	auto iter = diceList.begin();
	
	if ((iter + i - 1)->type == white)
		(iter + i - 1)->result = Randint(1, 12);
	else
		(iter + i - 1)->result = Randint(1, 6);
}

void l5rDice::drop(int i)
{
	if (i > diceList.size())
		return;

	auto iter = diceList.begin();
		(iter + i - 1)->status = diceStatus::dropped;

}

std::string l5rDice::encodeToDB()
{
	string result;
	auto iter = diceList.begin();
	for (; iter != diceList.end(); iter++)
	{
		if (iter->status == dropped) continue;

		if (iter->type == white)
		{
			result += 'w' + to_string(iter->result);
		}
		else
			result += 'b' + to_string(iter->result);

		if (iter->status == additional)
			result += 'a';
		if (iter->status == rolled)
			result += 'r';

		result += ' ';
	}
	return base64_encode(result);
}

void l5rDice::decode(std::string &read)
{
	read = base64_decode(read);
	string strBuf;
	stringstream ss;
	stringstream buf;
	char charBuf;
	int intBuf;
	ss.str(read);
	while (ss >> strBuf)
	{
		buf.clear();
		buf.str(strBuf);
		if (strBuf.length() == 3)
		{
			buf >> charBuf;
			buf >> intBuf;
			if (strBuf[2] == 'a')
			{
				if (charBuf == 'w')
				{
					diceList.push_back(l5rD(white, additional, intBuf));
				}
				else
				{
					diceList.push_back(l5rD(black, additional, intBuf));
				}
			}
			else if (strBuf[2] == 'r')
			{
				if (charBuf == 'w')
				{
					diceList.push_back(l5rD(white, rolled, intBuf));
				}
				else
				{
					diceList.push_back(l5rD(black, rolled, intBuf));
				}
			}
		}
		else
		{
			buf >> charBuf;
			buf >> intBuf;
			if (charBuf == 'w')
			{
				diceList.push_back(l5rD(white, normal, intBuf));
			}
			else
			{
				diceList.push_back(l5rD(black, normal, intBuf));
			}
		}
	}
}


int l5rDice::getDiceCnt()
{
	return diceList.size();
}

std::string l5rDice::getResult()
{
	string reply; 
	int i = 1;
	auto iter= diceList.begin();
	for (; iter != diceList.end(); iter++)
	{
		if (iter->status == dropped) continue;

		reply += "【" + to_string(i++) + "】";
		if (iter->type == white)
			reply += symbolW[iter->result] + "(w)";
		else
			reply += symbolB[iter->result] + "(b)";

		if (iter->status == additional)
			reply += "(add)";
	}
	return reply;
}
