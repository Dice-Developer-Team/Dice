/*
 * ×ÖµäÊ÷
 * Copyright (C) 2019-2022 String.Empty
 */
#pragma once
#include <map>
#include <unordered_set>
#include <string>
#include <cstring>
#include <stack>
#include <deque>
#include <memory>
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::stack;
using std::deque;
using std::shared_ptr;

template<class _Char, class sort, class Val = std::string>
class TrieNode {
	//using _String = std::basic_string<_Char>;
public:
	//_String word;
	//TrieNode(){}
	//TrieNode(const _String& s) :word(s){}
	map<_Char, TrieNode, sort> next;
	TrieNode* fail = nullptr;
	shared_ptr<Val> value;
};

template<class _Char, class sort, class Val = std::string>
class TrieG {
	using Node = TrieNode<_Char, sort, Val>;
	using _String = std::basic_string<_Char>;
	Node root{};
	string chsException;
public:
	TrieG(){}
	void clear() { root = {}; }
	void add(const _String s, const Val& val) {
		Node* p = &root;
		for (_Char ch : s) {
			//if (!p->next.count(ch)) {
			//	p->next[ch] = Node(p->word + ch);
			//}
			p = &(p->next[ch]);
		}
		p->value = std::make_shared<Val>(val);
	}
	void make_fail() {
		deque<Node*> queue{};
		Node* parent{ root.fail = &root };
		for (auto& [ch, kid] : parent->next) {
			kid.fail = &root;
			queue.push_back(&kid);
		}
		while (!queue.empty()) {
			parent = queue.front();
			queue.pop_front();
			for (auto& [ch, kid] : parent->next) {
				Node* p{ parent->fail };
				while (p != &root && !p->next.count(ch)) {
					p = p->fail;
				}
				if (auto it = p->next.find(ch); it != p->next.end()) {
					kid.fail = &(it->second);
					if (kid.fail->value && !kid.value) {
						kid.value = kid.fail->value;
					}
				}
				else {
					kid.fail = &root;
				}
				queue.push_back(&kid);
			}
		} 
	}
	void insert(const _String& key, const string& val) {
		add(key, val);
		make_fail();
	}
	//Ç°×ºÆ¥Åä
	shared_ptr<Val> match_head(const _String& s)const {
		const Node* p = &root;
		const Node* leaf{ nullptr };
		for (const auto& ch : s) {
			//if (ignored(ch))continue;
			if (!p->next.count(ch))break;
			p = &(p->next.at(ch));
			if (p->value) {
				leaf = p;
			}
		}
		return leaf ? leaf->value : std::make_shared<Val>();
	}
	bool match_head(const _String& s, stack<Val>& res)const {
		const Node* p = &root;
		for (const auto& ch : s) {
			//if (ignored(ch))continue;
			if (!p->next.count(ch))break;
			p = &(p->next.at(ch));
			if (p->value) {
				res.push(*p->value);
			}
		}
		return res.size();
	}
	//ÈÎÒâÎ»ÖÃ×Ö´®Æ¥Åä£¬²»ÖØ¸´¼ÇÂ¼
	bool search(const _String& s, vector<Val>& res, bool(* _Filter)(_Char) = [](_Char) {return false; })const {
		unordered_set<Val> words;
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
			if (p->value && !words.count(*p->value)) {
				words.insert(*p->value);
				res.push_back(*p->value);
			}
		}
		return res.size();
	}
};
