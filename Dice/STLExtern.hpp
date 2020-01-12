/*
 * ×Ô¶¨ÒåÈÝÆ÷
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <vector>
#include <map>
using std::initializer_list;
using std::pair;
using std::vector;
using std::map;
using std::multimap;
template<typename T>
class enumap{
public:
	typedef typename map<T, int> mapT;
	typedef typename std::initializer_list<T> initlist;
	vector<T> ary;
	mapT mVal;
	enumap(initlist items) :ary{items} {
		int index = 0;
		for (auto& it : items) {
			mVal.emplace(it, index++);
		}
    }
	bool count(T val)const {
		return mVal.find(val) != mVal.end();
	}
	int operator[](T& val)const {
		if (auto it = mVal.find(val); it != mVal.end())return it->second;
		return -1;
	}
	const T& operator[](size_t index)const {
		return ary[index];
	}
};

template<typename TKey, typename TVal>
class multi_range {
public:
	typedef typename multimap<TKey, TVal>::iterator iterator;
	std::pair<iterator, iterator>range;
	multi_range(multimap<TKey, TVal>& mmap, TKey key) {
		range = mmap.equal_range(key);
	}
	iterator begin() const {
		return range.first;
	}
	iterator end() const {
		return range.second;
	}
};

template<typename K,typename V>
typename multimap<K, V>::iterator match(multimap<K, V> mmp, K key, V val) {
	for (auto [it, ed] = mmp.equal_range(key);it!=ed;it++) {
		if (it->second == val)return it;
	}
	return mmp.end();
}