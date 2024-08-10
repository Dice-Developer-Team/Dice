#pragma once
/**
 * ��������ϸ
 * �����ݿ�ʽ�Ĺ���
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
	//������¼����:null,kick,ban,spam,other,ruler,local,extern;ö��֮��һ�ɷǷ�
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
	//Σ�յȼ�
	short danger = 0;
	//Ψһ�ƶ˱�ţ�0��ʾ����Ӧ�ƶˣ�
	int wid = 0;
	//˵��
	string note;
	//���ӱ�ע
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
	//warning�ı�����
	[[nodiscard]] string printJson(int tab) const;
	[[nodiscard]] string warning() const;
	[[nodiscard]] string getData() const;
	void fill_note();
	//�Ƿ�Ϸ�����
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
	//�ƶ˱��ӳ���
	unordered_map<int, unsigned int> mCloud;
	unordered_set<unsigned int> sIDEmpty;
	//���ظ�ӳ���
	multimap<string, unsigned int> mTimeIndex;
	unordered_set<unsigned int> sTimeEmpty;
	unordered_set<unsigned int> sGroupEmpty;
	unordered_set<unsigned int> sQQEmpty;
	//������ָ��ͬ�ļ�¼
	int find(const DDBlackMark&);
	//���¼�¼
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
	//δע����������Σ�յȼ�
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
	//��ȡjson��ʽ��������¼
	int loadJson(const std::filesystem::path& fpPath, bool isExtern = false);
	//��ȡ�ɰ汾�������б�
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
