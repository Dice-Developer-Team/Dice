#pragma once
/**
 * ×Ö·û´®¸¨Öúº¯Êý
 * Copyright (C) 2019-2022 String.Empty
 */

#include <string>
#include <vector>
#include <unordered_map>
#include "STLExtern.hpp"

using std::string;
using std::wstring;
using std::u16string;
using std::to_string;
using std::vector;

#define CP_GBK (936)
constexpr auto space_char{ " \t\r\n" };

bool isNumeric(const string&);
string toString(int num, unsigned short size = 0);

template<typename F>
typename std::enable_if_t<std::is_floating_point_v<F>, string>
toString(F num, unsigned short scale = 2, bool align = false) 
{
    string strNum{ to_string(num) };
    size_t dot(strNum.find('.') + scale + 1);
    if (align)return strNum.substr(0, dot);
    size_t last(strNum.find_last_not_of('0') + 1);
    if (last > dot)last = dot;
    if (strNum[last - 1] == '.')last--;
    return strNum.substr(0, last);
}

template<typename Dig>
string to_signed_string(Dig num) 
{
    if (num > 0)return "+" + to_string(num);
    return to_string(num);
}

int count_char(const string& s, char ch);

vector<string> getLines(const string& s, char delim = '\n');
vector<string> split(const string&, const string&);
string splitOnce(string& str, const string& sep = ".");
fifo_dict<string> splitPairs(const string&, char delim = '=', char br = '\n');

template<typename Con>
void splitID(const string& str , Con& list) {
    const char digits[]{ "0123456789" };
#ifndef linux
    const char* p = strpbrk(str.c_str(), digits);
    size_t len;
    while (p) {
        list.emplace(atoll(p));
        len = strspn(p, digits);
        p = strpbrk(p + len, digits);
    }
#else
    size_t p{ str.find_first_of(digits) };
    while (p != string::npos) {
        list.emplace(atoll(str.c_str() + p));
        p = str.find_first_of(digits, str.find_first_not_of(digits, p));
    }
#endif
}

string convert_w2a(const char16_t* wch);

u16string convert_a2w(const char* ch);

string convert_realw2a(const wchar_t* wch);

wstring convert_a2realw(const char* ch);

string printDuringTime(long long);

size_t wstrlen(const char*);
