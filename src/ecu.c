/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: ecu.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the ECU subsystem (Electronic Control Unit) for
 * the motorcycle simulation. Its job is to monitor the shared system state
 * and update derived values such as RPM zone, engine temperature category,
 * and signal state. This file does not handle display output or user input.
 */

#include "system_state.h"
#include <unistd.h>

/* ECU updates the system every 50 milliseconds */
#define ECU_UPDATE_INTERVAL_MS 50

/* Number of ECU updates before the signal changes state */
#define SIGNAL_CYCLE_TICKS (3000 / ECU_UPDATE_INTERVAL_MS)

/* Keeps track of when to switch the signal state */
static int signal_cycle_counter = 0;


/*
 * ECU thread:
 * This thread runs continuously in the background.
 * It monitors the current motorcycle state and updates
 * values that are derived from the raw system data.
 */
void *ecu_thread(void *arg) {
    (void)arg;

    while (1) {
        /* 
         * Cycle through signal states automatically for demonstration.
         * In Phase 1, this allows the dashboard to show visible
         * signal changes even without user interaction.
         */
        signal_cycle_counter++;

        if (signal_cycle_counter >= SIGNAL_CYCLE_TICKS) {
            signal_cycle_counter = 0;

            if (g_state.signal_state == SIGNAL_OFF) {
                g_state.signal_state = SIGNAL_LEFT;
            } else if (g_state.signal_state == SIGNAL_LEFT) {
                g_state.signal_state = SIGNAL_RIGHT;
            } else if (g_state.signal_state == SIGNAL_RIGHT) {
                g_state.signal_state = SIGNAL_HAZARD;
            } else {
                g_state.signal_state = SIGNAL_OFF;
            }
        }

        /* 
         * Classify the current RPM into a zone.
         * This helps the dashboard show whether the engine
         * is idle, normal, high, or in redline.
         */
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

        /* 
         * Classify the engine temperature in Celsius.
         * This allows the dashboard to show whether the
         * engine is cold, normal, hot, or overheating.
         */
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

        /* 
         * Reset trip distance when the engine is off.
         * The ECU controls this part of the shared state.
         */
        if (!g_state.engine_on) {
            g_state.trip_distance = 0.0;
        }

        /* Wait before the next ECU update cycle */
        usleep(ECU_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
