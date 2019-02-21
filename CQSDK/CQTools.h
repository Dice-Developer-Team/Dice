#pragma once
#include <string>

//base64±àÂë
std::string base64_encode(const std::string& decode_string);

//base64½âÂë
std::string base64_decode(const std::string& encoded_string);

//Ìæ»»
std::string& msg_replace(std::string& s, const std::string& old, const std::string& n);

//CQcode±àÂë
std::string& msg_encode(std::string& s, bool isCQ = false);

//CQcode½âÂë
std::string& msg_decode(std::string& s, bool isCQ = false);

