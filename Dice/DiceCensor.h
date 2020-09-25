#pragma once
#include <string>
#include "STLExtern.hpp"
#include <unordered_set>
using std::string;
using std::unordered_set;

class Censor {
	string dirWords;
public:
	enum class Level :size_t {
		Ignore,		//无视
		Notice,		//仅0级通知
		Caution,		//仅1级通知
		Warning,		//警告用户且拒绝称呼，并1级通知
		Danger,		//警告用户且拒绝指令，并3级通知
		Critical,		//仅占位，不启用
	};
	map<string, Level, less_ci> words;
	void insert(const string& word, Level);
	//void load();
	void build();
	size_t size()const { return words.size(); }
	int search(const string& text, unordered_set<string>& res);
};

inline Censor censor;

int load_words(const string& filename, Censor& cens);