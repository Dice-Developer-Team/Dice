/*
 *Dice! QQ dice robot for TRPG game
 *Copyright (C) 2018 w4123溯洄
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
#include "CQEVE_ALL.h"
#include "CQTools.h"
#include <cctype>
#include <sstream>
#include "RD.h"
#include "RDConstant.h"
using namespace std;
using namespace CQ;
inline void init(string &msg)
{
	msg_decode(msg);
}

inline void init2(string &msg)
{
	for (int i = 0; i != msg.length(); i++)
	{
		if (msg[i] < 0)
		{
			if ((msg[i] & 0xff) == 0xa1 && (msg[i + 1] & 0xff) == 0xa1)
			{
				msg[i] = 0x20;
				msg.erase(msg.begin() + i + 1);
			}
			else if ((msg[i] & 0xff) == 0xa3 && (msg[i + 1] & 0xff) >= 0xa1 && (msg[i + 1] & 0xff) <= 0xfe)
			{
				msg[i] = msg[i + 1] - 0x80;
				msg.erase(msg.begin() + i + 1);
			}
			else
			{
				i++;
			}
		}
	}

	while (isspace(msg[0]))
		msg.erase(msg.begin());
	while (!msg.empty() && isspace(msg[msg.length() - 1]))
		msg.erase(msg.end() - 1);
	if (msg.substr(0,2) == "。")
	{
		msg.erase(msg.begin());
		msg[0] = '.';
	}
	if (msg[0] == '!')
		msg[0] = '.';
}
inline void COC7D(string &strMAns)
{
	RD rd3D6("3D6");
	RD rd2D6p6("2D6+6");
	strMAns += "的人物作成:";
	strMAns += '\n';
	strMAns += "力量STR=3D6*5=";
	rd3D6.Roll();
	int STR = rd3D6.intTotal * 5;

	strMAns += to_string(STR) + "/" + to_string(STR / 2) + "/" + to_string(STR / 5);
	strMAns += '\n';
	strMAns += "体质CON=3D6*5=";
	rd3D6.Roll();
	int CON = rd3D6.intTotal * 5;

	strMAns += to_string(CON) + "/" + to_string(CON / 2) + "/" + to_string(CON / 5);
	strMAns += '\n';
	strMAns += "体型SIZ=(2D6+6)*5=";
	rd2D6p6.Roll();
	int SIZ = rd2D6p6.intTotal * 5;

	strMAns += to_string(SIZ) + "/" + to_string(SIZ / 2) + "/" + to_string(SIZ / 5);
	strMAns += '\n';
	strMAns += "敏捷DEX=3D6*5=";
	rd3D6.Roll();
	int DEX = rd3D6.intTotal * 5;

	strMAns += to_string(DEX) + "/" + to_string(DEX / 2) + "/" + to_string(DEX / 5);
	strMAns += '\n';
	strMAns += "外貌APP=3D6*5=";
	rd3D6.Roll();
	int APP = rd3D6.intTotal * 5;

	strMAns += to_string(APP) + "/" + to_string(APP / 2) + "/" + to_string(APP / 5);
	strMAns += '\n';
	strMAns += "智力INT=(2D6+6)*5=";
	rd2D6p6.Roll();
	int INT = rd2D6p6.intTotal * 5;

	strMAns += to_string(INT) + "/" + to_string(INT / 2) + "/" + to_string(INT / 5);
	strMAns += '\n';
	strMAns += "意志POW=3D6*5=";
	rd3D6.Roll();
	int POW = rd3D6.intTotal * 5;

	strMAns += to_string(POW) + "/" + to_string(POW / 2) + "/" + to_string(POW / 5);
	strMAns += '\n';
	strMAns += "教育EDU=(2D6+6)*5=";
	rd2D6p6.Roll();
	int EDU = rd2D6p6.intTotal * 5;

	strMAns += to_string(EDU) + "/" + to_string(EDU / 2) + "/" + to_string(EDU / 5);
	strMAns += '\n';
	strMAns += "幸运LUCK=3D6*5=";
	rd3D6.Roll();
	int LUCK = rd3D6.intTotal * 5;
	strMAns += to_string(LUCK);

	strMAns += '\n';
	strMAns += "共计:" + to_string(STR + CON + SIZ + APP + POW + EDU + DEX + INT + LUCK);

	strMAns += "\n理智SAN=POW=";
	int SAN = POW;
	strMAns += to_string(SAN);
	strMAns += "\n生命值HP=(SIZ+CON)/10=";
	int HP = (SIZ + CON) / 10;
	strMAns += to_string(HP);
	strMAns += "\n魔法值MP=POW/5=";
	int MP = POW / 5;
	strMAns += to_string(MP);
	string DB;
	int build = 0;
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
	int MOV = 0;
	if (DEX < SIZ&&STR < SIZ)
		MOV = 7;
	else if (DEX > SIZ && STR > SIZ)
		MOV = 9;
	else
		MOV = 8;
	strMAns += "\n移动力MOV=" + to_string(MOV);
}
inline void COC6D(string &strMAns)
{
	RD rd3D6("3D6");
	RD rd2D6p6("2D6+6");
	RD rd3D6p3("3D6+3");
	strMAns += "的人物作成:";
	strMAns += '\n';
	strMAns += "力量STR=3D6=";
	rd3D6.Roll();
	int STR = rd3D6.intTotal;

	strMAns += to_string(STR);
	strMAns += '\n';
	strMAns += "体质CON=3D6=";
	rd3D6.Roll();
	int CON = rd3D6.intTotal;

	strMAns += to_string(CON);
	strMAns += '\n';
	strMAns += "体型SIZ=2D6+6=";
	rd2D6p6.Roll();
	int SIZ = rd2D6p6.intTotal;

	strMAns += to_string(SIZ);
	strMAns += '\n';
	strMAns += "敏捷DEX=3D6=";
	rd3D6.Roll();
	int DEX = rd3D6.intTotal;

	strMAns += to_string(DEX);
	strMAns += '\n';
	strMAns += "外貌APP=3D6=";
	rd3D6.Roll();
	int APP = rd3D6.intTotal;

	strMAns += to_string(APP);
	strMAns += '\n';
	strMAns += "智力INT=2D6+6=";
	rd2D6p6.Roll();
	int INT = rd2D6p6.intTotal;

	strMAns += to_string(INT);
	strMAns += '\n';
	strMAns += "意志POW=3D6=";
	rd3D6.Roll();
	int POW = rd3D6.intTotal;

	strMAns += to_string(POW);
	strMAns += '\n';
	strMAns += "教育EDU=3D6+3=";
	rd3D6p3.Roll();
	int EDU = rd3D6p3.intTotal;

	strMAns += to_string(EDU);
	strMAns += '\n';
	strMAns += "共计:" + to_string(STR + CON + SIZ + APP + POW + EDU + DEX + INT);
	int SAN = POW * 5;
	int IDEA = INT * 5;
	int LUCK = POW * 5;
	int KNOW = EDU * 5;
	int HP = static_cast<int> (ceil(static_cast<double> (CON + SIZ) / 2.0));
	int MP = POW;
	strMAns += "\n理智SAN=POW*5=" + to_string(SAN) + "\n灵感IDEA=INT*5=" + to_string(IDEA) + "\n幸运LUCK=POW*5=" + to_string(LUCK) + "\n知识KNOW=EDU*5=" + to_string(KNOW);
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
}
inline void COC7(string &strMAns, int intNum)
{
	strMAns += "的人物作成:";
	string strProperty[] = { "力量","体质","体型","敏捷","外貌","智力","意志","教育","幸运" };
	string strRoll[] = { "3D6","3D6","2D6+6","3D6","3D6", "2D6+6","3D6", "2D6+6", "3D6" };
	int intAllTotal = 0;
	while (intNum--)
	{
		strMAns += '\n';
		for (int i = 0; i != 9; i++)
		{
			RD rdCOC(strRoll[i]);
			rdCOC.Roll();
			strMAns += strProperty[i] + ":" + to_string(rdCOC.intTotal * 5) + " ";
			intAllTotal += rdCOC.intTotal * 5;
		}
		strMAns += "共计:" + to_string(intAllTotal);
		intAllTotal = 0;
	}
}
inline void COC6(string &strMAns, int intNum)
{
	strMAns += "的人物作成:";
	string strProperty[] = { "力量","体质","体型","敏捷","外貌","智力","意志","教育" };
	string strRoll[] = { "3D6","3D6","2D6+6","3D6","3D6", "2D6+6","3D6", "3D6+3" };
	bool boolAddSpace = intNum != 1;
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
	}
}
inline void DND(string &strOutput, int intNum)
{
	strOutput += "的英雄作成:";
	RD rdDND("4D6K3");
	string strDNDName[6] = { "力量","体质","敏捷","智力","感知","魅力" };
	bool boolAddSpace = intNum != 1;
	int intAllTotal = 0;
	while (intNum--)
	{
		strOutput += "\n";
		for (int i = 0; i <= 5; i++)
		{
			rdDND.Roll();
			strOutput += strDNDName[i] + ":" + to_string(rdDND.intTotal) + " ";
			if (rdDND.intTotal < 10 && boolAddSpace)
				strOutput += "  ";
			intAllTotal += rdDND.intTotal;
		}
		strOutput += "共计:" + to_string(intAllTotal);
		intAllTotal = 0;
	}
}
inline void TempInsane(string &strAns)
{
	int intSymRes = Randint(1, 10);
	std::string strTI = "1D10=" + to_string(intSymRes) + "\n症状: " + TempInsanity[intSymRes];
	if (intSymRes == 9)
	{
		int intDetailSymRes = Randint(1, 100);
		strTI = format(strTI, { "1D10=" + to_string(Randint(1,10)),"1D100=" + to_string(intDetailSymRes),strFear[intDetailSymRes] });
	}
	else if (intSymRes == 10)
	{
		int intDetailSymRes = Randint(1, 100);
		strTI = format(strTI, { "1D10=" + to_string(Randint(1,10)),"1D100=" + to_string(intDetailSymRes),strPanic[intDetailSymRes] });
	}
	else
	{
		strTI = format(strTI, { "1D10=" + to_string(Randint(1,10)) });
	}
	strAns += strTI;
}

inline void LongInsane(string &strAns)
{
	int intSymRes = Randint(1, 10);
	std::string strLI = "1D10=" + to_string(intSymRes) + "\n症状: " + LongInsanity[intSymRes];
	if (intSymRes == 9)
	{
		int intDetailSymRes = Randint(1, 100);
		strLI = format(strLI, { "1D10=" + to_string(Randint(1,10)),"1D100=" + to_string(intDetailSymRes),strFear[intDetailSymRes] });
	}
	else if (intSymRes == 10)
	{
		int intDetailSymRes = Randint(1, 100);
		strLI = format(strLI, { "1D10=" + to_string(Randint(1,10)),"1D100=" + to_string(intDetailSymRes),strPanic[intDetailSymRes] });
	}
	else
	{
		strLI = format(strLI, { "1D10=" + to_string(Randint(1,10)) });
	}
	strAns += strLI;
}
