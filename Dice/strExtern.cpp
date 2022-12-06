#include <string>
#include <string_view>
#include <sstream>
#include <algorithm>
#include <cwchar>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif
#include "StrExtern.hpp"
#include "EncodingConvert.h"

using std::string;
using std::wstring;
using std::u16string;
using std::string_view;
using std::to_string;

bool isNumeric(const string& s) {
    bool hasDot{ false };
    size_t len{ s.length() };
    for (size_t i = 0; i < len; i++) {
        if (isdigit(s[i]))continue;
        if ((s[i] == '+' || s[i] == '-') && 0 == i)continue;
        if (s[i] == '.' && !hasDot && i != i - 1) {
            hasDot = true;
            continue;
        }
        return false;
    }
    return true;
}

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

vector<string> getLines(const string& s, char delim){
    vector<string> vLine;
    std::stringstream ss(s);
    string line;
    static const char* space{ " \t\r\n" };
    while (std::getline(ss, line, delim)){
        if (size_t l{ line.find_first_not_of(space) }; l != string::npos) {
            vLine.push_back(line.substr(l, line.find_last_not_of(space) + 1 - l));
        }
    }
    return vLine;
}
vector<string> split(const string& str, const string& sep) {
    vector<string> res;
    size_t l{ 0 }, pos{ str.find(sep) };
    while (pos != string::npos) {
        res.push_back(str.substr(l, pos - l));
        l = pos + sep.length();
        pos = str.find(sep, l);
    }
    if (l < str.length()) res.push_back(str.substr(l));
    return res;
}
string splitOnce(string& str, const string& sep) {
    string head;
    if (auto pos{ str.find(sep) }; pos != string::npos) {
        head = str.substr(0, pos);
        str = str.substr(pos + 1);
    }
    return head;
}
fifo_dict<> splitPairs(const string& s, char delim, char br) {
    fifo_dict<>dict;
    string_view sv(s),line;
    size_t posDe{ 0 }, posBr{ 0 }, p{ 0 };
    while (posBr != string::npos) {
        posBr = sv.find(br);
        line = sv.substr(p = sv.find_first_not_of(space_char, p), sv.find_last_not_of(space_char, posBr - 1) - p + 1);
        if ((posDe = line.find(delim)) != string::npos) {
            if(posDe)dict[string(line.substr(0, posDe))] = line.substr(posDe + 1);
        }
        else dict[string(line)] = {};
        sv.remove_prefix(posBr + 1);
    }
    return dict;
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
 
string convert_realw2a(const wchar_t* wch)
{
#if WCHAR_MAX == 0xFFFF || WCHAR_MAX == 0x7FFF
	return convert_w2a((const char16_t*)wch);
#elif WCHAR_MAX == 0xFFFFFFFF || WCHAR_MAX == 0x7FFFFFFF 
	return ConvertEncoding<char, wchar_t>(wch, "utf-32le", "gb18030");
#else
#error Unsupported WCHAR length
#endif
}

wstring convert_a2realw(const char* ch)
{
#if WCHAR_MAX == 0xFFFF || WCHAR_MAX == 0x7FFF
	return (const wchar_t*)convert_a2w(ch).c_str();
#elif WCHAR_MAX == 0xFFFFFFFF || WCHAR_MAX == 0x7FFFFFFF 
	return ConvertEncoding<wchar_t, char>(ch, "gb18030", "utf-32le");
#else
#error Unsupported WCHAR length
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
