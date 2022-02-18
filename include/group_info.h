#pragma once
#include "struct_def.h"
#include <memory>
#include <functional>

class GroupInfo
{
public:
    using MEMBER_LIST = std::list<std::shared_ptr<group_member_info>>;
    GroupInfo() {}
    ~GroupInfo();
    uint32_t from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len);
    uint32_t to_data(uint8_t *data, const uint32_t data_len);
    uint32_t length();
    uint32_t get_group_guid() { return group_guid; }
    uint32_t get_member_count() { return member_count; }
    uint32_t get_learder_guid() { return learder_guid; }
    std::shared_ptr<group_member_info> get_member_by_guid(uint32_t guid);
    //创建群组添加成员使用此接口，先填leader_guid
    //operator_guid填0,创建群组时不能设置管理员
    bool on_add_member(uint32_t operator_guid, uint32_t user_guid, uint32_t icon_guid, uint8_t *user_name);
    bool on_delete_member(uint32_t operator_guid, uint32_t user_guid);
    bool on_add_manager(uint32_t operator_guid, uint32_t user_guid);
    bool on_delete_manager(uint32_t operator_guid, uint32_t user_guid);
    bool on_change_permissions(uint32_t operator_guid, uint32_t user_guid, uint8_t permissions);
    bool traverse_managers(const std::function<bool(std::shared_ptr<group_member_info> &)> &func);
    bool traverse_normal_members(const std::function<bool(std::shared_ptr<group_member_info> &)> &func);
    bool traverse_all_members(const std::function<bool(std::shared_ptr<group_member_info> &)> &func);
    bool on_add_msg(std::shared_ptr<MsgInfo> &msg);
    bool on_delete_msg(uint32_t operator_guid, uint32_t msg_num);
    bool clear_msg(uint32_t operator_guid);
    bool traverse_msgs(const std::function<bool(std::shared_ptr<MsgInfo> &)> &func);

    void set_group_guid(uint32_t guid) { group_guid = guid; }
    void set_learder_guid(uint32_t guid) { learder_guid = guid; }

private:
    uint32_t group_guid = 0;   //组guid
    uint32_t member_count = 0; //成员数量
    uint32_t learder_guid = 0; //组长guid
    MEMBER_LIST members_list;
    std::map<uint32_t, MEMBER_LIST::iterator> managers_map; //管理员列表，包括组长
    std::map<uint32_t, MEMBER_LIST::iterator> members_map;  //普通成员列表
    std::list<std::shared_ptr<MsgInfo>> msg_list;
    uint32_t msg_limit = 0; ////消息队列最多保持group_msgs_max_len条
};