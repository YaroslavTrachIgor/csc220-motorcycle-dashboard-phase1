#define _XOPEN_SOURCE 700
#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *g_log_fp = NULL;
static int g_log_enabled = 1;

static const char *level_str(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        default:              return "LOG";
    }
}

void log_init(const char *filename) {
    if (filename && filename[0] != '\0') {
        g_log_fp = fopen(filename, "a");
        if (!g_log_fp) {
            g_log_fp = stderr;
        }
    } else {
        g_log_fp = stderr;
    }

    /* Optional env var override */
    {
        const char *env = getenv("BAZOOKI_LOG");
        if (env && strcmp(env, "0") == 0) {
            g_log_enabled = 0;
        }
    }
}

void log_close(void) {
    if (g_log_fp && g_log_fp != stderr) {
        fclose(g_log_fp);
    }
    g_log_fp = NULL;
}

void log_set_enabled(int enabled) {
    g_log_enabled = enabled;
}

int log_is_enabled(void) {
    return g_log_enabled;
}

void log_message(log_level_t level, const char *category, const char *fmt, ...) {
    if (!g_log_enabled) {
        return;
    }

    if (!g_log_fp) {
        g_log_fp = stderr;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);

    fprintf(g_log_fp, "[%s] %-5s %-8s ", ts, level_str(level), category ? category : "GENERAL");

    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log_fp, fmt, ap);
    va_end(ap);

    fputc('\n', g_log_fp);
    fflush(g_log_fp);
}