#pragma once
#include "common.h"
#include "struct_def.h"
#include "user_info.h"
#include "group_info.h"

#define MAXBUFSIZE 8192 //数据缓冲区大小

//#define USE_PHONE_TREE
#ifdef USE_PHONE_TREE
struct phone_tree
{
    phone_tree()
    {
        num_list = nullptr;
        user_guid = 0;
    }
    void release()
    {
        if (num_list)
        {
            for (auto &item : *num_list)
            {
                if (item)
                {
                    item->release();
                    delete item;
                    item = nullptr;
                }
            }
            delete num_list;
            num_list = nullptr;
        }
    }
    phone_tree *get_phone_tree(char a)
    {
        if (a < '0' || a > '9' || !num_list)
        {
            return nullptr;
        }
        return (*num_list)[(uint32_t)(a - '0')];
    }
    phone_tree **num_list[10];
    uint32_t user_guid;
};
#endif

class NetInfo
{
public:
    NetInfo() : temp_msg(0, recv_msg_type::RECV_MSG_TYPE_MAX)
    {
#ifdef USE_PHONE_TREE
        phone_user_tree = new phone_tree();
#endif
        recv_msg_head_length = temp_msg.head_length();

        //添加公共聊天室 guid 1
        GroupInfo info;
        info.set_group_guid(1);
        groups.emplace_back(info);
        groups_map[1] = std::prev(groups.end());
    }
    ~NetInfo()
    {
#ifdef USE_PHONE_TREE
        phone_user_tree->release();
        delete phone_user_tree;
        phone_user_tree = nullptr;
#endif
    }
    uint32_t parse_msg(const uint8_t *data, uint32_t len, int32_t fd, RecvMsgPkg &msg);
    uint32_t on_acount_add(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_modify(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_delete(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_login(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_logout(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_acount_can_reg(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_add(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_modify(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_delete(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_user_info_req(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_group_info_req(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_user_brief_info_req(const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_msg_send(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd);
    uint32_t on_msg_list_req();
    int32_t send_msg(int32_t fd, uint8_t *data, uint32_t len);
    uint32_t on_login(const std::string &acount, const std::string &sha256);
    uint32_t on_phone_unique(std::string acount);
    void send_client_ack(uint16_t msg_type, uint16_t ack_type, int32_t fd);

public:
    RecvMsgPkg temp_msg;
    uint32_t recv_msg_head_length = 0;

public:
    std::map<int32_t, uint32_t> conn_user_map;    //连接客户端列表
    std::map<uint32_t, int32_t> user_conn_map;    //连接客户端列表
    std::list<std::shared_ptr<MsgInfo>> msg_list; //待发送消息队列
    std::list<GroupInfo> groups;
    std::map<uint32_t, std::list<GroupInfo>::iterator> groups_map;
    std::list<UserInfo> users;
    std::map<uint32_t, std::list<UserInfo>::iterator> users_map;
    int32_t epfd;              //连接用的epoll
    epoll_event ev;            //事件临时变量
    char send_buf[MAXBUFSIZE]; //发送缓冲区
#ifdef USE_PHONE_TREE
    phone_tree *phone_user_tree;
#else
    std::map<size_t, uint32_t> phone_hash_map;
#endif
};