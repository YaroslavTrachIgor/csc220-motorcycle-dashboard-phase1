/**
 * ecu.c - ECU Subsystem (Electronic Control Unit)
 * Monitors and classifies system state. Updates derived state.
 * Does NOT handle display or user input.
 */

#include "system_state.h"
#include <unistd.h>

#define ECU_UPDATE_INTERVAL_MS 50
#define SIGNAL_CYCLE_TICKS      (3000 / ECU_UPDATE_INTERVAL_MS)
static int signal_cycle_counter = 0;

void *ecu_thread(void *arg) {
    (void)arg;

    while (1) {
        /* Cycle signal state for demo (static in Phase 1, but visible) */
        signal_cycle_counter++;
        if (signal_cycle_counter >= SIGNAL_CYCLE_TICKS) {
            signal_cycle_counter = 0;
            if (g_state.signal_state == SIGNAL_OFF)
                g_state.signal_state = SIGNAL_LEFT;
            else if (g_state.signal_state == SIGNAL_LEFT)
                g_state.signal_state = SIGNAL_RIGHT;
            else if (g_state.signal_state == SIGNAL_RIGHT)
                g_state.signal_state = SIGNAL_HAZARD;
            else
                g_state.signal_state = SIGNAL_OFF;
        }
        /* Classify RPM zone */
        int rpm = g_state.rpm;
        if (rpm < 100) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_IDLE_MAX) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_NORMAL_MAX) {
            g_state.rpm_zone = RPM_ZONE_NORMAL;
        } else if (rpm <= RPM_HIGH_MAX) {
            g_state.rpm_zone = RPM_ZONE_HIGH;
        } else {
            g_state.rpm_zone = RPM_ZONE_REDLINE;
        }

        /* Classify engine temperature (Celsius) */
        float temp = g_state.engine_temp_celsius;
        if (temp < 60) {
            g_state.temp_classification = TEMP_COLD;
        } else if (temp <= 95) {
            g_state.temp_classification = TEMP_NORMAL;
        } else if (temp <= 105) {
            g_state.temp_classification = TEMP_HOT;
        } else {
            g_state.temp_classification = TEMP_OVERHEAT;
        }

        /* Reset trip distance when engine off - ECU manages this */
        if (!g_state.engine_on) {
            g_state.trip_distance = 0.0;
        }

        usleep(ECU_UPDATE_INTERVAL_MS * 1000);
    }
    return NULL;
}
