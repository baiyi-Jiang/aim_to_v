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
    std::string get_sha256() { return std::string((char *)sha256, sizeof(sha256)); }
    size_t get_passwd() { return passwd; }
    uint32_t get_custom_length() { return custom_length; }
    void set_user_guid(uint32_t guid) { user_guid = guid; }
    void set_icon_guid(uint32_t guid) { icon_guid = guid; }
    void set_reg_time(uint32_t time) { reg_time = time; }
    void set_last_login_time(uint32_t time) { last_login_time = time; }
    void set_last_logout_time(uint32_t time) { last_logout_time = time; }
    void set_sex(uint8_t s) { sex = s; }
    void set_age(uint8_t a) { age = a; }
    void set_name(const std::string& s) { memcpy((char *)name, s.c_str(), common::min(sizeof(name), s.size())); }
    void set_phone(const std::string& s) { memcpy((char *)phone, s.c_str(), common::min(sizeof(phone), s.size())); }
    void set_qq(const std::string& s) { memcpy((char *)qq, s.c_str(), common::min(sizeof(qq), s.size())); }
    void set_wechat(const std::string& s) { memcpy((char *)wechat, s.c_str(), common::min(sizeof(wechat), s.size())); }
    void set_eamil(const std::string& s) { memcpy((char *)eamil, s.c_str(), common::min(sizeof(eamil), s.size())); }
    void set_city(const std::string& s) { memcpy((char *)city, s.c_str(), common::min(sizeof(city), s.size())); }
    void set_sha256(const std::string& s) { memcpy((char *)sha256, s.c_str(), common::min(sizeof(sha256), s.size())); }
    
private:
    void set_passwd(const std::string& s) { passwd = std::hash<std::string>{}(s); }

public:
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
    //?????????????????????????????????????????????
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
    uint32_t user_guid = 0;                     //??????GUID
    uint32_t icon_guid = 0;                     //??????guid
    uint32_t reg_time = 0;                      //????????????
    uint32_t last_login_time = 0;               //??????????????????
    uint32_t last_logout_time = 0;              //??????????????????
    uint8_t age = 0;                            //??????
    uint8_t sex = 0;                            //??????
    uint8_t online_status = 0;                  //???????????? //user_status
    uint8_t owner_groups = 0;                   //?????????????????????????????????????????????
    uint8_t name[32] = {0};                     //??????
    uint8_t phone[16] = {0};                    //????????????
    uint8_t qq[16] = {0};                       //qq???
    uint8_t wechat[32] = {0};                   //?????????
    uint8_t eamil[32] = {0};                    //??????
    uint8_t city[32] = {0};                     //??????
    uint8_t sha256[64] = {0};                   //??????????????????
    size_t passwd = 0;                          //?????????hash???
    uint32_t custom_length = 0;                 //?????????????????????????????????
    CustomData custom;                          //???????????????
    std::map<uint32_t, uint32_t> join_groups;   //???????????????guid,??????????????????????????????
    std::map<uint32_t, uint32_t> friends;       //??????guid???????????????????????????
    std::map<uint32_t, MSG_LIST *> msg_map;     //??????????????????
    std::map<uint32_t, uint32_t> msg_map_limit; //????????????????????????user_msgs_max_len???
};
