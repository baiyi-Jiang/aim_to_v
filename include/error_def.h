#pragma once

enum ErrorDef : uint16_t
{
    ERROR_OK = 0,
    ERROR_UNKNOW = 1,                        //未知错误
    ERROR_REFUSE = 2,                        //服务器拒绝
    ERROR_PARSE_MSG = 3,                     //解包失败
    ERROR_ACOUNT_ADD_FAILED = 10,            //新增账号失败
    ERROR_PHONE_EXIST = 11,                  //手机号已注册
    ERROR_PHONE_ERROR_CHARACTER = 12,        //手机号不是数字
    ERROR_ACOUNT_MODIFY = 20,                //修改账号失败
    ERROR_ACOUNT_DELETE = 30,                //删除账号失败
    ERROR_CANT_DELETE_OTHER = 31,            //不允许删除他人账号
    ERROR_ACOUNT_LOGIN = 40,                 //登录账号失败
    ERROR_PHONE_ISNOT_EXIST = 41,            //手机号未注册
    ERROR_PASSWD = 42,                       //密码错误
    ERROR_USER_ISNOT_EXIST = 43,             //用户不存在
    ERROR_PHONE_MAP = 44,                    //手机号关联关系未找到
    ERROR_ACOUNT_LOGOUT = 50,                //登出账号失败
    ERROR_CANT_LOGOUT_OTHER = 51,            //不能登出其他人
    ERROR_GROUP_ADD = 60,                    //新增群组失败
    ERROR_GROUP_EXIST = 61,                  //群组已存在
    ERROR_CREATE_GROUP_MAX = 62,             //群组创建达到上限
    ERROR_GROUP_MODIFY = 70,                 //修改群组失败
    ERROR_GROUP_MODIFY_TYPE = 71,            //修改群组参数类型错误
    ERROR_GROUP_NOT_EXIST = 72,              //群组不存在
    ERROR_GROUP_NOT_MANAGER = 73,            //不是群组管理员
    ERROR_GROUP_OPERATOR_NOT_EXIST = 74,     //操作者不存在
    ERROR_GROUP_TARGET_NOT_EXIST = 75,       //目标用户不存在
    ERROR_GROUP_DELETE = 80,                 //删除群组失败
    ERROR_GROUP_NOT_LEADER = 81,             //不是群组拥有者
    ERROR_USER_INFO_REQ = 90,                //请求联系人信息失败
    ERROR_GROUP_INFO_REQ = 100,              //请求群组信息失败
    ERROR_MSG_SEND = 110,                    //发送消息失败
    ERROR_USER_NOT_EXIST_OR_SEND_GUID = 111, //发送人不存在或者参数错误
    ERROR_GROUP_MSG_LIMIT = 112,             //发送群组消息时受限
    ERROR_MSG_LIST_REQ = 120,                //请求聊天记录失败
    ERROR_SERVER_MSG_TYPE_MAX = 150,         //不是服务端消息类型
    ERROR_MAX                                //错误上限
};