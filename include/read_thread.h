#pragma once
#include "common.h"
#include "struct_def.h"
#include "user_info.h"
#include "group_info.h"

#define MAXBUFSIZE 8192 //数据缓冲区大小

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
    uint32_t parse_msg(const uint8_t *data, uint32_t len, int32_t fd, struct recv_msg &msg);
    uint32_t on_acount_add(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_modify(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_delete(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_add(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_modify(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_delete(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_user_info_req(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_info_req(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_msg_send(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_msg_list_req();
    int32_t send_msg(int32_t fd, uint8_t *data, uint32_t len);

public:
    std::map<int32_t, uint32_t> conn_user_map;     //连接客户端列表
    std::map<uint32_t, int32_t> user_conn_map;     //连接客户端列表
    std::list<std::shared_ptr<msg_info>> msg_list; //待发送消息队列
    std::list<GroupInfo> groups;
    std::map<uint32_t, std::list<GroupInfo>::iterator> groups_map;
    std::list<UserInfo> users;
    std::map<uint32_t, std::list<UserInfo>::iterator> users_map;
    int32_t epfd;              //连接用的epoll
    struct epoll_event ev;     //事件临时变量
    char send_buf[MAXBUFSIZE]; //发送缓冲区
};