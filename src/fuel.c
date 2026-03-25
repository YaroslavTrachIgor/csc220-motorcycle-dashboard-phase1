/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: fuel.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the fuel subsystem for the motorcycle simulation.
 * Its job is to model fuel consumption over time based on engine activity.
 * Fuel is consumed at idle, but higher speed and higher RPM increase the
 * amount of fuel used. The fuel level is kept within the valid range.
 */

#include "system_state.h"
#include <unistd.h>

/* Fuel system updates every 100 milliseconds */
#define FUEL_UPDATE_INTERVAL_MS 100

/* Base fuel consumption when the engine is idling */
#define BASE_CONSUMPTION        0.00001f

/* Extra fuel consumption based on motorcycle speed */
#define SPEED_FACTOR            0.00003f

/* Extra fuel consumption based on engine RPM */
#define RPM_FACTOR              0.0000002f


/*
 * Fuel thread:
 * This thread runs continuously in the background.
 * It decreases the fuel level over time while the engine is on.
 * More speed and more RPM cause more fuel to be consumed.
 */
void *fuel_thread(void *arg) {
    (void)arg;

    while (1) {
        /* Only consume fuel when the engine is on and fuel is available */
        if (g_state.engine_on && g_state.fuel_gallons > FUEL_MIN_GALLONS) {

            /*
             * Total fuel usage is made from:
             * 1. a base idle amount
             * 2. an extra amount based on speed
             * 3. an extra amount based on RPM
             */
            float consumption = BASE_CONSUMPTION
                + (g_state.speed * SPEED_FACTOR)
                + (g_state.rpm * RPM_FACTOR);

            /* Subtract the fuel used during this update cycle */
            g_state.fuel_gallons -= consumption;

            /* Prevent fuel from dropping below the minimum allowed value */
            if (g_state.fuel_gallons < FUEL_MIN_GALLONS) {
                g_state.fuel_gallons = FUEL_MIN_GALLONS;
            }
        }

        /* Wait before the next fuel update */
        usleep(FUEL_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
