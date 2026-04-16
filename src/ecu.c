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
 */

#include "system_state.h"
#include <time.h>

#define ECU_TIMEDWAIT_MS        50
#define SIGNAL_CYCLE_TICKS      (3000 / ECU_TIMEDWAIT_MS)

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
    //CRITICAL SECTION begin -- ECU blocks on cond_ecu (timed) instead of usleep polling
    (void)pthread_cond_timedwait(&cond_ecu, &mtx_ecu, &ts);
    //CRITICAL SECTION end --
    pthread_mutex_unlock(&mtx_ecu);
}

void *ecu_thread(void *arg) {
    (void)arg;

    while (1) {
        ecu_timedwait();

        pthread_mutex_lock(&mtx_engine);
        //CRITICAL SECTION begin -- ECU reads RPM/temp/engine_on; engine writes same fields
        int rpm = g_state.rpm;
        float temp = g_state.engine_temp_celsius;
        bool engine_on = g_state.engine_on;
        //CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_engine);

        pthread_mutex_lock(&mtx_motion);
        //CRITICAL SECTION begin -- ECU resets trip when engine off; motion updates trip
        if (!engine_on) {
            g_state.trip_distance = 0.0;
        }
        //CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_motion);

        pthread_mutex_lock(&mtx_ecu);
        //CRITICAL SECTION begin -- ECU writes derived state; dashboard reads
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
        } else if (temp <= 105.0f) {
            g_state.temp_classification = TEMP_HOT;
        } else {
            g_state.temp_classification = TEMP_OVERHEAT;
        }
        //CRITICAL SECTION end --
        pthread_mutex_unlock(&mtx_ecu);
    }

    return NULL;
}
