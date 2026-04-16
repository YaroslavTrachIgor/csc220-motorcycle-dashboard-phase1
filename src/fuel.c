/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: fuel.c
 *
 * Description:
 * Fuel consumption. Phase II: pthread_cond_wait until engine is ON; consumption
 * uses mutex order engine -> motion -> fuel.
 */

#include "system_state.h"
#include <unistd.h>

#define FUEL_UPDATE_INTERVAL_MS 100
#define BASE_CONSUMPTION        0.00001f
#define SPEED_FACTOR            0.00003f
#define RPM_FACTOR              0.0000002f

void *fuel_thread(void *arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&mtx_engine);
        //CRITICAL SECTION begin -- wait engine ON (pthread_cond_t); read RPM under mtx_engine
        while (!g_state.engine_on) {
            pthread_cond_wait(&cond_engine_run, &mtx_engine);
        }

        int rpm_snap = g_state.rpm;
        pthread_mutex_lock(&mtx_motion);
        int speed_snap = g_state.speed;
        pthread_mutex_lock(&mtx_fuel);

        if (g_state.engine_on && g_state.fuel_gallons > (float)FUEL_MIN_GALLONS) {
            float consumption = BASE_CONSUMPTION
                + ((float)speed_snap * SPEED_FACTOR)
                + ((float)rpm_snap * RPM_FACTOR);

            //CRITICAL SECTION begin -- fuel_gallons update; dashboard reads fuel
            g_state.fuel_gallons -= consumption;

            if (g_state.fuel_gallons < (float)FUEL_MIN_GALLONS) {
                g_state.fuel_gallons = (float)FUEL_MIN_GALLONS;
            }
            //CRITICAL SECTION end --
        }

        pthread_mutex_unlock(&mtx_fuel);
        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);
        //CRITICAL SECTION end --

        sync_notify_ecu();

        usleep(FUEL_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
