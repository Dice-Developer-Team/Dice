#include "Jsonio.h"

nlohmann::json freadJson(std::string strPath) {
	std::ifstream fin(strPath);
	if (!fin)return nlohmann::json();
	nlohmann::json j;
	try {
		fin >> j;
	}
	catch (...) {
		return nlohmann::json();
	}
	return j;
}
