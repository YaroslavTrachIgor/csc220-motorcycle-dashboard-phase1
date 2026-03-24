/**
 * dashboard.c - Dashboard Subsystem
 * Reads shared state and displays formatted output. No simulation logic.
 */

#include "system_state.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define CONTENT_WIDTH       58   /* Chars between left ║ and right ║ (match ═ count) */
#define BAR_LENGTH          20
#define MILES_TO_KM         1.60934
#define REFRESH_INTERVAL_MS 50000  /* 50ms = 20 FPS */

static const char *rpm_zone_str(rpm_zone_t z) {
    switch (z) {
        case RPM_ZONE_IDLE:   return "IDLE";
        case RPM_ZONE_NORMAL: return "NORMAL";
        case RPM_ZONE_HIGH:   return "HIGH";
        case RPM_ZONE_REDLINE: return "REDLINE";
        default: return "---";
    }
}

static const char *temp_class_str(temp_classification_t t) {
    switch (t) {
        case TEMP_COLD:    return "COLD";
        case TEMP_NORMAL:  return "NORMAL";
        case TEMP_HOT:     return "HOT";
        case TEMP_OVERHEAT: return "OVERHEAT";
        default: return "---";
    }
}

static void format_time(time_t start, time_t now, char *buf) {
    long elapsed = (long)(now - start);
    if (elapsed < 0) elapsed = 0;
    int h = (int)(elapsed / 3600);
    int m = (int)((elapsed % 3600) / 60);
    int s = (int)(elapsed % 60);
    snprintf(buf, 16, "%02d:%02d:%02d", h, m, s);
}


/* Print a content line: pads to CONTENT_WIDTH so right border aligns */
static void print_line(const char *fmt, ...) {
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = len; i < CONTENT_WIDTH; i++) buf[i] = ' ';
    buf[CONTENT_WIDTH] = '\0';
    printf("║%.*s║\n", CONTENT_WIDTH, buf);
}

static void print_dashboard(void) {
    time_t now = time(NULL);
    char time_overall[16], time_trip[16];
    format_time(g_state.time_overall_start, now, time_overall);
    format_time(g_state.time_trip_start, now, time_trip);

    float temp_display = g_state.use_celsius ? g_state.engine_temp_celsius
        : (g_state.engine_temp_celsius * 9.0f / 5.0f + 32.0f);
    char temp_unit = g_state.use_celsius ? 'C' : 'F';

    /* Double-line border header */
    printf("\033[32m");  /* Green - terminal friendly */
    printf("╔══════════════════════════════════════════════════════════╗\n");
    print_line("                    BAZOOKI OS");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    print_line("ENG %s  │ RPM %5d [%s]",
        g_state.engine_on ? "●" : "○", g_state.rpm, rpm_zone_str(g_state.rpm_zone));
    {
        char bar_buf[32];
        int filled = (int)((float)g_state.rpm / RPM_MAX * BAR_LENGTH);
        if (filled > BAR_LENGTH) filled = BAR_LENGTH;
        int i;
        for (i = 0; i < filled; i++) bar_buf[i] = '#';
        for (; i < BAR_LENGTH; i++) bar_buf[i] = '.';
        bar_buf[BAR_LENGTH] = '\0';
        print_line("      %s", bar_buf);
    }
    print_line("TMP %5.0f°%c [%s]",
        temp_display, temp_unit, temp_class_str(g_state.temp_classification));
    {
        char bar_buf[32];
        int filled = (int)((float)g_state.speed / SPEED_MAX * BAR_LENGTH);
        if (filled > BAR_LENGTH) filled = BAR_LENGTH;
        int i;
        for (i = 0; i < filled; i++) bar_buf[i] = '#';
        for (; i < BAR_LENGTH; i++) bar_buf[i] = '.';
        bar_buf[BAR_LENGTH] = '\0';
        print_line("SPD %3d %s", g_state.speed, bar_buf);
    }
    {
        float fuel_pct = (g_state.fuel_gallons / FUEL_MAX_GALLONS) * 100.0f;
        char bar_buf[32];
        int filled = (int)(g_state.fuel_gallons / FUEL_MAX_GALLONS * BAR_LENGTH);
        if (filled > BAR_LENGTH) filled = BAR_LENGTH;
        int i;
        for (i = 0; i < filled; i++) bar_buf[i] = '#';
        for (; i < BAR_LENGTH; i++) bar_buf[i] = '.';
        bar_buf[BAR_LENGTH] = '\0';
        print_line("FUEL E%sF %5.1f%% (%.1f gal)", bar_buf, fuel_pct, g_state.fuel_gallons);
    }
    {
        double total_km = g_state.total_distance * MILES_TO_KM;
        double trip_km = g_state.trip_distance * MILES_TO_KM;
        print_line("DIST Total: %6.1f km   Trip: %6.1f km", total_km, trip_km);
    }
    print_line("TIME Overall: %s  Trip: %s", time_overall, time_trip);
    print_line(" ");
    {
        const char *left = (g_state.signal_state == SIGNAL_HAZARD || g_state.signal_state == SIGNAL_LEFT)
            ? "◀ LEFT " : "       ";
        const char *right = (g_state.signal_state == SIGNAL_HAZARD || g_state.signal_state == SIGNAL_RIGHT)
            ? "RIGHT ▶ " : "        ";
        print_line("%s%s│ %s HEADLIGHT", left, right, g_state.headlight_on ? "●" : "○");
    }
    if (g_state.fuel_gallons < FUEL_LOW_THRESHOLD) {
        print_line("⚠ LOW FUEL");
    }
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\033[0m");  /* Reset color */
}

void refresh_dashboard(void (*print_fn)(void)) {
    printf("\033[H\033[J");
    print_fn();
    fflush(stdout);
}

void *dashboard_thread(void *arg) {
    (void)arg;

    while (1) {
        refresh_dashboard(print_dashboard);
        usleep(REFRESH_INTERVAL_MS);
    }
    return NULL;
}
