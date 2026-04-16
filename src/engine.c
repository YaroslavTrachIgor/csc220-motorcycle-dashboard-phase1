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

/* ~15 s at 50 ms per tick — demonstrates fuel/motion blocking while engine off */
#define ENGINE_ON_TOGGLE_CYCLES   300

void *engine_thread(void *arg) {
    (void)arg;

    int cycle = 0;
    int engine_toggle_counter = 0;

    while (1) {
        pthread_mutex_lock(&mtx_engine);

        // CRITICAL SECTION begin -- engine_on / RPM / temp; other threads read or wait on engine_on
        if (g_state.engine_on) {
            pthread_mutex_lock(&mtx_motion);
            // CRITICAL SECTION begin -- engine reads speed while motion may update speed/distance
            int spd = g_state.speed;
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_motion);

            if (spd == 0) {
                int idle_span = RPM_IDLE_MAX - RPM_IDLE_MIN + 1;
                g_state.rpm = RPM_IDLE_MIN + (cycle % idle_span);
            } else {
                int target_rpm = 1000 + spd * 85;
                if (target_rpm > RPM_MAX) {
                    target_rpm = RPM_MAX;
                }

                int variation = (rand() % 101) - 50;
                g_state.rpm = target_rpm + variation;

                if (g_state.rpm < RPM_MIN) {
                    g_state.rpm = RPM_MIN;
                }
                if (g_state.rpm > RPM_MAX) {
                    g_state.rpm = RPM_MAX;
                }
            }

            float temp_target = TEMP_IDLE_TARGET + (g_state.rpm / 200.0f);
            if (temp_target > 105.0f) {
                temp_target = 105.0f;
            }
            if (temp_target < TEMP_AMBIENT) {
                temp_target = TEMP_AMBIENT;
            }

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
            g_state.rpm = 0;

            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2.0f;
                if (g_state.engine_temp_celsius < TEMP_AMBIENT) {
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
                }
            }
        }
        // CRITICAL SECTION end --

        engine_toggle_counter++;
        if (engine_toggle_counter >= ENGINE_ON_TOGGLE_CYCLES) {
            engine_toggle_counter = 0;

            // CRITICAL SECTION begin -- engine_on transition; motion/fuel wait on cond_engine_run
            g_state.engine_on = !g_state.engine_on;

            if (g_state.engine_on) {
                pthread_cond_broadcast(&cond_engine_run);
            }
            // CRITICAL SECTION end --
        }

        pthread_mutex_unlock(&mtx_engine);

        sync_notify_ecu();

        cycle++;
        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
