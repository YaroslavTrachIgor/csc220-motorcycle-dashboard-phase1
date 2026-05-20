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

static rpm_zone_t classify_rpm(int rpm) {
    if (rpm < 100) {
        return RPM_ZONE_IDLE;
    } else if (rpm <= RPM_IDLE_MAX) {
        return RPM_ZONE_IDLE;
    } else if (rpm <= RPM_NORMAL_MAX) {
        return RPM_ZONE_NORMAL;
    } else if (rpm <= RPM_HIGH_MAX) {
        return RPM_ZONE_HIGH;
    } else {
        return RPM_ZONE_REDLINE;
    }
}

static temp_classification_t classify_temp(float temp) {
    if (temp < 60.0f) {
        return TEMP_COLD;
    } else if (temp <= 95.0f) {
        return TEMP_NORMAL;
    } else if (temp <= OVERHEAT_TEMP_THRESHOLD) {
        return TEMP_HOT;
    } else {
        return TEMP_OVERHEAT;
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
        pthread_mutex_lock(&mtx_motion);
        pthread_mutex_lock(&mtx_fuel);
        pthread_mutex_lock(&mtx_ecu);

        int rpm = g_state.rpm;
        float temp = g_state.engine_temp_celsius;
        bool engine_on = g_state.engine_on;
        int speed = g_state.speed;
        float fuel = g_state.fuel_gallons;

        /*
         * ECU RULE:
         * If the engine is off, user input cannot make the motorcycle move.
         */
        if (!engine_on) {
            g_state.speed = 0;
            g_state.rpm = 0;
            g_state.pending_accel_steps = 0;
            g_state.pending_decel_steps = 0;
            g_state.cruise_active = false;

            rpm = g_state.rpm;
            speed = g_state.speed;
        }

        /*
         * ECU RULE:
         * Refueling is not allowed while the motorcycle is moving.
         */
        if (speed > 0 && g_state.refueling_active) {
            g_state.refueling_active = false;
            g_state.refuel_deadline = 0;
        }

        /*
         * ECU RULE:
         * If fuel is empty or the system requires refuel before start,
         * the engine must stay off.
         */
        if (g_state.needs_refuel_to_start || fuel <= FUEL_MIN_GALLONS) {
            g_state.engine_on = false;
            g_state.speed = 0;
            g_state.rpm = 0;
            g_state.pending_accel_steps = 0;
            g_state.pending_decel_steps = 0;
            g_state.cruise_active = false;

            engine_on = g_state.engine_on;
            rpm = g_state.rpm;
            speed = g_state.speed;
        }

        /*
         * ECU SAFETY RULE:
         * Limit RPM and speed during overheating.
         */
        if (temp > OVERHEAT_TEMP_THRESHOLD) {
            if (g_state.rpm > OVERHEAT_RPM_LIMIT) {
                g_state.rpm = OVERHEAT_RPM_LIMIT;
            }

            if (g_state.speed > OVERHEAT_SPEED_LIMIT) {
                g_state.speed = OVERHEAT_SPEED_LIMIT;
            }

            rpm = g_state.rpm;
            speed = g_state.speed;
        }

        /*
         * ECU SAFETY RULE:
         * Limit speed when fuel is low.
         */
        if (fuel <= FUEL_LOW_THRESHOLD) {
            if (g_state.speed > LOW_FUEL_SPEED_LIMIT) {
                g_state.speed = LOW_FUEL_SPEED_LIMIT;
            }

            speed = g_state.speed;
        }

        /*
         * ECU IDLE RULE:
         * If the engine is on and the motorcycle is stopped,
         * keep RPM in the idle range.
         */
        if (engine_on && speed == 0) {
            if (g_state.rpm < RPM_IDLE_MIN) {
                g_state.rpm = RPM_IDLE_MIN;
            }

            if (g_state.rpm > RPM_IDLE_MAX) {
                g_state.rpm = RPM_IDLE_MAX;
            }

            rpm = g_state.rpm;
        }

        derive_signal_state_locked();

        g_state.rpm_zone = classify_rpm(rpm);
        g_state.temp_classification = classify_temp(temp);

        pthread_mutex_unlock(&mtx_ecu);
        pthread_mutex_unlock(&mtx_fuel);
        pthread_mutex_unlock(&mtx_motion);
        pthread_mutex_unlock(&mtx_engine);
    }

    return NULL;
}