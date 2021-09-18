#include <CivetServer.h>
#include <fstream>
#include "filesystem.hpp"
#include "DiceManager.h"


bool setPassword(const std::string& password)
{
    // mg_modify_passwords_file似乎只接受ascii路径而不是其他函数的utf-8路径
    // anyway, try both
    if (!mg_modify_passwords_file(WebUIPasswordPath.u8string().c_str(), "DiceWebUI", "admin", password.c_str()))
    {
        std::error_code ec;
        if (!std::filesystem::exists(WebUIPasswordPath, ec))
        {
            ofstream(WebUIPasswordPath).close();
        }
        return mg_modify_passwords_file(getNativePathString(WebUIPasswordPath).c_str(), "DiceWebUI", "admin", password.c_str());
    }
    return true; 
}