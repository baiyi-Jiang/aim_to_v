#pragma once
#ifndef WIN32
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cctype>
#include <utility>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#else
//
#endif
#include <stdint.h>
#include <algorithm>
#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <functional>
#include "error_def.h"

#define Unused(parm) (void)parm

uint32_t get_time_sec();

void check_big_data();

bool get_is_big_data();

#ifndef WIN32
void setnonblocking(int32_t sock);

void setreuseaddr(int32_t sock);
#endif

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

#ifndef WIN32
void *read_thread(void *arg);

void *accept_thread(void *arg);
#endif

void split_str(std::string src, std::vector<std::string>& vec);

std::string custom_get_key(const std::string& src, const std::string& split_str);

std::string custom_get_str(const std::string& src, const std::string& split_str);

uint64_t custom_get_num(const std::string& src, const std::string& split_str);

time_t string2time_t(const std::string string_time);

std::string time_t2string(const time_t time_t_time);