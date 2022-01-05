#include "read_thread.h"
#include "user_info.h"
#include "group_info.h"
#include <iterator>
#include <memory>

uint32_t global_user_guid = 0;
uint32_t global_group_guid = 0;
uint32_t global_msg_num = 0;

bool NetInfo::parse_msg(const uint8_t *data, uint32_t len, int32_t fd, struct recv_msg &msg)
{
    uint32_t msg_head_length = msg.length();
    if (msg.type > recv_msg_type::SERVER_MSG_TYPE_MAX)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "Error, msg type error!");
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        return false;
    }
    switch (static_cast<recv_msg_type::msg_type>(msg.type))
    {
    case recv_msg_type::ACOUNT_ADD:
    {
        on_acount_add(data + msg_head_length, msg.data_length, fd);
        break;
    }
    case recv_msg_type::ACOUNT_MODIFY:
    {
        on_acount_modify(data + msg_head_length, msg.data_length, fd);
        break;
    }
    case recv_msg_type::ACOUNT_DELETE:
    {
        on_acount_delete(data + msg_head_length, msg.data_length, fd);
        break;
    }
    case recv_msg_type::GROUP_ADD:
    {
        break;
    }
    case recv_msg_type::GROUP_MODIFY:
    {
        break;
    }
    case recv_msg_type::GROUP_DELETE:
    {
        break;
    }
    case recv_msg_type::USER_INFO_REQ:
    {
        on_user_info_req(msg.guid, data + msg_head_length, len, fd);
        break;
    }
    case recv_msg_type::GROUP_INFO_REQ:
    {
        on_group_info_req(msg.guid, data + msg_head_length, len, fd);
        break;
    }
    case recv_msg_type::MSG_SEND:
    {
        on_msg_send(msg.guid, data + msg_head_length, len, fd);
        break;
    }
    case recv_msg_type::MSG_LIST_REQ:
    {
        break;
    }
    default:
    {
        break;
    }
    }
    return true;
}

void NetInfo::on_acount_add(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfo user;
    if (!user.from_data(global_user_guid, data, len))
        return;
    auto user_itor = users_map.find(user.get_user_guid());
    if (user_itor != users_map.end())
    {
        users.erase(user_itor->second);
        users_map.erase(user_itor);
    }
    user.on_set_online_status(user_status::STATUS_ON_LINE);
    users.push_back(user);
    users_map[user.get_user_guid()] = std::prev(users.end());
    user_conn_map[user.get_user_guid()] = fd;
    conn_user_map[fd] = user.get_user_guid();
    //TODO:ACK暂未添加
}

void NetInfo::on_acount_modify(const uint8_t *data, uint32_t len, int32_t fd)
{
    UserInfo user;
    if (!user.from_data(global_user_guid, data, len))
        return;
    auto user_itor = users_map.find(user.get_user_guid());
    if (user_itor != users_map.end())
    {
        users.erase(user_itor->second);
        users_map.erase(user_itor);
        users.push_back(user);
        users_map[user.get_user_guid()] = std::prev(users.end());
    }
    //TODO:ACK暂未添加
}

void NetInfo::on_acount_delete(const uint8_t *data, uint32_t len, int32_t fd)
{
    struct acount_delete ad;
    if (!ad.from_data(data, len))
        return;
    auto user_itor = users_map.find(ad.user_guid);
    if (user_itor != users_map.end())
    {
        users.erase(user_itor->second);
        users_map.erase(user_itor);
    }
    conn_user_map.erase(user_conn_map[ad.user_guid]);
    user_conn_map.erase(ad.user_guid);
    //TODO:ACK暂未添加
}

void NetInfo::on_group_add()
{
    //TODO:ACK暂未添加
}

void NetInfo::on_group_modify()
{
    //TODO:ACK暂未添加
}

void NetInfo::on_group_delete()
{
    //TODO:ACK暂未添加
}

void NetInfo::on_user_info_req(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd)
{
    struct user_info_req req;
    if (!req.from_data(data, len))
        return;
    auto itor = users_map.find(req.user_guid);
    if (itor != users_map.end())
    {
        struct recv_msg tmp_msg;
        tmp_msg.guid = 0;
        tmp_msg.type = recv_msg_type::CLIENT_USER_INFO_ACK;
        tmp_msg.data_length = itor->second->to_data((uint8_t *)send_buf + tmp_msg.length(), MAXBUFSIZE - tmp_msg.length());
        if (tmp_msg.data_length && tmp_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
        {
            send_msg(fd, (uint8_t *)send_buf, tmp_msg.data_length + tmp_msg.length());
        }
    }
}

void NetInfo::on_group_info_req(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd)
{
    struct group_info_req req;
    if (!req.from_data(data, len))
        return;
    auto itor = groups_map.find(req.group_guid);
    if (itor != groups_map.end())
    {
        struct recv_msg tmp_msg;
        tmp_msg.guid = 0;
        tmp_msg.type = recv_msg_type::CLIENT_GROUP_INFO_ACK;
        tmp_msg.data_length = itor->second->to_data((uint8_t *)send_buf + tmp_msg.length(), MAXBUFSIZE - tmp_msg.length());
        if (tmp_msg.data_length && tmp_msg.to_head((uint8_t *)send_buf, MAXBUFSIZE))
        {
            send_msg(fd, (uint8_t *)send_buf, tmp_msg.data_length + tmp_msg.length());
        }
    }
}

void NetInfo::on_msg_send(uint32_t guid, const uint8_t *data, uint32_t len, int32_t fd)
{
    std::shared_ptr<msg_info> msg = std::make_shared<msg_info>();
    if (!msg->from_data(data, len))
        return;
    msg_list.push_back(msg);
    auto conn_itor = conn_user_map.find(fd);
    if (conn_itor == conn_user_map.end() || conn_itor->second != guid)
    {
        user_conn_map[guid] = fd;
        conn_user_map[fd] = guid;
    }
}

void NetInfo::on_msg_list_req()
{
    //
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
            char log_buf[256] = {0};
            sprintf(log_buf, "ReadThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
            write_log(LOG_ERROR, (uint8_t *)log_buf);
            close(fd);
        }
        {
            char log_buf[256] = {0};
            sprintf(log_buf, "send msg failed.");
            write_log(LOG_ERROR, (uint8_t *)log_buf);
        }
    }
    return nsend;
}

void buf_to_msg(uint8_t *src, uint32_t src_len, recv_msg &msg)
{
    if (!src || src_len < 10)
        return;
    uint32_t index = 0;
    index += memcpy_u(msg.guid, src + index);
    index += memcpy_u(msg.type, src + index);
    index += memcpy_u(msg.data_length, src + index);
}

//读数据线程
void *read_thread(void *arg)
{
    common_info *comm = static_cast<common_info *>(arg);
    if (!comm)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "Error, read_thread, invalid args!");
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        return nullptr;
    }
    char log_buf[256] = {0};
    sprintf(log_buf, "ReadThread, enter");
    write_log(LOG_INFO, (uint8_t *)log_buf);

    NetInfo net_info;
    int32_t &epfd = net_info.epfd;        //连接用的epoll
    struct epoll_event &ev = net_info.ev; //事件临时变量
    char *send_buf = net_info.send_buf;   //发送缓冲区
    int32_t ret = 0;                      //临时变量,存放返回值
    int32_t i = 0;                        //临时变量,轮询数组用
    int32_t nfds = 0;                     //临时变量,有多少个socket有事件
    const int32_t MAXEVENTS = 1024;       //最大事件数
    struct epoll_event events[MAXEVENTS]; //监听事件数组
    int32_t iBackStoreSize = 1024;
    char buf[MAXBUFSIZE] = {0};
    int32_t buf_index = 0;
    struct recv_msg temp_recv_msg;
    const uint32_t recv_msg_head_length = temp_recv_msg.length();
    int32_t nread = 0;                                             //读到的字节数
    struct ipport tIpPort;                                         //地址端口信息
    struct peerinfo tPeerInfo;                                     //对方连接信息
    std::map<int32_t, struct ipport> mIpPort;                      //socket对应的对方地址端口信息
    std::map<int32_t, struct ipport>::iterator itIpPort;           //临时迭代子
    std::map<struct ipport, struct peerinfo>::iterator itPeerInfo; //临时迭代子
    struct pipemsg msg;                                            //消息队列数据

    //创建epoll,对2.6.8以后的版本,其参数无效,只要大于0的数值就行,内核自己动态分配
    epfd = epoll_create(iBackStoreSize);
    if (epfd < 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "ReadThread, epoll_create fail:%d,errno:%d", epfd, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        return nullptr;
    }

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
                        char log_buf[256] = {0};
                        sprintf(log_buf, "ReadThread, recv connect:%d,errno:%d", msg.fd, errno);
                        write_log(LOG_INFO, (uint8_t *)log_buf);
                        //把socket设置为非阻塞方式
                        setnonblocking(msg.fd);
                        //设置描述符信息和数组下标信息
                        ev.data.fd = msg.fd;
                        //设置用于注测的读操作事件
                        ev.events = EPOLLIN | EPOLLET;
                        //注册ev
                        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, msg.fd, &ev);
                        if (ret != 0)
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "ReadThread, epoll_ctl fail:%d,errno:%d", ret, errno);
                            write_log(LOG_ERROR, (uint8_t *)log_buf);
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
                        char log_buf[256] = {0};
                        sprintf(log_buf, "ReadThread, recv close:%d,errno:%d", msg.fd, errno);
                        write_log(LOG_INFO, (uint8_t *)log_buf);
                        close(msg.fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, msg.fd, &ev);
                        auto user_itor = net_info.users_map.find(net_info.conn_user_map[msg.fd]);
                        if (user_itor != net_info.users_map.end())
                        {
                            user_itor->second->on_set_online_status(user_status::STATUS_OFF_LINE);
                        }
                        net_info.user_conn_map.erase(net_info.conn_user_map[msg.fd]);
                        net_info.conn_user_map.erase(msg.fd);
                        itIpPort = mIpPort.find(msg.fd);
                        if (itIpPort != mIpPort.end())
                        {
                            mIpPort.erase(itIpPort);
                            itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
                            if (itPeerInfo != comm->conn_info.peer.end())
                            {
                                comm->conn_info.peer.erase(itPeerInfo);
                            }
                        }
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
            char log_buf[256] = {0};
            sprintf(log_buf, "ReadThread, events:%d,errno:%d", events[i].events, errno);
            write_log(LOG_INFO, (uint8_t *)log_buf);
            if (events[i].events & EPOLLIN) //有数据可读
            {
                do
                {
                    nread = read(events[i].data.fd, buf + buf_index, MAXBUFSIZE - buf_index);
                    if (nread > 0) //读到数据
                    {
                        char log_buf[256] = {0};
                        sprintf(log_buf, "ReadThread, read:%d,errno:%d", nread, errno);
                        write_log(LOG_INFO, (uint8_t *)log_buf);
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
                        if (nread > 14)
                        {
                            buf_to_msg((uint8_t *)buf, nread, temp_recv_msg);
                            if (temp_recv_msg.data_length + recv_msg_head_length <= (uint32_t)nread)
                            {
                                net_info.parse_msg((uint8_t *)buf, nread, events[i].data.fd, temp_recv_msg);
                            }
                            else
                            {
                                buf_index += nread;
                            }
                        }
                        else
                        {
                            buf_index += nread;
                        }
                        if (buf_index == MAXBUFSIZE)
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "Error, read data failed!");
                            write_log(LOG_ERROR, (uint8_t *)log_buf);
                            buf_index = 0;
                        }
                    }
                    else if (nread < 0) //读取失败
                    {
                        if (errno == EAGAIN) //没有数据了
                        {
                            // char log_buf[256] = {0};
                            // sprintf(log_buf, "ReadThread, read:%d,errno:%d,no data", nread, errno);
                            // write_log(LOG_INFO, (uint8_t *)log_buf);
                            break;
                        }
                        else if (errno == EINTR) //可能被内部中断信号打断,经过验证对非阻塞socket并未收到此错误,应该可以省掉该步判断
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "ReadThread, read:%d,errno:%d,interrupt", nread, errno);
                            write_log(LOG_INFO, (uint8_t *)log_buf);
                        }
                        else //客户端主动关闭
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "ReadThread, read:%d,errno:%d,peer error", nread, errno);
                            write_log(LOG_INFO, (uint8_t *)log_buf);
                            close(events[i].data.fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                            itIpPort = mIpPort.find(events[i].data.fd);
                            if (itIpPort != mIpPort.end())
                            {
                                mIpPort.erase(itIpPort);
                                itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
                                if (itPeerInfo != comm->conn_info.peer.end())
                                {
                                    comm->conn_info.peer.erase(itPeerInfo);
                                }
                            }

                            break;
                        }
                    }
                    else if (nread == 0) //客户端主动关闭
                    {
                        char log_buf[256] = {0};
                        sprintf(log_buf, "ReadThread, read:%d,errno:%d,peer close", nread, errno);
                        write_log(LOG_INFO, (uint8_t *)log_buf);
                        close(events[i].data.fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                        itIpPort = mIpPort.find(events[i].data.fd);
                        if (itIpPort != mIpPort.end())
                        {
                            mIpPort.erase(itIpPort);
                            itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
                            if (itPeerInfo != comm->conn_info.peer.end())
                            {
                                comm->conn_info.peer.erase(itPeerInfo);
                            }
                        }

                        break;
                    }
                } while (comm->running);
            }
            else if (events[i].events & EPOLLOUT) //有数据可写
            {
                ev.data.fd = events[i].data.fd;
                //设置用于注测的读操作事件
                //
                //
                ev.events = EPOLLIN | EPOLLET;
                //注册ev
                ret = epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
                if (ret != 0)
                {
                    char log_buf[256] = {0};
                    sprintf(log_buf, "ReadThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
                    write_log(LOG_ERROR, (uint8_t *)log_buf);
                    close(msg.fd);
                }
            }
            else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) //有异常发生
            {
                char log_buf[256] = {0};
                sprintf(log_buf, "ReadThread, read:%d,errno:%d,err or hup", nread, errno);
                write_log(LOG_ERROR, (uint8_t *)log_buf);
                close(events[i].data.fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                itIpPort = mIpPort.find(events[i].data.fd);
                if (itIpPort != mIpPort.end())
                {
                    mIpPort.erase(itIpPort);
                    itPeerInfo = comm->conn_info.peer.find(itIpPort->second);
                    if (itPeerInfo != comm->conn_info.peer.end())
                    {
                        comm->conn_info.peer.erase(itPeerInfo);
                    }
                }
            }
        }
        while (!net_info.msg_list.empty())
        {
            std::shared_ptr<msg_info> &msg_t = net_info.msg_list.front();
            temp_recv_msg.guid = msg_t->send_guid;
            temp_recv_msg.type = recv_msg_type::CLIENT_MSG_SEND;
            temp_recv_msg.data_length = msg_t->to_data((uint8_t *)(send_buf + recv_msg_head_length), sizeof(send_buf) - recv_msg_head_length);
            if (!temp_recv_msg.data_length || !temp_recv_msg.to_head((uint8_t *)send_buf, sizeof(send_buf)))
            {
                //TODO:error_log
                net_info.msg_list.pop_front();
                continue;
            }
            if (msg_t->msg_type == msg_info::MSG_TYPE_GROUP)
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
                            //TODO:发送提示
                            net_info.msg_list.pop_front();
                            continue;
                        }
                    }
                    itor->second->traverse_all_members([&](std::shared_ptr<group_member_info> &info) -> bool
                                                       {
                                                           auto user_itor = net_info.users_map.find(info->user_guid);
                                                           if (user_itor == net_info.users_map.end())
                                                               return false;
                                                           user_itor->second->on_update_group_active(msg_t->recv_guid);
                                                           auto conn_itor = net_info.user_conn_map.find(info->user_guid);
                                                           if (conn_itor != net_info.user_conn_map.end())
                                                           {
                                                               if (conn_itor->first != info->user_guid)
                                                               {
                                                                   net_info.send_msg(conn_itor->second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.data_length);
                                                               }
                                                               else
                                                               {
                                                                   ++info->msg_count;
                                                               }
                                                           }
                                                           return true;
                                                       });
                    itor->second->on_add_msg(msg_t);
                }
            }
            else if (msg_t->msg_type == msg_info::MSG_TYPE_USER)
            {
                auto user_itor = net_info.users_map.find(msg_t->recv_guid);
                if (user_itor == net_info.users_map.end())
                {
                    net_info.msg_list.pop_front();
                    continue;
                }
                user_itor->second->on_update_friend_active(msg_t->send_guid);
                user_itor->second->on_add_msg(msg_t);
                auto send_user_itor = net_info.users_map.find(msg_t->send_guid);
                if (send_user_itor == net_info.users_map.end())
                {
                    net_info.msg_list.pop_front();
                    continue;
                }
                send_user_itor->second->on_update_friend_active(msg_t->recv_guid);
                send_user_itor->second->on_add_msg(msg_t); 
                auto conn_itor = net_info.user_conn_map.find(msg_t->recv_guid);
                if (conn_itor != net_info.user_conn_map.end())
                {
                    if (conn_itor->first != msg_t->send_guid)
                    {
                        net_info.send_msg(conn_itor->second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.data_length);
                    }
                }
            }
            else if (msg_t->msg_type == msg_info::MSG_TYPE_ALL)
            {
                for (const auto &it : net_info.user_conn_map)
                {
                    if (it.first == msg_t->send_guid)
                    {
                        net_info.msg_list.pop_front();
                        continue;
                    }
                    net_info.send_msg(it.second, (uint8_t *)send_buf, recv_msg_head_length + temp_recv_msg.data_length);
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
    //char log_buf[256] = {0};
    sprintf(log_buf, "ReadThread, exit.");
    write_log(LOG_INFO, (uint8_t *)log_buf);
    return nullptr;
}