/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: system_state.h
 * Date: 03/24/2026
 *
 * Description:
 * This header file defines the shared system state for the BAZOOKI OS
 * motorcycle simulation. It includes the enums, constants, structure,
 * and function declaration needed by all subsystems. These shared values
 * allow the engine, motion, fuel, ECU, and dashboard threads to access
 * the same motorcycle data during the simulation.
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <time.h>
#include <stdbool.h>
#include <pthread.h>

/*
 * Phase II — mutex lock order (always acquire at most one direction; never
 * lock mtx_ecu before mtx_engine, etc.):
 *   mtx_engine -> mtx_motion -> mtx_fuel -> mtx_ecu
 *
 * cond_engine_run + mtx_engine: motion/fuel wait until engine_on is true.
 * cond_ecu + mtx_ecu: ECU blocks on timedwait; producers call sync_notify_ecu().
 */

/*
 * RPM zone classifications.
 * These values are used by the ECU to describe the current RPM range.
 */
typedef enum {
    RPM_ZONE_IDLE,
    RPM_ZONE_NORMAL,
    RPM_ZONE_HIGH,
    RPM_ZONE_REDLINE
} rpm_zone_t;

/*
 * Engine temperature classifications.
 * These values describe whether the engine is cold, normal, hot, or overheating.
 */
typedef enum {
    TEMP_COLD,
    TEMP_NORMAL,
    TEMP_HOT,
    TEMP_OVERHEAT
} temp_classification_t;

/*
 * Turn signal states.
 * These values represent the different signal modes of the motorcycle.
 */
typedef enum {
    SIGNAL_OFF,
    SIGNAL_LEFT,
    SIGNAL_RIGHT,
    SIGNAL_HAZARD
} signal_state_t;


/* RPM limits used in the simulation */
#define RPM_MIN            0
#define RPM_MAX            16500
#define RPM_IDLE_MAX       1299
#define RPM_NORMAL_MAX     7999
#define RPM_HIGH_MAX       14499
#define RPM_REDLINE_MAX    16500

/* Speed limits in miles per hour */
#define SPEED_MIN          0
#define SPEED_MAX          200

/* Fuel limits in gallons */
#define FUEL_MIN_GALLONS   0.0
#define FUEL_MAX_GALLONS   4.7
#define FUEL_LOW_THRESHOLD 0.7

/* Temperature limits in Celsius */
#define TEMP_COLD_MAX_C    59
#define TEMP_NORMAL_MAX_C  95
#define TEMP_HOT_MAX_C     105

/* Temperature limits in Fahrenheit */
#define TEMP_COLD_MAX_F    139
#define TEMP_NORMAL_MAX_F  203
#define TEMP_HOT_MAX_F     221


/*
 * Shared system state structure.
 * This structure stores all important motorcycle values that are shared
 * across the different subsystems and threads in the program.
 */
typedef struct {
    /* Engine subsystem outputs */
    int rpm;
    float engine_temp_celsius;

    /* Motion subsystem outputs */
    int speed;
    double total_distance;
    double trip_distance;

    /* Fuel subsystem outputs */
    float fuel_gallons;

    /* ECU outputs and related state */
    bool engine_on;
    rpm_zone_t rpm_zone;
    temp_classification_t temp_classification;
    signal_state_t signal_state;
    bool headlight_on;

    /* Time tracking values */
    time_t time_overall_start;   /* Random offset at program start */
    time_t time_trip_start;      /* When engine was last turned on */
    time_t program_start;        /* When program started */

    /* User display preference */
    bool use_celsius;
} system_state_t;


/* Global shared state defined in system_state.c */
extern system_state_t g_state;

/* Subsystem mutexes: engine (rpm, temp, engine_on), motion (speed, distances),
 * fuel (fuel_gallons), ECU (rpm_zone, temp_classification, signal, headlight). */
extern pthread_mutex_t mtx_engine;
extern pthread_mutex_t mtx_motion;
extern pthread_mutex_t mtx_fuel;
extern pthread_mutex_t mtx_ecu;

extern pthread_cond_t cond_engine_run;
extern pthread_cond_t cond_ecu;

/* Wake ECU after producer threads change inputs ECU derives from. */
void sync_notify_ecu(void);

/* Initializes the shared system state at program startup */
void system_state_init(void);

#endif /* SYSTEM_STATE_H */
