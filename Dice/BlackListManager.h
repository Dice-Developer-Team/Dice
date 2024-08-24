#pragma once
/**
 * 黑名单明细
 * 更数据库式的管理
 * Copyright (C) 2019-2024 String.Empty
 */

#include <utility>
#include "DiceAttrVar.h"

using std::pair;
using std::unordered_set;
using std::unordered_map;
using std::multimap;

class DiceEvent;

enum class BlackID { fromGID, fromUID, inviterQQ, ownerQQ };

class DDBlackMark
{
	//不良记录类型:null,kick,ban,spam,other,ruler,local,extern;枚举之外一律非法
	string type = "null";
	string time;
	//fromGID,fromUID,inviterQQ,ownerQQ
	using item = pair<long long, bool>;
	item fromGID{0, false};
	item fromUID{0, false};
	item inviterQQ{0, false};
	item ownerQQ{0, false};
	//
	long long DiceMaid = 0;
	long long masterID = 0;
	//危险等级
	short danger = 0;
	//唯一云端编号（0表示不对应云端）
	int wid = 0;
	//说明
	string note;
	//附加备注
	string comment{};
	void erase();
	void upload();
	int check_cloud();
public:
	DDBlackMark(long long, long long);
	DDBlackMark(const fifo_json&);
	DDBlackMark(const string&);
	friend class DDBlackManager;
	friend class DDBlackMarkFactory;
	//warning文本生成
	[[nodiscard]] string printJson(int tab) const;
	[[nodiscard]] string warning() const;
	[[nodiscard]] string getData() const;
	void fill_note();
	//是否合法构造
	bool isValid = false;
	bool isClear = false;
	[[nodiscard]] bool isType() const;
	[[nodiscard]] bool isType(const string& strType) const;
	[[nodiscard]] bool isSame(const DDBlackMark&) const;
	[[nodiscard]] bool isSource(long long) const;
    void check_remit();
	DDBlackMark& operator<<(const DDBlackMark&);
	//bool operator<(const DDBlackMark&)const;
};

class DDBlackManager
{
	std::vector<DDBlackMark> vBlackList;
	//云端编号映射表
	unordered_map<int, unsigned int> mCloud;
	unordered_set<unsigned int> sIDEmpty;
	//可重复映射表
	multimap<string, unsigned int> mTimeIndex;
	unordered_set<unsigned int> sTimeEmpty;
	unordered_set<unsigned int> sGroupEmpty;
	unordered_set<unsigned int> sQQEmpty;
	//发现所指相同的记录
	int find(const DDBlackMark&);
	//更新记录
	bool insert(DDBlackMark&);
	bool update(DDBlackMark&, unsigned int, int);
	void reset_group_danger(long long);
	void reset_qq_danger(long long);
	bool up_group_danger(long long, DDBlackMark&);
	bool up_qq_danger(long long, DDBlackMark&);
	// bool isActive = false;
public:
	multimap<long long, unsigned int> mGroupIndex;
	multimap<long long, unsigned int> mQQIndex;
	//未注销黑名单的危险等级
	unordered_map<long long, short> mQQDanger;
	unordered_map<long long, short> mGroupDanger;
	[[nodiscard]] short get_group_danger(long long) const;
	[[nodiscard]] short get_user_danger(long long) const;
	void isban(DiceEvent*);
	string list_group_warning(long long);
	string list_qq_warning(long long);
	string list_self_group_warning(long long);
	string list_self_qq_warning(long long);
	void add_black_group(long long, DiceEvent*);
	void add_black_qq(long long, DiceEvent*);
	void rm_black_group(long long, DiceEvent*);
	void rm_black_qq(long long, DiceEvent*);
	void verify(const fifo_json&, long long);
	void create(DDBlackMark&);
	//读取json格式黑名单记录
	int loadJson(const std::filesystem::path& fpPath, bool isExtern = false);
	//读取旧版本黑名单列表
	int loadHistory(const std::filesystem::path& fpLoc);
	void saveJson(const std::filesystem::path& fpPath) const;
};

extern std::unique_ptr<DDBlackManager> blacklist;

class DDBlackMarkFactory
{
	DDBlackMark mark;
	using Factory = DDBlackMarkFactory;
public:
	DDBlackMarkFactory(long long qq, long long group): mark(qq, group)
	{
		mark.comment.clear();
	}

	Factory& type(string strType)
	{
		mark.type = std::move(strType);
		return *this;
	}

	Factory& time(string strTime)
	{
		mark.time = std::move(strTime);
		return *this;
	}

	Factory& note(string strNote)
	{
		mark.note = std::move(strNote);
		return *this;
	}

	Factory& comment(string strNote) 
	{
		mark.comment = strNote;
		return *this;
	}

	Factory& inviter(long long qq)
	{
		mark.inviterQQ = {qq, true};
		return *this;
	}

	Factory& fromUID(long long qq)
	{
		mark.fromUID = {qq, true};
		return *this;
	}

	Factory& DiceMaid(long long qq)
	{
		mark.DiceMaid = qq;
		return *this;
	}

	Factory& master(long long qq)
	{
		mark.masterID = qq;
		return *this;
	}

	Factory& sign();

	Factory& upload()
	{
		mark.upload();
		return *this;
	}

	DDBlackMark& product()
	{
		return mark;
	}
};

void AddWarning(const ptr<AnysTable>& warning);
void warningHandler();
