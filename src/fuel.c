/**
 * fuel.c - Fuel Subsystem
 * Models fuel consumption. Higher speed/RPM = higher consumption.
 * Idle = lowest consumption. Range: 0.0 - 4.7 gallons.
 */

#include "system_state.h"
#include <unistd.h>

#define FUEL_UPDATE_INTERVAL_MS 100
#define BASE_CONSUMPTION        0.00001f   /* per tick at idle */
#define SPEED_FACTOR            0.00003f   /* consumption per mph */
#define RPM_FACTOR              0.0000002f /* consumption per rpm */

void *fuel_thread(void *arg) {
    (void)arg;

    while (1) {
        if (g_state.engine_on && g_state.fuel_gallons > FUEL_MIN_GALLONS) {
            /* Base consumption (idle) + speed factor + RPM factor */
            float consumption = BASE_CONSUMPTION
                + (g_state.speed * SPEED_FACTOR)
                + (g_state.rpm * RPM_FACTOR);

            g_state.fuel_gallons -= consumption;
            if (g_state.fuel_gallons < FUEL_MIN_GALLONS)
                g_state.fuel_gallons = FUEL_MIN_GALLONS;
        }

        usleep(FUEL_UPDATE_INTERVAL_MS * 1000);
    }
    return NULL;
}
