#pragma once
#ifndef __ENCODINGCONVERT__
#define __ENCODINGCONVERT__
#include <string>
std::string GBKtoUTF8(const std::string& strGBK);
std::string UTF8toGBK(const std::string& strUTF8);
#endif /*__ENCODINGCONVERT__*/
