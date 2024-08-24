#pragma once
#include <string>

//base64编码
std::string base64_encode(const std::string& decode_string);

//base64解码
std::string base64_decode(const std::string& encoded_string);

//替换
std::string& msg_replace(std::string& s, const std::string& old, const std::string& n);

//CQcode编码
std::string& msg_encode(std::string& s, bool isCQ = false);

//CQcode解码
std::string& msg_decode(std::string& s, bool isCQ = false);

