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

// �����Ϣ��
// Json serializable
struct ExtensionInfo
{
    // �������
    std::string name;
    // ����汾���ַ���
    // �磺1.2.3
    std::string version;
    // ����汾�ţ�����
    // ��Ϊ��������
    int version_code;
    // ����
    std::string desc;
    // ��������
    std::string download_link;
    // ����
    std::string author;
    // ����
    ExtensionType type;

    // ����GB18030����
    operator std::string() const
    {
        std::stringstream ss;
        ss << "���ƣ�" << UTF8toGBK(name) << std::endl;
        ss << "�汾��" << UTF8toGBK(version) << std::endl;
        ss << "������" << UTF8toGBK(desc) << std::endl;
        ss << "���ߣ�" << UTF8toGBK(author) << std::endl;
        ss << "���ͣ�" << UTF8toGBK(ExtensionTypeToString(type)) << std::endl;
        return ss.str();
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExtensionInfo, name, version, version_code, desc, download_link, author, type);

// ��������࣬�ṩ��װж�صȹ��ܣ���ע����Ҫʹ��UTF-8����
class ExtensionManager 
{
    // ����������б���
    std::map<std::string, ExtensionInfo> _index;
    std::shared_mutex _indexMutex;

    // �Ѱ�װ����б�
    std::map<std::string, std::pair<ExtensionInfo, std::filesystem::path>> _installedIndex;
    std::shared_mutex _installedIndexMutex;

    // ��װĿ¼
    std::filesystem::path deckPath = DiceDir / "PublicDeck";
    std::filesystem::path luaPath = DiceDir / "plugin";
    std::filesystem::path cardTemplatePath = DiceDir / "CardTemp";

public:
    // �ӷ�������ȡ���²���б�
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

    // ��ȡĳ�����Ͳ��Ӧ�ð�װ��λ��
    // @param e ��Ӧ����Ĳ����Ϣ
    // @returns ���Ӧ�ñ���װ��λ��
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

    // ��ȡIndex�еİ�����
    // @returns Index�еİ��ĸ���
    size_t getIndexCount()
    {
        std::shared_lock lock(_indexMutex);
        return _index.size();
    }

    // ��ȡ��ǰIndex��Ϣ
    // @returns Index��һ�ݸ���
    std::map<std::string, ExtensionInfo> getIndex()
    {
        std::shared_lock lock(_indexMutex);
        return _index;
    }

    // ��ȡ�Ѱ�װIndex��Ϣ
    // @returns Index��һ�ݸ���
    std::map<std::string, std::pair<ExtensionInfo, std::filesystem::path>> getInstalledIndex()
    {
        std::shared_lock lock(_indexMutex);
        return _installedIndex;
    }

    // д�������Ϣ
    // @param e ��Ӧ����Ĳ����Ϣ
    // @param p д��·��
    void writeExtensionInfo(const ExtensionInfo& e, const std::filesystem::path& p)
    {
        std::ofstream f(p);
        f << nlohmann::json(e).dump();
    }

    // ��ѯ�����Ϣ
    // @param name ������ƣ�UTF-8����
    // @returns �����Ϣ
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

    // ��ѯ�Ѱ�װ��չ��Ϣ
    // @param name ������ƣ�UTF-8����
    // @returns pair<�����Ϣ��·��>
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

    // ��װ��չ�������չ�ѱ���װ��ж��ԭ����չ
    // @param name ��չ���ƣ�UTF-8����
    // @param reloadData �Ƿ��Զ���������
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

    // ������չ
    // @param name ��չ���ƣ�UTF-8����
    // @param reloadData �Ƿ��Զ���������
    // @returns �����Ƿ�ִ��
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

    // ж����չ
    // @param name ��չ���ƣ�UTF-8����
    // @param reloadData �Ƿ��Զ���������
    void removePackage(const std::string& name, bool reloadData = true)
    {
        auto [info, path] = queryInstalledPackage(name);
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
        if(reloadData) loadData();
    }

    // ��InstalledIndex�����ĳ����չ
    // @param info �����Ϣ
    // @param path ���·��
    void addInstalledPackage(const ExtensionInfo& info, const std::filesystem::path& path)
    {
        std::unique_lock lock(_installedIndexMutex);
        _installedIndex[info.name] = std::make_pair(info, path);
    }

    // ��InstalledIndex��ɾ��ĳ����չ����ʵ��ɾ���ļ�
    // @param info �����Ϣ
    void removeInstalledPackage(const ExtensionInfo& info)
    {
        std::unique_lock lock(_installedIndexMutex);
        if (_installedIndex.count(info.name))
        {
            _installedIndex.erase(info.name);
        }
    }

    // ��ȡ����������
    // @returns ����������չ������
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

    // ����������չ
    // @returns ������չ����
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