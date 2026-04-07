/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: dashboard.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the dashboard subsystem of the motorcycle simulation.
 * Its job is to read the shared system state and display a formatted dashboard
 * in the terminal. This file does not control the simulation itself. It only
 * handles output such as engine status, RPM, temperature, speed, fuel level,
 * distance, timer values, signal indicators, and warning messages.
 */

#define _XOPEN_SOURCE 700

#include "system_state.h"
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

/* Width of the dashboard content area inside the border */
#define CONTENT_WIDTH       58

/* Number of characters used for progress bars */
#define BAR_LENGTH          20

/* Conversion value from miles to kilometers */
#define MILES_TO_KM         1.60934

/* Dashboard refresh delay in microseconds (50000 = 50 ms) */
#define REFRESH_INTERVAL_US 50000


/* Converts the RPM zone enum into a readable text label */
static const char *rpm_zone_str(rpm_zone_t z) {
    switch (z) {
        case RPM_ZONE_IDLE:    return "IDLE";
        case RPM_ZONE_NORMAL:  return "NORMAL";
        case RPM_ZONE_HIGH:    return "HIGH";
        case RPM_ZONE_REDLINE: return "REDLINE";
        default:               return "---";
    }
}


/* Converts the temperature classification enum into text */
static const char *temp_class_str(temp_classification_t t) {
    switch (t) {
        case TEMP_COLD:      return "COLD";
        case TEMP_NORMAL:    return "NORMAL";
        case TEMP_HOT:       return "HOT";
        case TEMP_OVERHEAT:  return "OVERHEAT";
        default:             return "---";
    }
}


/* Formats elapsed time into HH:MM:SS */
static void format_time(time_t start, time_t now, char *buf) {
    long elapsed = (long)(now - start);

    /* Prevent negative elapsed time */
    if (elapsed < 0) {
        elapsed = 0;
    }

    int h = (int)(elapsed / 3600);
    int m = (int)((elapsed % 3600) / 60);
    int s = (int)(elapsed % 60);

    snprintf(buf, 16, "%02d:%02d:%02d", h, m, s);
}


/*
 * Decodes one UTF-8 code point from *pp, advances *pp past it.
 * Returns the scalar value, or -1 at NUL. On invalid/truncated data,
 * consumes one byte and returns that byte (treated as width 1).
 *
 * Locale-independent: Docker/minimal images often use LC_CTYPE=C, where
 * mbrtowc does not decode UTF-8, which made strlen-style byte counting
 * break the padded box border.
 */
static int utf8_next_codepoint(const unsigned char **pp) {
    const unsigned char *p = *pp;
    unsigned char c = p[0];

    if (c == '\0') {
        return -1;
    }

    /* ASCII */
    if (c < 0x80u) {
        *pp = p + 1;
        return (int)c;
    }

    /* 2-byte */
    if ((c & 0xE0u) == 0xC0u && p[1] != '\0') {
        unsigned char c1 = p[1];
        if ((c1 & 0xC0u) != 0x80u) {
            *pp = p + 1;
            return (int)c;
        }
        uint32_t cp = ((uint32_t)(c & 0x1Fu) << 6) | (uint32_t)(c1 & 0x3Fu);
        if (cp < 0x80u) {
            *pp = p + 1;
            return (int)c;
        }
        *pp = p + 2;
        return (int)cp;
    }

    /* 3-byte */
    if ((c & 0xF0u) == 0xE0u && p[1] != '\0' && p[2] != '\0') {
        unsigned char c1 = p[1];
        unsigned char c2 = p[2];
        if ((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u) {
            *pp = p + 1;
            return (int)c;
        }
        uint32_t cp = ((uint32_t)(c & 0x0Fu) << 12)
            | ((uint32_t)(c1 & 0x3Fu) << 6)
            | (uint32_t)(c2 & 0x3Fu);
        if (cp < 0x800u || (cp >= 0xD800u && cp <= 0xDFFFu)) {
            *pp = p + 1;
            return (int)c;
        }
        *pp = p + 3;
        return (int)cp;
    }

    /* 4-byte */
    if ((c & 0xF8u) == 0xF0u && p[1] != '\0' && p[2] != '\0' && p[3] != '\0') {
        unsigned char c1 = p[1];
        unsigned char c2 = p[2];
        unsigned char c3 = p[3];
        if ((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u || (c3 & 0xC0u) != 0x80u) {
            *pp = p + 1;
            return (int)c;
        }
        uint32_t cp = ((uint32_t)(c & 0x07u) << 18)
            | ((uint32_t)(c1 & 0x3Fu) << 12)
            | ((uint32_t)(c2 & 0x3Fu) << 6)
            | (uint32_t)(c3 & 0x3Fu);
        if (cp < 0x10000u || cp > 0x10FFFFu) {
            *pp = p + 1;
            return (int)c;
        }
        *pp = p + 4;
        return (int)cp;
    }

    *pp = p + 1;
    return (int)c;
}


/*
 * Locale-independent terminal column width for a Unicode code point.
 * Returns 0 for control chars, 2 for East Asian Wide/Fullwidth, 1 otherwise.
 * Replaces wcwidth() which breaks under LC_CTYPE=C in minimal Docker images.
 */
static int codepoint_width(uint32_t cp) {
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
        return 0;

    if ((cp >= 0x1100 && cp <= 0x115F) ||
        cp == 0x2329 || cp == 0x232A ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3040 && cp <= 0x33BF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x4E00 && cp <= 0xA4CF) ||
        (cp >= 0xA960 && cp <= 0xA97C) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF01 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F300 && cp <= 0x1F9FF) ||
        (cp >= 0x20000 && cp <= 0x2FFFD) ||
        (cp >= 0x30000 && cp <= 0x3FFFD))
        return 2;

    return 1;
}


/*
 * Terminal display width of a UTF-8 string — fully locale-independent.
 */
static int utf8_display_width(const char *s) {
    int total = 0;
    const unsigned char *p = (const unsigned char *)s;

    for (;;) {
        int cp = utf8_next_codepoint(&p);
        if (cp < 0) {
            break;
        }
        total += codepoint_width((uint32_t)cp);
    }

    return total;
}


/* Cuts a UTF-8 string so it fits inside the dashboard width */
static void utf8_truncate_to_width(char *buf, int max_width) {
    while ((int)utf8_display_width(buf) > max_width && buf[0] != '\0') {
        size_t len = strlen(buf);

        if (len == 0) {
            break;
        }

        len--;

        /* Move backward safely if inside a multi-byte UTF-8 character */
        while (len > 0 && (buf[len] & 0xC0) == 0x80) {
            len--;
        }

        buf[len] = '\0';
    }
}


/*
 * Prints one padded line inside the dashboard border.
 * This keeps all lines aligned neatly in the terminal.
 */
static void print_line(const char *fmt, ...) {
    char buf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 64, fmt, ap);
    va_end(ap);

    utf8_truncate_to_width(buf, CONTENT_WIDTH);

    int w = utf8_display_width(buf);
    int pad = CONTENT_WIDTH - w;
    if (pad < 0) {
        pad = 0;
    }

    size_t len = strlen(buf);
    for (int i = 0; i < pad && len + 1 < sizeof(buf); i++) {
        buf[len++] = ' ';
    }
    buf[len] = '\0';

    printf("║%s║\n", buf);
}


/*
 * Logs only important state transitions so the log does not spam every refresh.
 * Compiled only when ENABLE_LOG is set; otherwise LOG_* macros drop arguments
 * and helpers used only here would trigger -Wunused-function.
 */
#ifdef ENABLE_LOG
static const char *signal_state_str(signal_state_t s) {
    switch (s) {
        case SIGNAL_OFF:    return "OFF";
        case SIGNAL_LEFT:   return "LEFT";
        case SIGNAL_RIGHT:  return "RIGHT";
        case SIGNAL_HAZARD: return "HAZARD";
        default:            return "---";
    }
}
#endif

static void log_dashboard_state_changes(void) {
#ifdef ENABLE_LOG
    static int initialized = 0;

    static int prev_engine_on;
    static rpm_zone_t prev_rpm_zone;
    static temp_classification_t prev_temp_classification;
    static signal_state_t prev_signal_state;
    static int prev_headlight_on;
    static int prev_low_fuel;

    int current_low_fuel = (g_state.fuel_gallons < FUEL_LOW_THRESHOLD);

    if (!initialized) {
        prev_engine_on = g_state.engine_on;
        prev_rpm_zone = g_state.rpm_zone;
        prev_temp_classification = g_state.temp_classification;
        prev_signal_state = g_state.signal_state;
        prev_headlight_on = g_state.headlight_on;
        prev_low_fuel = current_low_fuel;
        initialized = 1;

        LOG_INFO("DASH", "Initial dashboard state captured");
        return;
    }

    if (g_state.engine_on != prev_engine_on) {
        LOG_INFO("ENGINE", "Engine turned %s",
            g_state.engine_on ? "ON" : "OFF");
        prev_engine_on = g_state.engine_on;
    }

    if (g_state.rpm_zone != prev_rpm_zone) {
        LOG_INFO("RPM", "RPM zone changed: %s -> %s (RPM=%d)",
            rpm_zone_str(prev_rpm_zone),
            rpm_zone_str(g_state.rpm_zone),
            g_state.rpm);
        prev_rpm_zone = g_state.rpm_zone;
    }

    if (g_state.temp_classification != prev_temp_classification) {
        if (g_state.temp_classification == TEMP_OVERHEAT) {
            LOG_WARN("TEMP", "Temperature classification changed: %s -> %s (%.1f C)",
                temp_class_str(prev_temp_classification),
                temp_class_str(g_state.temp_classification),
                g_state.engine_temp_celsius);
        } else {
            LOG_INFO("TEMP", "Temperature classification changed: %s -> %s (%.1f C)",
                temp_class_str(prev_temp_classification),
                temp_class_str(g_state.temp_classification),
                g_state.engine_temp_celsius);
        }
        prev_temp_classification = g_state.temp_classification;
    }

    if (g_state.signal_state != prev_signal_state) {
        LOG_INFO("SIGNAL", "Signal changed: %s -> %s",
            signal_state_str(prev_signal_state),
            signal_state_str(g_state.signal_state));
        prev_signal_state = g_state.signal_state;
    }

    if (g_state.headlight_on != prev_headlight_on) {
        LOG_INFO("LIGHT", "Headlight turned %s",
            g_state.headlight_on ? "ON" : "OFF");
        prev_headlight_on = g_state.headlight_on;
    }

    if (current_low_fuel != prev_low_fuel) {
        if (current_low_fuel) {
            LOG_WARN("FUEL", "Low fuel warning entered: %.2f gallons remaining",
                g_state.fuel_gallons);
        } else {
            LOG_INFO("FUEL", "Low fuel warning cleared: %.2f gallons remaining",
                g_state.fuel_gallons);
        }
        prev_low_fuel = current_low_fuel;
    }
#endif
}


/*
 * Builds and prints the full dashboard.
 * It reads values from the shared global state and displays them.
 * No simulation logic happens here.
 */
static void print_dashboard(void) {
    time_t now = time(NULL);
    char time_overall[16], time_trip[16];

    format_time(g_state.time_overall_start, now, time_overall);
    format_time(g_state.time_trip_start, now, time_trip);

    /* Show temperature in Celsius or Fahrenheit based on settings */
    float temp_display = g_state.use_celsius
        ? g_state.engine_temp_celsius
        : (g_state.engine_temp_celsius * 9.0f / 5.0f + 32.0f);

    char temp_unit = g_state.use_celsius ? 'C' : 'F';

    /* Set text color to green for the dashboard */
    printf("\033[32m");

    printf("╔══════════════════════════════════════════════════════════╗\n");
    print_line("                    BAZOOKI OS");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    /* Engine status and RPM information */
    print_line("ENG %s  │ RPM %5d [%s]",
        g_state.engine_on ? "●" : "○",
        g_state.rpm,
        rpm_zone_str(g_state.rpm_zone));

    /* RPM bar visualization */
    {
        char bar_buf[32];
        int filled = (int)((float)g_state.rpm / RPM_MAX * BAR_LENGTH);

        if (filled < 0) {
            filled = 0;
        }
        if (filled > BAR_LENGTH) {
            filled = BAR_LENGTH;
        }

        int i;
        for (i = 0; i < filled; i++) {
            bar_buf[i] = '#';
        }
        for (; i < BAR_LENGTH; i++) {
            bar_buf[i] = '.';
        }
        bar_buf[BAR_LENGTH] = '\0';

        print_line("      %s", bar_buf);
    }

    /* Temperature display */
    print_line("TMP %5.0f°%c [%s]",
        temp_display,
        temp_unit,
        temp_class_str(g_state.temp_classification));

    /* Speed bar visualization */
    {
        char bar_buf[32];
        int filled = (int)((float)g_state.speed / SPEED_MAX * BAR_LENGTH);

        if (filled < 0) {
            filled = 0;
        }
        if (filled > BAR_LENGTH) {
            filled = BAR_LENGTH;
        }

        int i;
        for (i = 0; i < filled; i++) {
            bar_buf[i] = '#';
        }
        for (; i < BAR_LENGTH; i++) {
            bar_buf[i] = '.';
        }
        bar_buf[BAR_LENGTH] = '\0';

        print_line("SPD %3d %s", g_state.speed, bar_buf);
    }

    /* Fuel level bar and percentage */
    {
        float fuel_pct = (g_state.fuel_gallons / FUEL_MAX_GALLONS) * 100.0f;
        char bar_buf[32];
        int filled = (int)(g_state.fuel_gallons / FUEL_MAX_GALLONS * BAR_LENGTH);

        if (filled < 0) {
            filled = 0;
        }
        if (filled > BAR_LENGTH) {
            filled = BAR_LENGTH;
        }

        int i;
        for (i = 0; i < filled; i++) {
            bar_buf[i] = '#';
        }
        for (; i < BAR_LENGTH; i++) {
            bar_buf[i] = '.';
        }
        bar_buf[BAR_LENGTH] = '\0';

        print_line("FUEL E%sF %5.1f%% (%.1f gal)",
            bar_buf,
            fuel_pct,
            g_state.fuel_gallons);
    }

    /* Distance converted from miles to kilometers */
    {
        double total_km = g_state.total_distance * MILES_TO_KM;
        double trip_km = g_state.trip_distance * MILES_TO_KM;
        print_line("DIST Total: %6.1f km   Trip: %6.1f km", total_km, trip_km);
    }

    /* Overall running time and trip time */
    print_line("TIME Overall: %s  Trip: %s", time_overall, time_trip);

    print_line(" ");

    /* Turn signal and headlight indicators */
    {
        const char *left = (g_state.signal_state == SIGNAL_HAZARD || g_state.signal_state == SIGNAL_LEFT)
            ? "◀ LEFT " : "       ";
        const char *right = (g_state.signal_state == SIGNAL_HAZARD || g_state.signal_state == SIGNAL_RIGHT)
            ? "RIGHT ▶ " : "        ";

        print_line("%s%s│ %s HEADLIGHT", left, right, g_state.headlight_on ? "●" : "○");
    }

    /* Warning message when fuel is below the low threshold */
    if (g_state.fuel_gallons < FUEL_LOW_THRESHOLD) {
        print_line("⚠ LOW FUEL");
    }

    /* Latest log event (file tail shows full history; screen refresh clears stderr) */
    print_line("LOG %s", log_last_line());

    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\033[0m");
}


/*
 * Clears the terminal, prints the dashboard again,
 * and flushes output so the update appears immediately.
 */
void refresh_dashboard(void (*print_fn)(void)) {
    printf("\033[H\033[J");
    print_fn();
    fflush(stdout);
}


/*
 * Dashboard thread:
 * This thread runs continuously during the program.
 * Its only job is to refresh the terminal dashboard at a fixed interval
 * so the user can see updated motorcycle values in real time.
 */
void *dashboard_thread(void *arg) {
    (void)arg;

    LOG_INFO("DASH", "Dashboard thread started");

    while (1) {
        log_dashboard_state_changes();
        refresh_dashboard(print_dashboard);
        usleep(REFRESH_INTERVAL_US);
    }

    return NULL;
}