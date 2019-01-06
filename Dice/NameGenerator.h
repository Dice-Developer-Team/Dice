#pragma once
#include <string>
#include <vector>

class NameGenerator
{
	static const std::vector<std::string> ChineseFirstName;
	static const std::vector<std::string> ChineseSurname;
	static const std::vector<std::string> EnglishFirstName;
	static const std::vector<std::string> EnglishLastName;
	static const std::vector<std::string> EnglishLastNameChineseTranslation;
	static const std::vector<std::string> EnglishFirstNameChineseTranslation;
public:
	static std::string getChineseName();
	static std::string getEnglishName();
	static std::string getRandomName();
};

