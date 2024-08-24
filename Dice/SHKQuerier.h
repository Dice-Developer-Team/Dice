/*
 * 词条查询器
 * 基于倒排实现对待查询词条的相似匹配
 * Copyright (C) 2020-2024 String.Empty
 */
#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "StrExtern.hpp"
using std::string;
using std::u16string;
using std::vector;
using std::unordered_map;
using std::unordered_set;

struct WordNode {
	//该节点的分词，连续数字、连续字母或宽字符单字
	//string word;
	unordered_set<string>keys;
	unordered_map<u16string, unordered_set<string>>next;
};

class WordQuerier {
	unordered_map<u16string, WordNode> word_list;
public:
	static vector<u16string> cutter(const string& raw) {
		static u16string dot{ u"~!@#$%^&*()-=`_+[]\\{}|;':\",./<>?" };
		vector<u16string> res;
		u16string title{ convert_a2w(raw.c_str()) };
		u16string word;
		for (size_t pos = 0; pos < title.length(); pos++) {
			auto ch{ title[pos] };
			if (isspace((int)ch) || dot.find(ch) != u16string::npos) { //ignore and cut off
				if (!word.empty()) {
					res.push_back(word);
					word.clear();
				}
			}
			else {
				if (!isalnum((int)ch)) {// cut off
					if (!word.empty()) {
						res.push_back(word);
						word.clear();
					}
					res.push_back(u16string(1, ch));
					continue;
				}
				if (!word.empty()
					&& iswdigit(title[pos]) != iswdigit(word[0])) {
					res.push_back(word);
					word.clear();
					continue;
				}
				word += tolower(title[pos]);
			}
		}
		if (!word.empty())res.push_back(word);
		return res;
	}
	void insert(const string& key) {
		auto words{ cutter(key) };
		if (words.empty())return;
		word_list[words[0]].keys.insert(key);
		size_t idx(1);
		while (idx < words.size()) {
			word_list[words[idx - 1]].next[words[idx]].insert(key);
			word_list[words[idx++]].keys.insert(key);
		}
	}
	unordered_set<string> search(const string& key)const{
		auto words{ cutter(key) };
		if (words.empty())return{};
		unordered_set<string> res;
		unordered_set<string> sInter;
		const WordNode* last_word = nullptr;
		for (const auto& word : words) {
			//无该词，略过
			if (!word_list.count(word))continue;
			//检查连缀
			if (last_word && last_word->next.find(word) != last_word->next.end()) {
				for (const auto& w : last_word->next.find(word)->second) {
					if (res.count(w))sInter.insert(w);
				}
				if (!sInter.empty()) {
					res = sInter;
					sInter.clear();
					last_word = &word_list.find(word)->second;
					continue;
				}
			}
			//
			if (res.empty()) {
				res = word_list.find(word)->second.keys;
				last_word = &word_list.find(word)->second;
				continue;
			}
			for (auto& w : word_list.find(word)->second.keys) { 
				if (res.count(w))sInter.insert(w);
			}
			if (!sInter.empty()) {
				res = sInter;
				sInter.clear();
				last_word = &word_list.find(word)->second;
			}
		}
		return res;
	}
	void clear() {
		word_list.clear();
	}
};