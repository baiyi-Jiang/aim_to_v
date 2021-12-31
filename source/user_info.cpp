#include "user_info.h"
#include "common.h"

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

bool UserInfo::from_data(uint32_t global_guid, const uint8_t *data, uint32_t data_len)
{
    if (data_len < length())
    {
        return false;
    }
    int32_t index = 0;
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
    keep1 = data[index++];
    keep2 = data[index++];
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
    memcpy(passwd, data + index, sizeof(passwd));
    index += sizeof(passwd);
    memcpy(custom, data + index, sizeof(custom));
    index += sizeof(custom);
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
    length += sizeof(keep1);
    length += sizeof(keep2);
    length += sizeof(name);
    length += sizeof(phone);
    length += sizeof(qq);
    length += sizeof(wechat);
    length += sizeof(eamil);
    length += sizeof(city);
    length += sizeof(passwd);
    length += sizeof(custom);
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
    index += memcpy_u(data + index, keep1);
    index += memcpy_u(data + index, keep2);
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
    memcpy(data + index, passwd, sizeof(passwd));
    index += sizeof(passwd);
    memcpy(data + index, custom, sizeof(custom));
    index += sizeof(custom);
    return index;
}

bool UserInfo::traverse_list(uint32_t guid, const std::function<bool(std::shared_ptr<msg_info> &)> &func)
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

bool UserInfo::on_add_msg(std::shared_ptr<msg_info> &msg)
{
    if (msg->msg_type != msg_info::MSG_TYPE_USER)
        return false;
    if (user_guid != msg->recv_guid)
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
        std::shared_ptr<msg_info> &msg = *rbegin;
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

bool UserInfo::clear_msg(uint32_t recv_guid, uint32_t msg_num)
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
    auto itor = std::find(join_groups.begin(), join_groups.end(), guid);
    if (itor == join_groups.end())
    {
        join_groups.push_back(guid);
        return true;
    }
    return false;
}

bool UserInfo::on_leave_group(uint32_t guid)
{
    auto itor = std::find(join_groups.begin(), join_groups.end(), guid);
    if (itor != join_groups.end())
    {
        join_groups.erase(itor);
        return true;
    }
    return false;
}