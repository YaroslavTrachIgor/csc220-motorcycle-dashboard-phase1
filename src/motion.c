/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: motion.c
 *
 * Description:
 * Motion subsystem: speed and distance. Phase II gates speed/distance updates
 * on engine_on using pthread_cond_wait (not a busy loop).
 */

#include "system_state.h"
#include <unistd.h>

#define MOTION_UPDATE_INTERVAL_MS 1000
#define MPH_TO_MPS (1.0 / 3600.0)

void *motion_thread(void *arg) {
    (void)arg;

    int speed_direction = 1;

    while (1) {
        pthread_mutex_lock(&mtx_engine);
        //CRITICAL SECTION begin -- cond_engine_run wait + speed/distance; lock order engine->motion
        while (!g_state.engine_on) {
            pthread_cond_wait(&cond_engine_run, &mtx_engine);
        }
        pthread_mutex_lock(&mtx_motion);
        if (g_state.engine_on) {
            g_state.speed += speed_direction;

            if (g_state.speed >= 70) {
                g_state.speed = 70;
                speed_direction = -1;
            } else if (g_state.speed <= 50) {
                g_state.speed = 50;
                speed_direction = 1;
            }

            double delta_miles = g_state.speed * MPH_TO_MPS;
            g_state.total_distance += delta_miles;
            g_state.trip_distance += delta_miles;
        } else {
            g_state.speed = 0;
        }

        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);
        //CRITICAL SECTION end --

        sync_notify_ecu();

        usleep(MOTION_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
