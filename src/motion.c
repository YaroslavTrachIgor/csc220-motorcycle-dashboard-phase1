/**
 * Phase III: speed from W/S formulas, cruise, distance only when engine on.
 */

#include "system_state.h"
#include <unistd.h>

#define MOTION_UPDATE_INTERVAL_MS 50

static double motion_dt(void) {
    return (double)MOTION_UPDATE_INTERVAL_MS / 1000.0;
}

/* Miles advanced at constant speed_mph over dt_seconds */
static double miles_delta_for_speed(int speed_mph, double dt_seconds) {
    return (double)speed_mph * (1.0 / 3600.0) * dt_seconds;
}

void *motion_thread(void *arg) {
    (void)arg;

    while (!g_shutdown_request) {
        pthread_mutex_lock(&mtx_engine);
        pthread_mutex_lock(&mtx_motion);

        while (!g_shutdown_request && !g_state.engine_on && g_state.speed == 0) {
            pthread_mutex_unlock(&mtx_motion);
            pthread_cond_wait(&cond_engine_run, &mtx_engine);
            pthread_mutex_lock(&mtx_motion);
        }

        if (g_shutdown_request) {
            pthread_mutex_unlock(&mtx_motion);
            pthread_mutex_unlock(&mtx_engine);
            break;
        }

        bool engine_on = g_state.engine_on;
        float ar = g_state.accel_rate;
        float dr = g_state.decel_rate;
        bool cruise = g_state.cruise_active;
        int paccel = g_state.pending_accel_steps;
        int pdecel = g_state.pending_decel_steps;
        g_state.pending_accel_steps = 0;
        g_state.pending_decel_steps = 0;

        float spd_f = (float)g_state.speed;
        double dt = motion_dt();

        if (engine_on && !cruise) {
            for (int i = 0; i < paccel; i++) {
                spd_f += ((float)SPEED_MAX - spd_f) * ar * (float)dt;
            }
            for (int j = 0; j < pdecel; j++) {
                spd_f -= dr * (float)dt;
            }
        }

        int spd = (int)(spd_f + 0.5f);
        if (spd > SPEED_MAX) {
            spd = SPEED_MAX;
        }
        if (spd < SPEED_MIN) {
            spd = SPEED_MIN;
        }
        g_state.speed = spd;

        if (engine_on && g_state.speed > 0) {
            double delta_miles = miles_delta_for_speed(g_state.speed, dt);
            g_state.total_distance += delta_miles;
            g_state.trip_distance += delta_miles;
        }

        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);

        sync_notify_ecu();

        usleep(MOTION_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
