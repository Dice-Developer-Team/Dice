/*
 * ÷»ÄïÍøÂç
 * Copyright (C) 2019 String.Empty
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinInet.h>
#include "DiceCloud.h"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include "CQAPI_EX.h"
#include "DiceNetwork.h"
#include "DiceConsole.h"
#include "DiceMsgSend.h"
using namespace std;
namespace Cloud {
	void update()
	{
		const string strVer = GBKtoUTF8(string(Dice_Ver));
		string data = "DiceQQ=" + to_string(DiceMaid) + "&masterQQ=" + to_string(console.master()) + "&Ver=" + strVer + "&isGlobalOn="+to_string(!console["DisabledGlobal"]) + "&isPublic=" + to_string(!console["Private"]) + "&isVisible=" + to_string(console["CloudVisible"]);
		char *frmdata = new char[data.length() + 1];
		strcpy_s(frmdata, data.length() + 1, data.c_str());
		string temp;
		const bool reqRes = Network::POST("shiki.stringempty.xyz", "/DiceCloud/update.php", 80, frmdata, temp);
		//AddMsgToQueue(temp, masterQQ);
		delete[] frmdata;
		return;
	}
	void upWarning(const char* warning)
	{
		char *frmdata = new char[strlen(warning) + 1];
		strcpy_s(frmdata, strlen(warning) + 1, warning);
		string temp;
		const bool reqRes = Network::POST("shiki.stringempty.xyz", "/DiceCloud/warning_upload.php", 80, frmdata, temp);
		delete[] frmdata;
		return;
	}
	int checkWarning(const char* warning)
	{
		char *frmdata = new char[strlen(warning) + 1];
		strcpy_s(frmdata, strlen(warning) + 1, warning);
		string temp;
		const bool reqRes = Network::POST("shiki.stringempty.xyz", "/DiceCloud/warning_check.php", 80, frmdata, temp);
		delete[] frmdata;
		if (temp == "exist") {
			return 1;
		}
		else if (temp == "erased") {
			return -1;
		}
		return 0;
	}
}