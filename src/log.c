#define _XOPEN_SOURCE 700
#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *g_log_fp = NULL;
static int g_log_enabled = 1;

/* Snapshot of last message text for on-screen dashboard (subsystem + message) */
static char g_last_line[96];

const char *log_last_line(void) {
    return g_last_line[0] != '\0' ? g_last_line : "(no events yet)";
}

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

    char payload[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(payload, sizeof(payload), fmt, ap);
    va_end(ap);

    (void)snprintf(g_last_line, sizeof(g_last_line), "%s: %s",
        category ? category : "GENERAL", payload);

    fprintf(g_log_fp, "[%s] %-5s %-8s %s\n",
        ts, level_str(level), category ? category : "GENERAL", payload);
    fflush(g_log_fp);
}