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
    int32_t rfd;                     //管道读端
    int32_t wfd;                     //管道写端
    std::map<ipport, peerinfo> peer; //对方信息
};

enum class SexType : uint8_t
{
    MALE = 0x00,
    FEMALE,
    OTHERSEX
};

struct recv_msg_type
{
    enum msg_type : uint16_t
    {
        ACOUNT_ADD = 00,             //新增账号
        ACOUNT_MODIFY = 01,          //修改账号
        ACOUNT_DELETE = 02,          //删除账号
        ACOUNT_LOGIN = 03,           //登录账号
        ACOUNT_LOGOUT = 04,          //登出账号
        ACOUNT_CAN_REG = 05,         //注册检查
        GROUP_ADD = 30,              //新增群组
        GROUP_MODIFY = 31,           //修改群组
        GROUP_DELETE = 32,           //删除群组
        USER_INFO_REQ = 60,          //请求联系人信息
        GROUP_INFO_REQ = 61,         //请求群组信息
        USER_INFO_BRIEF_REQ = 62,    //请求联系人简略信息
        MSG_SEND = 90,               //发送消息
        MSG_LIST_REQ = 91,           //请求聊天记录
        SERVER_MSG_TYPE_MAX = 10000, //服务端消息类型限制

        CLIENT_ACOUNT_ADD_ACK = 10000,         //新增账号回包
        CLIENT_ACOUNT_MODIFY_ACK = 10001,      //修改账号回包
        CLIENT_ACOUNT_DELETE_ACK = 10002,      //删除账号回包
        CLIENT_ACOUNT_LOGIN_ACK = 10003,       //登录账号回包
        CLIENT_ACOUNT_LOGOUT_ACK = 10004,      //登出账号回包
        CLIENT_ACOUNT_CAN_REG_ACK = 10005,     //注册检查回包
        CLIENT_GROUP_ADD_ACK = 10030,          //新增群组回包
        CLIENT_GROUP_MODIFY_ACK = 10031,       //修改群组回包
        CLIENT_GROUP_DELETE_ACK = 10032,       //删除群组回包
        CLIENT_USER_INFO_ACK = 10060,          //回复联系人信息
        CLIENT_GROUP_INFO_ACK = 10061,         //回复群组信息
        CLIENT_USER_INFO_BRIEF_NOTIFY = 10062, //推送联系人简略信息
        CLIENT_MSG_SEND_ACK = 10090,           //发送消息回包
        CLIENT_MSG_LIST_ACK = 10091,           //回复聊天记录
        CLIENT_MSG_SEND = 20090,               //发送消息
        CLIENT_ACK_SEND = 20001,               //一般消息回包
        RECV_MSG_TYPE_MAX
    };
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
    group_member_info()
    {
        group_guid = 0;
        user_guid = 0;
        icon_guid = 0;
        msg_count = 0;
        permissions = group_permission::CANT_SEND_MSG;
        job = GROUP_JOB_MEMBER;
        memset(name, '\0', sizeof(name));
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(group_guid, data + index);
        index += common::memcpy_u(user_guid, data + index);
        index += common::memcpy_u(icon_guid, data + index);
        index += common::memcpy_u(msg_count, data + index);
        permissions = data[index++];
        job = data[index++];
        memset(name, '\0', sizeof(name));
        memcpy(name, data + index, sizeof(name));
        index += sizeof(name);
        return index;
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
        index += common::memcpy_u(data + index, group_guid);
        index += common::memcpy_u(data + index, user_guid);
        index += common::memcpy_u(data + index, icon_guid);
        index += common::memcpy_u(data + index, msg_count);
        index += common::memcpy_u(data + index, permissions);
        index += common::memcpy_u(data + index, job);
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

class RecvMsgPkg
{
public:
    RecvMsgPkg(uint32_t guid, uint16_t type)
    {
        pkg_sender_guid = guid;
        pkg_type = type;
        sub_pkg_data_length = 0;
        sub_pkg_data = nullptr;
    }
    RecvMsgPkg() = delete;
    RecvMsgPkg(const RecvMsgPkg &) = delete;
    RecvMsgPkg &operator=(const RecvMsgPkg &) = delete;
    virtual ~RecvMsgPkg() {}

    uint32_t fill_head(const uint8_t *src_data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!src_data || len < head_length())
            return index;
        index += common::memcpy_u(pkg_sender_guid, src_data + index);
        index += common::memcpy_u(pkg_type, src_data + index);
        index += common::memcpy_u(sub_pkg_data_length, src_data + index);
        if (len - head_length() < sub_pkg_data_length)
            return index;
        sub_pkg_data = (uint8_t *)(src_data + index);
        index += sub_pkg_data_length;
        return index;
    }
    uint32_t head_length()
    {
        uint32_t len = sizeof(pkg_sender_guid);
        len += sizeof(pkg_type);
        len += sizeof(sub_pkg_data_length);
        return len;
    }
    uint32_t to_head(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (!data || len < head_length())
            return index;
        index += common::memcpy_u(data + index, pkg_sender_guid);
        index += common::memcpy_u(data + index, pkg_type);
        index += common::memcpy_u(data + index, sub_pkg_data_length);
        return index;
    }
    uint32_t make_pkg(uint8_t *data, const uint32_t len)
    {
        uint32_t index = 0;
        if (len < head_length() + length())
            return index;
        sub_pkg_data_length = to_data((uint8_t *)data + head_length(), len - head_length());
        if (sub_pkg_data_length && to_head((uint8_t *)data, len))
            index = sub_pkg_data_length + head_length();
        return index;
    }
    uint32_t get_pkg_sender_guid() { return pkg_sender_guid; }
    void set_pkg_sender_guid(uint32_t guid) { pkg_sender_guid = guid; }
    uint16_t get_pkg_type() { return pkg_type; }
    void set_pkg_type(uint16_t type) { pkg_type = type; }
    uint32_t get_sub_pkg_data_length() { return sub_pkg_data_length; }
    bool set_sub_pkg_data_length(uint32_t len)
    {
        if (!len)
            return false;
        sub_pkg_data_length = len;
        return true;
    }
    uint8_t *get_sub_pkg_data() { return sub_pkg_data; }
    virtual uint32_t from_data(const uint8_t *data, const uint32_t len)
    {
        Unused(data);
        Unused(len);
        return 0;
    }
    virtual uint32_t length() { return 0; }
    virtual uint32_t to_data(uint8_t *data, const uint32_t len)
    {
        Unused(data);
        Unused(len);
        return 0;
    }

public:
    uint32_t pkg_sender_guid;
    uint16_t pkg_type; // recv_msg_type:type
private:
    uint32_t sub_pkg_data_length;
    uint8_t *sub_pkg_data;
};

//消息存储结构
class MsgInfo : public RecvMsgPkg
{
public:
    enum MSGTYPE
    {
        MSG_TYPE_ALL,
        MSG_TYPE_GROUP,
        MSG_TYPE_USER,
        MSG_TYPE_MAX,
    };
    MsgInfo() : RecvMsgPkg(0, recv_msg_type::MSG_SEND)
    {
        send_guid = 0;
        recv_guid = 0;
        time_sec = 0;
        msg_length = 0;
        msg_num = 0;
        msg_type = 0;
        keep1 = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(send_guid, data + index);
        index += common::memcpy_u(recv_guid, data + index);
        index += common::memcpy_u(time_sec, data + index);
        index += common::memcpy_u(msg_length, data + index);
        index += common::memcpy_u(msg_num, data + index);
        msg_type = data[index++];
        keep1 = data[index++];
        if (len - length() < msg_length)
        {
            index = 0;
            return index;
        }
        msg.assign((char *)data, (size_t)msg_length);
        return index;
    }
    uint32_t length() override
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
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length() + msg.size())
            return index;
        index += common::memcpy_u(data + index, send_guid);
        index += common::memcpy_u(data + index, recv_guid);
        index += common::memcpy_u(data + index, time_sec);
        index += common::memcpy_u(data + index, msg_length);
        index += common::memcpy_u(data + index, msg_num);
        index += common::memcpy_u(data + index, msg_type);
        index += common::memcpy_u(data + index, keep1);
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

class AcountDelete : public RecvMsgPkg
{
public:
    AcountDelete() : RecvMsgPkg(0, recv_msg_type::ACOUNT_DELETE)
    {
        user_guid = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(user_guid, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(user_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, user_guid);
        return index;
    }
    uint32_t user_guid;
};

class UserInfoReq : public RecvMsgPkg
{
public:
    UserInfoReq() : RecvMsgPkg(0, recv_msg_type::USER_INFO_REQ)
    {
        user_guid = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(user_guid, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(user_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, user_guid);
        return index;
    }
    uint32_t user_guid;
};

class GroupInfoReq : public RecvMsgPkg
{
public:
    GroupInfoReq() : RecvMsgPkg(0, recv_msg_type::GROUP_INFO_REQ)
    {
        group_guid = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(group_guid, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(group_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, group_guid);
        return index;
    }
    uint32_t group_guid;
};

class GroupModify : public RecvMsgPkg
{
public:
    enum ModifyType : uint8_t
    {
        MEMBER_ADD = 0x00,
        MEMBER_DELETE = 0x01,
        MEMBER_SET_PERMISSION = 0x02,
        MANAGER_ADD = 0x03,
        MANAGER_DELETE = 0x04,
        ModifyTypeMAX
    };
    GroupModify() : RecvMsgPkg(0, recv_msg_type::GROUP_MODIFY)
    {
        group_guid = 0;
        operate_guid = 0;
        target_guid = 0;
        opreadte_type = ModifyTypeMAX;
        permission = group_permission::CANT_SEND_MSG;
        keep1 = 0;
        keep2 = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(group_guid, data + index);
        index += common::memcpy_u(operate_guid, data + index);
        index += common::memcpy_u(target_guid, data + index);
        index += common::memcpy_u(opreadte_type, data + index);
        index += common::memcpy_u(permission, data + index);
        index += common::memcpy_u(keep1, data + index);
        index += common::memcpy_u(keep2, data + index);
        return index;
    }
    uint32_t length() override
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
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, group_guid);
        index += common::memcpy_u(data + index, operate_guid);
        index += common::memcpy_u(data + index, target_guid);
        index += common::memcpy_u(data + index, opreadte_type);
        index += common::memcpy_u(data + index, permission);
        index += common::memcpy_u(data + index, keep1);
        index += common::memcpy_u(data + index, keep2);
        return index;
    }
    uint32_t group_guid;
    uint32_t operate_guid;
    uint32_t target_guid;
    uint8_t opreadte_type; // ModifyType
    uint8_t permission;
    uint8_t keep1;
    uint8_t keep2;
};

class GroupDelete : public RecvMsgPkg
{
public:
    GroupDelete() : RecvMsgPkg(0, recv_msg_type::GROUP_DELETE)
    {
        group_guid = 0;
        operate_guid = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(group_guid, data + index);
        index += common::memcpy_u(operate_guid, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(group_guid);
        length += sizeof(operate_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, group_guid);
        index += common::memcpy_u(data + index, operate_guid);
        return index;
    }
    uint32_t group_guid;
    uint32_t operate_guid;
};

class AcountLogin : public RecvMsgPkg
{
public:
    AcountLogin() : RecvMsgPkg(0, recv_msg_type::ACOUNT_LOGIN)
    {
        memset(phone, '\0', sizeof(phone));
        passwd = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        memcpy(phone, data + index, sizeof(phone));
        index += sizeof(phone);
        index += common::memcpy_u(passwd, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(phone);
        length += sizeof(passwd);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        memcpy(data + index, phone, sizeof(phone));
        index += sizeof(phone);
        index += common::memcpy_u(data + index, passwd);
        return index;
    }
    uint8_t phone[16] = {0}; //电话号码
    size_t passwd;
};

class AcountLogout : public RecvMsgPkg
{
public:
    AcountLogout() : RecvMsgPkg(0, recv_msg_type::ACOUNT_LOGOUT)
    {
        user_guid = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(user_guid, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(user_guid);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, user_guid);
        return index;
    }
    uint32_t user_guid;
};

class AcountCanReg : public RecvMsgPkg
{
public:
    AcountCanReg() : RecvMsgPkg(0, recv_msg_type::ACOUNT_CAN_REG)
    {
        memset(phone, '\0', sizeof(phone));
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        memcpy(phone, data + index, sizeof(phone));
        index += sizeof(phone);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(phone);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        memcpy(data + index, phone, sizeof(phone));
        index += sizeof(phone);
        return index;
    }
    uint8_t phone[16] = {0}; //电话号码
};

class ClientAck : public RecvMsgPkg
{
public:
    ClientAck() : RecvMsgPkg(0, recv_msg_type::CLIENT_ACK_SEND)
    {
        err_no = 0;
    }
    ClientAck(recv_msg_type::msg_type type) : RecvMsgPkg(0, type)
    {
        err_no = 0;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(err_no, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(err_no);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, err_no);
        return index;
    }
    uint16_t err_no;
};

class BriefUserInfo : public RecvMsgPkg
{
public:
    BriefUserInfo() : RecvMsgPkg(0, recv_msg_type::CLIENT_USER_INFO_BRIEF_NOTIFY)
    {
        user_guid = 0;
        icon_guid = 0;
        memset(name, '\0', sizeof(name));
        online_status = user_status::STATUS_OFF_LINE;
    }
    uint32_t from_data(const uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(user_guid, data + index);
        index += common::memcpy_u(icon_guid, data + index);
        memcpy(name, data + index, sizeof(name));
        index += sizeof(name);
        index += common::memcpy_u(online_status, data + index);
        return index;
    }
    uint32_t length() override
    {
        uint32_t length = sizeof(user_guid);
        length += sizeof(icon_guid);
        length += sizeof(name);
        length += sizeof(online_status);
        return length;
    }
    uint32_t to_data(uint8_t *data, const uint32_t len) override
    {
        uint32_t index = 0;
        if (!data || len < length())
            return index;
        index += common::memcpy_u(data + index, user_guid);
        index += common::memcpy_u(data + index, icon_guid);
        memcpy(data + index, name, sizeof(name));
        index += sizeof(name);
        index += common::memcpy_u(data + index, online_status);
        return index;
    }
    uint32_t user_guid;
    uint32_t icon_guid;
    uint8_t name[32];      //用户姓名
    uint8_t online_status; //在线状态 //user_status
};

//全局运行信息
struct common_info
{
    bool running;
    conninfo conn_info;
};
