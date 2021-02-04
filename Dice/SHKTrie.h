/*
 * 字典树
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <map>
#include <unordered_set>
#include <string>
#include <cstring>
using std::map;
using std::unordered_set;
using std::string;

template<class sort>
class TrieNode {
public:
	map<char, TrieNode, sort> next{};
	TrieNode* fail = nullptr;
	bool isleaf = false;
	string value{};
};

template<class sort>
class TrieG {
	using Node = TrieNode<sort>;
	Node root{};
	string chsException;
	//是为目标节点的子节点匹配fail
	void make_fail(Node& node) {
		if (&node == &root) {
			for (auto& [ch, kid] : node.next) {
				kid.fail = &node;
			}
		}
		else {
			for (auto& [ch,kid] : node.next) {
				if (auto it = node.fail->next.find(ch); it != node.fail->next.end()) {
					kid.fail = &(it->second);
				}
				else {
					kid.fail = &root;
				}
			}
		}
		for (auto& [ch, kid] : node.next) {
			make_fail(kid);
		}
	}
	static bool ignored(char ch) {
		static const char* dot{ "~!@#$%^&*()-=`_+[]\\{}|;':\",./<>?" };
		return isspace(static_cast<unsigned char>(ch)) || strchr(dot, ch);
	}
	void add(const string& s) {
		Node* p = &root;
		for (auto ch : s) {
			if (!p->next.count(ch)) {
				p->next[ch] = Node();
			}
			p = &(p->next[ch]);
		}
		p->value = s;
		p->isleaf = true;
	}
public:
	TrieG(){}
	template<typename Con>
	TrieG(const Con& dir) {
		for (const auto& [key,val]: dir) {
			add(key);
		}
		make_fail(root);
	}
	template<typename Con>
	void build(const Con& dir) {
		new(this)TrieG(dir);
	}
	void insert(const string& key) {
		add(key);
		make_fail(root);
	}
	//前缀匹配
	bool match_head(const string& s, string& res)const {
		const Node* p = &root;
		for (const auto& ch : s) {
			//if (ignored(ch))continue;
			if (!p->next.count(ch))break;
			p = &(p->next.find(ch)->second);
			if (p->isleaf) {
				res = p->value;
			}
		}
		return !res.empty();
	}
	bool match_head(const string& s, unordered_set<string>& res)const {
		const Node* p = &root;
		for (const auto& ch : s) {
			if (ignored(ch))continue;
			if (!p->next.count(ch))break;
			p = &(p->next.find(ch)->second);
			if (p->isleaf) {
				res.insert(p->value);
			}
		}
		return res.size();
	}
	//任意位置字串匹配
	bool search(const string& s, unordered_set<string>& res)const {
		const Node* p = &root;
		for (const auto& ch : s) {
			if (ignored(ch))continue;
			while (1) {
				if (auto it = p->next.find(ch); it != p->next.end()) {
					p = &(it->second);
					break;
				}
				if (p == &root) {
					break;
				}
				p = p->fail;
			}
			if (p->isleaf) {
				res.insert(p->value);
			}
		}
		return res.size();
	}
};
