#include "custom_data.h"
#include "log.h"

bool CustomData::from_data(const uint8_t *data, const uint32_t len)
{
    if (!data)
        return false;
    std::string str_data((const char *)data, len);
    std::vector<std::string> vec;
    common::split_str(str_data, vec);
    if (vec.size() % 2 != 0)
        return false;
    bool is_key = true;
    std::string key, str;
    uint64_t num = 0;
    for (const auto &item : vec)
    {
        if (is_key)
        {
            key = common::custom_get_str(item, "k:");
        }
        else
        {
            if (!key.empty())
            {
                str = common::custom_get_str(item, "s:");
                if (str.empty())
                {
                    num = common::custom_get_num(item, "n:");
                    if (!num)
                        num_map[key] = num;
                }
                else
                {
                    str_map[key] = str;
                }
            }
            else
            {
                return false;
            }
        }
        is_key = !is_key;
    }
    if (size())
        data_len = len;
    return true;
}

uint32_t CustomData::size()
{
    return (uint32_t)(str_map.size() + num_map.size());
}

uint32_t CustomData::to_data(uint8_t *data, const uint32_t len)
{
    uint32_t index = 0;
    if (!data || len < length())
        return index;
    std::string str;
    for (const auto &item : str_map)
    {
        if (!str.empty())
            str.push_back(',');
        str.append("k:");
        str.append(item.first);
        str.push_back(',');
        str.append("s:");
        str.append(item.second);
    }
    if (len < index + str.size())
        return index;
    memcpy(data + index, str.c_str(), str.size());
    index += (uint32_t)str.size();
    bool is_append = !str.empty();
    for (const auto &item : num_map)
    {
        std::string tmp_str;
        if (is_append)
            tmp_str.push_back(',');
        tmp_str.append("k:");
        tmp_str.append(item.first);
        tmp_str.push_back(',');
        tmp_str.append("n:");
        if (len < index + tmp_str.size())
        {
            index = 0;
            return index;
        }
        memcpy(data + index, tmp_str.c_str(), tmp_str.size());
        index += (uint32_t)tmp_str.size();
        if (len < index + sizeof(item.second))
        {
            index = 0;
            return index;
        }
        index += common::memcpy_u(data + index, item.second);
        is_append = true;
    }
    if (length() != index)
    {
        log_print(LOG_ERROR, u8"自定义变量封包错误!");
        index = 0;
    }
    return index;
}

bool CustomData::add_custom(std::string key, std::string str)
{
    if (key.empty())
        return false;
    if (key.find(':') != std::string::npos || key.find(',') != std::string::npos)
        return false;
    if (str.find(':') != std::string::npos || str.find(',') != std::string::npos || str.empty())
        return false;
    auto itor = str_map.find(key);
    if (itor != str_map.end())
        delete_custom_str(key);
    if (str.empty())
        return true;
    str_map[key] = str;
    if (data_len != 0)
        ++data_len;                             //加上两个键值之间的','
    data_len += (uint32_t)key.size() + sizeof("k:,") - 1; //key.size()不含'\0',sizeof("k:,")包含'\0'
    data_len += (uint32_t)str.size() + sizeof("s:") - 1;
    return true;
}

bool CustomData::add_custom(std::string key, uint64_t num)
{
    if (key.empty())
        return false;
    if (key.find(':') != std::string::npos || key.find(',') != std::string::npos)
        return false;
    if (!num)
        return false;
    auto itor = num_map.find(key);
    if (itor != num_map.end())
        delete_custom_num(key);
    if (!num)
        return true;
    num_map[key] = num;
    if (data_len != 0)
        ++data_len; //加上两个键值之间的','
    data_len += (uint32_t)key.size() + sizeof("k:,") - 1;
    data_len += sizeof(num) + sizeof("n:") - 1;
    return true;
}

bool CustomData::delete_custom_str(std::string key)
{
    auto str_itor = str_map.find(key);
    if (str_itor != str_map.end())
    {
        data_len -= (uint32_t)key.size() + sizeof("k:,") - 1;
        data_len -= (uint32_t)(str_map[key].size()) + sizeof("s:") - 1;
        if (data_len)
            --data_len;
        str_map.erase(str_itor);
    }
    return true;
}

bool CustomData::delete_custom_num(std::string key)
{
    auto num_itor = num_map.find(key);
    if (num_itor != num_map.end())
    {
        data_len -= (uint32_t)key.size() + sizeof("k:,") - 1;
        data_len -= sizeof(num_map[key]) + sizeof("n:") - 1;
        if (data_len)
            --data_len;
        num_map.erase(num_itor);
    }
    return true;
}

std::string CustomData::get_custom_str(std::string key)
{
    auto str_itor = str_map.find(key);
    if (str_itor == str_map.end())
        return "";
    return str_itor->second;
}

uint64_t CustomData::get_custom_num(std::string key)
{
    auto num_itor = num_map.find(key);
    if (num_itor == num_map.end())
        return 0;
    return num_itor->second;
}