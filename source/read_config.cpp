#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "read_config.h"
#include "common.h"
#include "log.h"

//#map=this注释
// map=that有效项

std::vector<std::string> split_boost(const std::string &str, const std::string &pattern)
{
    std::vector<std::string> v_str;
    boost::split(v_str, str, boost::is_any_of(pattern), boost::token_compress_on);
    return v_str;
}

// std::vector<std::string> split(const std::string &str, const std::string &pattern)
// {
//     char *strc = new char[strlen(str.c_str()) + 1];
//     strcpy(strc, str.c_str());
//     std::vector<std::string> result_vec;
//     char *tmpStr = strtok(strc, pattern.c_str());
//     while (tmpStr != NULL)
//     {
//         result_vec.push_back(std::string(tmpStr));
//         tmpStr = strtok_r(NULL, pattern.c_str(), &tmpStr);
//     }
//     delete[] strc;
//     return result_vec;
// };

ReadConfig::ReadConfig(const std::string &file_name)
{
    _file_name = file_name;
    read_all_config();
}

ReadConfig::~ReadConfig()
{
    write_all_config();
    if (_fp)
    {
        fclose(_fp);
        _fp = nullptr;
    }
}

std::string ReadConfig::read_config(const std::string &key)
{
    std::string value;
    {
        return value;
    }
    auto itor = _config_map.find(key);
    if (itor != _config_map.end())
    {
        value = itor->second;
        return value;
    }
    return value;
}

bool ReadConfig::write_config(const std::string &key, const std::string &value)
{
    _config_map[key] = value;
    return true;
}

bool ReadConfig::read_all_config()
{
    _fp = fopen(_file_name.c_str(), "rb");
    if (!_fp)
    {
        log_print(LOG_ERROR, u8"read config failed, errno:%d", errno);
        return false;
    }
    char line[1024] = {0};
    while (fgets(line, sizeof(line) - 1, _fp))
    {
        std::vector<std::string> tmp_vec = split_boost(line, u8"=");
        if (tmp_vec.size() == 2)
        {
            auto tmp_pos = tmp_vec[1].find_first_of(u8"\r\n");
            if (tmp_pos != std::string::npos)
            {
                tmp_vec[1] = tmp_vec[1].substr(0, tmp_pos);
            }
            _config_map[tmp_vec[0]] = tmp_vec[1];
        }
        memset(line, '\0', sizeof(line));
    }
    fclose(_fp);
    _fp = nullptr;
    return true;
}

bool ReadConfig::write_all_config()
{
    _fp = fopen(_file_name.c_str(), "wb");
    if (!_fp)
    {
        log_print(LOG_ERROR, u8"write config failed, errno:%d", errno);
        return false;
    }
    for (const auto &item : _config_map)
    {
        fprintf(_fp, u8"%s=%s\n", item.first.c_str(), item.second.c_str());
    }
    fclose(_fp);
    _fp = nullptr;
    return true;
}
