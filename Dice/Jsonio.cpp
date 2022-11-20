#include "filesystem.hpp"
#include <fstream>
#include <iomanip>
#include "StrExtern.hpp"
#include "Jsonio.h"

[[deprecated]] fifo_json freadJson(const std::string& strPath)
{
	std::ifstream fin(strPath);
	if (!fin)return fifo_json();
	fifo_json j;
	try
	{
		fin >> j;
	}
	catch (...)
	{
		return fifo_json();
	}
	return j;
}

fifo_json freadJson(const std::filesystem::path& path){
	fifo_json j;
	std::ifstream fin(path);
	if (!fin)return j;
	try {
		fin >> j;
	}
	catch (...) {}
	return j;
}

[[deprecated]] void fwriteJson(const std::string& strPath, const fifo_json& j)
{
	std::ofstream fout(strPath);
	fout << std::setw(2) << j;
}

void fwriteJson(const std::filesystem::path& fpPath, const fifo_json& j, const int indent)
{
	std::ofstream fout(fpPath);
	fout << std::setw(2) << j.dump(indent);
}

