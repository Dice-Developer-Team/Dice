#pragma once
/**
 * ×Ö·û´®¸¨Öúº¯Êý
 * Copyright (C) 2019-2020 String.Empty
 */

#include <string>
#include <windows.h>

using std::string;
using std::to_string;

static string toString(int num, unsigned short size = 0) {
    string res{ to_string(num) };
    string sign{""};
    if (res[0] == '-') {
        res.erase(res.begin());
        sign = "-";
    }
    while (res.length() < size)res = "0" + res;
    return sign + res;
}

static int count_char(string s, char ch) {
    int cnt = 0;
    for (auto c : s) {
        if (c == ch)cnt++;
    }
    return cnt;
}

static string convert_w2a(wchar_t* wch) {
    int len = WideCharToMultiByte(CP_ACP, 0, wch, wcslen(wch), NULL, 0, NULL, NULL);
    char* m_char = new char[len + 1];
    WideCharToMultiByte(CP_ACP, 0, wch, wcslen(wch), m_char, len, NULL, NULL);
    m_char[len] = '\0';
    std::string str(m_char);
    delete[] m_char;
    return str;
}

struct less_ci {
    bool operator()(const string& str1, const string& str2)const {
        string::const_iterator it1 = str1.begin(), it2 = str2.begin();
        while (it1 != str1.end() && it2 != str2.end()) {
            if (tolower(*it1) < tolower(*it2))return true;
            else if (tolower(*it2) < tolower(*it1))return false;
            it1++;
            it2++;
        }
        return str1.length() < str2.length();
    }
};