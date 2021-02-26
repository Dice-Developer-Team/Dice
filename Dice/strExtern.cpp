#include <string>
#include <string_view>
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif
#include "StrExtern.hpp"
#include "EncodingConvert.h"

using std::string;
using std::u16string;
using std::string_view;
using std::to_string;

#define CP_GBK (936)

string toString(int num, unsigned short size)
{
	string res = to_string(num);
	string sign;
	if (res[0] == '-')
	{
		res.erase(res.begin());
		sign = "-";
	}
	string ret(size - res.length(), '0');
	ret.append(res);
	return sign + ret;
}

int count_char(const string& s, char ch)
{
	return std::count(s.begin(), s.end(), ch);
}

string convert_w2a(const char16_t* wch)
{
#ifdef _WIN32
	const int len = WideCharToMultiByte(CP_GBK, 0, (const wchar_t*)wch, -1, nullptr, 0, nullptr, nullptr);
	char* m_char = new char[len];
	WideCharToMultiByte(CP_GBK, 0, (const wchar_t*)wch, -1, m_char, len, nullptr, nullptr);
	std::string str(m_char);
	delete[] m_char;
	return str;
#else
	return ConvertEncoding<char, char16_t>(wch, "utf-16le", "gb18030");
#endif
}

u16string convert_a2w(const char* ch)
{
#ifdef _WIN32
    const int len = MultiByteToWideChar(CP_GBK, 0, ch, -1, nullptr, 0);
    wchar_t* m_char = new wchar_t[len];
    MultiByteToWideChar(CP_GBK, 0, ch, -1, m_char, len);
    std::u16string wstr((char16_t*)m_char);
    delete[] m_char;
    return wstr;
#else
	return ConvertEncoding<char16_t, char>(ch, "gb18030", "utf-16le");
#endif
}
size_t wstrlen(const char* ch) {
#ifdef _WIN32
    return MultiByteToWideChar(CP_GBK, 0, ch, -1, nullptr, 0);
#else
	return ConvertEncoding<char16_t, char>(ch, "gb18030", "utf-16le").length();
#endif
}

string printDuringTime(long long seconds) 
{
    if (seconds < 0) {
        return "N/A";
    }
    else if (seconds < 60) {
        return std::to_string(seconds) + "秒";
    }
    int mins = int(seconds / 60);
    seconds = seconds % 60;
    if (mins < 60) {
        return std::to_string(mins) + "分" + std::to_string(seconds) + "秒";
    }
    int hours = mins / 60;
    mins = mins % 60;
    if (hours < 24) {
        return std::to_string(hours) + "小时" + std::to_string(mins) + "分" + std::to_string(seconds) + "秒";
    }
    int days = hours / 24;
    hours = hours % 24;
    return std::to_string(days) + "天" + std::to_string(hours) + "小时" + std::to_string(mins) + "分" + std::to_string(seconds) + "秒";
}
