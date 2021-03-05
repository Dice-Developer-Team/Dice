#pragma once
/**
 * ×Ö·û´®¸¨Öúº¯Êý
 * Copyright (C) 2019-2020 String.Empty
 */

#include <string>

using std::string;
using std::u16string;
using std::to_string;

#define CP_GBK (936)

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

string convert_w2a(const char16_t* wch);

u16string convert_a2w(const char* ch);

string printDuringTime(long long);

size_t wstrlen(const char*);
