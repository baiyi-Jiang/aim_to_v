#pragma once
#include "struct_def.h"
#include <memory>
#include <functional>

class UserInfo
{
public:
    using MSG_LIST = std::list<std::shared_ptr<msg_info>>;
    enum sex_type : uint8_t
    {
        MALE = 0x00,
        FEMALE,
        OTHER
    };
    UserInfo()
    {
    }
    UserInfo(uint32_t global_guid, const uint8_t *data, uint32_t data_len)
    {
        from_data(global_guid, data, data_len);
    }
    ~UserInfo();

    bool from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len);
    uint32_t length();
    uint32_t get_user_guid() { return user_guid; }
    uint32_t get_icon_guid() { return icon_guid; }
    uint32_t get_reg_time() { return reg_time; }
    uint32_t get_last_login_time() { return last_login_time; }
    uint32_t get_last_logout_time() { return last_logout_time; }
    uint8_t get_sex() { return sex; }
    uint8_t get_age() { return age; }
    std::string get_name() { return std::string((char *)name, sizeof(name)); }
    std::string get_phone() { return std::string((char *)phone, sizeof(phone)); }
    std::string get_qq() { return std::string((char *)qq, sizeof(qq)); }
    std::string get_wechat() { return std::string((char *)wechat, sizeof(wechat)); }
    std::string get_eamil() { return std::string((char *)eamil, sizeof(eamil)); }
    std::string get_city() { return std::string((char *)city, sizeof(city)); }
    std::string get_passwd() { return std::string((char *)passwd, sizeof(passwd)); }
    std::string get_custom() { return std::string((char *)custom, sizeof(custom)); }
    bool on_add_msg(std::shared_ptr<msg_info> &msg);
    bool on_delete_msg(uint32_t recv_guid, uint32_t msg_num);
    bool clear_msg(uint32_t recv_guid, uint32_t msg_num);
    bool on_join_group(uint32_t guid);
    bool on_leave_group(uint32_t guid);
    //遍历聊天记录链表并进行某种操作
    bool traverse_list(uint32_t guid, const std::function<bool(std::shared_ptr<msg_info> &)> &func);

private:
    uint32_t user_guid = 0;                     //用户GUID
    uint32_t icon_guid = 0;                     //头像guid
    uint32_t reg_time = 0;                      //注册时间
    uint32_t last_login_time = 0;               //上次登录时间
    uint32_t last_logout_time = 0;              //上次登出时间
    uint8_t age = 0;                            //年龄
    uint8_t sex = 0;                            //性别
    uint8_t keep1 = 0;                          //保留字段1
    uint8_t keep2 = 0;                          //保留字段2
    uint8_t name[32] = {0};                     //名称
    uint8_t phone[16] = {0};                    //电话号码
    uint8_t qq[16] = {0};                       //qq号
    uint8_t wechat[32] = {0};                   //微信号
    uint8_t eamil[32] = {0};                    //邮箱
    uint8_t city[32] = {0};                     //城市
    uint8_t passwd[32] = {0};                   //密码<TODO：增加加密>l
    uint8_t custom[64] = {0};                   //自定义变量
    std::vector<uint32_t> join_groups;          //加入的群组guid
    std::map<uint32_t, MSG_LIST *> msg_map;     //聊天记录索引
    std::map<uint32_t, uint32_t> msg_map_limit; //消息队列最多保持user_msgs_max_len条
};
