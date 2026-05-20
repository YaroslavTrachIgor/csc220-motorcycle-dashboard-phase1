/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Project: Motorcycle Dashboard — Phase III (state + kill/ignite helpers)
 */

#include "system_state.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

system_state_t g_state;

volatile sig_atomic_t g_shutdown_request = 0;

pthread_mutex_t mtx_engine = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_motion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_fuel   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_ecu    = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_engine_run = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_ecu        = PTHREAD_COND_INITIALIZER;

void sync_notify_ecu(void) {
    pthread_mutex_lock(&mtx_ecu);
    pthread_cond_broadcast(&cond_ecu);
    pthread_mutex_unlock(&mtx_ecu);
}

void system_state_init(void) {
    time_t now = time(NULL);

    g_state.program_start = now;
    g_state.time_overall_start = now - (time_t)(rand() % (100 * 3600));
    g_state.total_distance = (double)(rand() % 500000) / 10.0;
    g_state.time_trip_start = now;

    g_state.engine_on = true;
    g_state.rpm = 0;
    g_state.engine_temp_celsius = 85.0f;
    g_state.speed = 0;

    g_state.trip_distance = 0.0;
    g_state.fuel_gallons = 3.5f;

    g_state.signal_state = SIGNAL_OFF;
    g_state.headlight_on = true;
    g_state.signal_left_on = false;
    g_state.signal_right_on = false;
    g_state.hazard_on = false;
    g_state.use_celsius = true;

    g_state.rpm_zone = RPM_ZONE_IDLE;
    g_state.temp_classification = TEMP_NORMAL;

    g_state.accel_rate = DEFAULT_ACCEL_RATE;
    g_state.decel_rate = DEFAULT_DECEL_RATE;
    g_state.cruise_active = false;
    g_state.pending_accel_steps = 0;
    g_state.pending_decel_steps = 0;

    g_state.refueling_active = false;
    g_state.refuel_deadline = 0;
    g_state.needs_refuel_to_start = false;

    g_state.overall_timer_paused = false;
    g_state.overall_elapsed_sec = 0;
    g_state.trip_timer_running = true;
}

void system_state_init_from_args(int rpm, int engine_state, int speed, int fuel_level,
                                 char accel_mode, float accel_rate, float decel_rate) {
    system_state_init();

    (void)accel_mode;

    pthread_mutex_lock(&mtx_engine);
    g_state.rpm = rpm;
    g_state.engine_on = (engine_state != 0);
    bool engine_started = g_state.engine_on;
    pthread_mutex_unlock(&mtx_engine);

    pthread_mutex_lock(&mtx_motion);
    g_state.speed = speed;

    if (accel_rate > 0.0f) {
        g_state.accel_rate = accel_rate;
    }

    if (decel_rate > 0.0f) {
        g_state.decel_rate = decel_rate;
    }

    g_state.trip_timer_running = engine_started;

    if (!engine_started) {
        g_state.overall_timer_paused = true;
        g_state.overall_elapsed_sec = (long)(time(NULL) - g_state.time_overall_start);

        if (g_state.overall_elapsed_sec < 0) {
            g_state.overall_elapsed_sec = 0;
        }
    }

    pthread_mutex_unlock(&mtx_motion);

    pthread_mutex_lock(&mtx_fuel);
    g_state.fuel_gallons = ((float)fuel_level / 100.0f) * (float)FUEL_MAX_GALLONS;

    if (g_state.fuel_gallons < (float)FUEL_MIN_GALLONS) {
        g_state.fuel_gallons = (float)FUEL_MIN_GALLONS;
    }

    if (g_state.fuel_gallons > (float)FUEL_MAX_GALLONS) {
        g_state.fuel_gallons = (float)FUEL_MAX_GALLONS;
    }

    if (g_state.fuel_gallons <= (float)FUEL_MIN_GALLONS) {
        g_state.needs_refuel_to_start = true;
    }

    pthread_mutex_unlock(&mtx_fuel);

    if (engine_started) {
        pthread_cond_broadcast(&cond_engine_run);
    }

    sync_notify_ecu();
}

void system_engine_kill(void) {
    time_t now = time(NULL);

    pthread_mutex_lock(&mtx_engine);

    if (!g_state.engine_on) {
        pthread_mutex_unlock(&mtx_engine);
        return;
    }

    g_state.engine_on = false;
    g_state.rpm = 0;

    pthread_mutex_unlock(&mtx_engine);

    pthread_mutex_lock(&mtx_motion);

    g_state.speed = 0;
    g_state.trip_distance = 0.0;
    g_state.overall_elapsed_sec = (long)(now - g_state.time_overall_start);

    if (g_state.overall_elapsed_sec < 0) {
        g_state.overall_elapsed_sec = 0;
    }

    g_state.overall_timer_paused = true;
    g_state.trip_timer_running = false;
    g_state.cruise_active = false;
    g_state.pending_accel_steps = 0;
    g_state.pending_decel_steps = 0;

    pthread_mutex_unlock(&mtx_motion);

    sync_notify_ecu();
}

void system_engine_ignite(void) {
    pthread_mutex_lock(&mtx_engine);
    pthread_mutex_lock(&mtx_fuel);

    bool no_fuel = (g_state.fuel_gallons <= (float)FUEL_MIN_GALLONS);

    if (g_state.engine_on ||
        g_state.needs_refuel_to_start ||
        g_state.refueling_active ||
        no_fuel) {
        pthread_mutex_unlock(&mtx_fuel);
        pthread_mutex_unlock(&mtx_engine);
        sync_notify_ecu();
        return;
    }

    g_state.engine_on = true;

    pthread_mutex_unlock(&mtx_fuel);

    pthread_cond_broadcast(&cond_engine_run);

    pthread_mutex_unlock(&mtx_engine);

    pthread_mutex_lock(&mtx_motion);

    if (g_state.overall_timer_paused) {
        time_t now = time(NULL);
        g_state.time_overall_start = now - g_state.overall_elapsed_sec;
        g_state.overall_timer_paused = false;
    }

    g_state.trip_timer_running = true;
    g_state.time_trip_start = time(NULL);
    g_state.cruise_active = false;
    g_state.pending_accel_steps = 0;
    g_state.pending_decel_steps = 0;

    pthread_mutex_unlock(&mtx_motion);

    sync_notify_ecu();
}