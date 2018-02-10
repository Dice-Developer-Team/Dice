// sqlite.cpp : 定义控制台应用程序的入口点。
//

#include "windows.h"
#include <iostream>
#include <string>
#include "sqlitetool.h"

using namespace std;

char*strUtf8;
//gbk转UTF-8
//回传的char*自行销毁
char* U(const char* strGbk)//传入的strGbk是GBK编码  
{
	char* strUtf8;
    //delete[]strUtf8;

    //gbk转unicode  
    auto len = MultiByteToWideChar(CP_ACP, 0, strGbk, -1, NULL, 0);
    auto *strUnicode = new wchar_t[len];
    wmemset(strUnicode, 0, len);
    MultiByteToWideChar(CP_ACP, 0, strGbk, -1, strUnicode, len);

    //unicode转UTF-8  
    len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	strUtf8 = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, strUtf8, len, NULL, NULL);

    //此时的strUtf8是UTF-8编码  
    delete[]strUnicode;
    strUnicode = NULL;
    return strUtf8;
}

//UTF-8转gbk  
//回传的char*自行销毁
char* G(const char* strUtf8)//传入的strUtf8是UTF-8编码  
{
	char *strGbk;
    //delete[]strGbk;
    //UTF-8转unicode  
    int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL, 0);
    wchar_t * strUnicode = new wchar_t[len];//len = 2  
    wmemset(strUnicode, 0, len);
    MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, strUnicode, len);

    //unicode转gbk  
    len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
    strGbk = new char[len];//len=3 本来为2，但是char*后面自动加上了\0  
    memset(strGbk, 0, len);
    WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strGbk, len, NULL, NULL);

    //此时的strTemp是GBK编码  
    delete[]strUnicode;
    strUnicode = NULL;
    return strGbk;
}


