#include <filesystem>
#include <fstream>
#include "StrExtern.hpp"
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

nlohmann::json freadJson(const std::filesystem::path& path) {
   	std::ifstream fin(convert_w2a(path.wstring().c_str()));
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