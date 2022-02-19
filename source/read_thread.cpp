#include "read_thread.h"
#include "user_info.h"
#include "group_info.h"
#include <iterator>
#include <memory>
#include "log.h"

uint32_t global_user_guid = 1;
uint32_t global_group_guid = 1;

uint32_t NetInfo::parse_msg(const uint8_t *data, uint32_t len, int32_t fd, RecvMsgPkg &msg)
{
    static uint32_t parse_msg_head_length = msg.head_length();
    uint32_t index = 0;
    if (msg.get_pkg_type() > recv_msg_type::SERVER_MSG_TYPE_MAX)
    {
        log_print(LOG_ERROR, u8"Error, msg type error!");
        return index;
    }
    switch (static_cast<recv_msg_type::msg_type>(msg.get_pkg_type()))
    {
    case recv_msg_type::ACOUNT_ADD:
    {
        index = on_acount_add(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::ACOUNT_MODIFY:
    {
        index = on_acount_modify(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::ACOUNT_DELETE:
    {
        index = on_acount_delete(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::ACOUNT_LOGIN:
    {
        index = on_acount_login(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::ACOUNT_LOGOUT:
    {
        index = on_acount_logout(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::ACOUNT_CAN_REG:
    {
        index = on_acount_can_reg(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::GROUP_ADD:
    {
        index = on_group_add(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::GROUP_MODIFY:
    {
        index = on_group_modify(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::GROUP_DELETE:
    {
        index = on_group_delete(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::USER_INFO_REQ:
    {
        index = on_user_info_req(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::GROUP_INFO_REQ:
    {
        index = on_group_info_req(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::USER_INFO_BRIEF_REQ:
    {
        index = on_user_brief_info_req(data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::MSG_SEND:
    {
        index = on_msg_send(msg.get_pkg_sender_guid(), data + parse_msg_head_length, msg.get_sub_pkg_data_length(), fd);
        break;
    }
    case recv_msg_type::MSG_LIST_REQ:
    {
        break;
    }
    default:
    {
        log_print(LOG_ERROR, u8"Unknow type:%d", msg.get_pkg_type());
        break;
    }
    }
    if (!index)
    {
        log_print(LOG_ERROR, u8"Error pkg type:%d", msg.get_pkg_type());
        send_client_ack(recv_msg_type::CLIENT_ACK_SEND, ERROR_PARSE_MSG, fd);
    }
    else
    {
        index += parse_msg_head_length;
    }
    return index;
}

uint32_t NetInfo::on_acount_add(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfo user;
    uint32_t index = user.from_data(global_user_guid, data, len);
    if (!index)
        return index;
    uint32_t err_no = on_phone_unique(user.get_phone());
    if (err_no != ERROR_OK)
    {
        ClientAcountAddAck ack(0);
        ack.err_no = err_no;
        ack.make_pkg((uint8_t *)send_buf, MAXBUFSIZE);
        send_msg(fd, (uint8_t *)send_buf, recv_msg_head_length + ack.get_sub_pkg_data_length());
        return index;
    }
    auto user_itor = users_map.find(user.get_user_guid());
    if (user_itor != users_map.end())
    {
        users.erase(user_itor->second);
        users_map.erase(user_itor);
    }
    uint32_t user_guid = user.get_user_guid();
    user.set_reg_time(common::get_time_sec());
    user.on_set_online_status(user_status::STATUS_ON_LINE);
    users.emplace_back(user);
    users_map[user_guid] = std::prev(users.end());
    ++global_user_guid;
    std::string phone = users_map[user_guid]->get_phone();
#ifdef USE_PHONE_TREE
    phone_tree *ptr = phone_user_tree;
    for (auto a : phone)
    {
        if (!ptr->num_list)
        {
            ptr->num_list = new phone_tree *[10];
        }
        if (!(*ptr->num_list)[(uint32_t)(a - '0')])
        {
            (*ptr->num_list)[(uint32_t)(a - '0')] = new phone_tree();
        }
        ptr = (*ptr->num_list)[(uint32_t)(a - '0')]
    }
    ptr->user_guid = user_guid;
#else
    //“123” 和“123\0\0\0”的hash不同
    size_t phone_hash = std::hash<std::string>{}(phone);
    phone_hash_map[phone_hash] = user_guid;
    // log_print(LOG_DEBUG, u8"parse msg end,phone:%s, size:%d, phone_hash: %d,user_guid: %d!", phone.c_str(), phone.size(), phone_hash, user_guid);
#endif
    //添加到公共聊天室
    if (users_map.size() == 1)
    {
        groups_map[1]->set_learder_guid(user_guid);
    }
    groups_map[1]->on_add_member(0, user_guid, users_map[user_guid]->get_icon_guid(), (uint8_t *)(users_map[user_guid]->get_name().c_str()));

    ClientAcountAddAck ack(user_guid);
    ack.err_no = ERROR_OK;
    ack.make_pkg((uint8_t *)send_buf, MAXBUFSIZE);
    send_msg(fd, (uint8_t *)send_buf, recv_msg_head_length + ack.get_sub_pkg_data_length());
    return index;
}

uint32_t NetInfo::on_acount_modify(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfo user;
    uint32_t index = user.from_data(global_user_guid, data, len);
    if (!index)
        return index;
    uint32_t user_guid = user.get_user_guid();
    if (user_guid == global_user_guid + 1)
    {
        return index;
    }
    auto user_itor = users_map.find(user_guid);
    if (user_itor != users_map.end())
    {
        std::map<uint32_t, uint32_t> &temp_groups = user.get_join_groups();
        for (const auto &group : temp_groups)
        {
            auto group_itor = groups_map.find(group.second);
            if (group_itor != groups_map.end())
            {
                auto member = group_itor->second->get_member_by_guid(user_guid);
                member->icon_guid = user.get_icon_guid();
                memcpy(member->name, user.get_name().c_str(), sizeof(member->name));
            }
        }
#ifdef USE_PHONE_TREE

#else
        size_t phone_hash = std::hash<std::string>{}(users_map[user_guid]->get_phone());
        phone_hash_map.erase(phone_hash);
#endif
        users.erase(user_itor->second);
        users_map.erase(user_itor);
        users.emplace_back(user);
        users_map[user_guid] = std::prev(users.end());
    }
#ifdef USE_PHONE_TREE

#else
    size_t phone_hash = std::hash<std::string>{}(users_map[user_guid]->get_phone());
    phone_hash_map[phone_hash] = user_guid;
#endif
    send_client_ack(recv_msg_type::CLIENT_ACOUNT_MODIFY_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_acount_delete(const uint8_t *data, uint32_t len, int32_t fd)
{
    AcountDelete ad;
    uint32_t index = ad.from_data(data, len);
    if (!index)
        return index;
    if (ad.user_guid != conn_user_map[fd])
    {
        send_client_ack(recv_msg_type::CLIENT_ACOUNT_DELETE_ACK, ERROR_CANT_DELETE_OTHER, fd);
        return index;
    }
    auto user_itor = users_map.find(ad.user_guid);
    if (user_itor != users_map.end())
    {
        std::map<uint32_t, uint32_t> &temp_groups = user_itor->second->get_join_groups();
        for (const auto &group : temp_groups)
        {
            auto group_itor = groups_map.find(group.first);
            if (group_itor != groups_map.end())
            {
                if (!group_itor->second->on_delete_manager(group_itor->second->get_learder_guid(), user_itor->second->get_user_guid()))
                    group_itor->second->on_delete_member(group_itor->second->get_learder_guid(), user_itor->second->get_user_guid());
            }
        }
        std::map<uint32_t, uint32_t> &temp_friends = user_itor->second->get_friends();
        for (const auto &friend_it : temp_friends)
        {
            auto friend_itor = users_map.find(friend_it.first);
            if (friend_itor != users_map.end())
            {
                friend_itor->second->on_delete_friend(user_itor->second->get_user_guid());
            }
        }
#ifdef USE_PHONE_TREE

#else
        size_t phone_hash = std::hash<std::string>{}(user_itor->second->get_phone());
        phone_hash_map.erase(phone_hash);
#endif
        users.erase(user_itor->second);
        users_map.erase(user_itor);
    }
    conn_user_map.erase(user_conn_map[ad.user_guid]);
    user_conn_map.erase(ad.user_guid);
    send_client_ack(recv_msg_type::CLIENT_ACOUNT_DELETE_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_acount_login(const uint8_t *data, uint32_t len, int32_t fd)
{
    uint32_t user_guid = 0;
    AcountLogin login;
    uint32_t index = login.from_data(data, len);
    if (!index)
        return index;
    std::string phone((const char *)login.phone, sizeof(login.phone));
    std::string sha256((const char *)login.sha256, sizeof(login.sha256));
    uint16_t err_no = on_login(phone, sha256);
    if (err_no == ERROR_OK)
    {
        size_t phone_hash = std::hash<std::string>{}(phone);
        auto user_itor = users_map.find(phone_hash_map[phone_hash]);
        if (user_itor != users_map.end())
        {
            user_guid = user_itor->second->get_user_guid();
            user_itor->second->set_last_login_time(common::get_time_sec());
            user_itor->second->set_last_logout_time(user_itor->second->get_last_login_time());
            user_itor->second->on_set_online_status(user_status::STATUS_ON_LINE);
        }
    }

    user_conn_map[user_guid] = fd;
    conn_user_map[fd] = user_guid;
    ClientAcountLoginAck ack(user_guid);
    ack.err_no = err_no;
    ack.make_pkg((uint8_t *)send_buf, MAXBUFSIZE);
    send_msg(fd, (uint8_t *)send_buf, recv_msg_head_length + ack.get_sub_pkg_data_length());
    return index;
}

uint32_t NetInfo::on_acount_logout(const uint8_t *data, uint32_t len, int32_t fd)
{
    AcountLogout logout;
    uint32_t index = logout.from_data(data, len);
    if (!index)
        return index;
    if (logout.user_guid != conn_user_map[fd])
    {
        send_client_ack(recv_msg_type::CLIENT_ACOUNT_LOGOUT_ACK, ERROR_CANT_LOGOUT_OTHER, fd);
        return index;
    }
    auto user_itor = users_map.find(logout.user_guid);
    if (user_itor != users_map.end())
    {
        user_itor->second->on_set_online_status(user_status::STATUS_OFF_LINE);
        user_itor->second->set_last_logout_time(common::get_time_sec());
    }
    conn_user_map.erase(user_conn_map[logout.user_guid]);
    user_conn_map.erase(logout.user_guid);
    send_client_ack(recv_msg_type::CLIENT_ACOUNT_LOGOUT_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_acount_can_reg(const uint8_t *data, uint32_t len, int32_t fd)
{
    AcountCanReg reg;
    uint32_t index = reg.from_data(data, len);
    if (!index)
        return index;
    uint32_t err_no = on_phone_unique(std::string((const char *)reg.phone, sizeof(reg.phone)));
    send_client_ack(recv_msg_type::CLIENT_ACOUNT_CAN_REG_ACK, err_no, fd);
    return index;
}

uint32_t NetInfo::on_group_add(const uint8_t *data, uint32_t len, int32_t fd)
{
    GroupInfo info;
    uint32_t index = info.from_data(global_group_guid, data, len);
    if (!index)
        return index;
    auto operate_itor = users_map.find(info.get_learder_guid());
    if (operate_itor == users_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_ADD_ACK, ERROR_GROUP_OPERATOR_NOT_EXIST, fd);
        return index;
    }
    if (!operate_itor->second->can_create_group())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_ADD_ACK, ERROR_CREATE_GROUP_MAX, fd);
        return index;
    }
    info.traverse_all_members([&](std::shared_ptr<group_member_info> &member) -> bool
                              {
                                  auto user_itor = users_map.find(member->user_guid);
                                  if (user_itor == users_map.end())
                                  {
                                      return false;
                                  }
                                  member->group_guid = info.get_group_guid();
                                  member->icon_guid = user_itor->second->get_icon_guid();
                                  member->msg_count = 0;
                                  member->permissions = group_permission::SEND_TO_GROUP;
                                  memcpy(member->name, user_itor->second->get_name().c_str(), sizeof(member->name));
                                  if (info.get_learder_guid() == member->user_guid)
                                      member->job = group_member_info::GROUP_JOB_LEADER;
                                  else
                                      member->job = group_member_info::GROUP_JOB_MEMBER;
                                  user_itor->second->on_join_group(member->group_guid);
                                  return true; });
    uint32_t group_guid = info.get_group_guid();
    groups.emplace_back(info);
    groups_map[group_guid] = std::prev(groups.end());
    operate_itor->second->on_create_group();
    send_client_ack(recv_msg_type::CLIENT_GROUP_ADD_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_group_modify(const uint8_t *data, uint32_t len, int32_t fd)
{
    GroupModify gm;
    uint32_t index = gm.from_data(data, len);
    if (!index)
        return index;
    if (gm.opreadte_type > GroupModify::ModifyTypeMAX)
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_MODIFY_ACK, ERROR_GROUP_MODIFY_TYPE, fd);
        return index;
    }
    auto group_itor = groups_map.find(gm.group_guid);
    if (group_itor == groups_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_MODIFY_ACK, ERROR_GROUP_NOT_EXIST, fd);
        return index;
    }
    auto operate_itor = users_map.find(gm.operate_guid);
    auto user_itor = users_map.find(gm.target_guid);
    if (gm.operate_guid != 0 && operate_itor == users_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_MODIFY_ACK, ERROR_GROUP_OPERATOR_NOT_EXIST, fd);
        return index;
    }
    if (gm.target_guid != 0 && user_itor == users_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_MODIFY_ACK, ERROR_GROUP_TARGET_NOT_EXIST, fd);
        return index;
    }
    switch (gm.opreadte_type)
    {
    case GroupModify::MEMBER_ADD:
    {
        if (gm.operate_guid == 0 || gm.target_guid == 0)
            break;
        if (group_itor->second->on_add_member(gm.operate_guid, gm.target_guid, user_itor->second->get_icon_guid(), (uint8_t *)user_itor->second->get_name().c_str()))
            user_itor->second->on_join_group(gm.group_guid);
        break;
    }
    case GroupModify::MEMBER_DELETE:
    {
        if (gm.operate_guid == 0 || gm.target_guid == 0)
            break;
        if (group_itor->second->on_delete_member(gm.operate_guid, gm.target_guid))
            user_itor->second->on_leave_group(gm.group_guid);
        break;
    }
    case GroupModify::MEMBER_SET_PERMISSION:
    {
        if (gm.operate_guid == 0 || gm.target_guid == 0)
            break;
        group_itor->second->on_change_permissions(gm.operate_guid, gm.target_guid, gm.permission);
        break;
    }
    case GroupModify::MANAGER_ADD:
    {
        if (gm.operate_guid == 0 || gm.target_guid == 0)
            break;
        group_itor->second->on_add_manager(gm.operate_guid, gm.target_guid);
        break;
    }
    case GroupModify::MANAGER_DELETE:
    {
        if (gm.operate_guid == 0 || gm.target_guid == 0)
            break;
        group_itor->second->on_delete_manager(gm.operate_guid, gm.target_guid);
        break;
    }
    default:
        break;
    }
    send_client_ack(recv_msg_type::CLIENT_GROUP_MODIFY_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_group_delete(const uint8_t *data, uint32_t len, int32_t fd)
{
    GroupDelete gd;
    uint32_t index = gd.from_data(data, len);
    if (!index)
        return index;
    auto group_itor = groups_map.find(gd.group_guid);
    if (group_itor == groups_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_DELETE_ACK, ERROR_GROUP_NOT_EXIST, fd);
        return index;
    }
    if (gd.operate_guid != group_itor->second->get_learder_guid())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_DELETE_ACK, ERROR_GROUP_NOT_LEADER, fd);
        return index;
    }
    auto operate_itor = users_map.find(gd.operate_guid);
    if (operate_itor == users_map.end())
    {
        send_client_ack(recv_msg_type::CLIENT_GROUP_DELETE_ACK, ERROR_GROUP_OPERATOR_NOT_EXIST, fd);
        return index;
    }
    group_itor->second->traverse_all_members([&](std::shared_ptr<group_member_info> &member) -> bool
                                             {
                                                 auto user_itor = users_map.find(member->user_guid);
                                                 if (user_itor == users_map.end())
                                                     return false;
                                                 user_itor->second->on_leave_group(gd.group_guid);
                                                 return true; });
    groups.erase(group_itor->second);
    groups_map.erase(group_itor);
    operate_itor->second->on_delete_group();
    send_client_ack(recv_msg_type::CLIENT_GROUP_DELETE_ACK, ERROR_OK, fd);
    return index;
}

uint32_t NetInfo::on_user_info_req(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfoReq req;
    uint32_t index = req.from_data(data, len);
    if (!index)
        return index;
    auto itor = users_map.find(req.user_guid);
    if (itor != users_map.end())
    {
        RecvMsgPkg tmp_msg(0, recv_msg_type::CLIENT_USER_INFO_ACK);
        if (tmp_msg.set_sub_pkg_data_length(itor->second->to_data((uint8_t *)send_buf + tmp_msg.head_length(), MAXBUFSIZE - tmp_msg.head_length())) && tmp_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
        {
            send_msg(fd, (uint8_t *)send_buf, tmp_msg.get_sub_pkg_data_length() + tmp_msg.head_length());
        }
    }
    return index;
}

uint32_t NetInfo::on_group_info_req(const uint8_t *data, uint32_t len, int32_t fd)
{
    GroupInfoReq req;
    uint32_t index = req.from_data(data, len);
    if (!index)
        return index;
    auto itor = groups_map.find(req.group_guid);
    if (itor != groups_map.end())
    {
        RecvMsgPkg tmp_msg(0, recv_msg_type::CLIENT_GROUP_INFO_ACK);
        if (tmp_msg.set_sub_pkg_data_length(itor->second->to_data((uint8_t *)send_buf + tmp_msg.head_length(), MAXBUFSIZE - tmp_msg.head_length())) && tmp_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
        {
            send_msg(fd, (uint8_t *)send_buf, tmp_msg.get_sub_pkg_data_length() + tmp_msg.head_length());
        }
    }
    return index;
}

uint32_t NetInfo::on_user_brief_info_req(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfoReq req;
    uint32_t index = req.from_data(data, len);
    if (!index)
        return index;
    auto itor = users_map.find(req.user_guid);
    if (itor != users_map.end())
    {
        BriefUserInfo info;
        itor->second->make_brief_data(info);
        RecvMsgPkg tmp_msg(0, recv_msg_type::CLIENT_USER_INFO_BRIEF_NOTIFY);
        if (tmp_msg.set_sub_pkg_data_length(info.to_data((uint8_t *)send_buf + tmp_msg.head_length(), MAXBUFSIZE - tmp_msg.head_length())) && tmp_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
        {
            send_msg(fd, (uint8_t *)send_buf, tmp_msg.get_sub_pkg_data_length() + tmp_msg.head_length());
        }
    }
    return index;
}

uint32_t NetInfo::on_msg_send(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd)
{
    std::shared_ptr<MsgInfo> msg = std::make_shared<MsgInfo>();
    uint32_t index = msg->from_data(data, len);
    if (!index)
        return index;
    auto conn_itor = conn_user_map.find(fd);
    if (conn_itor == conn_user_map.end() || conn_itor->second != guid)
    {
        send_client_ack(recv_msg_type::CLIENT_MSG_SEND_ACK, ERROR_USER_NOT_EXIST_OR_SEND_GUID, fd);
    }
    else
    {
        msg_list.push_back(msg);
        //send_client_ack(recv_msg_type::CLIENT_MSG_SEND_ACK, ERROR_OK, fd);
    }
    return index;
}

uint32_t NetInfo::on_msg_list_req()
{
    // TODO:待完善
    return 0;
}

int32_t NetInfo::send_msg(int32_t fd, uint8_t *data, uint32_t len)
{
    if (!data || !len)
    {
        return -1;
    }
    int32_t nsend = send(fd, data, len, 0);
    if (nsend == EAGAIN)
    {
        ev.data.fd = fd;
        //设置用于注测的读操作事件
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        //注册ev
        int32_t ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
        if (ret != 0)
        {
            log_print(LOG_ERROR, u8"ReadThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
            // close(fd);
        }
    }
    return nsend;
}

uint32_t NetInfo::on_login(const std::string &acount, const std::string &sha256)
{
    uint32_t user_guid = 0;
#ifdef USE_PHONE_TREE
    phone_tree *ptr = phone_user_tree;
    for (auto a : acount)
    {
        if (ptr)
            ptr = ptr->get_phone_tree(a);
        else
            return ERROR_PHONE_ISNOT_EXIST;
    }
    if (ptr->user_guid == 0)
    {
        return ERROR_PHONE_ISNOT_EXIST;
    }
    user_guid = ptr->user_guid;
#else
    size_t phone_hash = std::hash<std::string>{}(acount);
    // log_print(LOG_DEBUG, u8"phone:%s,size:%d, phone_hash: %d!", acount.c_str(), acount.size(), phone_hash);
    auto itor = phone_hash_map.find(phone_hash);
    if (itor == phone_hash_map.end())
    {
        return ERROR_PHONE_MAP;
    }
    user_guid = itor->second;
#endif
    size_t passwd = std::hash<std::string>{}(sha256);
    auto user_itor = users_map.find(user_guid);
    if (user_itor != users_map.end())
    {
        /*log_print(LOG_DEBUG, u8"sha256:%s, passwd: %d, u_sha256:%s, u_passwd: %d!", sha256.c_str(), passwd,
                  user_itor->second->get_sha256().c_str(), user_itor->second->get_passwd());*/
        if (passwd == user_itor->second->get_passwd())
        {
            return ERROR_OK;
        }
        return ERROR_PASSWD;
    }
    return ERROR_USER_ISNOT_EXIST;
}

uint32_t NetInfo::on_phone_unique(std::string acount)
{
#ifdef USE_PHONE_TREE
    phone_tree *ptr = phone_user_tree;
    for (auto a : acount)
    {
        if (a < '0' || a > '9')
        {
            return ERROR_PHONE_ERROR_CHARACTER;
        }
        if (ptr)
            ptr = ptr->get_phone_tree(a);
        else
            return ERROR_OK;
    }
    if (ptr)
    {
        if (ptr->user_guid != 0)
        {
            return ERROR_PHONE_EXIST;
        }
    }
    return ERROR_OK;
#else
    size_t phone_hash = std::hash<std::string>{}(acount);
    if (phone_hash_map.find(phone_hash) != phone_hash_map.end())
    {
        return ERROR_PHONE_EXIST;
    }
    return ERROR_OK;
#endif
}

void NetInfo::send_client_ack(uint16_t msg_type, uint16_t ack_type, int32_t fd)
{
    ClientAck ack((recv_msg_type::msg_type)msg_type);
    ack.err_no = ack_type;
    ack.make_pkg((uint8_t *)send_buf, MAXBUFSIZE);
    send_msg(fd, (uint8_t *)send_buf, recv_msg_head_length + ack.get_sub_pkg_data_length());
}

//读数据线程
void *common::read_thread(void *arg)
{
    common_info *comm = static_cast<common_info *>(arg);
    if (!comm)
    {
        log_print(LOG_ERROR, u8"read_thread, invalid args!");
        return nullptr;
    }
    log_print(LOG_INFO, u8"ReadThread, enter");

    NetInfo net_info;
    int32_t &epfd = net_info.epfd;      //连接用的epoll
    epoll_event &ev = net_info.ev;      //事件临时变量
    char *send_buf = net_info.send_buf; //发送缓冲区
    int32_t ret = 0;                    //临时变量,存放返回值
    int32_t i = 0;                      //临时变量,轮询数组用
    int32_t nfds = 0;                   //临时变量,有多少个socket有事件
    const int32_t MAXEVENTS = 1024;     //最大事件数
    epoll_event events[MAXEVENTS];      //监听事件数组
    int32_t iBackStoreSize = 1024;
    char buf[MAXBUFSIZE] = {0};
    uint32_t buf_index = 0;
    RecvMsgPkg &temp_recv_msg = net_info.temp_msg;
    const uint32_t recv_msg_head_length = temp_recv_msg.head_length();
    int32_t nread = 0;                               //读到的字节数
    ipport tIpPort;                                  //地址端口信息
    peerinfo tPeerInfo;                              //对方连接信息
    std::map<int32_t, ipport> mIpPort;               // socket对应的对方地址端口信息
    std::map<int32_t, ipport>::iterator itIpPort;    //临时迭代子
    std::map<ipport, peerinfo>::iterator itPeerInfo; //临时迭代子
    pipemsg msg;                                     //消息队列数据

    //创建epoll,对2.6.8以后的版本,其参数无效,只要大于0的数值就行,内核自己动态分配
    epfd = epoll_create(iBackStoreSize);
    if (epfd < 0)
    {
        log_print(LOG_ERROR, u8"ReadThread, epoll_create fail:%d,errno:%d", epfd, errno);
        return nullptr;
    }
    std::function<void(int, int)> func_close = [&](int fd, int error_no)
    {
        log_print(LOG_INFO, u8"ReadThread, close:%d,errno:%d", fd, error_no);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
        auto user_itor = net_info.users_map.find(net_info.conn_user_map[fd]);
        if (user_itor != net_info.users_map.end())
        {
            user_itor->second->on_set_online_status(user_status::STATUS_OFF_LINE);
        }
        net_info.user_conn_map.erase(net_info.conn_user_map[fd]);
        net_info.conn_user_map.erase(fd);
        itIpPort = mIpPort.find(fd);
        if (itIpPort != mIpPort.end())
        {
            mIpPort.erase(itIpPort);
            itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
            if (itPeerInfo != comm->conn_info.peer.end())
            {
                comm->conn_info.peer.erase(itPeerInfo);
            }
        }
    };

    while (comm->running)
    {
        //从管道读数据
        do
        {
            ret = read(comm->conn_info.rfd, &msg, 14);
            if (ret > 0)
            {
                //队列中的fd必须是有效的
                if (ret == 14 && msg.fd > 0)
                {
                    if (msg.op == 1) //收到新的连接
                    {
                        log_print(LOG_INFO, u8"ReadThread, recv connect:%d,errno:%d", msg.fd, errno);
                        //把socket设置为非阻塞方式
                        common::setnonblocking(msg.fd);
                        //设置描述符信息和数组下标信息
                        ev.data.fd = msg.fd;
                        //设置用于注测的读操作事件
                        ev.events = EPOLLIN | EPOLLET;
                        //注册ev
                        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, msg.fd, &ev);
                        if (ret != 0)
                        {
                            log_print(LOG_ERROR, u8"ReadThread, epoll_ctl fail:%d,errno:%d", ret, errno);
                            close(msg.fd);
                        }
                        else
                        {
                            mIpPort[msg.fd] = tIpPort;
                            tPeerInfo.fd = msg.fd;
                            tPeerInfo.contime = time(NULL);
                            tPeerInfo.rcvtime = 0;
                            tPeerInfo.rcvbyte = 0;
                            tPeerInfo.sndtime = 0;
                            tPeerInfo.sndbyte = 0;
                            comm->conn_info.peer[tIpPort] = tPeerInfo;
                        }
                    }
                    else if (msg.op == 2) //断开某个连接
                    {
                        func_close(msg.fd, errno);
                    }
                }
            }
            else
            {
                break;
            }
        } while (comm->running);

        //等待epoll事件的发生,如果当前有信号的句柄数大于输出事件数组的最大大小,超过部分会在下次epoll_wait时输出,事件不会丢
        nfds = epoll_wait(epfd, events, MAXEVENTS, 500);
        //处理所发生的所有事件
        for (i = 0; i < nfds && comm->running; ++i)
        {
            if (events[i].events & EPOLLIN) //有数据可读
            {
                do
                {
                    nread = read(events[i].data.fd, buf + buf_index, MAXBUFSIZE - buf_index);
                    if (nread > 0) //读到数据
                    {
                        itIpPort = mIpPort.find(events[i].data.fd);
                        if (itIpPort != mIpPort.end())
                        {
                            itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
                            if (itPeerInfo != comm->conn_info.peer.end())
                            {
                                itPeerInfo->second.rcvtime = time(NULL);
                                itPeerInfo->second.rcvbyte += nread;
                            }
                        }
                        uint32_t read_index = 0;
                        while ((uint32_t)nread > recv_msg_head_length)
                        {
                            temp_recv_msg.fill_head((uint8_t *)buf, nread);
                            if (temp_recv_msg.get_sub_pkg_data_length() + recv_msg_head_length <= (uint32_t)nread)
                            {
                                uint32_t temp_read_index = net_info.parse_msg((uint8_t *)(buf + read_index), nread, events[i].data.fd, temp_recv_msg);
                                if (temp_read_index == 0)
                                {
                                    log_print(LOG_ERROR, u8"parse msg failed!");
                                    read_index = 0;
                                    nread = 0;
                                    buf_index = 0;
                                    break;
                                }
                                nread -= temp_read_index;
                                read_index += temp_read_index;
                            }
                            else
                            {
                                if (net_info.users_map.find(temp_recv_msg.get_pkg_sender_guid()) == net_info.users_map.end() &&
                                    net_info.groups_map.find(temp_recv_msg.get_pkg_sender_guid()) == net_info.groups_map.end())
                                {
                                    log_print(LOG_ERROR, u8"pkg_sender_guid not exit!");
                                    buf_index = MAXBUFSIZE;
                                    nread = 0;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                        if (buf_index < read_index)
                            buf_index = 0;
                        if (read_index)
                            memcpy(buf, buf + read_index, nread);
                        buf_index += (uint32_t)nread;
                        if (buf_index == MAXBUFSIZE)
                        {
                            log_print(LOG_ERROR, u8"read data failed!");
                            buf_index = 0;
                        }
                        // log_print(LOG_DEBUG, u8"parse msg end,nread: %d,buf_index: %d,read_index: %d!", nread, buf_index, read_index);
                    }
                    else if (nread < 0) //读取失败
                    {
                        if (errno == EAGAIN) //没有数据了
                        {
                            break;
                        }
                        else if (errno == EINTR) //可能被内部中断信号打断,经过验证对非阻塞socket并未收到此错误,应该可以省掉该步判断
                        {
                            log_print(LOG_INFO, u8"ReadThread, read:%d,errno:%d,interrupt", nread, errno);
                        }
                        else //客户端主动关闭
                        {
                            func_close(events[i].data.fd, errno);
                            break;
                        }
                    }
                    else if (nread == 0) //客户端主动关闭
                    {
                        func_close(events[i].data.fd, errno);
                        break;
                    }
                } while (comm->running);
            }
            else if (events[i].events & EPOLLOUT) //有数据可写
            {
                ev.data.fd = events[i].data.fd;
                ev.events = EPOLLIN | EPOLLET;
                //注册ev
                ret = epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                if (ret != 0)
                {
                    log_print(LOG_ERROR, u8"ReadThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
                    // close(msg.fd);
                }
            }
            else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) //有异常发生
            {
                func_close(events[i].data.fd, errno);
            }
        }
        while (!net_info.msg_list.empty())
        {
            std::shared_ptr<MsgInfo> &msg_t = net_info.msg_list.front();
            temp_recv_msg.set_pkg_sender_guid(msg_t->send_guid);
            temp_recv_msg.set_pkg_type(recv_msg_type::CLIENT_MSG_SEND);
            if (msg_t->msg_type == MsgInfo::MSG_TYPE_GROUP)
            {
                auto itor = net_info.groups_map.find(msg_t->recv_guid);
                if (itor != net_info.groups_map.end())
                {
                    auto sender = itor->second->get_member_by_guid(msg_t->send_guid);
                    if (sender->permissions > group_permission::SEND_MSG)
                    {
                        if ((sender->permissions == group_permission::SEND_MSG_COUNT_LIMIT_50 && sender->msg_count > 50) ||
                            (sender->permissions == group_permission::SEND_MSG_COUNT_LIMIT_10 && sender->msg_count > 10) ||
                            sender->permissions == group_permission::CANT_SEND_MSG)
                        {
                            net_info.send_client_ack(recv_msg_type::CLIENT_MSG_SEND_ACK, ERROR_GROUP_MSG_LIMIT, net_info.user_conn_map[msg_t->send_guid]);
                            net_info.msg_list.pop_front();
                            continue;
                        }
                    }
                    itor->second->on_add_msg(msg_t);
                    if (!temp_recv_msg.set_sub_pkg_data_length(msg_t->to_data((uint8_t *)(send_buf + recv_msg_head_length), MAXBUFSIZE - recv_msg_head_length)) || !temp_recv_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
                    {
                        log_print(LOG_ERROR, u8"封包失败!");
                        net_info.msg_list.pop_front();
                        continue;
                    }
                    itor->second->traverse_all_members([&](std::shared_ptr<group_member_info> &info) -> bool
                                                       {
                                                           // log_print(LOG_DEBUG, u8"msg_t = %s!", msg_t->msg.c_str());
                                                           auto user_itor = net_info.users_map.find(info->user_guid);
                                                           if (user_itor == net_info.users_map.end())
                                                               return false;
                                                           user_itor->second->on_update_group_active(msg_t->recv_guid);
                                                           auto conn_itor = net_info.user_conn_map.find(info->user_guid);
                                                           if (conn_itor != net_info.user_conn_map.end())
                                                           {
                                                               if (conn_itor->first == msg_t->send_guid)
                                                               {
                                                                   ++info->msg_count;
                                                               }
                                                               net_info.send_msg(conn_itor->second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.get_sub_pkg_data_length());
                                                           }
                                                           return true; });
                }
            }
            else if (msg_t->msg_type == MsgInfo::MSG_TYPE_USER)
            {
                auto user_itor = net_info.users_map.find(msg_t->recv_guid);
                if (user_itor == net_info.users_map.end())
                {
                    net_info.msg_list.pop_front();
                    continue;
                }
                if (user_itor->second->on_update_friend_active(msg_t->send_guid))
                {
                    user_itor->second->on_add_msg(msg_t);
                }
                else
                {
                    // TODO 提示先添加好友
                    continue;
                }
                auto send_user_itor = net_info.users_map.find(msg_t->send_guid);
                if (send_user_itor == net_info.users_map.end())
                {
                    net_info.msg_list.pop_front();
                    continue;
                }
                if (send_user_itor->second->on_update_friend_active(msg_t->recv_guid))
                    send_user_itor->second->on_add_msg(msg_t);
                if (!temp_recv_msg.set_sub_pkg_data_length(msg_t->to_data((uint8_t *)(send_buf + recv_msg_head_length), MAXBUFSIZE - recv_msg_head_length)) || !temp_recv_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
                {
                    log_print(LOG_ERROR, u8"封包失败!");
                    net_info.msg_list.pop_front();
                    continue;
                }
                auto conn_itor = net_info.user_conn_map.find(msg_t->recv_guid);
                if (conn_itor != net_info.user_conn_map.end())
                {
                    net_info.send_msg(conn_itor->second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.get_sub_pkg_data_length());
                }
                auto conn_s_itor = net_info.user_conn_map.find(msg_t->send_guid);
                if (conn_s_itor != net_info.user_conn_map.end() && msg_t->send_guid != msg_t->recv_guid)
                    net_info.send_msg(conn_s_itor->second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.get_sub_pkg_data_length());
            }
            else if (msg_t->msg_type == MsgInfo::MSG_TYPE_ALL)
            {
                for (const auto &it : net_info.user_conn_map)
                {
                    if (it.first == msg_t->send_guid)
                    {
                        net_info.msg_list.pop_front();
                        continue;
                    }
                    if (!temp_recv_msg.set_sub_pkg_data_length(msg_t->to_data((uint8_t *)(send_buf + recv_msg_head_length), MAXBUFSIZE - recv_msg_head_length)) || !temp_recv_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
                    {
                        log_print(LOG_ERROR, u8"封包失败!");
                        net_info.msg_list.pop_front();
                        continue;
                    }
                    net_info.send_msg(it.second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.get_sub_pkg_data_length());
                }
            }
            net_info.msg_list.pop_front();
        }
    }

    //关闭所有连接
    for (itIpPort = mIpPort.begin(); itIpPort != mIpPort.end(); itIpPort++)
    {
        if (itIpPort->first > 0)
        {
            close(itIpPort->first);
        }
    }
    //关闭创建的epoll
    if (epfd > 0)
    {
        close(epfd);
    }
    log_print(LOG_INFO, u8"ReadThread, exit.");
    return nullptr;
}