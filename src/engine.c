/**
 * engine.c - Engine Subsystem
 * Simulates engine RPM and temperature. Updates shared state continuously.
 */

#include "system_state.h"
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

#define ENGINE_UPDATE_INTERVAL_MS 50
#define TEMP_COOLING_RATE         0.02f
#define TEMP_HEATING_RATE         0.05f
#define TEMP_IDLE_TARGET          75.0f
#define TEMP_AMBIENT             20.0f

void *engine_thread(void *arg) {
    (void)arg;
    int cycle = 0;

    while (1) {
        if (g_state.engine_on) {
            if (g_state.speed == 0) {
                /* Idle: fluctuate between 100 and 1299 RPM */
                int idle_base = 100 + (cycle % 1200);
                g_state.rpm = 100 + (idle_base % 1200);
            } else {
                /* Moving: RPM scales with speed, 1000 base + speed factor */
                int target_rpm = 1000 + g_state.speed * 85;
                if (target_rpm > RPM_MAX) target_rpm = RPM_MAX;
                /* Add slight variation for realism */
                int variation = (rand() % 101) - 50;
                g_state.rpm = target_rpm + variation;
                if (g_state.rpm < RPM_MIN) g_state.rpm = RPM_MIN;
                if (g_state.rpm > RPM_MAX) g_state.rpm = RPM_MAX;
            }

            /* Engine temperature: responds to RPM */
            float temp_target = TEMP_IDLE_TARGET + (g_state.rpm / 200.0f);
            if (temp_target > 105.0f) temp_target = 105.0f;
            if (temp_target < TEMP_AMBIENT) temp_target = TEMP_AMBIENT;

            if (g_state.engine_temp_celsius < temp_target) {
                g_state.engine_temp_celsius += TEMP_HEATING_RATE;
                if (g_state.engine_temp_celsius > temp_target)
                    g_state.engine_temp_celsius = temp_target;
            } else {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE;
                if (g_state.engine_temp_celsius < temp_target)
                    g_state.engine_temp_celsius = temp_target;
            }
        } else {
            g_state.rpm = 0;
            /* Cool down when engine off */
            if (g_state.engine_temp_celsius > TEMP_AMBIENT) {
                g_state.engine_temp_celsius -= TEMP_COOLING_RATE * 2;
                if (g_state.engine_temp_celsius < TEMP_AMBIENT)
                    g_state.engine_temp_celsius = TEMP_AMBIENT;
            }
        }

        cycle++;
        usleep(ENGINE_UPDATE_INTERVAL_MS * 1000);
    }
    return NULL;
}
