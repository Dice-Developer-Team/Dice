#include "CQTools.h"

#include <string>


using namespace std;

//代码来源于网络
static const string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static bool is_base64(const unsigned char c) noexcept
{
	return isalnum(c) || c == '+' || c == '/';
}

string base64_encode(const string& decode_string)
{
	auto in_len = decode_string.size();
	auto bytes_to_encode = decode_string.data();
	string ret;
	auto i = 0;
	int j;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while (i++ < 3)
			ret += '=';
	}

	return ret;
}

string base64_decode(const string& encoded_string)
{
	int in_len = encoded_string.size();
	auto i = 0;
	int j;
	auto in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

std::string& msg_replace(std::string& s, const std::string& old, const std::string& n)
{
	size_t st = 0;
	while ((st = s.find(old, st)) < s.size())
	{
		s.replace(st, old.size(), n);
		st += n.size();
	}
	return s;
}

std::string& msg_encode(std::string& s, const bool isCQ)
{
	msg_replace(s, "&", "&amp;");
	msg_replace(s, "[", "&#91;");
	msg_replace(s, "]", "&#93;");
	msg_replace(s, "\t", "&#44;");
	if (isCQ)
		msg_replace(s, ",", "&#44;");
	return s;
}

std::string& msg_decode(std::string& s, const bool isCQ)
{
	if (isCQ)
		msg_replace(s, "&#44;", ",");
	msg_replace(s, "&#91;", "[");
	msg_replace(s, "&#93;", "]");
	msg_replace(s, "&#44;", "\t");
	msg_replace(s, "&amp;", "&");
	//msg_replace(s, "&nbsp;", " ");
	return s;
}
