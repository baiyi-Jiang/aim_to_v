#pragma once
#include "struct_def.h"

class CustomData
{
public:
    CustomData() {}
    ~CustomData() {}
    bool from_data(const uint8_t *data, const uint32_t len);
    uint32_t size();                       //键值对个数
    uint32_t length() { return data_len; } //流化后的长度
    uint32_t to_data(uint8_t *data, const uint32_t len);
    bool add_custom(std::string key, std::string str);
    bool add_custom(std::string key, uint64_t num);
    bool delete_custom_str(std::string key);
    bool delete_custom_num(std::string key);
    std::string get_custom_str(std::string key);
    uint64_t get_custom_num(std::string key);

private:
    std::map<std::string, std::string> str_map;
    std::map<std::string, uint64_t> num_map;
    uint32_t data_len = 0;
};
