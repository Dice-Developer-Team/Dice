#pragma once
/**
 * ���ű����õ��Զ���������
 * 2022/8/17 ��չSelfData��ȫ�ַ�ΧӦ��
 * 2022/12/10 ֧��toml��д
 * 2023/3/14 ֧��toml��д
 */
#include <mutex>
#include <filesystem>
#include "DiceAttrVar.h"
#include "STLExtern.hpp"
class SelfData : public std::enable_shared_from_this<SelfData> {
	enum FileType { Bin, Json, Toml, Yaml};
	FileType type{ Bin };
	std::filesystem::path pathFile;
public:
	std::mutex exWrite;
	SelfData(const std::filesystem::path& p);
	AttrVar data;
	void save();
};
//filename stem by native charset
extern dict<ptr<SelfData>> selfdata_byFile;
//filename stem by gbk charset
extern dict<ptr<SelfData>> selfdata_byStem;
