/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard — Phase II
 * File: engine.c
 * Date: 03/24/2026 (Phase I); Phase II — 04/15/2026
 *
 * Description:
 * Simulates RPM and coolant temperature. All writes to rpm, engine_temp_celsius,
 * and engine_on happen under mtx_engine. Speed is read under mtx_motion after
 * mtx_engine (global lock order). When engine_on flips to false, this thread
 * zeros speed under mtx_motion so motion/fuel see a consistent stop. Periodic
 * engine_on toggles call pthread_cond_broadcast(&cond_engine_run) so motion
 * and fuel coordinate via pthread_cond_wait (not polling). sync_notify_ecu()
 * wakes the ECU after each engine update. The outer while(1) plus usleep is
 * the simulation timestep, not a spin-wait on shared variables.
 */

#include "system_state.h"
#include <unistd.h>
#include <stdlib.h>

#define ENGINE_UPDATE_INTERVAL_MS 50
#define TEMP_COOLING_RATE         0.02f
#define TEMP_HEATING_RATE         0.05f
#define TEMP_IDLE_TARGET          75.0f
#define TEMP_AMBIENT              20.0f

/* About 15 seconds at 50 ms per cycle */
#define ENGINE_ON_TOGGLE_CYCLES   300

void *engine_thread(void *arg) {
    (void)arg;   // unused parameter

    int cycle = 0;                  // keeps track of total update cycles
    int engine_toggle_counter = 0;  // counts cycles before toggling engine on/off

    /* 
     * Phase II startup delay:
     * This gives the dashboard and ECU time to reflect the command-line
     * initialized state before the engine simulation begins updating RPM.
     */
    sleep(1);

    while (1) {
        /* Lock engine mutex before reading/writing engine-related shared state */
        pthread_mutex_lock(&mtx_engine);

        // Engine is ON: calculate RPM and engine temperature
        if (g_state.engine_on) {
            /* Lock motion mutex after engine mutex to safely read speed */
            pthread_mutex_lock(&mtx_motion);
            int spd = g_state.speed;   // snapshot of current speed
            pthread_mutex_unlock(&mtx_motion);

            /* If bike is not moving, keep RPM in idle range */
            if (spd == 0) {
                int idle_span = RPM_IDLE_MAX - RPM_IDLE_MIN + 1;
                g_state.rpm = RPM_IDLE_MIN + (cycle % idle_span);
            } else {
                /* Estimate RPM based on speed */
                int target_rpm = 1000 + spd * 85;
                if (target_rpm > RPM_MAX) {
                    target_rpm = RPM_MAX;
                }

                /* Add small random variation for more realistic RPM behavior */
                int variation = (rand() % 101) - 50;
                g_state.rpm = target_rpm + variation;

                /* Clamp RPM so it stays within valid range */
                if (g_state.rpm < RPM_MIN) {
                    g_state.rpm = RPM_MIN;
                }
                if (g_state.rpm > RPM_MAX) {
                    g_state.rpm = RPM_MAX;
                }
            }

            /* Compute target engine temperature from RPM */
            float temp_target = TEMP_IDLE_TARGET + (g_state.rpm / 200.0f);

            /* Clamp target temperature to safe range */
            if (temp_target > 105.0f) {
                temp_target = 105.0f;
            }
            if (temp_target < TEMP_AMBIENT) {
                temp_target = TEMP_AMBIENT;
            }

            /* Move current temperature gradually toward target */
            if (g_state.engine_temp_celsius < temp_target) {
                g_state.engine_temp_celsius += TEMP_HEATING_RATE;
                if (g_state.engine_temp_celsius > temp_target) {
                    g_state.engine_temp_celsius = temp_target;
                }
            } else {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE;
                if (g_state.engine_temp_celsius < temp_target) {
                    g_state.engine_temp_celsius = temp_target;
                }
            }
        } else {
            /* Engine is OFF: RPM drops to zero */
            g_state.rpm = 0;

            /* Cool engine down toward ambient temperature */
            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2.0f;
                if (g_state.engine_temp_celsius < TEMP_AMBIENT) {
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
                }
            }
        }

        /* Count cycles until it is time to toggle engine state */
        engine_toggle_counter++;
        if (engine_toggle_counter >= ENGINE_ON_TOGGLE_CYCLES) {
            engine_toggle_counter = 0;

            /* Flip engine state */
            g_state.engine_on = !g_state.engine_on;

            /* If engine just turned on, wake up waiting threads */
            if (g_state.engine_on) {
                pthread_cond_broadcast(&cond_engine_run);
            }
        }

        /* Done updating engine state */
        pthread_mutex_unlock(&mtx_engine);

        /* Notify ECU thread that engine state has changed */
        sync_notify_ecu();

        cycle++;

        /* Sleep to control simulation speed */
        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}