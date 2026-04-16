/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: motion.c
 *
 * Description:
 * Motion subsystem: speed and distance. Phase II uses condition variables
 * to avoid busy waiting, but still allows distance to accumulate while the
 * motorcycle is coasting to a stop after the engine is turned OFF.
 */

#include "system_state.h"
#include <unistd.h>

#define MOTION_UPDATE_INTERVAL_MS 1000
#define MPH_TO_MILES_PER_TICK (1.0 / 3600.0)

void *motion_thread(void *arg) {
    (void)arg;

    int speed_direction = 1;

    while (1) {
        pthread_mutex_lock(&mtx_engine);
        pthread_mutex_lock(&mtx_motion);
        // CRITICAL SECTION begin -- motion reads engine_on and updates speed/distance; lock order engine->motion

        /*
         * Wait only when the motorcycle is fully stopped and engine is OFF.
         * If engine is OFF but speed > 0, do not wait yet because the bike
         * may still be rolling and distance must continue accumulating until
         * speed reaches 0.
         */
        while (!g_state.engine_on && g_state.speed == 0) {
            pthread_mutex_unlock(&mtx_motion);
            pthread_cond_wait(&cond_engine_run, &mtx_engine);
            pthread_mutex_lock(&mtx_motion);
        }

        if (g_state.engine_on) {
            /*
             * Normal Phase I motion behavior while engine is ON.
             * ECU may still cap speed separately in Phase II.
             */
            g_state.speed += speed_direction;

            if (g_state.speed >= 70) {
                g_state.speed = 70;
                speed_direction = -1;
            } else if (g_state.speed <= 50) {
                g_state.speed = 50;
                speed_direction = 1;
            }
        }

        /*
         * Distance should accumulate whenever the motorcycle is still moving,
         * even if the engine has been turned OFF and the bike is coasting.
         * Once speed reaches 0, distance stops accumulating naturally.
         */
        if (g_state.speed > 0) {
            double delta_miles = g_state.speed * MPH_TO_MILES_PER_TICK;
            g_state.total_distance += delta_miles;
            g_state.trip_distance += delta_miles;
        }

        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);

        sync_notify_ecu();

        usleep(MOTION_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
