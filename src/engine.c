/**
 * Phase III: RPM/temperature; engine_on changes only via ignition/kill/fuel (no auto toggle).
 */

#include "system_state.h"
#include <unistd.h>
#include <stdlib.h>

#define ENGINE_UPDATE_INTERVAL_MS 50
#define TEMP_COOLING_RATE         0.02f
#define TEMP_HEATING_RATE         0.05f
#define TEMP_IDLE_TARGET          75.0f
#define TEMP_AMBIENT              20.0f

void *engine_thread(void *arg) {
    (void)arg;

    int cycle = 0;

    sleep(1);

    while (!g_shutdown_request) {
        pthread_mutex_lock(&mtx_engine);

        if (g_state.engine_on) {
            pthread_mutex_lock(&mtx_motion);
            int spd = g_state.speed;
            pthread_mutex_unlock(&mtx_motion);

            if (spd == 0) {
                int idle_span = RPM_IDLE_MAX - RPM_IDLE_MIN + 1;
                g_state.rpm = RPM_IDLE_MIN + (cycle % idle_span);
            } else {
                int target_rpm = 1000 + spd * 85;
                if (target_rpm > RPM_MAX) {
                    target_rpm = RPM_MAX;
                }

                int variation = (rand() % 101) - 50;
                g_state.rpm = target_rpm + variation;

                if (g_state.rpm < RPM_MIN) {
                    g_state.rpm = RPM_MIN;
                }
                if (g_state.rpm > RPM_MAX) {
                    g_state.rpm = RPM_MAX;
                }
            }

            float temp_target = TEMP_IDLE_TARGET + (g_state.rpm / 200.0f);

            if (temp_target > 105.0f) {
                temp_target = 105.0f;
            }
            if (temp_target < TEMP_AMBIENT) {
                temp_target = TEMP_AMBIENT;
            }

            if (g_state.engine_temp_celsius < temp_target) {
                g_state.engine_temp_celsius += TEMP_HEATING_RATE;
                if (g_state.engine_temp_celsius > temp_target) {
                    g_state.engine_temp_celsius = temp_target;
                }
            } else {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE;
                if (g_state.engine_temp_celsius < temp_target) {
                    g_state.engine_temp_celsius = temp_target;
                }
            }
        } else {
            g_state.rpm = 0;

            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2.0f;
                if (g_state.engine_temp_celsius < TEMP_AMBIENT) {
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
                }
            }
        }

        pthread_mutex_unlock(&mtx_engine);

        sync_notify_ecu();

        cycle++;

        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
