/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard — Phase II (synchronization)
 * File: system_state.h
 * Date: 03/24/2026 (Phase I); Phase II sync — 04/15/2026
 *
 * Description:
 * Central shared motorcycle state (`system_state_t` / `g_state`) for all
 * subsystems — same Phase I layout, extended with POSIX synchronization only.
 *
 * Mutex groups (no single global lock):
 *   mtx_engine — rpm, engine_temp_celsius, engine_on
 *   mtx_motion — speed, total_distance, trip_distance, accel/decel/cruise,
 *                pending W/S steps, timer pause flags / trip timer running
 *   mtx_fuel   — fuel_gallons, refueling_active, refuel_deadline, needs_refuel_to_start
 *   mtx_ecu    — rpm_zone, temp_classification, signal_state, headlight_on,
 *                signal_left_on, signal_right_on, hazard_on
 *
 * Lock acquisition order (every thread that takes more than one lock):
 *   mtx_engine -> mtx_motion -> mtx_fuel -> mtx_ecu
 * This strict order prevents circular wait and deadlocks when threads overlap
 * their critical sections (for example engine then motion, or dashboard snapshot).
 *
 * Thread coordination (not only mutual exclusion):
 *   cond_engine_run + mtx_engine — predicate is engine_on; motion and fuel
 *   block in pthread_cond_wait until the engine broadcasts after starting.
 *   cond_ecu + mtx_ecu — ECU uses pthread_cond_timedwait so it reacts to
 *   producer updates via sync_notify_ecu() without spinning on shared data.
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

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
#define RPM_IDLE_MIN       900
#define RPM_IDLE_MAX       1299
#define RPM_NORMAL_MAX     7999
#define RPM_HIGH_MAX       14499
#define RPM_REDLINE_MAX    16500

/* Speed limits in miles per hour */
#define SPEED_MIN          0
#define SPEED_MAX          200

/* Phase III W/S non-linear driver model defaults (dimensionless rates; scaled by dt) */
#define DEFAULT_ACCEL_RATE 0.35f
#define DEFAULT_DECEL_RATE 0.18f

#define REFUEL_DURATION_SEC 10

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
    float accel_rate;
    float decel_rate;
    bool cruise_active;
    /* Consumed each motion tick; incremented by input on W/S */
    int pending_accel_steps;
    int pending_decel_steps;

    /* Fuel subsystem outputs */
    float fuel_gallons;
    bool refueling_active;
    time_t refuel_deadline;
    bool needs_refuel_to_start;

    /* ECU outputs and related state */
    bool engine_on;
    rpm_zone_t rpm_zone;
    temp_classification_t temp_classification;
    signal_state_t signal_state;
    bool headlight_on;
    bool signal_left_on;
    bool signal_right_on;
    bool hazard_on;

    /* Time tracking values */
    time_t time_overall_start;   /* Anchor so (now - start) = overall elapsed while running */
    time_t time_trip_start;      /* Trip timer anchor while trip_timer_running */
    time_t program_start;        /* When program started */
    bool overall_timer_paused;
    long overall_elapsed_sec;    /* Frozen overall HH:MM:SS when paused after kill */
    bool trip_timer_running;

    /* User display preference */
    bool use_celsius;
} system_state_t;


/* Global shared state defined in system_state.c */
extern system_state_t g_state;

/* Phase III: cooperative shutdown (Q, SIGINT); threads exit loops when non-zero */
extern volatile sig_atomic_t g_shutdown_request;

/* Subsystem mutexes: engine (rpm, temp, engine_on), motion (speed, distances),
 * fuel (fuel_gallons), ECU (rpm_zone, temp_classification, signal, headlight). */
extern pthread_mutex_t mtx_engine;
extern pthread_mutex_t mtx_motion;
extern pthread_mutex_t mtx_fuel;
extern pthread_mutex_t mtx_ecu;

extern pthread_cond_t cond_engine_run;
extern pthread_cond_t cond_ecu;

/*
 * Wake ECU after engine / motion / fuel commit changes to RPM, temperature,
 * speed, fuel, or engine_on — ECU waits on cond_ecu instead of polling g_state.
 */
void sync_notify_ecu(void);

/* Initializes the shared system state at program startup */
void system_state_init(void);

/*
 * Initializes the shared system state using command-line arguments.
 * This is used in Phase II so the simulation can begin with user-defined
 * startup values instead of only hardcoded or randomized defaults.
 */
void system_state_init_from_args(int rpm, int engine_state, int speed, int fuel_level,
                                 char accel_mode, float accel_rate, float decel_rate);

/*
 * Kill switch semantics (engine was ON): engine off, speed 0, trip distance 0,
 * timers paused per Phase III; does not set needs_refuel_to_start.
 */
void system_engine_kill(void);

/*
 * Ignition: no-op if needs_refuel_to_start or refueling_active (caller may pre-check).
 * Resumes overall timer if paused; starts trip timer and clears cruise.
 */
void system_engine_ignite(void);

#endif /* SYSTEM_STATE_H */