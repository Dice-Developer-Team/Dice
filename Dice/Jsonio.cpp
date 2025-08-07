#include "Jsonio.h"

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

void fwriteJson(const std::filesystem::path& fpPath, const fifo_json& j, const int indent)
{
	std::ofstream fout(fpPath);
	fout << std::setw(2) << j.dump(indent);
}
