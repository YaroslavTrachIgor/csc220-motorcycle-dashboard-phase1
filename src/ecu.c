/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: ecu.c
 *
 * Description:
 * ECU derives RPM zone, temperature class, and signal state. Phase II uses
 * pthread_cond_timedwait on cond_ecu instead of busy polling; producers call
 * sync_notify_ecu().
 *
 * Phase II ECU rules enforced here:
 * 1. Engine OFF behavior
 * 2. Overheat protection
 * 3. Low fuel constraint
 * 4. Idle state enforcement
 */

#include "system_state.h"
#include <time.h>

#define ECU_TIMEDWAIT_MS        50
#define SIGNAL_CYCLE_TICKS      (3000 / ECU_TIMEDWAIT_MS)

/* Phase II rule values */
#define ENGINE_OFF_DECEL_STEP   2
#define OVERHEAT_TEMP_THRESHOLD 105.0f
#define OVERHEAT_RPM_LIMIT      8000
#define OVERHEAT_SPEED_LIMIT    65
#define LOW_FUEL_SPEED_LIMIT    45

static int signal_cycle_counter = 0;

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
    // CRITICAL SECTION begin -- ECU blocks on cond_ecu (timed) instead of busy polling
    (void)pthread_cond_timedwait(&cond_ecu, &mtx_ecu, &ts);
    // CRITICAL SECTION end --
    pthread_mutex_unlock(&mtx_ecu);
}

void *ecu_thread(void *arg) {
    (void)arg;

    while (1) {
        ecu_timedwait();

        /* Read engine-owned values */
        pthread_mutex_lock(&mtx_engine);
        // CRITICAL SECTION begin -- ECU reads RPM/temp/engine_on; engine writes same fields
        int rpm = g_state.rpm;
        float temp = g_state.engine_temp_celsius;
        bool engine_on = g_state.engine_on;
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_engine);

        /* Read motion-owned values */
        pthread_mutex_lock(&mtx_motion);
        // CRITICAL SECTION begin -- ECU reads speed; motion writes speed/distance
        int speed = g_state.speed;
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_motion);

        /* Read fuel-owned value */
        pthread_mutex_lock(&mtx_fuel);
        // CRITICAL SECTION begin -- ECU reads fuel level; fuel thread writes fuel
        float fuel = g_state.fuel_gallons;
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_fuel);

        pthread_mutex_lock(&mtx_ecu);
        // CRITICAL SECTION begin -- ECU writes derived signal/rpm/temp classifications; dashboard reads
        signal_cycle_counter++;

        if (signal_cycle_counter >= SIGNAL_CYCLE_TICKS) {
            signal_cycle_counter = 0;

            if (g_state.signal_state == SIGNAL_OFF) {
                g_state.signal_state = SIGNAL_LEFT;
            } else if (g_state.signal_state == SIGNAL_LEFT) {
                g_state.signal_state = SIGNAL_RIGHT;
            } else if (g_state.signal_state == SIGNAL_RIGHT) {
                g_state.signal_state = SIGNAL_HAZARD;
            } else {
                g_state.signal_state = SIGNAL_OFF;
            }
        }

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
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_ecu);

        /*
         * Rule 4.1 - Engine OFF Behavior
         * If engine is OFF:
         * - RPM must be 0
         * - Speed must gradually return to 0
         * - Distance should keep recording until speed reaches 0
         *
         * Note: ECU must NOT reset trip_distance here.
         */
        if (!engine_on) {
            pthread_mutex_lock(&mtx_engine);
            // CRITICAL SECTION begin -- ECU forces RPM to 0 when engine is OFF
            g_state.rpm = 0;
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_engine);

            pthread_mutex_lock(&mtx_motion);
            // CRITICAL SECTION begin -- ECU steadily reduces speed to 0 when engine is OFF
            if (g_state.speed > 0) {
                g_state.speed -= ENGINE_OFF_DECEL_STEP;
                if (g_state.speed < 0) {
                    g_state.speed = 0;
                }
            }
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_motion);
        }

        /*
         * Rule 4.2 - Overheat Protection
         * If temperature exceeds threshold:
         * - RPM must be capped
         * - Speed must be capped
         * - Dashboard can indicate overheat using temp_classification
         */
        if (temp > OVERHEAT_TEMP_THRESHOLD) {
            pthread_mutex_lock(&mtx_engine);
            // CRITICAL SECTION begin -- ECU limits RPM during overheat
            if (g_state.rpm > OVERHEAT_RPM_LIMIT) {
                g_state.rpm = OVERHEAT_RPM_LIMIT;
            }
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_engine);

            pthread_mutex_lock(&mtx_motion);
            // CRITICAL SECTION begin -- ECU limits speed during overheat
            if (g_state.speed > OVERHEAT_SPEED_LIMIT) {
                g_state.speed = OVERHEAT_SPEED_LIMIT;
            }
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_motion);
        }

        /*
         * Rule 4.3 - Low Fuel Constraint
         * If fuel level drops below threshold:
         * - ECU must limit maximum speed
         * - Dashboard can display low fuel based on fuel threshold
         */
        if (fuel <= FUEL_LOW_THRESHOLD) {
            pthread_mutex_lock(&mtx_motion);
            // CRITICAL SECTION begin -- ECU limits speed under low fuel
            if (g_state.speed > LOW_FUEL_SPEED_LIMIT) {
                g_state.speed = LOW_FUEL_SPEED_LIMIT;
            }
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_motion);
        }

        /*
         * Rule 4.4 - Idle State Enforcement
         * If engine is ON and speed is 0:
         * - RPM must remain within the idle range
         */
        if (engine_on && speed == 0) {
            pthread_mutex_lock(&mtx_engine);
            // CRITICAL SECTION begin -- ECU keeps RPM in idle range while vehicle is stopped
            if (g_state.rpm < RPM_MIN) {
                g_state.rpm = RPM_MIN;
            }
            if (g_state.rpm > RPM_IDLE_MAX) {
                g_state.rpm = RPM_IDLE_MAX;
            }
            // CRITICAL SECTION end --
            pthread_mutex_unlock(&mtx_engine);
        }

        /* Recompute RPM zone after rule enforcement in case ECU changed RPM */
        pthread_mutex_lock(&mtx_engine);
        // CRITICAL SECTION begin -- ECU re-reads RPM after enforcement
        rpm = g_state.rpm;
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_engine);

        pthread_mutex_lock(&mtx_ecu);
        // CRITICAL SECTION begin -- ECU updates derived RPM zone after enforcement
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
        // CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_ecu);
    }

    return NULL;
}
