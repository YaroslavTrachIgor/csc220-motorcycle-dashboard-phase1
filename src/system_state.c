/**
 * system_state.c - Shared system state implementation
 */

#include "system_state.h"
#include <stdlib.h>
#include <stdio.h>

system_state_t g_state;

void system_state_init(void) {
    time_t now = time(NULL);
    g_state.program_start = now;

    /* Random total time elapsed at start (e.g., 0-100 hours in seconds) */
    g_state.time_overall_start = now - (time_t)(rand() % (100 * 3600));

    /* Random total distance at start (e.g., 0-50000 km) */
    g_state.total_distance = (double)(rand() % 500000) / 10.0;

    g_state.time_trip_start = now;
    g_state.engine_on = true;
    g_state.rpm = 0;
    g_state.engine_temp_celsius = 85.0f;
    g_state.speed = 50;
    g_state.trip_distance = 0.0;
    g_state.fuel_gallons = 3.5f;
    g_state.signal_state = SIGNAL_OFF;
    g_state.headlight_on = true;
    g_state.use_celsius = true;

    /* Default derived states - ECU will update these */
    g_state.rpm_zone = RPM_ZONE_IDLE;
    g_state.temp_classification = TEMP_NORMAL;
}
