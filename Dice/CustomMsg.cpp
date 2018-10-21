#include "json.hpp"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include <fstream>
#include <sstream>
#include "RDConstant.h"

void ReadCustomMsg(std::ifstream& in)
{
	std::stringstream buffer;
	buffer << in.rdbuf();
	nlohmann::json customMsg = nlohmann::json::parse(buffer.str());
	for (nlohmann::json::iterator it = customMsg.begin(); it != customMsg.end(); ++it) {
		if(GlobalMsg.count(it.key()))
		{
			if (it.key() != "strHlpMsg")
			{
				GlobalMsg[it.key()] = UTF8toGBK(it.value().get<std::string>());
			}
			else
			{
				GlobalMsg[it.key()] = Dice_Short_Ver + "\n" + UTF8toGBK(it.value().get<std::string>());
			}
		}
	}
}
