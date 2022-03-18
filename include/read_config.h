#include<string>
#include<map>
#define NOT_FIND_KEY -1

class ReadConfig
{
public:
    ReadConfig() = delete;
    ReadConfig(const std::string &file_name);
    ~ReadConfig();
    std::string read_config(const std::string &key);
    bool write_config(const std::string &key, const std::string &value);

private:
    bool read_all_config();
    bool write_all_config();

private:
    FILE *_fp = nullptr;
    std::map<std::string, std::string> _config_map;
    std::string _file_name;
};