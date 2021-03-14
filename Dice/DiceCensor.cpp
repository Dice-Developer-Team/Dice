#include "DiceCensor.h"
#include "SHKTrie.h"
#include "DiceFile.hpp"
#include "Jsonio.h"
#include "STLExtern.hpp"
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include <bitset>
#include <cstring>
#include <filesystem>

TrieG<less_ci> wordG;

enumap<string> sens{ "Ignore","Notice","Caution","Warning","Danger" };

int load_words(const std::filesystem::path& path, Censor& cens) {
	int cnt(0);
	ifstream fin(path);
	if (!fin)return -1;
	bool isUTF8{ false };
	string word;
	Censor::Level danger = Censor::Level::Warning;
	while (getline(fin, word)) {
		if (word.empty())break;
		if (word[0] == '#') {
			word = word.substr(1);
			//注释敏感等级
			if (sens.count(word)) {
				danger = (Censor::Level)sens[word];
			}
			//注释文件编码
			else if (word == "UTF8") {
				isUTF8 = true;
			}
		}
		else {
			if (isUTF8)word = UTF8toGBK(word);
			cens.insert(word, danger);
			cnt++;
		}
	}
	return cnt;
}

void Censor::insert(const string& word, Level danger = Level::Warning) {
	words[word] = danger;
}
void Censor::add_word(const string& word, Level danger = Level::Warning) {
	CustomWords[word] = danger;
	if (!words.count(word))wordG.insert(word);
	words[word] = danger;
	save();
}
bool Censor::rm_word(const string& word) {
	if (!CustomWords.count(word)) {
		//无可移除
		if (!words.count(word)) {
			return false;
		}
		//无自定义，但词库包含
		else {
			words[word] = Level::Ignore;
			CustomWords[word] = Level::Ignore;
		}
	}
	else{
		words[word] = Level::Ignore;
		CustomWords.erase(word);
	}
	save();
	return true;
}

Censor::Level Censor::get_level(const string& lv) {
	if (!sens.count(lv))return Level::Warning;
	return (Censor::Level)sens[lv];
}

void Censor::build() {
	map_merge(words, CustomWords);
	wordG.build(words);
}
void Censor::save() {
	saveJMap(DiceDir / "conf" / "CustomCensor.json", censor.CustomWords);
}

int Censor::search(const string& text, unordered_set<string>& res) {
	std::bitset<6> sens;
	wordG.search(text, res);
	for (auto& word : res) {
		sens.set((size_t)words[word]);
	}
	return sens[5] ? 5
		: sens[4] ? 4
		: sens[3] ? 3
		: sens[2] ? 2
		: sens[1] ? 1
		: 0;
}