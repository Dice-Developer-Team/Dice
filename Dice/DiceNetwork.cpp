/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
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
#include <Windows.h>
#include <WinInet.h>
#include <string>
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "DiceNetwork.h"
#pragma comment(lib, "WinInet.lib")

namespace Network
{	
	std::string getLastErrorMsg()
	{
		DWORD dwError = GetLastError();
		if (dwError == ERROR_INTERNET_EXTENDED_ERROR)
		{
			DWORD size = 512;
			char *szFormatBuffer = new char[size];
			if (InternetGetLastResponseInfoA(&dwError, szFormatBuffer, &size))
			{
				std::string ret(szFormatBuffer);
				while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
				delete[] szFormatBuffer;
				return ret;
			}
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				delete[] szFormatBuffer;
				szFormatBuffer = new char[size];
				if (InternetGetLastResponseInfoA(&dwError, szFormatBuffer, &size))
				{
					std::string ret(szFormatBuffer);
					while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
					delete[] szFormatBuffer;
					return ret;
				}
				delete[] szFormatBuffer;
				return GlobalMsg["strUnableToGetErrorMsg"];

			}
			delete[] szFormatBuffer;
			return GlobalMsg["strUnableToGetErrorMsg"];
		}
		char szFormatBuffer[512];
		const DWORD dwBaseLength = FormatMessageA(
			FORMAT_MESSAGE_FROM_HMODULE,             // dwFlags
			GetModuleHandleA("wininet.dll"),         // lpSource
			dwError,                                 // dwMessageId
			0,                                       // dwLanguageId
			szFormatBuffer,                          // lpBuffer
			sizeof(szFormatBuffer),                  // nSize
			nullptr);
		if (dwBaseLength)
		{
			std::string ret(szFormatBuffer);
			while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
			return ret;
		}
		return GlobalMsg["strUnableToGetErrorMsg"];
	}
	

	bool POST(const char* const serverName, const char* const objectName, const unsigned short port, char *const frmdata, std::string& des)
	{
		const char *acceptTypes[] = { "*/*", nullptr };
		const char *header = "Content-Type: application/x-www-form-urlencoded";

		const HINTERNET hInternet = InternetOpenA(DiceRequestHeader.c_str(), INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
		const HINTERNET hConnect = InternetConnectA(hInternet, serverName, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		const HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", objectName, "HTTP/1.1", nullptr, acceptTypes, 0, 0);
		const BOOL res = HttpSendRequestA(hRequest, header, strlen(header), frmdata, strlen(frmdata));


		if (res)
		{
			DWORD dwRetCode;
			DWORD dwBufferLength = sizeof(dwRetCode);
			if (!HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwBufferLength, nullptr))
			{
				des = getLastErrorMsg();
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			if (dwRetCode != 200)
			{
				des = format(GlobalMsg["strRequestRetCodeErr"], {std::to_string(dwRetCode) });
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			DWORD preRcvCnt;
			if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))
			{
				des = getLastErrorMsg();
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			if (preRcvCnt == 0)
			{
				des = GlobalMsg["strRequestNoResponse"];
				return false;
			}
			std::string finalRcvData;
			while (preRcvCnt)
			{
				char *rcvData = new char[preRcvCnt];
				DWORD rcvCnt;

				if (!InternetReadFile(hRequest, rcvData, preRcvCnt, &rcvCnt))
				{
					des = getLastErrorMsg();
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					delete[] rcvData;
					return false;
				}

				if (rcvCnt != preRcvCnt)
				{
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					des = GlobalMsg["strUnknownErr"];
					delete[] rcvData;
					return false;
				}

				finalRcvData += std::string(rcvData, rcvCnt);

				if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))
				{
					des = getLastErrorMsg();
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					delete[] rcvData;
					return false;
				}

				delete[] rcvData;
			}

			des = finalRcvData;

			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return true;
		}
		des = getLastErrorMsg();
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return false;
	}

	bool GET(const char* const serverName, const char* const objectName, const unsigned short port, std::string& des)
	{
		const char *acceptTypes[] = { "*/*", nullptr };

		const HINTERNET hInternet = InternetOpenA(DiceRequestHeader.c_str(), INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
		const HINTERNET hConnect = InternetConnectA(hInternet, serverName, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		const HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", objectName, "HTTP/1.1", nullptr, acceptTypes, INTERNET_FLAG_NO_CACHE_WRITE, 0);
		const BOOL res = HttpSendRequestA(hRequest, nullptr, 0, nullptr, 0);


		if (res)
		{
			DWORD dwRetCode;
			DWORD dwBufferLength = sizeof(dwRetCode);
			if (!HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwBufferLength, nullptr))
			{
				des = getLastErrorMsg();
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			if (dwRetCode != 200)
			{
				des = format(GlobalMsg["strRequestRetCodeErr"], { std::to_string(dwRetCode) });
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			DWORD preRcvCnt;
			if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))
			{
				des = getLastErrorMsg();
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			if (preRcvCnt == 0)
			{
				des = GlobalMsg["strRequestNoResponse"];
				return false;
			}
			std::string finalRcvData;
			while (preRcvCnt)
			{
				char *rcvData = new char[preRcvCnt];
				DWORD rcvCnt;

				if (!InternetReadFile(hRequest, rcvData, preRcvCnt, &rcvCnt))
				{
					des = getLastErrorMsg();
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					delete[] rcvData;
					return false;
				}

				if (rcvCnt != preRcvCnt)
				{
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					des = GlobalMsg["strUnknownErr"];
					delete[] rcvData;
					return false;
				}

				finalRcvData += std::string(rcvData, rcvCnt);

				if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))
				{
					des = getLastErrorMsg();
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hConnect);
					InternetCloseHandle(hInternet);
					delete[] rcvData;
					return false;
				}

				delete[] rcvData;
			}

			des = finalRcvData;

			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return true;
		}
		des = getLastErrorMsg();
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return false;
	}
}
