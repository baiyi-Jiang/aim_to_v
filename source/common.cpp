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

uint32_t get_time_sec()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (uint32_t)now.tv_sec;
}

void write_log(log_level level, uint8_t *msg)
{
    if (!msg || level > global_log_level)
        return;
    std::cout << msg << std::endl;
}

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

void memcpy_uint16(uint16_t& src_num, const uint8_t* data)
{
    if(is_big_data)
    {
        memcpy(&src_num + 1, data + 0, 1);
        memcpy(&src_num + 0, data + 1, 1);
    }
    else
    {
        memcpy(&src_num, data, 2);
    }
}

void memcpy_uint32(uint32_t& src_num, const uint8_t* data)
{
    if(is_big_data)
    {
        memcpy(&src_num + 3, data + 0, 1);
        memcpy(&src_num + 2, data + 1, 1);
        memcpy(&src_num + 1, data + 2, 1);
        memcpy(&src_num + 0, data + 3, 1);
    }
    else
    {
        memcpy(&src_num, data, 4);
    }
}