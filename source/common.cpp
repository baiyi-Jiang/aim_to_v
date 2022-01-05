#include "common.h"
log_level global_log_level = LOG_INFO;

union big_data
{
    uint32_t data;
    uint8_t num;
};
bool is_big_data = false;

void check_big_data()
{
    big_data da;
    da.data = 1;
    if (da.num != 1)
    {
        is_big_data = true;
    }
}

bool get_is_big_data()
{
    return is_big_data;
}

uint32_t get_time_sec()
{
    // struct timeval now;
    // gettimeofday(&now, NULL);
    // return (uint32_t)now.tv_sec;
    return (uint32_t)time(0);
}

void write_log(log_level level, uint8_t *msg)
{
    if (!msg || level > global_log_level)
        return;
    std::cout << msg << std::endl;
}

#ifndef WIN32
void setnonblocking(int32_t sock)
{
    int32_t opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}

void setreuseaddr(int32_t sock)
{
    int32_t opt;
    opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(&opt)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }
}
#endif