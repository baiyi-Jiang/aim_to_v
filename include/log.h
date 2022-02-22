#pragma once
#include <stdarg.h>
#include <stdio.h>
#include "instance.h"
#include "common.h"

enum log_level : uint8_t
{
    LOG_SYSTEM,
    LOG_ERROR,
    LOG_WARING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_MAX
};

class LogSystem
{
public:
    LogSystem()
    {
        log_time_str_ = common::time_t2string(time(0));
    }
    ~LogSystem() {}

    void log_print_s(log_level level, const char *file_name, int line_num, const char *func, const char *format, ...);
    void log_print_v(log_level level, const char *file_name, int line_num, const char *func, const char *format, va_list ap);
    void set_print_log_level(log_level level) { print_level_ = level; }

private:
    log_level print_level_ = LOG_MAX;
    FILE *fp_ = nullptr;
    std::string log_time_str_;
};

using TheLogSystem = Singleton<LogSystem>;

#define log_print(level, ...) \
    TheLogSystem::instance().log_print_s(level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
