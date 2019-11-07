/*
 * 后台系统
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#include <set>
#include <map>
#include <vector>
#include "DiceFile.hpp"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "CardDeck.h"
using std::string;
using std::to_string;
using std::set;
using std::map;
using std::vector;
constexpr auto CQ_IMAGE = "[CQ:image,file=";

//加载数据
void loadData();
//保存数据
void dataBackUp();
//被引用的图片列表
static set<string> sReferencedImage;
static map<long long, string> WelcomeMsg;

static void scanImage(string s, set<string>& list) {
	int l = 0, r = 0;
	while ((l = s.find('[', r)) != string::npos && (r = s.find(']', l)) != string::npos) {
		if (s.substr(l, 15) != CQ_IMAGE)continue;
		list.insert(s.substr(l + 15, r - l - 15) + ".cqimg");
	}
}
static void scanImage(const vector<string>& v, set<string>& list) {
	for (auto it : v) {
		scanImage(it, sReferencedImage);
	}
}
template<typename TKey,typename TVal>
static void scanImage(const map<TKey, TVal>& m, set<string>& list) {
	for (auto it : m) {
		scanImage(it.second, sReferencedImage);
	}
}

static int clearImage() {
	scanImage(GlobalMsg, sReferencedImage);
	scanImage(HelpDoc, sReferencedImage);
	scanImage(CardDeck::mPublicDeck, sReferencedImage);
	scanImage(CardDeck::mReplyDeck, sReferencedImage);
	scanImage(CardDeck::mGroupDeck, sReferencedImage);
	scanImage(CardDeck::mPrivateDeck, sReferencedImage);
	scanImage(WelcomeMsg, sReferencedImage);
	string strLog = "整理被引用图片" + to_string(sReferencedImage.size()) + "项";
	addRecord(strLog);
	return clrDir("data\\image\\", sReferencedImage);
}