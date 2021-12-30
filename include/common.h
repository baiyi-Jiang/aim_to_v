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

void write_log(log_level level, uint8_t *msg);

void setnonblocking(int32_t sock);

void setreuseaddr(int32_t sock);

void memcpy_uint16(uint16_t& src_num, const uint8_t* data);

void memcpy_uint32(uint32_t& src_num, const uint8_t* data);

void *read_thread(void *arg);

void *accept_thread(void *arg);