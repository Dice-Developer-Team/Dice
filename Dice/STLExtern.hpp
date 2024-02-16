/*
 * 自定义容器
 * Copyright (C) 2019-2022 String.Empty
 * 2022/08/18 添加grad_map
 * 2022/09/07 添加fifo_map&fifo_cmpr_ci
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include "fifo_map.hpp"
using std::initializer_list;
using std::pair;
using std::vector;
using std::map;
using std::unordered_map;
using std::multimap;
using std::string;
using std::to_string;
using nlohmann::fifo_map;
template<typename T>
using ptr = std::shared_ptr<T>;

template <typename Map1, typename Map2>
size_t map_merge(Map1& m1, const Map2& m2) {
	size_t t{ 0 };
	for (auto& [k, v] : m2) {
		m1[k] = v;
		++t;
	}
	return t;
}

inline string toLower(string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
	return s;
}

struct less_ci
{
	bool operator()(const char& ch1, const char& ch2) const {
		return ((ch1 & ~(unsigned char)0xff) | tolower(static_cast<unsigned char>(ch1)))
			< ((ch2 & ~(unsigned char)0xff) | tolower(static_cast<unsigned char>(ch2)));
	}
	template<typename _Char>
	bool operator()(const _Char& ch1, const _Char& ch2) const {
		return tolower(ch1) < tolower(ch2);
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
struct hash_ci {
	[[nodiscard]] size_t operator()(const string& str)const {
		size_t seed = 131;
		size_t hash = 0;
		for (char ch : str) {
			hash = hash * seed + tolower(static_cast<unsigned char>(ch));
		}
		return (hash & 0x7FFFFFFF);
	}
};
struct equal_ci
{
	bool operator()(const char& ch1, const char& ch2) const {
		return ((ch1 & ~(unsigned char)0xff) | tolower(static_cast<unsigned char>(ch1)))
			== ((ch2 & ~(unsigned char)0xff) | tolower(static_cast<unsigned char>(ch2)));
	}
	template<typename _Char>
	bool operator()(const _Char& ch1, const _Char& ch2) const {
		return tolower(static_cast<unsigned char>(ch1 & 0xff)) == tolower(static_cast<unsigned char>(ch2 & 0xff));
	}
	bool operator()(const string& str1, const string& str2) const {
		string::const_iterator it1 = str1.cbegin(), it2 = str2.cbegin();
		while (it1 != str1.cend() && it2 != str2.cend()) {
			if (tolower(static_cast<unsigned char>(*it2)) != tolower(static_cast<unsigned char>(*it1)))return false;
			++it1;
			++it2;
		}
		return str1.length() == str2.length();
	}
};
template<typename T = std::string>
using dict = std::unordered_map<string, T>;
template<typename T = std::string>
using dict_ci = std::unordered_map<string, T, hash_ci, equal_ci>;
template<typename T = std::string>
using multidict_ci = std::unordered_multimap<string, T, hash_ci, equal_ci>;

class fifo_cmpr_ci {
public:
	fifo_cmpr_ci(
		dict<size_t>* keys,
		std::size_t timestamp = 1)
		:
		m_timestamp(timestamp),
		m_keys(keys)
	{}
	bool operator()(const string & lhs, const string & rhs) const
	{
		if (lhs == rhs)return false;
		// look up timestamps for both keys
		const auto timestamp_lhs = m_keys->find(toLower(lhs));
		const auto timestamp_rhs = m_keys->find(toLower(rhs));

		if (timestamp_lhs == m_keys->end())
		{
			// timestamp for lhs not found - cannot be smaller than for rhs
			return false;
		}

		if (timestamp_rhs == m_keys->end())
		{
			// timestamp for rhs not found - timestamp for lhs is smaller
			return true;
		}

		// compare timestamps
		return timestamp_lhs->second < timestamp_rhs->second;
	}

	void add_key(const string& key){
		m_keys->insert({ toLower(key), m_timestamp++ });
	}

	void remove_key(const string& key)
	{
		m_keys->erase(toLower(key));
	}
private:
	template <
		class MapKey,
		class MapT,
		class MapCompare,
		class MapAllocator
	> friend class nlohmann::fifo_map;
	/// the next valid insertion timestamp
	std::size_t m_timestamp = 1;

	/// pointer to a mapping from keys to insertion timestamps
	dict<size_t>* m_keys = nullptr;
};
template<typename T = std::string>
using fifo_dict = fifo_map<string, T>;
template<typename T = std::string>
using fifo_dict_ci = fifo_map<string, T, fifo_cmpr_ci>;

template <typename TKey, typename TVal>
class grad_map {
	map<TKey, TVal>grades;
	TVal valElse;
public:
	grad_map& set_else(const TVal& val) {
		valElse = val;
		return *this;
	}
	const TVal& get_else() const{
		return valElse;
	}
	grad_map& set_step(TKey key, const TVal& val) {
		grades[key] = val;
		return *this;
	}
	const TVal& operator[](TKey key) const{
		if (auto it{ grades.upper_bound(key) }; it != grades.begin()) {
			return (--it)->second;
		}
		else return valElse;
	}
};
template <typename T>
class enumap
{
public:
	typedef unordered_map<T, int> mapT;
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
class enumap_ci
{
public:
	typedef std::initializer_list<string> initlist;
	vector<string> ary;
	dict_ci<int> mVal;

	enumap_ci(initlist items) : ary{ items }
	{
		int index = 0;
		for (auto& it : items)
		{
			mVal.emplace(it, index++);
		}
	}

	[[nodiscard]] bool count(const string& val) const
	{
		return mVal.find(val) != mVal.end();
	}

	size_t operator[](const string& val) const
	{
		if (auto it = mVal.find(val); it != mVal.end())return it->second;
		return -1;
	}

	const string& operator[](size_t index) const
	{
		return ary[index];
	}
};

template <typename TKey, typename TCon>
class multi_range
{
public:
	typedef typename TCon::const_iterator iterator;
	std::pair<iterator, iterator> range;

	multi_range(const TCon& mmap, const TKey& key) : range(mmap.equal_range(key))
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

class ShowList {
	vector<string>list;
	size_t len{ 0 };
public:
	ShowList& operator<<(const string& item) {
		list.push_back(item);
		len += item.length();
		return *this;
	}
	bool empty()const { return list.empty(); }
	size_t size()const { return list.size(); }
	size_t length()const { return len; }
	string show(const string& sepa = "|") {
		string res;
		for (auto it = list.begin(); it != list.end(); ++it) {
			if (it != list.begin())res += sepa;
			res += *it;
		}
		return res;
	}
};
template<class Con>
std::string listID(const Con& list, const string& sepa = "|") {
	ShowList res;
	for (auto id : list) {
		res << to_string(id);
	}
	return res.show(sepa);
}
template<class Con>
std::string listIndex(const Con& list, const string& sepa = "\n") {
	ShowList res;
	for (auto id : list) {
		res << id.to_string();
	}
	return res.show(sepa);
}
template<class Con>
std::string listItem(const Con& list, const string& sepa = "|") {
	ShowList res;
	for (auto id : list) {
		res << id;
	}
	return res.show(sepa);
}


//按优先级输出项目
template<typename Elem>
class PriorList{
	std::priority_queue<std::pair<Elem, string>> qItem;
public:
	template<class Con>
	PriorList(const Con& mItem)
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

