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
        memcpy_uint32(send_guid, data + index);
        index += sizeof(send_guid);
        memcpy_uint32(recv_guid, data + index);
        index += sizeof(recv_guid);
        memcpy_uint32(time_sec, data + index);
        index += sizeof(time_sec);
        memcpy_uint32(msg_length, data + index);
        index += sizeof(msg_length);
        memcpy_uint16(msg_num, data + index);
        index += sizeof(msg_num);
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
        memcpy(data + index, &send_guid, sizeof(send_guid));
        index += sizeof(send_guid);
        memcpy(data + index, &recv_guid, sizeof(recv_guid));
        index += sizeof(recv_guid);
        memcpy(data + index, &time_sec, sizeof(time_sec));
        index += sizeof(time_sec);
        memcpy(data + index, &msg_length, sizeof(msg_length));
        index += sizeof(msg_length);
        memcpy(data + index, &msg_num, sizeof(msg_num));
        index += sizeof(msg_num);
        memcpy(data + index, &msg_type, sizeof(msg_type));
        index += sizeof(msg_type);
        memcpy(data + index, &keep1, sizeof(keep1));
        index += sizeof(keep1);
        memcpy(data + index, msg.c_str(), msg.size());
        index += msg.size();
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
        memcpy_uint32(group_guid, data + index);
        index += sizeof(group_guid);
        memcpy_uint32(user_guid, data + index);
        index += sizeof(user_guid);
        memcpy_uint32(icon_guid, data + index);
        index += sizeof(icon_guid);
        memcpy_uint16(msg_count, data + index);
        index += sizeof(msg_count);
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
        memcpy(data + index, &group_guid, sizeof(group_guid));
        index += sizeof(group_guid);
        memcpy(data + index, &user_guid, sizeof(user_guid));
        index += sizeof(user_guid);
        memcpy(data + index, &icon_guid, sizeof(icon_guid));
        index += sizeof(icon_guid);
        memcpy(data + index, &msg_count, sizeof(msg_count));
        index += sizeof(msg_count);
        memcpy(data + index, &permissions, sizeof(permissions));
        index += sizeof(permissions);
        memcpy(data + index, &job, sizeof(job));
        index += sizeof(job);
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
        SERVER_MSG_TYPE_MAX = 10000, //服务端消息类型限制
        
        CLIENT_USER_INFO_ACK = 10060,  //回复联系人信息
        CLIENT_GROUP_INFO_ACK = 10061, //回复群组信息
        CLIENT_MSG_SEND = 10090,       //发送消息
        CLIENT_MSG_LIST_ACK = 10091,   //回复聊天记录
        RECV_MSG_TYPE_MAX
    };
};

struct user_info_req
{
    bool from_data(const uint8_t *data, const uint32_t len)
    {
        if (!data || len < length())
            return false;
        uint32_t index = 0;
        memcpy_uint32(user_guid, data + index);
        index += sizeof(user_guid);
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
        memcpy(data + index, &user_guid, sizeof(user_guid));
        index += sizeof(user_guid);
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
        memcpy_uint32(group_guid, data + index);
        index += sizeof(group_guid);
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
        memcpy(data + index, &group_guid, sizeof(group_guid));
        index += sizeof(group_guid);
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
        memcpy_uint32(guid, src_data + index);
        index += sizeof(guid);
        memcpy_uint16(type, src_data + index);
        index += sizeof(type);
        memcpy_uint32(data_length, src_data + index);
        index += sizeof(data_length);
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
        memcpy(data + index, &guid, sizeof(guid));
        index += sizeof(guid);
        memcpy(data + index, &type, sizeof(type));
        index += sizeof(type);
        memcpy(data + index, &data_length, sizeof(data_length));
        index += sizeof(data_length);
        return index;
    }
    uint32_t guid;
    uint16_t type; //recv_msg_type:type
    uint32_t data_length;
    uint8_t *data;
};

//全局运行信息
struct common_info
{
    bool running;
    struct conninfo conn_info;
};
