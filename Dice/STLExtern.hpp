/*
 * 自定义容器
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <queue>
using std::initializer_list;
using std::pair;
using std::vector;
using std::map;
using std::multimap;
using std::string;
using std::to_string;

struct less_ci
{
	bool operator()(const char& ch1, const char& ch2) const {
		return tolower(static_cast<unsigned char>(ch1)) < tolower(static_cast<unsigned char>(ch2));
	}
	bool operator()(const string& str1, const string& str2) const
	{
		string::const_iterator it1 = str1.cbegin(), it2 = str2.cbegin();
		while (it1 != str1.cend() && it2 != str2.cend())
		{
			if (tolower(static_cast<unsigned char>(*it1)) < tolower(static_cast<unsigned char>(*it2)))return true;
			if (tolower(static_cast<unsigned char>(*it2)) < tolower(static_cast<unsigned char>(*it1)))return false;
			++it1;
			++it2;
		}
		return str1.length() < str2.length();
	}
};

template <typename T>
class enumap
{
public:
	typedef map<T, int> mapT;
	typedef std::initializer_list<T> initlist;
	vector<T> ary;
	mapT mVal;

	enumap(initlist items) : ary{items}
	{
		int index = 0;
		for (auto& it : items)
		{
			mVal.emplace(it, index++);
		}
	}

	[[nodiscard]] bool count(T val) const
	{
		return mVal.find(val) != mVal.end();
	}

	size_t operator[](const T& val) const
	{
		if (auto it = mVal.find(val); it != mVal.end())return it->second;
		return -1;
	}

	const T& operator[](size_t index) const
	{
		return ary[index];
	}
};

template <typename TKey, typename TVal>
class multi_range
{
public:
	typedef typename multimap<TKey, TVal>::const_iterator iterator;
	std::pair<iterator, iterator> range;

	multi_range(const multimap<TKey, TVal>& mmap, const TKey& key) : range(mmap.equal_range(key))
	{
	}

	iterator begin() const
	{
		return range.first;
	}

	iterator end() const
	{
		return range.second;
	}
};

template <typename K, typename V>
typename multimap<K, V>::iterator match(multimap<K, V>& mmp, K key, V val)
{
	for (auto [it, ed] = mmp.equal_range(key); it != ed; ++it)
	{
		if (it->second == val)return it;
	}
	return mmp.end();
}

using prior_item = std::pair<int, string>;

//按优先级输出项目
class PriorList
{
	std::priority_queue<prior_item> qItem;
public:
	PriorList(const map<string, int>& mItem)
	{
		for (auto& [key, prior] : mItem)
		{
			qItem.emplace(prior, key);
		}
	}

	string show()
	{
		string res;
		int index = 0;
		while (!qItem.empty())
		{
			auto [prior, item] = qItem.top();
			qItem.pop();
			res += "\n" + to_string(++index) + "." + item + ":" + to_string(prior);
		}
		return res;
	}
};

