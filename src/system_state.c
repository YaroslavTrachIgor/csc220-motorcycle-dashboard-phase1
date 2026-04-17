/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard — Phase II
 * File: system_state.c
 * Date: 03/24/2026 (Phase I); Phase II — 04/15/2026
 *
 * Description:
 * Defines `g_state` and subsystem synchronization. Mutexes and condition
 * variables use static initializers (see Phase II aiden branch). system_state_init()
 * sets simulation defaults before any thread runs; system_state_init_from_args()
 * applies defaults then overrides from the command line (main passes six args).
 */

#include "system_state.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

system_state_t g_state;

pthread_mutex_t mtx_engine = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_motion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_fuel   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_ecu    = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_engine_run = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_ecu        = PTHREAD_COND_INITIALIZER;

/*
 * ECU blocks on cond_ecu (timed wait). Hold mtx_ecu around signal/broadcast so
 * the wakeup is well-defined with respect to ECU waiters (Aiden Phase II).
 */
void sync_notify_ecu(void) {
    pthread_mutex_lock(&mtx_ecu);
    pthread_cond_broadcast(&cond_ecu);
    pthread_mutex_unlock(&mtx_ecu);
}

void system_state_init(void) {
    time_t now = time(NULL);

    //CRITICAL SECTION begin -- single-threaded init before pthread_create
    g_state.program_start = now;
    g_state.time_overall_start = now - (time_t)(rand() % (100 * 3600));
    g_state.total_distance = (double)(rand() % 500000) / 10.0;
    g_state.time_trip_start = now;

    g_state.engine_on = true;
    g_state.rpm = 0;
    g_state.engine_temp_celsius = 85.0f;
    g_state.speed = 50;

    g_state.trip_distance = 0.0;
    g_state.fuel_gallons = 3.5f;

    g_state.signal_state = SIGNAL_OFF;
    g_state.headlight_on = true;
    g_state.use_celsius = true;

    g_state.rpm_zone = RPM_ZONE_IDLE;
    g_state.temp_classification = TEMP_NORMAL;
    //CRITICAL SECTION end --
}

/*
 * Phase II: default init then apply CLI overrides (rpm, engine on/off, speed,
 * fuel %, accel mode char). Isolated for easy removal in Phase III.
 */
void system_state_init_from_args(int rpm, int engine_state, int speed, int fuel_level, char accel_mode) {
    system_state_init();

    pthread_mutex_lock(&mtx_engine);
    //CRITICAL SECTION begin -- CLI overrides engine fields
    g_state.rpm = rpm;
    g_state.engine_on = (engine_state != 0);
    bool engine_started = g_state.engine_on;
    //CRITICAL SECTION end --
    pthread_mutex_unlock(&mtx_engine);

    pthread_mutex_lock(&mtx_motion);
    //CRITICAL SECTION begin -- CLI overrides motion fields
    g_state.speed = speed;
    //CRITICAL SECTION end --
    pthread_mutex_unlock(&mtx_motion);

    pthread_mutex_lock(&mtx_fuel);
    //CRITICAL SECTION begin -- CLI fuel is percentage of tank capacity
    g_state.fuel_gallons = ((float)fuel_level / 100.0f) * (float)FUEL_MAX_GALLONS;
    if (g_state.fuel_gallons < (float)FUEL_MIN_GALLONS) {
        g_state.fuel_gallons = (float)FUEL_MIN_GALLONS;
    }
    if (g_state.fuel_gallons > (float)FUEL_MAX_GALLONS) {
        g_state.fuel_gallons = (float)FUEL_MAX_GALLONS;
    }
    //CRITICAL SECTION end --
    pthread_mutex_unlock(&mtx_fuel);

    if (engine_started) {
        pthread_cond_broadcast(&cond_engine_run);
    }

    sync_notify_ecu();

    (void)accel_mode;
}
