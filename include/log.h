#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/* Log levels */
typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG
} log_level_t;

/* Logging system setup/cleanup */
void log_init(const char *filename);   /* NULL => stderr */
void log_close(void);

/* Runtime enable/disable */
void log_set_enabled(int enabled);
int  log_is_enabled(void);

/* Main logging function */
void log_message(log_level_t level, const char *category, const char *fmt, ...);

/* Convenience macros */
#ifdef ENABLE_LOG
    #define LOG_INFO(cat, fmt, ...)  log_message(LOG_LEVEL_INFO,  cat, fmt, ##__VA_ARGS__)
    #define LOG_WARN(cat, fmt, ...)  log_message(LOG_LEVEL_WARN,  cat, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(cat, fmt, ...) log_message(LOG_LEVEL_ERROR, cat, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(cat, fmt, ...) log_message(LOG_LEVEL_DEBUG, cat, fmt, ##__VA_ARGS__)
#else
    #define LOG_INFO(cat, fmt, ...)  ((void)0)
    #define LOG_WARN(cat, fmt, ...)  ((void)0)
    #define LOG_ERROR(cat, fmt, ...) ((void)0)
    #define LOG_DEBUG(cat, fmt, ...) ((void)0)
#endif

#endif