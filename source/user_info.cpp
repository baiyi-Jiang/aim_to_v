#include "user_info.h"
#include "common.h"
#include "log.h"

uint32_t user_msgs_max_len = 50;

UserInfo::~UserInfo()
{
    auto itor = msg_map.begin();
    while (itor != msg_map.end())
    {
        MSG_LIST *l_ptr = itor->second;
        if (l_ptr)
        {
            l_ptr->clear();
            delete l_ptr;
        }
    }
    msg_map_limit.clear();
}

uint32_t UserInfo::from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len)
{
    int32_t index = 0;
    if (data_len < length())
    {
        return index;
    }
    index += memcpy_u(user_guid, data + index);
    if (0 == user_guid)
        user_guid = ++global_guid;
    index += memcpy_u(icon_guid, data + index);
    index += memcpy_u(reg_time, data + index);
    if (0 == reg_time)
        reg_time = get_time_sec();
    index += memcpy_u(last_login_time, data + index);
    if (0 == last_login_time)
        last_login_time = get_time_sec();
    index += memcpy_u(last_logout_time, data + index);
    if (0 == last_logout_time)
        last_logout_time = get_time_sec();
    age = data[index++];
    sex = data[index++];
    online_status = data[index++];
    owner_groups = data[index++];
    memcpy(name, data + index, sizeof(name));
    index += sizeof(name);
    memcpy(phone, data + index, sizeof(phone));
    index += sizeof(phone);
    memcpy(qq, data + index, sizeof(qq));
    index += sizeof(qq);
    memcpy(wechat, data + index, sizeof(wechat));
    index += sizeof(wechat);
    memcpy(eamil, data + index, sizeof(eamil));
    index += sizeof(eamil);
    memcpy(city, data + index, sizeof(city));
    index += sizeof(city);
    index += memcpy_u(passwd, data + index);
    index += memcpy_u(custom_length, data + index);
    if (data_len < custom_length + index)
    {
        log_print(LOG_ERROR,u8"数据不完整！");
        index = 0;
        return index;
    }
    if (!custom.from_data(data + index, custom_length))
    {
        log_print(LOG_ERROR,u8"自定义变量解析失败！");
        index = 0;
        return index;
    }
    index += custom_length;
    return index;
}

uint32_t UserInfo::length()
{
    uint32_t length = sizeof(user_guid);
    length += sizeof(icon_guid);
    length += sizeof(reg_time);
    length += sizeof(last_login_time);
    length += sizeof(last_logout_time);
    length += sizeof(age);
    length += sizeof(sex);
    length += sizeof(online_status);
    length += sizeof(owner_groups);
    length += sizeof(name);
    length += sizeof(phone);
    length += sizeof(qq);
    length += sizeof(wechat);
    length += sizeof(eamil);
    length += sizeof(city);
    length += sizeof(passwd);
    length += sizeof(custom_length);
    length += custom.length();
    return length;
}

uint32_t UserInfo::to_data(uint8_t *data, const uint32_t len)
{
    uint32_t index = 0;
    if (!data || len < length())
    {
        return index;
    }
    index += memcpy_u(data + index, user_guid);
    index += memcpy_u(data + index, icon_guid);
    index += memcpy_u(data + index, reg_time);
    index += memcpy_u(data + index, last_login_time);
    index += memcpy_u(data + index, last_logout_time);
    index += memcpy_u(data + index, age);
    index += memcpy_u(data + index, sex);
    index += memcpy_u(data + index, online_status);
    index += memcpy_u(data + index, owner_groups);
    memcpy(data + index, name, sizeof(name));
    index += sizeof(name);
    memcpy(data + index, phone, sizeof(phone));
    index += sizeof(phone);
    memcpy(data + index, qq, sizeof(qq));
    index += sizeof(qq);
    memcpy(data + index, wechat, sizeof(wechat));
    index += sizeof(wechat);
    memcpy(data + index, eamil, sizeof(eamil));
    index += sizeof(eamil);
    memcpy(data + index, city, sizeof(city));
    index += sizeof(city);
    passwd = 0U; //不向外界输出密码
    index += memcpy_u(data + index, passwd);
    index += memcpy_u(data + index, custom_length);
    index += custom.to_data(data + index, len - index);
    return index;
}

bool UserInfo::traverse_list(uint32_t guid, const std::function<bool(std::shared_ptr<MsgInfo> &)> &func)
{
    auto list_ptr = msg_map.find(guid);
    if (list_ptr == msg_map.end())
        return false;
    MSG_LIST *msgs = list_ptr->second;
    auto begin = msgs->begin();
    auto end = msgs->end();
    while (begin != end)
    {
        if (!func || !func(*begin))
            break;
        ++begin;
    }
    return true;
}

bool UserInfo::on_add_msg(std::shared_ptr<MsgInfo> &msg)
{
    if (msg->msg_type != MsgInfo::MSG_TYPE_USER)
        return false;
    if (user_guid != msg->recv_guid || user_guid != msg->send_guid)
        return false;
    auto msg_list_itor = msg_map.find(msg->send_guid);
    if (msg_list_itor == msg_map.end())
    {
        msg_map[msg->send_guid] = new MSG_LIST;
        msg_map_limit[msg->send_guid] = 0;
    }
    msg_map[msg->send_guid]->push_back(msg);
    ++msg_map_limit[msg->send_guid];
    if (msg_map_limit[msg->send_guid] > user_msgs_max_len)
    {
        msg_map[msg->send_guid]->pop_front();
        --msg_map_limit[msg->send_guid];
    }
    return true;
}

bool UserInfo::on_delete_msg(uint32_t recv_guid, uint32_t msg_num)
{
    auto msg_list_itor = msg_map.find(recv_guid);
    if (msg_list_itor == msg_map.end())
        return false;
    MSG_LIST *msgs_list = msg_list_itor->second;
    if (!msgs_list)
        return false;
    auto rbegin = msgs_list->rbegin();
    auto rend = msgs_list->rend();
    while (rbegin != rend)
    {
        std::shared_ptr<MsgInfo> &msg = *rbegin;
        if (msg->msg_num == msg_num)
        {
            msgs_list->erase((++rbegin).base());
            --msg_map_limit[msg->send_guid];
            break;
        }
        ++rbegin;
    }
    return true;
}

bool UserInfo::clear_msg(uint32_t recv_guid)
{
    auto msg_list_itor = msg_map.find(recv_guid);
    if (msg_list_itor == msg_map.end())
        return false;
    if (!msg_list_itor->second)
        return false;
    msg_list_itor->second->clear();
    msg_map_limit[recv_guid] = 0;
    return true;
}

bool UserInfo::on_join_group(uint32_t guid)
{
    auto itor = join_groups.find(guid);
    if (itor == join_groups.end())
    {
        join_groups[guid] = get_time_sec();
        return true;
    }
    return false;
}

bool UserInfo::on_leave_group(uint32_t guid)
{
    auto itor = join_groups.find(guid);
    if (itor != join_groups.end())
    {
        join_groups.erase(itor);
        return true;
    }
    return false;
}

bool UserInfo::on_update_group_active(uint32_t guid)
{
    auto itor = join_groups.find(guid);
    if (itor != join_groups.end())
    {
        itor->second = get_time_sec();
        return true;
    }
    return false;
}

bool UserInfo::on_add_friend(uint32_t guid)
{
    auto itor = friends.find(guid);
    if (itor == friends.end())
    {
        friends[guid] = get_time_sec();
        return true;
    }
    return false;
}

bool UserInfo::on_delete_friend(uint32_t guid)
{
    auto itor = friends.find(guid);
    if (itor != friends.end())
    {
        friends.erase(itor);
        return true;
    }
    return false;
}

bool UserInfo::on_update_friend_active(uint32_t guid)
{
    auto itor = friends.find(guid);
    if (itor != friends.end())
    {
        itor->second = get_time_sec();
        return true;
    }
    return false;
}

bool UserInfo::on_set_online_status(uint8_t status)
{
    online_status = status;
    return true;
}

bool UserInfo::on_create_group()
{
    if (owner_groups > OWN_GROUP_MAX)
        return false;
    ++owner_groups;
    return true;
}

bool UserInfo::on_delete_group()
{
    if (owner_groups == 0)
        return false;
    --owner_groups;
    return true;
}

bool UserInfo::make_brief_data(BriefUserInfo &info)
{
    info.user_guid = get_user_guid();
    info.icon_guid = get_icon_guid();
    std::string user_name = get_name();
    memcpy(info.name, user_name.c_str(), std::min(sizeof(info.name), user_name.size()));
    info.online_status = get_online_status();
    return true;
}