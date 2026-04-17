/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: engine.c
 *
 * Description:
 * Engine subsystem: RPM and temperature simulation. Phase II uses mutexes
 * and condition variables; engine_on is toggled periodically so motion/fuel
 * can coordinate on cond_engine_run.
 */

#include "system_state.h"
#include <unistd.h>
#include <stdlib.h>

#define ENGINE_UPDATE_INTERVAL_MS 50
#define TEMP_COOLING_RATE         0.02f
#define TEMP_HEATING_RATE         0.05f
#define TEMP_IDLE_TARGET          75.0f
#define TEMP_AMBIENT              20.0f

/* ~15 s at 50 ms per tick — demo engine stop/start for Phase II coordination */
#define ENGINE_ON_TOGGLE_CYCLES   300

void *engine_thread(void *arg) {
    (void)arg;

    int cycle = 0;
    int engine_toggle_counter = 0;

    /* 
     * Phase II startup delay:
     * This gives the dashboard and ECU time to reflect the command-line
     * initialized state before the engine simulation begins updating RPM.
     */
    sleep(1);

    while (1) {
        pthread_mutex_lock(&mtx_engine);

        //CRITICAL SECTION begin -- engine_on / RPM / temp; other threads read or wait on engine_on
        if (g_state.engine_on) {
            pthread_mutex_lock(&mtx_motion);
            //CRITICAL SECTION begin -- engine reads speed while motion may update distance/speed
            int spd = g_state.speed;
            //CRITICAL SECTION end --

            if (spd == 0) {
                int idle_base = 100 + (cycle % 1200);
                g_state.rpm = 100 + (idle_base % 1200);
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

            pthread_mutex_unlock(&mtx_motion);
        } else {
            g_state.rpm = 0;
            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2.0f;
                if (g_state.engine_temp_celsius < TEMP_AMBIENT) {
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
                }
            }
        }
        //CRITICAL SECTION end --

        engine_toggle_counter++;
        if (engine_toggle_counter >= ENGINE_ON_TOGGLE_CYCLES) {
            engine_toggle_counter = 0;
            //CRITICAL SECTION begin -- engine_on transition; motion/fuel wait on cond_engine_run
            g_state.engine_on = !g_state.engine_on;
            pthread_cond_broadcast(&cond_engine_run);
            if (!g_state.engine_on) {
                pthread_mutex_lock(&mtx_motion);
                //CRITICAL SECTION begin -- engine forces speed 0 when engine stops
                g_state.speed = 0;
                //CRITICAL SECTION end --
                pthread_mutex_unlock(&mtx_motion);
            }
            //CRITICAL SECTION end --
        }

        pthread_mutex_unlock(&mtx_engine);

        sync_notify_ecu();

        cycle++;
        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}