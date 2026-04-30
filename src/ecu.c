/**
 * Motorcycle Dashboard — Phase III ECU: classifications, driver signals, rules.
 */

#include "system_state.h"
#include <time.h>

#define ECU_TIMEDWAIT_MS        50

#define OVERHEAT_TEMP_THRESHOLD 105.0f
#define OVERHEAT_RPM_LIMIT      8000
#define OVERHEAT_SPEED_LIMIT    65
#define LOW_FUEL_SPEED_LIMIT    45

static void ecu_timedwait(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long add_ns = (long)ECU_TIMEDWAIT_MS * 1000000L;
    ts.tv_nsec += add_ns;
    while (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    }

    pthread_mutex_lock(&mtx_ecu);
    (void)pthread_cond_timedwait(&cond_ecu, &mtx_ecu, &ts);
    pthread_mutex_unlock(&mtx_ecu);
}

static void derive_signal_state_locked(void) {
    if (g_state.hazard_on) {
        g_state.signal_state = SIGNAL_HAZARD;
    } else if (g_state.signal_left_on && g_state.signal_right_on) {
        g_state.signal_state = SIGNAL_HAZARD;
    } else if (g_state.signal_left_on) {
        g_state.signal_state = SIGNAL_LEFT;
    } else if (g_state.signal_right_on) {
        g_state.signal_state = SIGNAL_RIGHT;
    } else {
        g_state.signal_state = SIGNAL_OFF;
    }
}

void *ecu_thread(void *arg) {
    (void)arg;

    while (!g_shutdown_request) {
        ecu_timedwait();
        if (g_shutdown_request) {
            break;
        }

        pthread_mutex_lock(&mtx_engine);
        int rpm = g_state.rpm;
        float temp = g_state.engine_temp_celsius;
        bool engine_on = g_state.engine_on;
        pthread_mutex_unlock(&mtx_engine);

        pthread_mutex_lock(&mtx_motion);
        int speed = g_state.speed;
        pthread_mutex_unlock(&mtx_motion);

        pthread_mutex_lock(&mtx_fuel);
        float fuel = g_state.fuel_gallons;
        pthread_mutex_unlock(&mtx_fuel);

        pthread_mutex_lock(&mtx_ecu);
        derive_signal_state_locked();

        if (rpm < 100) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_IDLE_MAX) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_NORMAL_MAX) {
            g_state.rpm_zone = RPM_ZONE_NORMAL;
        } else if (rpm <= RPM_HIGH_MAX) {
            g_state.rpm_zone = RPM_ZONE_HIGH;
        } else {
            g_state.rpm_zone = RPM_ZONE_REDLINE;
        }

        if (temp < 60.0f) {
            g_state.temp_classification = TEMP_COLD;
        } else if (temp <= 95.0f) {
            g_state.temp_classification = TEMP_NORMAL;
        } else if (temp <= OVERHEAT_TEMP_THRESHOLD) {
            g_state.temp_classification = TEMP_HOT;
        } else {
            g_state.temp_classification = TEMP_OVERHEAT;
        }
        pthread_mutex_unlock(&mtx_ecu);

        if (temp > OVERHEAT_TEMP_THRESHOLD) {
            pthread_mutex_lock(&mtx_engine);
            if (g_state.rpm > OVERHEAT_RPM_LIMIT) {
                g_state.rpm = OVERHEAT_RPM_LIMIT;
            }
            pthread_mutex_unlock(&mtx_engine);

            pthread_mutex_lock(&mtx_motion);
            if (g_state.speed > OVERHEAT_SPEED_LIMIT) {
                g_state.speed = OVERHEAT_SPEED_LIMIT;
            }
            pthread_mutex_unlock(&mtx_motion);
        }

        if (fuel <= FUEL_LOW_THRESHOLD) {
            pthread_mutex_lock(&mtx_motion);
            if (g_state.speed > LOW_FUEL_SPEED_LIMIT) {
                g_state.speed = LOW_FUEL_SPEED_LIMIT;
            }
            pthread_mutex_unlock(&mtx_motion);
        }

        if (engine_on && speed == 0) {
            pthread_mutex_lock(&mtx_engine);
            if (g_state.rpm < RPM_IDLE_MIN) {
                g_state.rpm = RPM_IDLE_MIN;
            }
            if (g_state.rpm > RPM_IDLE_MAX) {
                g_state.rpm = RPM_IDLE_MAX;
            }
            pthread_mutex_unlock(&mtx_engine);
        }

        pthread_mutex_lock(&mtx_engine);
        rpm = g_state.rpm;
        pthread_mutex_unlock(&mtx_engine);

        pthread_mutex_lock(&mtx_ecu);
        if (rpm < 100) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_IDLE_MAX) {
            g_state.rpm_zone = RPM_ZONE_IDLE;
        } else if (rpm <= RPM_NORMAL_MAX) {
            g_state.rpm_zone = RPM_ZONE_NORMAL;
        } else if (rpm <= RPM_HIGH_MAX) {
            g_state.rpm_zone = RPM_ZONE_HIGH;
        } else {
            g_state.rpm_zone = RPM_ZONE_REDLINE;
        }
        pthread_mutex_unlock(&mtx_ecu);
    }

    return NULL;
}
