/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 2
 * File: system_state.c
 * Date: 03/24/2026
 *
 * Description:
 * This file implements the shared system state for the motorcycle simulation.
 * It defines the global system state structure and initializes all starting
 * values used by the subsystems. These values include time, engine status,
 * RPM, temperature, speed, distance, fuel, signals, and display settings.
 * Phase II adds pthread mutexes and condition variables for safe sharing.
 */

#include "system_state.h"
#include <stdlib.h>
#include <stdio.h>

/* Global shared state accessed by all subsystems */
system_state_t g_state;

pthread_mutex_t mtx_engine;
pthread_mutex_t mtx_motion;
pthread_mutex_t mtx_fuel;
pthread_mutex_t mtx_ecu;

pthread_cond_t cond_engine_run;
pthread_cond_t cond_ecu;

void sync_notify_ecu(void) {
    pthread_cond_broadcast(&cond_ecu);
}

/*
 * Initializes the shared system state with default starting values.
 * This function is called once at the beginning of the program before
 * the subsystem threads start running.
 */
void system_state_init(void) {
    pthread_mutex_init(&mtx_engine, NULL);
    pthread_mutex_init(&mtx_motion, NULL);
    pthread_mutex_init(&mtx_fuel, NULL);
    pthread_mutex_init(&mtx_ecu, NULL);
    pthread_cond_init(&cond_engine_run, NULL);
    pthread_cond_init(&cond_ecu, NULL);

    /* Get the current system time */
    time_t now = time(NULL);

    //CRITICAL SECTION begin -- single-threaded init of shared g_state (time fields)
    g_state.program_start = now;

    /*
     * Start the overall timer at a random time in the past.
     * This makes the dashboard show a non-zero total elapsed time.
     */
    g_state.time_overall_start = now - (time_t)(rand() % (100 * 3600));

    /*
     * Start the total distance at a random value.
     * This makes the motorcycle appear to already have mileage on it.
     */
    g_state.total_distance = (double)(rand() % 500000) / 10.0;

    /* Initialize trip time to start now */
    g_state.time_trip_start = now;

    /* Set starting values for engine and movement */
    g_state.engine_on = true;
    g_state.rpm = 0;
    g_state.engine_temp_celsius = 85.0f;
    g_state.speed = 50;

    /* Set starting trip and fuel values */
    g_state.trip_distance = 0.0;
    g_state.fuel_gallons = 3.5f;

    /* Set starting indicator and light states */
    g_state.signal_state = SIGNAL_OFF;
    g_state.headlight_on = true;

    /* Use Celsius by default for temperature display, we can always add F if needed */
    g_state.use_celsius = true;

    /*
     * Initialize default derived states.
     * These values may later be updated by the ECU subsystem.
     */
    g_state.rpm_zone = RPM_ZONE_IDLE;
    g_state.temp_classification = TEMP_NORMAL;
    //CRITICAL SECTION end --
}
