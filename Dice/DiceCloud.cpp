/*
 * 骰娘网络
 * Copyright (C) 2019-2024 String.Empty
 */
#include <cstring>
#include "filesystem.hpp"
#include "StrExtern.hpp"
#include "DiceCloud.h"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include "DiceNetwork.h"
#include "DiceConsole.h"
#include "DiceMsgSend.h"
#include "DiceEvent.h"
#include "DDAPI.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinInet.h>
#include <urlmon.h>
#include <io.h>
#pragma comment(lib, "urlmon.lib")
#endif

using namespace std;

namespace Cloud
{
	void heartbeat()
	{
		static const string strVer = Dice_Ver;
		const string data = "&masterID=" + to_string(console) + "&Ver=" +
			strVer + "&isGlobalOn=" + to_string(!console["DisabledGlobal"]) + "&isPublic=" +
			to_string(!console["Private"]) + "&isVisible=" + to_string(console["CloudVisible"]);
		DD::heartbeat(data);
	}
	
	int DownloadFile(const char* url, const std::filesystem::path& downloadPath)
	{
#ifdef _WIN32
		DeleteUrlCacheEntryA(url);
		if (URLDownloadToFileW(nullptr, reinterpret_cast<const wchar_t*>(convert_a2w(url).c_str()), reinterpret_cast<const wchar_t*>(downloadPath.u16string().c_str()), 0, nullptr) != S_OK) return -1;
		std::error_code ec;
		if (!std::filesystem::exists(downloadPath, ec) || ec)return -2;
		return 0;
#else
		return -1;
#endif
	}

	int checkUpdate(DiceEvent* msg)
	{
		std::string strVerInfo;
		if (!Network::GET("http://shiki.stringempty.xyz/DiceVer/update", strVerInfo))
		{
			msg->reply("{self}获取版本信息时遇到错误: \n" + strVerInfo);
			return -1;
		}
		try
		{
			fifo_json jInfo(fifo_json::parse(strVerInfo, nullptr, false));
			unsigned short nBuild = jInfo["release"]["build"];
			if (nBuild > Dice_Build)
			{
				msg->note(
					"发现Dice!的发布版更新:" + jInfo["release"]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n更新说明：" +
					jInfo["release"]["changelog"].get<string>(), 1);
				return 1;
			}
			if (nBuild = jInfo["dev"]["build"]; nBuild > Dice_Build)
			{
				msg->note(
					"发现Dice!的开发版更新:" + jInfo["dev"]["ver"].get<string>() + "(" + to_string(nBuild) + ")\n更新说明：" +
					jInfo["dev"]["changelog"].get<string>(), 1);
				return 2;
			}
			msg->reply("未发现版本更新。");
			return 0;
		}
		catch (std::exception& e){
			msg->set("err", e.what());
			msg->reply("{self}获取更新失败！{err}");
			return -2;
		}
		return 0;
	}
}
