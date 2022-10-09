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
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinInet.h>
#pragma comment(lib, "WinInet.lib")
#else
#include <curl/curl.h>
#endif

#include <string>
#include <regex>
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "DiceNetwork.h"
#include "StrExtern.hpp"

namespace Network
{
#ifndef _WIN32
	thread_local CURLcode lastError;

	size_t curlWriteToString(void *contents, size_t size, size_t nmemb, std::string *s)
	{
		size_t newLength = size*nmemb;
		s->append((char*)contents, newLength);
		return newLength;
	}
#endif

	std::string getLastErrorMsg()
	{
#ifdef _WIN32
		DWORD dwError = GetLastError();
		if (dwError == ERROR_INTERNET_EXTENDED_ERROR)
		{
			DWORD size = 512;
			wchar_t* szFormatBuffer = new wchar_t[size];
			if (InternetGetLastResponseInfoW(&dwError, szFormatBuffer, &size))
			{
				std::string ret(convert_w2a(reinterpret_cast<char16_t*>(szFormatBuffer)));
				while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
				delete[] szFormatBuffer;
				return ret;
			}
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				size++;
				delete[] szFormatBuffer;
				szFormatBuffer = new wchar_t[size];
				if (InternetGetLastResponseInfoW(&dwError, szFormatBuffer, &size))
				{
					std::string ret(convert_w2a(reinterpret_cast<char16_t*>(szFormatBuffer)));
					while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
					delete[] szFormatBuffer;
					return ret;
				}
				delete[] szFormatBuffer;
				return getMsg("strUnableToGetErrorMsg");
			}
			delete[] szFormatBuffer;
			return getMsg("strUnableToGetErrorMsg");
		}
		wchar_t szFormatBuffer[512];
		DWORD dwBaseLength = FormatMessageW(
			FORMAT_MESSAGE_FROM_HMODULE, // dwFlags
			GetModuleHandleW(L"wininet.dll"), // lpSource
			dwError, // dwMessageId
			MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), // dwLanguageId
			szFormatBuffer, // lpBuffer
			512, // nSize
			nullptr);

		if (dwBaseLength == 0)
		{
			dwBaseLength = FormatMessageW(
				FORMAT_MESSAGE_FROM_HMODULE, // dwFlags
				GetModuleHandleW(L"wininet.dll"), // lpSource
				dwError, // dwMessageId
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // dwLanguageId
				szFormatBuffer, // lpBuffer
				512, // nSize
				nullptr);
		}
		
		if (dwBaseLength)
		{
			std::string ret(convert_w2a(reinterpret_cast<char16_t*>(szFormatBuffer)));
			while (ret[ret.length() - 1] == '\n' || ret[ret.length() - 1] == '\r')ret.erase(ret.length() - 1);
			return ret;
		}
		return getMsg("strUnableToGetErrorMsg");
#else
		return curl_easy_strerror(lastError);
#endif
	}

	bool POST(const string& url, const string& postContent, const string& postHeader, std::string& des)
	{
		std::string strHeader = postHeader.empty() ? "Content-Type: application/x-www-form-urlencoded" : postHeader;
		std::string UserAgent{ DiceRequestHeader };
		static std::regex re{R"(User-Agent: ([^\r\n]*))"};
		std::smatch match;
		if (std::regex_search(strHeader, match, re)) {
			UserAgent = match[1].str();
		}
#ifdef _WIN32
		URL_COMPONENTSA urlComponents;
		urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
		urlComponents.dwHostNameLength = 1024;
		urlComponents.dwPasswordLength = 0;
		urlComponents.dwSchemeLength = 1024;
		urlComponents.dwUrlPathLength = 1024;
		urlComponents.dwUserNameLength = 0;
		urlComponents.dwExtraInfoLength = 1024;
		urlComponents.lpszHostName = nullptr;
		urlComponents.lpszPassword = nullptr;
		urlComponents.lpszScheme = nullptr;
		urlComponents.lpszUrlPath = nullptr;
		urlComponents.lpszUserName = nullptr;
		urlComponents.lpszExtraInfo = nullptr;
		if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComponents))
		{
			des = getLastErrorMsg();
			return false;
		}

		if (urlComponents.nScheme != INTERNET_SCHEME_HTTP && urlComponents.nScheme != INTERNET_SCHEME_HTTPS)
		{
			des = "Unknown URL Scheme";
			return false;
		}

		const char* acceptTypes[] = {"*/*", nullptr};

		const HINTERNET hInternet = InternetOpenA(UserAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		const HINTERNET hConnect = InternetConnectA(hInternet, std::string(urlComponents.lpszHostName, urlComponents.dwHostNameLength).c_str(), urlComponents.nPort, nullptr, nullptr, 
			INTERNET_SERVICE_HTTP, 0, 0);
		const HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", (std::string(urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength) + std::string(urlComponents.lpszExtraInfo, urlComponents.dwExtraInfoLength)).c_str(), "HTTP/1.1", nullptr, acceptTypes,
			(urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0), 0);
		const BOOL res = HttpSendRequestA(hRequest, strHeader.c_str(), strHeader.length(), (void*)postContent.c_str(), postContent.length());


		if (res)
		{
			DWORD dwRetCode = 0;
			DWORD dwBufferLength = sizeof(dwRetCode);
			if (!HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwBufferLength,
			                    nullptr))
			{
				des = getLastErrorMsg();
				goto InternetClose;
			}
			if (dwRetCode != 200)
			{
				des = getMsg("strRequestRetCodeErr", AttrVars{ {"error", std::to_string(dwRetCode)} });
				goto InternetClose;
			}
			DWORD preRcvCnt;
			if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))
			{
				des = getLastErrorMsg();
				goto InternetClose;
			}
			if (preRcvCnt == 0)
			{
				des = getMsg("strRequestNoResponse");
				return false;
			}
			std::string finalRcvData;
			while (preRcvCnt)
			{
				char* rcvData = new char[preRcvCnt];
				DWORD rcvCnt;

				if (!InternetReadFile(hRequest, rcvData, preRcvCnt, &rcvCnt))
				{
					des = getLastErrorMsg();
					delete[] rcvData;
					goto InternetClose;
				}

				if (rcvCnt != preRcvCnt){
					des = getMsg("strUnknownErr");
					delete[] rcvData;
					goto InternetClose;
				}

				finalRcvData += std::string(rcvData, rcvCnt);

				if (!InternetQueryDataAvailable(hRequest, &preRcvCnt, 0, 0))	{
					des = getLastErrorMsg();
					delete[] rcvData;
					goto InternetClose;
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
InternetClose:
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return false;

#else
		
		CURL* curl;
		curl = curl_easy_init();
		if (curl)
		{
			struct curl_slist* header = NULL;
			header = curl_slist_append(header, strHeader.c_str());
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postContent.c_str());
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &des);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

			lastError = curl_easy_perform(curl);
			if (lastError != CURLE_OK)
			{
				des = getLastErrorMsg();
			}
			curl_slist_free_all(header);
			curl_easy_cleanup(curl);
			return lastError == CURLE_OK;
		}
		return false;
#endif

	}

	bool GET(const string& url, std::string& des) {
#ifdef _WIN32
		URL_COMPONENTSA urlComponents;
		urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
		urlComponents.dwHostNameLength = 1024;
		urlComponents.dwPasswordLength = 0;
		urlComponents.dwSchemeLength = 1024;
		urlComponents.dwUrlPathLength = 1024;
		urlComponents.dwUserNameLength = 0;
		urlComponents.dwExtraInfoLength = 1024;
		urlComponents.lpszHostName = nullptr;
		urlComponents.lpszPassword = nullptr;
		urlComponents.lpszScheme = nullptr;
		urlComponents.lpszUrlPath = nullptr;
		urlComponents.lpszUserName = nullptr;
		urlComponents.lpszExtraInfo = nullptr;
		if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComponents))
		{
			des = getLastErrorMsg();
			return false;
		}

		if (urlComponents.nScheme != INTERNET_SCHEME_HTTP && urlComponents.nScheme != INTERNET_SCHEME_HTTPS)
		{
			des = "Unknown URL Scheme";
			return false;
		}

		const char* acceptTypes[] = {"*/*", nullptr};

		const HINTERNET hInternet = InternetOpenA(DiceRequestHeader, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		const HINTERNET hConnect = InternetConnectA(hInternet, std::string(urlComponents.lpszHostName, urlComponents.dwHostNameLength).c_str(), urlComponents.nPort, nullptr, nullptr, 
			INTERNET_SERVICE_HTTP, 0, 0);
		const HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", (std::string(urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength) + std::string(urlComponents.lpszExtraInfo, urlComponents.dwExtraInfoLength)).c_str(), "HTTP/1.1", nullptr, acceptTypes,
			(urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
		const BOOL res = HttpSendRequestA(hRequest, nullptr, 0, nullptr, 0);


		if (res)
		{
			DWORD dwRetCode = 0;
			DWORD dwBufferLength = sizeof(dwRetCode);
			if (!HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwBufferLength,
			                    nullptr))
			{
				des = getLastErrorMsg();
				InternetCloseHandle(hRequest);
				InternetCloseHandle(hConnect);
				InternetCloseHandle(hInternet);
				return false;
			}
			if (dwRetCode != 200)
			{
				des = getMsg("strRequestRetCodeErr", AttrVars{ {"error", std::to_string(dwRetCode)} });
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
				des = getMsg("strRequestNoResponse");
				return false;
			}
			std::string finalRcvData;
			while (preRcvCnt)
			{
				char* rcvData = new char[preRcvCnt];
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
					des = getMsg("strUnknownErr");
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
#else
		CURL* curl;
		curl = curl_easy_init();
		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, DiceRequestHeader);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &des);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

			lastError = curl_easy_perform(curl);
			if (lastError != CURLE_OK)
			{
				des = getLastErrorMsg();
			}

			curl_easy_cleanup(curl);
			return lastError == CURLE_OK;
		}
		return false;
#endif
	}

}
