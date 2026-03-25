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
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <wchar.h>
#include <limits.h>

/* Width of the dashboard content area inside the border */
#define CONTENT_WIDTH       58

/* Number of characters used for progress bars */
#define BAR_LENGTH          20

/* Conversion value from miles to kilometers */
#define MILES_TO_KM         1.60934

/* Dashboard refresh delay in microseconds (50000 = 50 ms) */
#define REFRESH_INTERVAL_MS 50000


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
 * Finds the terminal display width of a UTF-8 string.
 * This is used instead of strlen() so special symbols line up correctly.
 */
static int utf8_display_width(const char *s) {
    mbstate_t mbs;
    memset(&mbs, 0, sizeof(mbs));
    int total = 0;
    const unsigned char *p = (const unsigned char *)s;

    while (*p) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, (const char *)p, MB_LEN_MAX, &mbs);

        /* Handle invalid UTF-8 safely */
        if (n == (size_t)-1 || n == (size_t)-2) {
            total += 1;
            p++;
            memset(&mbs, 0, sizeof(mbs));
            continue;
        }

        if (n == 0) {
            break;
        }

        int cw = wcwidth(wc);
        if (cw < 0) {
            cw = 1;
        }

        total += cw;
        p += n;
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

    while (1) {
        refresh_dashboard(print_dashboard);
        usleep(REFRESH_INTERVAL_MS);
    }

    return NULL;
}
