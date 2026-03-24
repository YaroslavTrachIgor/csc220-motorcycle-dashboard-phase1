/**
 * motion.c - Motion Subsystem
 * Simulates motorcycle speed and distance. Speed cycles 50->70->50 mph.
 * Distance accumulates based on speed (miles per hour -> miles per second).
 */

#include "system_state.h"
#include <unistd.h>

#define MOTION_UPDATE_INTERVAL_MS 1000  /* 1 second per speed step (per spec) */
#define MPH_TO_MPS              (1.0 / 3600.0)  /* miles per second per mph */

void *motion_thread(void *arg) {
    (void)arg;
    int speed_direction = 1;  /* 1 = accelerating, -1 = decelerating */

    while (1) {
        if (g_state.engine_on) {
            /* Cycle speed: 50 -> 70 -> 50 */
            g_state.speed += speed_direction;

            if (g_state.speed >= 70) {
                g_state.speed = 70;
                speed_direction = -1;
            } else if (g_state.speed <= 50) {
                g_state.speed = 50;
                speed_direction = 1;
            }

            /* Distance: speed (mph) * time (hours) = miles
             * Update every 1 second: miles += speed * (1/3600) */
            double delta_miles = g_state.speed * MPH_TO_MPS;
            g_state.total_distance += delta_miles;
            g_state.trip_distance += delta_miles;
        } else {
            g_state.speed = 0;
            /* Trip distance resets when engine off - handled by ECU/state */
        }

        usleep(MOTION_UPDATE_INTERVAL_MS * 1000);
    }
    return NULL;
}
