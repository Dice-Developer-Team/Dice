/*
 * 字典树
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <map>
#include <unordered_set>
#include <string>
#include <cstring>
#include <stack>
using std::map;
using std::unordered_set;
using std::stack;

template<class _Char, class sort>
class TrieNode {
public:
	map<_Char, TrieNode, sort> next{};
	TrieNode* fail = nullptr;
	bool isleaf = false;
	std::string value{};
};

template<class _Char, class sort>
class TrieG {
	using Node = TrieNode<_Char, sort>;
	using _String = std::basic_string<_Char>;
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
public:
	TrieG(){}
	/*template<typename Con>
	TrieG(const Con& dir) {
		for (const auto& [key,val]: dir) {
			add(key);
		}
		make_fail(root);
	}*/
	/*template<typename Con>
	void build(const Con& dir) {
		this->~TrieG();
		new(this)TrieG(dir);
	}*/
	void clear() { root = {}; }
	void add(const _String s, const string& val) {
		Node* p = &root;
		for (_Char ch : s) {
			if (!p->next.count(ch)) {
				p->next[ch] = Node();
			}
			p = &(p->next[ch]);
		}
		p->value = val;
		p->isleaf = true;
	}
	void make_fail() {
		make_fail(root);
	}
	void insert(const _String& key, const string& val) {
		add(key, val);
		make_fail(root);
	}
	//前缀匹配
	bool match_head(const _String& s, _String& res)const {
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
	bool match_head(const _String& s, stack<_String>& res)const {
		const Node* p = &root;
		for (const auto& ch : s) {
			//if (ignored(ch))continue;
			if (!p->next.count(ch))break;
			p = &(p->next.find(ch)->second);
			if (p->isleaf) {
				res.push(p->value);
			}
		}
		return res.size();
	}
	//任意位置字串匹配，不重复记录
	bool search(const _String& s, vector<string>& res, bool(* _Filter)(_Char) = [](_Char) {return false; })const {
		unordered_set<string> words;
		const Node* p = &root;
		for (const auto& ch : s) {
			if (_Filter(ch))continue;
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
			if (p->isleaf && !words.count(p->value)) {
				words.insert(p->value);
				res.push_back(p->value);
			}
		}
		return res.size();
	}
};
