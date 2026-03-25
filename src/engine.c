/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: engine.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the engine subsystem for the motorcycle simulation.
 * Its job is to simulate engine behavior by continuously updating RPM and
 * engine temperature in the shared system state. The engine reacts differently
 * depending on whether the motorcycle is idle, moving, or turned off.
 */

#include "system_state.h"
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

/* Engine updates every 50 milliseconds */
#define ENGINE_UPDATE_INTERVAL_MS 50

/* Rate at which the engine cools down */
#define TEMP_COOLING_RATE         0.02f

/* Rate at which the engine heats up */
#define TEMP_HEATING_RATE         0.05f

/* Target temperature when the engine is idling */
#define TEMP_IDLE_TARGET          75.0f

/* Ambient temperature when the engine is off */
#define TEMP_AMBIENT              20.0f


/*
 * Engine thread:
 * This thread runs continuously in the background.
 * It simulates RPM behavior and engine temperature changes
 * based on whether the motorcycle is on, idle, or moving.
 */
void *engine_thread(void *arg) {
    (void)arg;

    int cycle = 0;

    while (1) {
        /* Check whether the engine is currently on */
        if (g_state.engine_on) {

            /* 
             * If speed is 0, the motorcycle is idling.
             * RPM fluctuates in a repeating pattern to simulate
             * small idle changes instead of staying completely fixed.
             */
            if (g_state.speed == 0) {
                int idle_base = 100 + (cycle % 1200);
                g_state.rpm = 100 + (idle_base % 1200);
            } else {
                /* 
                 * If the motorcycle is moving, RPM increases with speed.
                 * A base RPM of 1000 is used, then speed is multiplied
                 * to create a higher target RPM.
                 */
                int target_rpm = 1000 + g_state.speed * 85;

                /* Prevent RPM from going above the allowed maximum */
                if (target_rpm > RPM_MAX) {
                    target_rpm = RPM_MAX;
                }

                /* Add a small random variation to make RPM look more realistic */
                int variation = (rand() % 101) - 50;
                g_state.rpm = target_rpm + variation;

                /* Keep RPM inside allowed minimum and maximum limits */
                if (g_state.rpm < RPM_MIN) {
                    g_state.rpm = RPM_MIN;
                }
                if (g_state.rpm > RPM_MAX) {
                    g_state.rpm = RPM_MAX;
                }
            }

            /* 
             * Engine temperature depends on RPM.
             * Higher RPM causes the target temperature to rise.
             */
            float temp_target = TEMP_IDLE_TARGET + (g_state.rpm / 200.0f);

            /* Keep target temperature within a safe simulation range */
            if (temp_target > 105.0f) {
                temp_target = 105.0f;
            }
            if (temp_target < TEMP_AMBIENT) {
                temp_target = TEMP_AMBIENT;
            }

            /* 
             * If current temperature is below the target, heat up gradually.
             * If current temperature is above the target, cool down gradually.
             */
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
            /* 
             * If the engine is off, RPM becomes 0.
             * The engine temperature slowly cools down toward ambient temperature.
             */
            g_state.rpm = 0;

            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2;

                if (g_state.engine_temp_celsius < TEMP_AMBIENT) {
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
                }
            }
        }

        /* Move to the next simulation cycle */
        cycle++;

        /* Wait before updating the engine again */
        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
