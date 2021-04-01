#include <filesystem>
#include <fstream>
#include <iomanip>
#include "StrExtern.hpp"
#include "Jsonio.h"

[[deprecated]] nlohmann::json freadJson(const std::string& strPath)
{
	std::ifstream fin(strPath);
	if (!fin)return nlohmann::json();
	nlohmann::json j;
	try
	{
		fin >> j;
	}
	catch (...)
	{
		return nlohmann::json();
	}
	return j;
}

nlohmann::json freadJson(const std::filesystem::path& path)
{
	std::ifstream fin(path);
	if (!fin)return nlohmann::json();
	nlohmann::json j;
	try
	{
		fin >> j;
	}
	catch (...)
	{
		return nlohmann::json();
	}
	return j;
}

[[deprecated]] void fwriteJson(const std::string& strPath, const json& j) 
{
	std::ofstream fout(strPath);
	fout << std::setw(2) << j;
}

void fwriteJson(const std::filesystem::path& fpPath, const json& j) 
{
	std::ofstream fout(fpPath);
	fout << std::setw(2) << j;
}

