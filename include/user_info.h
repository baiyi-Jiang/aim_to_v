#pragma once
#include <memory>
#include <functional>
#include "struct_def.h"
#include "custom_data.h"

#define OWN_GROUP_MAX 50

class UserInfo
{
public:
    using MSG_LIST = std::list<std::shared_ptr<MsgInfo>>;
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

    uint32_t from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len);
    uint32_t length();
    uint32_t to_data(uint8_t *data, const uint32_t len);
    uint32_t get_user_guid() { return user_guid; }
    uint32_t get_icon_guid() { return icon_guid; }
    uint32_t get_reg_time() { return reg_time; }
    uint32_t get_last_login_time() { return last_login_time; }
    uint32_t get_last_logout_time() { return last_logout_time; }
    uint8_t get_sex() { return sex; }
    uint8_t get_age() { return age; }
    uint8_t get_online_status() { return online_status; }
    std::string get_name() { return std::string((char *)name, sizeof(name)); }
    std::string get_phone() { return std::string((char *)phone, sizeof(phone)); }
    std::string get_qq() { return std::string((char *)qq, sizeof(qq)); }
    std::string get_wechat() { return std::string((char *)wechat, sizeof(wechat)); }
    std::string get_eamil() { return std::string((char *)eamil, sizeof(eamil)); }
    std::string get_city() { return std::string((char *)city, sizeof(city)); }
    size_t get_passwd() { return passwd; }
    uint32_t get_custom_length() { return custom_length; }
    void set_reg_time(uint32_t time) { reg_time = time; }
    void set_last_login_time(uint32_t time) { last_login_time = time; }
    void set_last_logout_time(uint32_t time) { last_logout_time = time; }
    bool on_add_msg(std::shared_ptr<MsgInfo> &msg);
    bool on_delete_msg(uint32_t recv_guid, uint32_t msg_num);
    bool clear_msg(uint32_t recv_guid);
    bool on_join_group(uint32_t guid);
    bool on_leave_group(uint32_t guid);
    bool on_update_group_active(uint32_t guid);
    bool on_add_friend(uint32_t guid);
    bool on_delete_friend(uint32_t guid);
    bool on_update_friend_active(uint32_t guid);
    bool on_set_online_status(uint8_t status);
    //遍历聊天记录链表并进行某种操作
    bool traverse_list(uint32_t guid, const std::function<bool(std::shared_ptr<MsgInfo> &)> &func);
    bool on_create_group();
    bool on_delete_group();
    bool can_create_group() { return owner_groups < (uint8_t)OWN_GROUP_MAX; }
    std::map<uint32_t, uint32_t> &get_join_groups() { return join_groups; }
    std::map<uint32_t, uint32_t> &get_friends() { return friends; }
    std::string get_custom_str(std::string key) { return custom.get_custom_str(key); }
    uint64_t get_custom_num(std::string key) { return custom.get_custom_num(key); }
    bool set_custom(std::string key, std::string str) { return custom.add_custom(key, str); }
    bool set_custom(std::string key, uint64_t num) { return custom.add_custom(key, num); }
    bool make_brief_data(BriefUserInfo &info);

private:
    uint32_t user_guid = 0;                     //用户GUID
    uint32_t icon_guid = 0;                     //头像guid
    uint32_t reg_time = 0;                      //注册时间
    uint32_t last_login_time = 0;               //上次登录时间
    uint32_t last_logout_time = 0;              //上次登出时间
    uint8_t age = 0;                            //年龄
    uint8_t sex = 0;                            //性别
    uint8_t online_status = 0;                  //在线状态 //user_status
    uint8_t owner_groups = 0;                   //拥有的群，创建群加一，删群减一
    uint8_t name[32] = {0};                     //名称
    uint8_t phone[16] = {0};                    //电话号码
    uint8_t qq[16] = {0};                       //qq号
    uint8_t wechat[32] = {0};                   //微信号
    uint8_t eamil[32] = {0};                    //邮箱
    uint8_t city[32] = {0};                     //城市
    size_t passwd = 0;                          //密码的hash值
    uint32_t custom_length = 0;                 //自定义变量流化后的长度
    CustomData custom;                          //自定义变量
    std::map<uint32_t, uint32_t> join_groups;   //加入的群组guid,以及群组最后活跃时间
    std::map<uint32_t, uint32_t> friends;       //好友guid，以及最后聊天时间
    std::map<uint32_t, MSG_LIST *> msg_map;     //聊天记录索引
    std::map<uint32_t, uint32_t> msg_map_limit; //消息队列最多保持user_msgs_max_len条
};
