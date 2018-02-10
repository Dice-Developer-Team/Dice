#pragma once
#include <vector>
#include "cqdefine.h"



//base64编码
std::string base64_encode(std::string const& s);

//base64解码
std::string base64_decode(std::string const& s);

//替换
std::string&msg_tihuan(std::string & s, std::string const old, std::string const New);

//CQcode编码
std::string&msg_encode(std::string & s, bool isCQ = false);

//CQcode解码
std::string&msg_decode(std::string & s, bool isCQ = false);

//获取cpu启动后经历的周期..
inline unsigned __int64 GetCycleCount()
{
    __asm _emit 0x0F
    __asm _emit 0x31
}
