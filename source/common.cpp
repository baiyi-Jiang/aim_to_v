#include <string>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif
#include "common.h"
#include "log.h"

void LogSystem::log_print_s(log_level level, const char *file_name, int line_num, const char *func, const char *format, ...)
{
    if (level > print_level_)
        return;
    va_list ap;
    va_start(ap, format);
    log_print_v(level, file_name, line_num, func, format, ap);
    va_end(ap);
}

void LogSystem::log_print_v(log_level level, const char *file_name, int line_num, const char *func, const char *format, va_list ap)
{
#ifdef _WIN32
    int size = ::_vscprintf(format, ap);
#else
    va_list arg_copy;
    va_copy(arg_copy, ap);
    int size = vsnprintf(NULL, 0, format, arg_copy);
    va_end(arg_copy);
#endif
    char log_buf[2048] = {0};
    char *print_buf = nullptr;
    if (size > 1024)
    {
        print_buf = new char(size + 1024);
        if (!print_buf)
            return;
    }
    std::vector<char> body(size + 1);
#ifdef _WIN32
    ::vsprintf_s(body.data(), body.size(), format, ap);
#else
    vsnprintf(body.data(), body.size(), format, ap);
#endif
    std::string buffer = std::string(body.data(), size);
    std::string time_str = common::time_t2string(time(0));
    int32_t write_count = 0;
    switch (level)
    {
    case LOG_SYSTEM:
    {
        write_count = sprintf(print_buf ? print_buf : log_buf, "%s LOG_SYSTEM:%s,%d,%s,%s\n", time_str.c_str(), file_name, line_num, func, buffer.c_str());
        break;
    }
    case LOG_ERROR:
    {
        write_count = sprintf(print_buf ? print_buf : log_buf, "%s LOG_ERROR:%s,%d,%s,%s\n", time_str.c_str(), file_name, line_num, func, buffer.c_str());
        break;
    }
    case LOG_WARING:
    {
        write_count = sprintf(print_buf ? print_buf : log_buf, "%s LOG_WARING:%s,%d,%s,%s\n", time_str.c_str(), file_name, line_num, func, buffer.c_str());
        break;
    }
    case LOG_INFO:
    {
        write_count = sprintf(print_buf ? print_buf : log_buf, "%s LOG_INFO:%s,%d,%s,%s\n", time_str.c_str(), file_name, line_num, func, buffer.c_str());
        break;
    }
    case LOG_DEBUG:
    {
        write_count = sprintf(print_buf ? print_buf : log_buf, "%s LOG_DEBUG:%s,%d,%s,%s\n", time_str.c_str(), file_name, line_num, func, buffer.c_str());
        break;
    }
    default:
        break;
    }
    if (strncmp(log_time_str_.c_str(), time_str.c_str(), 10) != 0)
    {
        log_time_str_ = time_str;
        if (fp_)
            fclose(fp_);
        fp_ = nullptr;
    }
    if (!fp_)
    {
        fp_ = fopen((log_time_str_ + u8".log").c_str(), "wb+");
        if (!fp_)
        {
            printf(u8"ERROR!!!,create log file failed!\n");
            if (print_buf)
            {
                delete print_buf;
                print_buf = nullptr;
            }
            return;
        }
    }
    fwrite(print_buf ? print_buf : log_buf, write_count, 1, fp_);
    fflush(fp_);
    if (print_buf)
    {
        delete print_buf;
        print_buf = nullptr;
    }
}

namespace common
{
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
        return (uint32_t)time(0);
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

    void split_str(std::string src, std::vector<std::string> &vec)
    {
        char *token = strtok((char *)src.c_str(), ",");
        while (token != nullptr)
        {
            vec.push_back(token);
            token = strtok(nullptr, ",");
        }
    }

    std::string custom_get_str(const std::string &src, const std::string &split_str)
    {
        std::string str;
        if (src.size() > split_str.size() && strncmp(src.c_str(), split_str.c_str(), split_str.size()) == 0)
        {
            str = src.substr(2, src.size() - split_str.size());
        }
        return str;
    }

    uint64_t custom_get_num(const std::string &src, const std::string &split_str)
    {
        uint64_t num = 0;
        std::string str = custom_get_str(src, split_str);
        if (!str.empty())
            memcpy_u(num, (const uint8_t *)str.c_str());
        return num;
    }

    // string 转换为time_t  时间格式为2014_03_28 18_25_26
    time_t string2time_t(const std::string string_time)
    {
        tm tm1;
        memset(&tm1, 0, sizeof(tm1));
        time_t time1;
        sscanf(string_time.c_str(), "%04d_%02d_%02d %02d_%02d_%02d",
               &(tm1.tm_year),
               &(tm1.tm_mon),
               &(tm1.tm_mday),
               &(tm1.tm_hour),
               &(tm1.tm_min),
               &(tm1.tm_sec));
        tm1.tm_year -= 1900;
        tm1.tm_mon -= 1;
        time1 = mktime(&tm1);
        return time1;
    }

    // time_t转换为string  时间格式为2014_03_28 18_25_26
    std::string time_t2string(const time_t time_t_time)
    {
        char szTime[100] = {'\0'};
        struct tm *ptm = localtime(&time_t_time);
        ptm->tm_year += 1900;
        ptm->tm_mon += 1;
        sprintf(szTime, "%04d_%02d_%02d %02d_%02d_%02d",
                ptm->tm_year,
                ptm->tm_mon,
                ptm->tm_mday,
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);

        std::string strTime = szTime;
        return strTime;
    }

    int const MAX_STR_LEN = 200;

    //在指定路径读取文件名 file_suffix文件后缀名
    bool getFilename(const std::string &file_path, const std::string &file_suffix, std::vector<std::string> &tempvector, std::string &error_msg)
    {
        if (file_path.empty())
            return false;
#ifdef WIN32
        char path[1024] = {0};
        sprintf_s(path, "%s*.%s", file_path.c_str(), file_suffix.c_str());
        _finddatai64_t file;
        intptr_t longf;
        if ((longf = _findfirsti64(path, &file)) == -1l)
        {
            error_msg = u8"在指定路径下没有找到文件";
            return false;
        }
        else
        {
            do
            {
                tempvector.push_back(file.name);
            } while (_findnexti64(longf, &file) == 0);
        }
        _findclose(longf);
#else
        struct stat s;
        lstat(file_path.c_str(), &s);
        if (!S_ISDIR(s.st_mode))
        {
            error_msg = u8"路径不能为空";
            return false;
        }
        struct dirent *filename;
        DIR *dir;
        dir = opendir(file_path.c_str());
        if (nullptr == dir)
        {
            error_msg = u8"路径不存在";
            return false;
        }
        /* read all the files in the dir ~ */
        while ((filename = readdir(dir)) != NULL)
        {
            if (strcmp(filename->d_name, ".") == 0 ||
                strcmp(filename->d_name, "..") == 0)
                continue;
            tempvector.push_back(filename->d_name);
        }
#endif
        return true;
    }
}