/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 杏牙
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

#include "makedocx.h"
#include <C:/Python2786/include/Python.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

vector<string> split(string str, const char* c)
{
	char *cstr, *p;
	vector<string> res;
	cstr = new char[str.size() + 1];
	strcpy(cstr, str.c_str());
	p = strtok(cstr, c);
	while (p != NULL)
	{
		res.push_back(p);
		p = strtok(NULL, c);
	}
	return res;
}

int makedocx::make(std::string str) {
	// 把本函数的代码直接放在dice.cpp中会无法导入Python.h中的所有符号.未知为何出现这种问题。
	Py_Initialize();

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('F:/')");

	PyObject* pModule = NULL;
	PyObject* pList = NULL;
	PyObject* pFunc = NULL;
	PyObject* pFunc2 = NULL;
	PyObject* pArgs = NULL;

	pModule = PyImport_ImportModule("logDocx");
	pFunc = PyObject_GetAttrString(pModule, "gen");


	/*pFunc2 = PyObject_GetAttrString(pModule, "genFromTxt");
	PyObject_CallFunction(pFunc2, "s", "str");
	return 0;*/

	vector<string> strlist = split(str, "\n");
	cout << strlist[0] << endl;
	pArgs = PyTuple_New(1);
	pList = PyList_New(0);
	//PyList_Append(pList, Py_BuildValue("i", 2));
	
	for (auto val : strlist)
	{
		PyList_Append(pList, Py_BuildValue("s", val.data()));
	}

	PyTuple_SetItem(pArgs, 0, pList);
	PyEval_CallObject(pFunc, pArgs);

	Py_Finalize();
	return 0;
}

makedocx::makedocx()
{
}


makedocx::~makedocx()
{
}
