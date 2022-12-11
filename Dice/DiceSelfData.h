#pragma once
/**
 * 供lua调用的类
 * 2022/8/17 扩展SelfData到全局范围应用
 * 2022/12/10 支持toml读写
 */
#include <mutex>
#include <filesystem>
#include "DiceAttrVar.h"
#include "STLExtern.hpp"
class SelfData : public std::enable_shared_from_this<SelfData> {
	enum FileType { Bin, Json, Toml};
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
