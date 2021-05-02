#include "DiceZip.h"
#include "filesystem.hpp"
#include <string>
#include <zip.h>
#include <fstream>
namespace fs = std::filesystem;

namespace Zip
{

    class Unzipper
    {
        // zip错误
        zip_error_t error;

        // 错误信息/文件缓存
        char buf[256];

        // 原zip文件字节
        std::string src;

        // 目标文件夹
        fs::path destFolder;

        // zip文件结构
        zip_t* zip = nullptr;

    public:
        Unzipper(const std::string& src, const fs::path& destFolder) : src(src), destFolder(destFolder)
        {
            // 初始化zip_error
            zip_error_init(&error);

            // 创建zip_source
            zip_source_t* source = zip_source_buffer_create(this->src.c_str(), this->src.size() * sizeof(char), 0, &error);
            if (error.zip_err != ZIP_ER_OK)
            {
                throw ZipExtractionFailedException(zip_error_strerror(&error));
            }
            // 打开zip
            zip = zip_open_from_source(source, ZIP_RDONLY, &error);
            if (error.zip_err != ZIP_ER_OK)
            {
                throw ZipExtractionFailedException(zip_error_strerror(&error));
            }
        }
        ~Unzipper()
        {
            // 如果已经打开zip，释放内存并抛弃一切更改
            if(zip) zip_discard(zip);

            // 释放zip_error
            zip_error_fini(&error);
        }

        void extractAll()
        {
            // 枚举所有文件
            for (int i = 0; i != zip_get_num_entries(zip, 0); i++)
            {
                // 初始化文件信息结构体
                zip_stat_t stat;
                zip_stat_init(&stat);

                // 获取信息
                if (zip_stat_index(zip, i, 0, &stat) == -1)
                {
                    throw ZipExtractionFailedException(zip_strerror(zip));
                }

                // 需要文件名称和大小
                if(!(stat.valid & ZIP_STAT_NAME))
                {
                    throw ZipExtractionFailedException("Failed to get file name");
                }

                if(!(stat.valid & ZIP_STAT_SIZE))
                {
                    throw ZipExtractionFailedException("Failed to get file size");
                }

                std::string name = stat.name;
                if (name.empty())
                {
                    throw ZipExtractionFailedException("Failed to get file name");
                }

                // 如果是文件夹则创建，然后跳到下个文件
                if (name[name.length() - 1] == '/')
                {
                    std::error_code ec;
                    fs::create_directories(destFolder / name, ec);
                    continue;
                }

                fs::path path = destFolder / name;
                std::error_code ec;
                fs::create_directories(path.parent_path(), ec);
                // 打开输出文件
                std::ofstream f(path, std::ios::out | std::ios::trunc | std::ios::binary);

                if (!f)
                {
                    throw std::runtime_error("Failed to open file for writing");
                }

                // 打开压缩文件
                zip_file* file = zip_fopen_index(zip, i, 0);
                if (!file)
                {
                    throw ZipExtractionFailedException(zip_strerror(zip));
                }

                // 解压并写入文件
                zip_uint64_t written_size = 0;
                while (stat.size != written_size)
                {
                    zip_int64_t bytes_read = zip_fread(file, buf, sizeof(buf));
                    if (bytes_read == -1)
                    {
                        zip_fclose(file);
                        throw ZipExtractionFailedException("Failed to extract file " + name);
                    }
                    f << std::string(buf, bytes_read);
                    written_size += bytes_read;
                }
                zip_fclose(file);
            }
        }
    };

    void extractZip(const std::string& src, const fs::path& destFolder)
    {
        Unzipper unzip(src, destFolder);
        unzip.extractAll();
    }
}
