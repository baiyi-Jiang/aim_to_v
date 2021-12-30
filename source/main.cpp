#include "common.h"
#include "struct_def.h"

common_info comm;

static void sig_pro(int32_t signum)
{
    char log_buf[256] = {0};
    sprintf(log_buf, "sig_pro, recv signal:%d", signum);
    write_log(LOG_INFO, (uint8_t *)log_buf);
    if (signum == SIGQUIT)
    {
        comm.running = false;
    }
}

int32_t main(int32_t argc, char *argv[])
{
    int32_t ret;
    int32_t fd[2];             //读写管道
    pthread_t iAcceptThreadId; //接收连接线程ID
    pthread_t iReadThreadId;   //读数据线程ID

    //为让应用程序不必对慢速系统调用的errno做EINTR检查,可以采取两种方式:1.屏蔽中断信号,2.处理中断信号
    //1.由signal()函数安装的信号处理程序，系统默认会自动重启动被中断的系统调用，而不是让它出错返回，
    //  所以应用程序不必对慢速系统调用的errno做EINTR检查，这就是自动重启动机制.
    //2.对sigaction()的默认动作是不自动重启动被中断的系统调用，
    //  因此如果我们在使用sigaction()时需要自动重启动被中断的系统调用，就需要使用sigaction的SA_RESTART选项

    //忽略信号
    //sigset_t newmask;
    //sigemptyset(&newmask);
    //sigaddset(&newmask, SIGINT);
    //sigaddset(&newmask, SIGUSR1);
    //sigaddset(&newmask, SIGUSR2);
    //sigaddset(&newmask, SIGQUIT);
    //pthread_sigmask(SIG_BLOCK, &newmask, NULL);

    //处理信号
    //默认自动重启动被中断的系统调用,而不是让它出错返回,应用程序不必对慢速系统调用的errno做EINTR检查
    //signal(SIGINT, sig_pro);
    //signal(SIGUSR1, sig_pro);
    //signal(SIGUSR2, sig_pro);
    //signal(SIGQUIT, sig_pro);

    struct sigaction sa;
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_pro;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    //设置为运行状态
    comm.running = true;
    check_big_data();

    //创建管道
    ret = pipe(fd);
    if (ret < 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "main, pipe fail:%d,errno:%d", ret, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        comm.running = false;
        return 0;
    }
    comm.conn_info.rfd = fd[0];
    comm.conn_info.wfd = fd[1];

    //读端设置为非阻塞方式
    setnonblocking(comm.conn_info.rfd);

    //创建线程时采用的参数
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); //设置绑定的线程,以获取较高的响应速度
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);        //设置分离的线程

    //创建接收连接线程
    ret = pthread_create(&iAcceptThreadId, &attr, accept_thread, &comm);
    if (ret != 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "main, pthread_create AcceptThread fail:%d,errno:%d", ret, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        comm.running = false;
        close(comm.conn_info.rfd);
        close(comm.conn_info.wfd);
        return 0;
    }

    //创建接收连接线程
    ret = pthread_create(&iReadThreadId, &attr, read_thread, &comm);
    if (ret != 0)
    {
        char log_buf[256] = {0};
        sprintf(log_buf, "main, pthread_create ReadThread fail:%d,errno:%d", ret, errno);
        write_log(LOG_ERROR, (uint8_t *)log_buf);
        comm.running = false;
        pthread_join(iAcceptThreadId, NULL);
        close(comm.conn_info.rfd);
        close(comm.conn_info.wfd);
        return 0;
    }

    //主循环什么事情也不做
    while (comm.running)
    {
        sleep(1);
    }

    //等待子线程终止
    pthread_join(iAcceptThreadId, NULL);
    pthread_join(iReadThreadId, NULL);
    close(comm.conn_info.rfd);
    close(comm.conn_info.wfd);
    return 0;
}