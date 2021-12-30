#pragma once
#include "common.h"
#include "struct_def.h"
#include "user_info.h"
#include "group_info.h"

class NetInfo
{
public:
    NetInfo()
    {
        //
    }
    ~NetInfo()
    {
        //
    }
    bool parse_msg(const uint8_t *data, uint32_t len, int32_t fd, const recv_msg &msg);
    void on_acount_add(const uint8_t *data, uint32_t len, int32_t fd);
    void on_acount_modify();
    void on_acount_delete();
    void on_group_add();
    void on_group_modify();
    void on_group_delete();
    void on_user_info_req();
    void on_msg_send(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd);
    void on_msg_list_req();
    int32_t send_msg(int32_t fd, uint8_t *data, uint32_t len, struct epoll_event &ev, int32_t epfd);

public:
    std::map<int32_t, uint32_t> conn_user_map;     //连接客户端列表
    std::map<uint32_t, int32_t> user_conn_map;     //连接客户端列表
    std::list<std::shared_ptr<msg_info>> msg_list; //待发送消息队列
    std::list<GroupInfo> groups;
    std::map<uint32_t, std::list<GroupInfo>::iterator> groups_map;
    std::list<UserInfo> users;
    std::map<uint32_t, std::list<UserInfo>::iterator> users_map;
};