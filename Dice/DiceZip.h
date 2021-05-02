#pragma once
#include <string>
#include <stdexcept>
#include "filesystem.hpp"

namespace Zip
{
    // Zip提取失败异常
    class ZipExtractionFailedException : public std::runtime_error
    {
    public:
        ZipExtractionFailedException(const std::string& what) : std::runtime_error("Failed to extract zip: " + what) {}
    };

    // 提取一个Zip文件
    // @param src 内存中的zip文件字节流
    // @param destFolder 目标文件夹, 请先保证此文件夹存在
    // @throws ZipExtractionFailedException
    void extractZip(const std::string& src, const std::filesystem::path& destFolder);
}