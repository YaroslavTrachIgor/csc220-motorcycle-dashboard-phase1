/**
 * Phase III: consumption when engine on; empty-tank kill; refuel timer completion.
 */

#include "system_state.h"
#include <unistd.h>
#include <time.h>

#define FUEL_UPDATE_INTERVAL_MS 100
#define BASE_CONSUMPTION        0.00001f
#define SPEED_FACTOR            0.00003f
#define RPM_FACTOR              0.0000002f

void *fuel_thread(void *arg) {
    (void)arg;

    while (!g_shutdown_request) {
        time_t now = time(NULL);

        pthread_mutex_lock(&mtx_fuel);
        if (g_state.refueling_active && now >= g_state.refuel_deadline) {
            g_state.fuel_gallons = (float)FUEL_MAX_GALLONS;
            g_state.refueling_active = false;
            g_state.needs_refuel_to_start = false;
        }
        pthread_mutex_unlock(&mtx_fuel);

        pthread_mutex_lock(&mtx_engine);
        while (!g_shutdown_request && !g_state.engine_on) {
            pthread_cond_wait(&cond_engine_run, &mtx_engine);
        }

        if (g_shutdown_request) {
            pthread_mutex_unlock(&mtx_engine);
            break;
        }

        int rpm_snap = g_state.rpm;

        pthread_mutex_lock(&mtx_motion);
        int speed_snap = g_state.speed;
        pthread_mutex_lock(&mtx_fuel);

        bool hit_empty = false;
        if (g_state.engine_on && g_state.fuel_gallons > (float)FUEL_MIN_GALLONS) {
            float consumption = BASE_CONSUMPTION
                + ((float)speed_snap * SPEED_FACTOR)
                + ((float)rpm_snap * RPM_FACTOR);

            g_state.fuel_gallons -= consumption;

            if (g_state.fuel_gallons <= (float)FUEL_MIN_GALLONS) {
                g_state.fuel_gallons = (float)FUEL_MIN_GALLONS;
                hit_empty = true;
            }
        }

        pthread_mutex_unlock(&mtx_fuel);
        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);

        if (hit_empty) {
            system_engine_kill();
            pthread_mutex_lock(&mtx_fuel);
            g_state.needs_refuel_to_start = true;
            pthread_mutex_unlock(&mtx_fuel);
        }

        sync_notify_ecu();

        usleep(FUEL_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
