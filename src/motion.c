/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: motion.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the motion subsystem for the motorcycle simulation.
 * Its job is to simulate motorcycle speed and distance over time. When the
 * engine is on, the speed cycles between 50 and 70 mph, and distance is
 * updated based on the current speed. When the engine is off, speed is set
 * to zero.
 */

#include "system_state.h"
#include <unistd.h>

/* Motion updates every 1000 milliseconds, which is 1 second */
#define MOTION_UPDATE_INTERVAL_MS 1000

/* Conversion from miles per hour to miles per second */
#define MPH_TO_MPS (1.0 / 3600.0)


/*
 * Motion thread:
 * This thread runs continuously in the background.
 * It updates motorcycle speed and distance while the engine is on.
 */
void *motion_thread(void *arg) {
    (void)arg;

    /* 1 means speeding up, -1 means slowing down */
    int speed_direction = 1;

    while (1) {
        if (g_state.engine_on) {
            /*
             * Simulate speed cycling between 50 and 70 mph.
             * The motorcycle accelerates up to 70, then slows
             * back down to 50, and repeats.
             */
            g_state.speed += speed_direction;

            if (g_state.speed >= 70) {
                g_state.speed = 70;
                speed_direction = -1;
            } else if (g_state.speed <= 50) {
                g_state.speed = 50;
                speed_direction = 1;
            }

            /*
             * Update distance traveled.
             * Since speed is in miles per hour and this updates once
             * every second, convert mph to miles per second first.
             */
            double delta_miles = g_state.speed * MPH_TO_MPS;
            g_state.total_distance += delta_miles;
            g_state.trip_distance += delta_miles;
        } else {
            /*
             * If the engine is off, the motorcycle is not moving,
             * so speed becomes 0.
             */
            g_state.speed = 0;

            /* Trip distance reset is handled in another subsystem */
        }

        /* Wait before the next motion update */
        usleep(MOTION_UPDATE_INTERVAL_MS * 1000);
    }

    return NULL;
}
