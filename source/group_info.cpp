#include "group_info.h"
#include "common.h"
uint32_t group_msgs_max_len = 100;

GroupInfo::~GroupInfo()
{
    members_list.clear();
    msg_list.clear();
    msg_limit = 0;
}

bool GroupInfo::from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len)
{
    if (data_len < length())
    {
        return false;
    }
    int32_t index = 0;
    index += memcpy_u(group_guid, data + index);
    if (0 == group_guid)
        group_guid = ++global_guid;
    index += memcpy_u(member_count, data + index);
    index += memcpy_u(learder_guid, data + index);
    struct group_member_info info;
    uint32_t info_length = info.length();
    if (member_count * info_length > data_len - index)
    {
        return false;
    }
    for (uint32_t i = 0; i < member_count; ++i)
    {
        info.from_data(data + index, data_len - index);
        index += info_length;
    }
    return true;
}

uint32_t GroupInfo::to_data(uint8_t *data, const uint32_t data_len)
{
    uint32_t index = 0;
    if (!data || data_len < length())
    {
        return index;
    }
    index += memcpy_u(data + index, group_guid);
    index += memcpy_u(data + index, member_count);
    index += memcpy_u(data + index, learder_guid);
    struct group_member_info info;
    if (member_count * info.length() > data_len - index)
    {
        index = 0;
        return index;
    }
    //TODO:这种写法有风险，group_member_info内若有不定长的数据则会出错，有待改进
    for (uint32_t i = 0; i < member_count; ++i)
    {
        index += info.to_data(data + index, data_len - index);
    }
    return index;
}

uint32_t GroupInfo::length()
{
    uint32_t length = sizeof(group_guid);
    length += sizeof(member_count);
    length += sizeof(learder_guid);
    struct group_member_info info;
    length += member_count * info.length();
    return length;
}

std::shared_ptr<group_member_info> GroupInfo::get_member_by_guid(uint32_t guid)
{
    auto member_itor = members_map.find(guid);
    if (member_itor != members_map.end())
    {
        return *(member_itor->second);
    }
    auto managers_itor = managers_map.find(guid);
    if (managers_itor != managers_map.end())
    {
        return *(managers_itor->second);
    }
    return nullptr;
}

bool GroupInfo::on_add_member(uint32_t operator_guid, uint32_t user_guid, uint32_t icon_guid, uint8_t *user_name)
{
    if (!user_name)
        return false;
    auto manager_itor = managers_map.find(operator_guid);
    if (manager_itor == managers_map.end() && operator_guid != 0)
    {
        return false;
    }
    auto member_itor = members_map.find(user_guid);
    if (member_itor != members_map.end())
    {
        return false;
    }
    std::shared_ptr<group_member_info> group_info = std::make_shared<group_member_info>();
    group_info->group_guid = group_guid;
    group_info->user_guid = user_guid;
    group_info->icon_guid = icon_guid;
    group_info->msg_count = 0;
    group_info->permissions = group_permission::SEND_TO_GROUP;
    group_info->job = group_member_info::GROUP_JOB_MEMBER;
    memcpy(group_info->name, user_name, sizeof(group_info->name));
    members_list.push_back(group_info);
    if (operator_guid == 0 && user_guid == learder_guid)
    {
        managers_map[user_guid] = std::prev(members_list.end());
        group_info->job = group_member_info::GROUP_JOB_LEADER;
        ++member_count;
        return true;
    }
    members_map[user_guid] = std::prev(members_list.end());
    ++member_count;
    return true;
}

bool GroupInfo::on_delete_member(uint32_t operator_guid, uint32_t user_guid)
{
    auto manager_itor = managers_map.find(user_guid);
    if (manager_itor != managers_map.end())
    {
        if (operator_guid != learder_guid)
        {
            return false;
        }
        members_list.erase(manager_itor->second);
        managers_map.erase(manager_itor);
        --member_count;
        return true;
    }
    auto operator_itor = managers_map.find(operator_guid);
    if (operator_itor == managers_map.end())
    {
        return false;
    }
    auto member_itor = members_map.find(user_guid);
    if (member_itor == members_map.end())
    {
        return false;
    }
    members_list.erase(member_itor->second);
    members_map.erase(member_itor);
    --member_count;
    return true;
}

bool GroupInfo::on_add_manager(uint32_t operator_guid, uint32_t user_guid)
{
    if (operator_guid != learder_guid)
    {
        return false;
    }
    auto manager_itor = managers_map.find(user_guid);
    if (manager_itor != managers_map.end())
    {
        return false;
    }
    auto member_itor = members_map.find(user_guid);
    if (member_itor == members_map.end())
    {
        return false;
    }
    managers_map[user_guid] = members_map[user_guid];
    std::shared_ptr<group_member_info> &info = *managers_map[user_guid];
    if (info != nullptr)
        info->job = group_member_info::GROUP_JOB_MANAGER;
    members_map.erase(member_itor);
    return true;
}

bool GroupInfo::on_delete_manager(uint32_t operator_guid, uint32_t user_guid)
{
    if (operator_guid != learder_guid)
    {
        return false;
    }
    auto manager_itor = managers_map.find(user_guid);
    if (manager_itor == managers_map.end())
    {
        return false;
    }
    auto member_itor = members_map.find(user_guid);
    if (member_itor != members_map.end())
    {
        return false;
    }
    members_map[user_guid] = managers_map[user_guid];
    std::shared_ptr<group_member_info> &info = *members_map[user_guid];
    if (info != nullptr)
        info->job = group_member_info::GROUP_JOB_MEMBER;
    managers_map.erase(manager_itor);
    return true;
}

bool GroupInfo::on_change_permissions(uint32_t operator_guid, uint32_t user_guid, uint8_t permissions)
{
    auto operator_manager_itor = managers_map.find(operator_guid);
    if (operator_manager_itor == managers_map.end())
    {
        return false;
    }
    std::shared_ptr<group_member_info> &operator_member = *operator_manager_itor->second;
    if (operator_member.use_count() > 0)
    {
        uint8_t operator_permission = operator_member->permissions;
        if (operator_permission > permissions)
        {
            return false;
        }
    }
    auto user_manager_itor = managers_map.find(user_guid);
    if (user_manager_itor != managers_map.end())
    {
        if (operator_guid != learder_guid)
            return false;
        std::shared_ptr<group_member_info> &member = *user_manager_itor->second;
        if (member.use_count() > 0)
        {
            member->permissions = permissions;
        }
        return true;
    }
    auto member_itor = members_map.find(user_guid);
    if (member_itor != members_map.end())
    {
        std::shared_ptr<group_member_info> &member = *member_itor->second;
        if (member.use_count() > 0)
        {
            member->permissions = permissions;
        }
        return true;
    }
    return false;
}

bool GroupInfo::traverse_managers(const std::function<bool(std::shared_ptr<group_member_info> &)> &func)
{
    auto begin = managers_map.begin();
    auto end = managers_map.end();
    while (begin != end)
    {
        if (!func || !func(*(begin->second)))
            break;
        ++begin;
    }
    return true;
}

bool GroupInfo::traverse_normal_members(const std::function<bool(std::shared_ptr<group_member_info> &)> &func)
{
    auto begin = members_map.begin();
    auto end = members_map.end();
    while (begin != end)
    {
        if (!func || !func(*(begin->second)))
            break;
        ++begin;
    }
    return true;
}

bool GroupInfo::traverse_all_members(const std::function<bool(std::shared_ptr<group_member_info> &)> &func)
{
    return traverse_managers(func) && traverse_normal_members(func);
}

bool GroupInfo::on_add_msg(std::shared_ptr<msg_info> &msg)
{
    if (msg->msg_type != msg_info::MSG_TYPE_GROUP)
        return false;
    if (group_guid != msg->recv_guid)
        return false;
    msg_list.push_back(msg);
    ++msg_limit;
    if (msg_limit > group_msgs_max_len)
    {
        msg_list.pop_front();
        --msg_limit;
    }
    return true;
}

bool GroupInfo::on_delete_msg(uint32_t operator_guid, uint32_t msg_num)
{
    auto manager_itor = managers_map.find(operator_guid);
    if (manager_itor == managers_map.end())
        return false;
    auto rbegin = msg_list.rbegin();
    auto rend = msg_list.rend();
    while (rbegin != rend)
    {
        std::shared_ptr<msg_info> &msg = *rbegin;
        if (msg->msg_num == msg_num)
        {
            msg_list.erase((++rbegin).base());
            --msg_limit;
            break;
        }
        ++rbegin;
    }
    return true;
}

bool GroupInfo::clear_msg(uint32_t operator_guid)
{
    auto manager_itor = managers_map.find(operator_guid);
    if (manager_itor == managers_map.end())
        return false;
    msg_list.clear();
    msg_limit = 0;
    return true;
}

bool GroupInfo::traverse_msgs(const std::function<bool(std::shared_ptr<msg_info> &)> &func)
{
    auto begin = msg_list.begin();
    auto end = msg_list.end();
    while (begin != end)
    {
        if (!func || !func(*begin))
            break;
        ++begin;
    }
    return true;
}