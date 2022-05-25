#pragma once

#include <exception>
#include <stdexcept>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <fstream>
#include "json.hpp"
#include "filesystem.hpp"
#include "DiceNetwork.h"
#include "DiceZip.h"
#include "DiceExtensionManager.h"
#include "EncodingConvert.h"

extern std::filesystem::path DiceDir;
void loadData();

// Currently support three different types of extensions
enum class ExtensionType
{
	Deck,
	Lua,
	CardTemplate
};

constexpr const char* ExtensionTypeToString(const ExtensionType& e)
{
	switch (e)
	{
	case ExtensionType::Deck:
		return "Deck";
	case ExtensionType::Lua:
		return "Lua";
	case ExtensionType::CardTemplate:
		return "CardTemplate";
	}
	return "";
}

NLOHMANN_JSON_SERIALIZE_ENUM( ExtensionType, {
	{ ExtensionType::Deck, ExtensionTypeToString(ExtensionType::Deck) },
	{ ExtensionType::Lua, ExtensionTypeToString(ExtensionType::Lua) },
	{ ExtensionType::CardTemplate, ExtensionTypeToString(ExtensionType::CardTemplate) }
})

// 插件信息类
// Json serializable
struct ExtensionInfo
{
	// 插件名称
	std::string name;
	// 插件版本，字符串
	// 如：1.2.3
	std::string version;
	// 插件版本号，整数
	// 作为更新依据
	int version_code;
	// 描述
	std::string desc;
	// 下载链接
	std::string download_link;
	// 作者
	std::string author;
	// 类型
	ExtensionType type;

	// 返回GB18030编码
	operator std::string() const
	{
		std::stringstream ss;
		ss << "名称：" << UTF8toGBK(name) << std::endl;
		ss << "版本：" << UTF8toGBK(version) << std::endl;
		ss << "描述：" << UTF8toGBK(desc) << std::endl;
		ss << "作者：" << UTF8toGBK(author) << std::endl;
		ss << "类型：" << UTF8toGBK(ExtensionTypeToString(type)) << std::endl;
		return ss.str();
	}
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExtensionInfo, name, version, version_code, desc, download_link, author, type);

// 插件管理类，提供安装卸载等功能，请注意需要使用UTF-8编码
class ExtensionManager 
{
	// 服务器插件列表缓存
	std::map<std::string, ExtensionInfo> _index;
	std::shared_mutex _indexMutex;

	// 已安装插件列表
	std::map<std::string, std::pair<ExtensionInfo, std::filesystem::path>> _installedIndex;
	std::shared_mutex _installedIndexMutex;

	// 安装目录
	std::filesystem::path deckPath = DiceDir / "PublicDeck";
	std::filesystem::path luaPath = DiceDir / "plugin";
	std::filesystem::path cardTemplatePath = DiceDir / "CardTemp";

public:
	// 从服务器获取最新插件列表
	// @throws ConcurrentIndexRefreshException, IndexRefreshFailedException
	void refreshIndex()
	{
		std::unique_lock lock(_indexMutex, std::try_to_lock);
		if (!lock.owns_lock())
		{
			throw ConcurrentIndexRefreshException();
		}

		// get&parse
		std::string des;
		if (!Network::GET("https://raw.sevencdn.com/Dice-Developer-Team/DiceExtensions/main/index.json", des))
		{
			throw IndexRefreshFailedException("Network: " + des);
		}

		try
		{
			nlohmann::json j = nlohmann::json::parse(des);
			_index.clear();
			for (const auto& item : j)
			{
				_index[item["name"].get<std::string>()] = item.get<ExtensionInfo>();
			}
		}
		catch(const std::exception& e)
		{
			throw IndexRefreshFailedException(std::string("Json Parse: ") + e.what());
		}
	}

	// 获取某个类型插件应该安装的位置
	// @param e 对应插件的插件信息
	// @returns 插件应该被安装的位置
	std::filesystem::path getInstallPath(const ExtensionInfo& e)
	{
		string name = e.name;
#ifdef _WIN32
		name = UTF8toGBK(name);
#endif
		if (e.type == ExtensionType::CardTemplate)
		{
			return cardTemplatePath / name;
		}
		if (e.type == ExtensionType::Deck)
		{
			return deckPath / name;
		}
		if (e.type == ExtensionType::Lua)
		{
			return luaPath / name;
		}
		return "";
	}

	// 获取Index中的包个数
	// @returns Index中的包的个数
	size_t getIndexCount()
	{
		std::shared_lock lock(_indexMutex);
		return _index.size();
	}

	// 获取当前Index信息
	// @returns Index的一份复制
	std::map<std::string, ExtensionInfo> getIndex()
	{
		std::shared_lock lock(_indexMutex);
		return _index;
	}

	// 获取已安装Index信息
	// @returns Index的一份复制
	std::map<std::string, std::pair<ExtensionInfo, std::filesystem::path>> getInstalledIndex()
	{
		std::shared_lock lock(_indexMutex);
		return _installedIndex;
	}

	// 写出插件信息
	// @param e 对应插件的插件信息
	// @param p 写出路径
	void writeExtensionInfo(const ExtensionInfo& e, const std::filesystem::path& p)
	{
		std::ofstream f(p);
		f << nlohmann::json(e).dump();
	}

	// 查询插件信息
	// @param name 插件名称，UTF-8编码
	// @returns 插件信息
	// @throws ExtensionNotFoundException
	ExtensionInfo queryPackage(const std::string& name)
	{
		std::shared_lock lock(_indexMutex);
		if (_index.count(name))
		{
			return _index.at(name);
		}
		throw ExtensionNotFoundException(name);
	}

	// 查询已安装拓展信息
	// @param name 插件名称，UTF-8编码
	// @returns pair<插件信息，路径>
	// @throws ExtensionNotFoundException
	std::pair<ExtensionInfo, std::filesystem::path> queryInstalledPackage(const std::string& name)
	{
		std::shared_lock lock(_installedIndexMutex);
		if (_installedIndex.count(name))
		{
			return _installedIndex.at(name);
		}
		throw ExtensionNotFoundException(name);
	}

	// 安装拓展，如果拓展已被安装会卸载原有拓展
	// @param name 拓展名称，UTF-8编码
	// @param reloadData 是否自动重载数据
	// @throws ExtensionNotFoundException, PackageInstallFailedException, ZipExtractionFailedException and maybe other exceptions
	void installPackage(const std::string& name, bool reloadData = true)
	{   
		ExtensionInfo ext = queryPackage(name);
		try 
		{
			removePackage(name, false);
		}
		catch (const ExtensionNotFoundException&) {}
		std::string des;

		if (!Network::GET("https://raw.sevencdn.com/Dice-Developer-Team/DiceExtensions/main/" + UrlEncode(ext.name) + ".zip", des))
		{
			throw PackageInstallFailedException("Network: " + des);
		}

		std::filesystem::path installPath = getInstallPath(ext);
		std::error_code ec1;
		std::filesystem::create_directories(installPath, ec1);
		Zip::extractZip(des, installPath);
		writeExtensionInfo(ext, installPath / ".info.json");
		if(reloadData) loadData();
	}

	// 升级拓展
	// @param name 拓展名称，UTF-8编码
	// @param reloadData 是否自动重载数据
	// @returns 升级是否被执行
	// @throws ExtensionNotFoundException, PackageInstallFailedException, ZipExtractionFailedException and maybe other exceptions
	bool upgradePackage(const std::string& name, bool reloadData = true)
	{
		ExtensionInfo ext = queryPackage(name);
		auto [currExt, _] = queryInstalledPackage(name);
		if (currExt.version_code < ext.version_code)
		{
			installPackage(name, reloadData);
			return true;
		}
		return false;
	}

	// 卸载拓展
	// @param name 拓展名称，UTF-8编码
	// @param reloadData 是否自动重载数据
	void removePackage(const std::string& name, bool reloadData = true)
	{
		auto [info, path] = queryInstalledPackage(name);
		std::error_code ec;
		std::filesystem::remove_all(path, ec);
		if(reloadData) loadData();
	}

	// 向InstalledIndex中添加某个拓展
	// @param info 插件信息
	// @param path 插件路径
	void addInstalledPackage(const ExtensionInfo& info, const std::filesystem::path& path)
	{
		std::unique_lock lock(_installedIndexMutex);
		_installedIndex[info.name] = std::make_pair(info, path);
	}

	// 从InstalledIndex中删除某个拓展，不实际删除文件
	// @param info 插件信息
	void removeInstalledPackage(const ExtensionInfo& info)
	{
		std::unique_lock lock(_installedIndexMutex);
		if (_installedIndex.count(info.name))
		{
			_installedIndex.erase(info.name);
		}
	}

	// 获取可升级数量
	// @returns 可升级的拓展包数量
	int getUpgradableCount()
	{
		std::shared_lock lock1(_installedIndexMutex, std::defer_lock);
		std::shared_lock lock2(_indexMutex, std::defer_lock);
		std::lock(lock1, lock2);

		int cnt = 0;

		for (const auto& i : _installedIndex)
		{
			if(_index.count(i.second.first.name) && _index[i.second.first.name].version_code > i.second.first.version_code)
			{
				cnt++;
			}
		}

		return cnt;
	}

	// 升级所有拓展
	// @returns 升级拓展数量
	int upgradeAllPackages()
	{
		int cnt = 0;
		for (const auto& i : getInstalledIndex())
		{
			try
			{
				if (upgradePackage(i.second.first.name, false))
				{
					cnt++;
				}
			}
			catch(const ExtensionNotFoundException&) {}
		}
		if (cnt) loadData();
		return cnt;
	}

	class ExtensionNotFoundException : public std::runtime_error
	{
	public:
		ExtensionNotFoundException(const std::string& what) : std::runtime_error("Unable to find package with name " + what) {}
	};

	class ConcurrentIndexRefreshException : public std::runtime_error
	{
	public:
		ConcurrentIndexRefreshException() : std::runtime_error("Failed to acquire lock: Another thread is refreshing the index") {}
	};

	class IndexRefreshFailedException : public std::runtime_error
	{
	public:
		IndexRefreshFailedException(const std::string& what) : std::runtime_error("Failed to refresh index: " + what) {}
	};

	class PackageInstallFailedException : public std::runtime_error
	{
	public:
		PackageInstallFailedException(const std::string& what) : std::runtime_error("Failed to install package: " + what) {}
	};
};