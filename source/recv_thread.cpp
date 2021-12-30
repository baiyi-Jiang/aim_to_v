#include "common.h"
#include "struct_def.h"

//接收连接线程
void *accept_thread(void *arg)
{
    common_info *comm = static_cast<common_info *>(arg);
    if (!comm)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "Error, accept_thread, invalid args!");
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        return nullptr;
    }
    char log_buf[256] = {0};
    sprintf(log_buf, "AcceptThread, enter");
    write_log(LOG_INFO, (uint8_t *)log_buf);

    int32_t ret;      //临时变量,存放返回值
    int32_t epfd;     //监听用的epoll
    int32_t listenfd; //监听socket
    int32_t connfd;   //接收到的连接socket临时变量
    int32_t i;        //临时变量,轮询数组用
    int32_t nfds;     //临时变量,有多少个socket有事件

    struct epoll_event ev;                //事件临时变量
    const int32_t MAXEVENTS = 1024;       //最大事件数
    struct epoll_event events[MAXEVENTS]; //监听事件数组
    socklen_t clilen;                     //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
    struct sockaddr_in cliaddr;
    struct sockaddr_in svraddr;

    uint16_t uListenPort = 5000;
    int32_t iBacklogSize = 5;
    int32_t iBackStoreSize = 1024;

    struct pipemsg msg; //消息队列数据

    //创建epoll,对2.6.8以后的版本,其参数无效,只要大于0的数值就行,内核自己动态分配
    epfd = epoll_create(iBackStoreSize);
    if (epfd < 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "AcceptThread, epoll_create fail:%d,errno:%d.", epfd, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        return nullptr;
    }

    //创建监听socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "AcceptThread, socket fail:%d,errno:%d.", epfd, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        close(epfd);
        return nullptr;
    }

    //把监听socket设置为非阻塞方式
    setnonblocking(listenfd);
    //设置监听socket为端口重用
    setreuseaddr(listenfd);

    //设置与要处理的事件相关的文件描述符
    ev.data.fd = listenfd;
    //设置要处理的事件类型
    ev.events = EPOLLIN | EPOLLET;
    //注册epoll事件
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    if (ret != 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "AcceptThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        close(listenfd);
        close(epfd);

        return nullptr;
    }

    memset(&svraddr, '\0', sizeof(svraddr));
    svraddr.sin_family = AF_INET;
    svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svraddr.sin_port = htons(uListenPort);
    bind(listenfd, (sockaddr *)&svraddr, sizeof(svraddr));
    //监听,准备接收连接
    ret = listen(listenfd, iBacklogSize);
    if (ret != 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "AcceptThread, listen fail:%d,errno:%d.", ret, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        close(listenfd);
        close(epfd);

        return nullptr;
    }

    while (comm->running)
    {
        //等待epoll事件的发生,如果当前有信号的句柄数大于输出事件数组的最大大小,超过部分会在下次epoll_wait时输出,事件不会丢
        nfds = epoll_wait(epfd, events, MAXEVENTS, 500);

        //处理所发生的所有事件
        for (i = 0; i < nfds && comm->running; ++i)
        {
            if (events[i].data.fd == listenfd) //是本监听socket上的事件
            {
                char log_buf[256] = {0};
                sprintf(log_buf, "AcceptThread, events:%d,errno:%d.", events[i].events, errno);
                write_log(LOG_INFO, (uint8_t *)log_buf);
                if (events[i].events & EPOLLIN) //有连接到来
                {
                    do
                    {
                        clilen = sizeof(struct sockaddr);
                        connfd = accept(listenfd, (sockaddr *)&cliaddr, &clilen);
                        if (connfd > 0)
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "AAcceptThread, accept:%d,errno:%d,connect:%s:%d.", connfd, errno, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                            write_log(LOG_INFO, (uint8_t *)log_buf);
                            //往管道写数据
                            msg.op = 1;
                            msg.fd = connfd;
                            msg.ip = cliaddr.sin_addr.s_addr;
                            msg.port = cliaddr.sin_port;
                            ret = write(comm->conn_info.wfd, &msg, 14);
                            if (ret != 14)
                            {
                                char log_buf[256] = {0};
                                sprintf(log_buf, "AcceptThread, write fail:%d,errno:%d.", connfd, errno);
                                write_log(LOG_ERROR, (uint8_t *)log_buf);
                                close(connfd);
                            }
                        }
                        else
                        {
                            char log_buf[256] = {0};
                            sprintf(log_buf, "AcceptThread, accept:%d,errno:%d.", connfd, errno);
                            write_log(LOG_INFO, (uint8_t *)log_buf);
                            if (errno == EAGAIN) //没有连接需要接收了
                            {
                                break;
                            }
                            else if (errno == EINTR) //可能被中断信号打断,,经过验证对非阻塞socket并未收到此错误,应该可以省掉该步判断
                            {
                                ;
                            }
                            else //其它情况可以认为该描述字出现错误,应该关闭后重新监听
                            {

                                //此时说明该描述字已经出错了,需要重新创建和监听
                                close(listenfd);
                                epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, &ev);

                                //创建监听socket
                                listenfd = socket(AF_INET, SOCK_STREAM, 0);
                                if (listenfd < 0)
                                {
                                    char log_buf[256] = {0};
                                    sprintf(log_buf, "AcceptThread, socket fail:%d,errno:%d.", epfd, errno);
                                    write_log(LOG_ERROR, (uint8_t *)log_buf);
                                    close(epfd);
                                    return nullptr;
                                }

                                //把监听socket设置为非阻塞方式
                                setnonblocking(listenfd);
                                //设置监听socket为端口重用
                                setreuseaddr(listenfd);

                                //设置与要处理的事件相关的文件描述符
                                ev.data.fd = listenfd;
                                //设置要处理的事件类型
                                ev.events = EPOLLIN | EPOLLET;
                                //注册epoll事件
                                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
                                if (ret != 0)
                                {
                                    char log_buf[256] = {0};
                                    sprintf(log_buf, "AcceptThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
                                    write_log(LOG_ERROR, (uint8_t *)log_buf);
                                    close(listenfd);
                                    close(epfd);
                                    return nullptr;
                                }

                                memset(&svraddr, '\0', sizeof(svraddr));
                                svraddr.sin_family = AF_INET;
                                svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
                                svraddr.sin_port = htons(uListenPort);
                                bind(listenfd, (sockaddr *)&svraddr, sizeof(svraddr));
                                //监听,准备接收连接
                                ret = listen(listenfd, iBacklogSize);
                                if (ret != 0)
                                {
                                    char log_buf[256] = {0};
                                    sprintf(log_buf, "AcceptThread, listen fail:%d,errno:%d.", ret, errno);
                                    write_log(LOG_ERROR, (uint8_t *)log_buf);
                                    close(listenfd);
                                    close(epfd);
                                    return nullptr;
                                }
                            }
                        }
                    } while (comm->running);
                }
                else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) //有异常发生
                {
                    //此时说明该描述字已经出错了,需要重新创建和监听
                    close(listenfd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, &ev);

                    //创建监听socket
                    listenfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (listenfd < 0)
                    {
                        char log_buf[256] = {0};
                        sprintf(log_buf, "AcceptThread, socket fail:%d,errno:%d.", epfd, errno);
                        write_log(LOG_ERROR, (uint8_t *)log_buf);
                        close(epfd);

                        return nullptr;
                    }

                    //把监听socket设置为非阻塞方式
                    setnonblocking(listenfd);
                    //设置监听socket为端口重用
                    setreuseaddr(listenfd);

                    //设置与要处理的事件相关的文件描述符
                    ev.data.fd = listenfd;
                    //设置要处理的事件类型
                    ev.events = EPOLLIN | EPOLLET;
                    //注册epoll事件
                    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
                    if (ret != 0)
                    {
                        char log_buf[256] = {0};
                        sprintf(log_buf, "AcceptThread, epoll_ctl fail:%d,errno:%d.", ret, errno);
                        write_log(LOG_ERROR, (uint8_t *)log_buf);
                        close(listenfd);
                        close(epfd);
                        return nullptr;
                    }

                    memset(&svraddr, '\0', sizeof(svraddr));
                    svraddr.sin_family = AF_INET;
                    svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
                    svraddr.sin_port = htons(uListenPort);
                    bind(listenfd, (sockaddr *)&svraddr, sizeof(svraddr));
                    //监听,准备接收连接
                    ret = listen(listenfd, iBacklogSize);
                    if (ret != 0)
                    {
                        char log_buf[256] = {0};
                        sprintf(log_buf, "AcceptThread, listen fail:%d,errno:%d.", ret, errno);
                        write_log(LOG_ERROR, (uint8_t *)log_buf);
                        close(listenfd);
                        close(epfd);
                        return nullptr;
                    }
                }
            }
        }
    }

    //关闭监听描述字
    if (listenfd > 0)
    {
        close(listenfd);
    }
    //关闭创建的epoll
    if (epfd > 0)
    {
        close(epfd);
    }
    //char log_buf[256] = {0};
    sprintf(log_buf, "AcceptThread, exit.");
    write_log(LOG_INFO, (uint8_t *)log_buf);
    return nullptr;
}