#ifndef LOGGER_H
#define LOGGER_H

#include "zlog.h"


#ifdef __cplusplus
extern "C" {
#endif

enum LogLevel
{
    LL_Trace = 0,
    LL_Debug,
    LL_Info,
    LL_Warn,
    LL_Error,
    LL_FATAL
};


extern zlog_category_t* _dev_category;


int log_get_tid();

int log_init(const char* cat);
int log_uninit();

//LogLevel log_get_level();
//int log_set_level(LogLevel lv);


#define LOG_TRACE(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, 10, "%d " format, log_get_tid(), ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, ZLOG_LEVEL_DEBUG, "%d " format, log_get_tid(), ##__VA_ARGS__)
#define LOG_INFO(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, ZLOG_LEVEL_INFO, "%d " format, log_get_tid(), ##__VA_ARGS__)
#define LOG_WARN(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, ZLOG_LEVEL_WARN, "%d " format, log_get_tid(), ##__VA_ARGS__)
#define LOG_ERROR(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, ZLOG_LEVEL_ERROR, "%d " format, log_get_tid(), ##__VA_ARGS__)
#define LOG_FATAL(format, ...) \
    zlog(_dev_category, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, \
    __LINE__, ZLOG_LEVEL_FATAL, "%d " format, log_get_tid(), ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif // LOGGER_H