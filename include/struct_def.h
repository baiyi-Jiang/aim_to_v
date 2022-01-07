#pragma once
#include <deque>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include "common.h"

//管道消息结构
struct pipemsg
{
    int32_t op;
    int32_t fd;
    uint32_t ip;
    uint16_t port;
};

//地址端口结构
struct ipport
{
    uint32_t ip;
    uint16_t port;
    bool operator<(const ipport rhs) const { return (ip < rhs.ip || (ip == rhs.ip && port < rhs.port)); }
    bool operator==(const ipport rhs) const { return (ip == rhs.ip && port == rhs.port); }
};

//对应于对方地址端口的连接信息
struct peerinfo
{
    int32_t fd;       //对应连接句柄
    uint32_t contime; //最后连接时间
    uint32_t rcvtime; //收到数据时间
    uint32_t rcvbyte; //收到字节个数
    uint32_t sndtime; //发送数据时间
    uint32_t sndbyte; //发送字节个数
};

//连接结构
struct conninfo
{
    int32_t rfd;                                   //管道读端
    int32_t wfd;                                   //管道写端
    std::map<struct ipport, struct peerinfo> peer; //对方信息
};

//消息存储结构
struct msg_info
{
    enum MSGTYPE
    {
        MSG_TYPE_ALL,
        MSG_TYPE_GROUP,
        MSG_TYPE_USER,
        MSG_TYPE_MAX,
    };
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(send_guid, data + index);
        index += memcpy_u(recv_guid, data + index);
        index += memcpy_u(time_sec, data + index);
        index += memcpy_u(msg_length, data + index);
        index += memcpy_u(msg_num, data + index);
        msg_type = data[index++];
        keep1 = data[index++];
        if (len - length() < msg_length)
            return false;
        msg.assign((char *)data, (size_t)msg_length);
        return true;
    }
    uint32_t length()
    {
        uint32_t len = sizeof(send_guid);
        len += sizeof(recv_guid);
        len += sizeof(time_sec);
        len += sizeof(msg_length);
        len += sizeof(msg_num);
        len += sizeof(msg_type);
        len += sizeof(keep1);
        return len;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length() + msg.size())
            return index;
        index += memcpy_u(data + index, send_guid);
        index += memcpy_u(data + index, recv_guid);
        index += memcpy_u(data + index, time_sec);
        index += memcpy_u(data + index, msg_length);
        index += memcpy_u(data + index, msg_num);
        index += memcpy_u(data + index, msg_type);
        index += memcpy_u(data + index, keep1);
        memcpy(data + index, msg.c_str(), msg.size());
        index += (uint32_t)msg.size();
        return index;
    }
    uint32_t send_guid;  //发送者guid
    uint32_t recv_guid;  //接收者guid
    uint32_t time_sec;   //发送消息的时间戳
    uint32_t msg_length; //消息的长度
    uint16_t msg_num;    //消息序号
    uint8_t msg_type;    //消息类型 0公共频道消息 1群组消息 2私人消息
    uint8_t keep1;       //保留字段1
    std::string msg;     //消息
};

struct user_status
{
    enum status : uint8_t
    {
        STATUS_ON_LINE = 0x00,  //在线
        STATUS_OFF_LINE = 0x01, //离线
        STATUS_MAX
    };
};

struct group_permission
{
    enum permission : uint8_t
    {
        SEND_TO_ALL = 0x00,             //可以发送给所有人
        SEND_TO_GROUP = 0x01,           //可以在群组内发任何消息
        SEND_NOTIFY = 0x02,             //可以发布公告
        SEND_FILE = 0x03,               //可以发送文件，包括图片和音视频等
        SEND_MEDIA = 0x04,              //可以发起音视频会话
        SEND_MSG = 0x05,                //只能发送消息，包括内置表情
        SEND_MSG_COUNT_LIMIT_50 = 0x06, //只允许发送50条消息
        SEND_MSG_COUNT_LIMIT_10 = 0x07, //只能发送10条消息
        CANT_SEND_MSG = 0x08,           //不允许发送消息
        SEND_MAX
    };
};

struct group_member_info
{
    enum JOB : uint8_t
    {
        GROUP_JOB_LEADER,  //组长
        GROUP_JOB_MANAGER, //管理员
        GROUP_JOB_MEMBER,  //普通成员
        GROUP_JOB_MAX
    };
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(group_guid, data + index);
        index += memcpy_u(user_guid, data + index);
        index += memcpy_u(icon_guid, data + index);
        index += memcpy_u(msg_count, data + index);
        permissions = data[index++];
        job = data[index++];
        memset(name, '\0', sizeof(name));
        memcpy(name, data + index, sizeof(name));
        index += sizeof(name);
        return true;
    }
    uint32_t length()
    {
        uint32_t len = sizeof(group_guid);
        len += sizeof(user_guid);
        len += sizeof(icon_guid);
        len += sizeof(msg_count);
        len += sizeof(permissions);
        len += sizeof(job);
        len += sizeof(name);
        return len;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, group_guid);
        index += memcpy_u(data + index, user_guid);
        index += memcpy_u(data + index, icon_guid);
        index += memcpy_u(data + index, msg_count);
        index += memcpy_u(data + index, permissions);
        index += memcpy_u(data + index, job);
        memcpy(data + index, name, sizeof(name));
        index += sizeof(name);
        return index;
    }
    uint32_t group_guid; //组guid
    uint32_t user_guid;  //用户GUID
    uint32_t icon_guid;  //头像GUID
    uint16_t msg_count;  //发言统计
    uint8_t permissions; //权限group_permission::permission
    uint8_t job;         //群组中的职务 //JOB
    uint8_t name[32];    //用户姓名
};

struct recv_msg_type
{
    enum msg_type : uint16_t
    {
        ACOUNT_ADD = 00,    //新增账号
        ACOUNT_MODIFY = 01, //修改账号
        ACOUNT_DELETE = 02, //删除账号
        GROUP_ADD = 30,     //新增群组
        GROUP_MODIFY = 31,  //修改群组
        GROUP_DELETE = 32,  //删除群组
        // GROUP_JOIN = 33,  //加入群组 //加在修改群组下
        // GROUP_LEAVE = 34,  //离开群组 //加在修改群组下
        // GROUP_MDIFY_PERMISSION = 35,  //修改群组成员权限 //加在修改群组下
        // GROUP_ADD_MANAGER = 36,  //增加群组管理员 //加在修改群组下
        // MSG_LIST_REQ = 37,  //删除群组管理员 //加在修改群组下
        USER_INFO_REQ = 60,          //请求联系人信息
        GROUP_INFO_REQ = 61,         //请求群组信息
        MSG_SEND = 90,               //发送消息
        MSG_LIST_REQ = 91,           //请求聊天记录
        LINE_ON = 120,               //用户上线
        LINE_OFF = 121,              //用户下线
        SERVER_MSG_TYPE_MAX = 10000, //服务端消息类型限制

        CLIENT_ACOUNT_ADD_ACK = 10000,    //新增账号回包
        CLIENT_ACOUNT_MODIFY_ACK = 10001, //修改账号回包
        CLIENT_ACOUNT_DELETE_ACK = 10002, //删除账号回包
        CLIENT_GROUP_ADD_ACK = 10030,     //新增群组回包
        CLIENT_GROUP_MODIFY_ACK = 10031,  //修改群组回包
        CLIENT_GROUP_DELETE_ACK = 10032,  //删除群组回包
        CLIENT_USER_INFO_ACK = 10060,     //回复联系人信息
        CLIENT_GROUP_INFO_ACK = 10061,    //回复群组信息
        CLIENT_MSG_SEND = 10090,          //发送消息
        CLIENT_MSG_LIST_ACK = 10091,      //回复聊天记录
        RECV_MSG_TYPE_MAX
    };
};

struct acount_delete
{
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(user_guid, data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t length = sizeof(user_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, user_guid);
        return index;
    }
    uint32_t user_guid;
};

struct user_info_req
{
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(user_guid, data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t length = sizeof(user_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, user_guid);
        return index;
    }
    uint32_t user_guid;
};

struct group_info_req
{
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(group_guid, data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t length = sizeof(group_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, group_guid);
        return index;
    }
    uint32_t group_guid;
};

struct recv_msg
{
    bool from_data(const uint8_t *src_data, const uint32_t len)
    {
        if (!src_data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(guid, src_data + index);
        index += memcpy_u(type, src_data + index);
        index += memcpy_u(data_length, src_data + index);
        if (len - length() < data_length)
        {
            return false;
        }
        data = (uint8_t *)(src_data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t len = sizeof(guid);
        len += sizeof(type);
        len += sizeof(data_length);
        return len;
    }
    uint32_t to_head(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, guid);
        index += memcpy_u(data + index, type);
        index += memcpy_u(data + index, data_length);
        return index;
    }
    uint32_t guid;
    uint16_t type; //recv_msg_type:type
    uint32_t data_length;
    uint8_t *data;
};

struct group_modify
{
    enum ModifyType : uint8_t
    {
        MEMBER_ADD = 0x00,
        MEMBER_DELETE = 0x01,
        MEMBER_SET_PERMISSION = 0x02,
        MANAGER_ADD = 0x03,
        MANAGER_DELETE = 0x04,
        ModifyTypeMAX
    };
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(group_guid, data + index);
        index += memcpy_u(operate_guid, data + index);
        index += memcpy_u(target_guid, data + index);
        index += memcpy_u(opreadte_type, data + index);
        index += memcpy_u(permission, data + index);
        index += memcpy_u(keep1, data + index);
        index += memcpy_u(keep2, data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t length = sizeof(group_guid);
        length += sizeof(operate_guid);
        length += sizeof(target_guid);
        length += sizeof(opreadte_type);
        length += sizeof(permission);
        length += sizeof(keep1);
        length += sizeof(keep2);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, group_guid);
        index += memcpy_u(data + index, operate_guid);
        index += memcpy_u(data + index, target_guid);
        index += memcpy_u(data + index, opreadte_type);
        index += memcpy_u(data + index, permission);
        index += memcpy_u(data + index, keep1);
        index += memcpy_u(data + index, keep2);
        return index;
    }
    uint32_t group_guid;
    uint32_t operate_guid;
    uint32_t target_guid;
    uint8_t opreadte_type; //ModifyType
    uint8_t permission;
    uint8_t keep1;
    uint8_t keep2;
};

struct group_delete
{
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        index += memcpy_u(group_guid, data + index);
        index += memcpy_u(operate_guid, data + index);
        return true;
    }
    uint32_t length()
    {
        uint32_t length = sizeof(group_guid);
        length += sizeof(operate_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += memcpy_u(data + index, group_guid);
        index += memcpy_u(data + index, operate_guid);
        return index;
    }
    uint32_t group_guid;
    uint32_t operate_guid;
};

//全局运行信息
struct common_info
{
    bool running;
    struct conninfo conn_info;
};
