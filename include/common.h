#pragma once
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cctype>
#include <sstream>
#include <utility>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>

enum log_level : uint8_t
{
    LOG_SYSTEM,
    LOG_ERROR,
    LOG_WARING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_MAX
};

uint32_t get_time_sec();

void check_big_data();

bool get_is_big_data();

void write_log(log_level level, uint8_t *msg);

void setnonblocking(int32_t sock);

void setreuseaddr(int32_t sock);

template <class U>
uint32_t memcpy_u(U &src_num, const uint8_t *data)
{
    if (!get_is_big_data())
    {
        memcpy(&src_num, data, sizeof(U));
    }
    else
    {
        for (uint32_t i = 0; i < sizeof(U); ++i)
        {
            memcpy(&src_num + (sizeof(U) - i - 1), data + i, 1);
        }
    }
    return sizeof(U);
}

template <class U>
uint32_t memcpy_u(uint8_t *data, U &src_num)
{
    if (!get_is_big_data())
    {
        memcpy(data, &src_num, sizeof(U));
    }
    else
    {
        for (uint32_t i = 0; i < sizeof(U); ++i)
        {
            memcpy(data + i, &src_num + (sizeof(U) - i - 1), 1);
        }
    }
    return sizeof(U);
}

void *read_thread(void *arg);

void *accept_thread(void *arg);