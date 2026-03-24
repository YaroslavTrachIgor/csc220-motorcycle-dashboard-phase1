/**
 * system_state.h - Shared system state for BAZOOKI OS
 * Defines all shared variables and enums used across subsystems.
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <time.h>
#include <stdbool.h>

/* RPM zone classifications */
typedef enum {
    RPM_ZONE_IDLE,
    RPM_ZONE_NORMAL,
    RPM_ZONE_HIGH,
    RPM_ZONE_REDLINE
} rpm_zone_t;

/* Engine temperature classifications */
typedef enum {
    TEMP_COLD,
    TEMP_NORMAL,
    TEMP_HOT,
    TEMP_OVERHEAT
} temp_classification_t;

/* Turn signal states */
typedef enum {
    SIGNAL_OFF,
    SIGNAL_LEFT,
    SIGNAL_RIGHT,
    SIGNAL_HAZARD
} signal_state_t;

/* Constants */
#define RPM_MIN            0
#define RPM_MAX            16500
#define RPM_IDLE_MAX       1299
#define RPM_NORMAL_MAX     7999
#define RPM_HIGH_MAX       14499
#define RPM_REDLINE_MAX    16500

#define SPEED_MIN          0
#define SPEED_MAX          200  /* MPH */

#define FUEL_MIN_GALLONS   0.0
#define FUEL_MAX_GALLONS   4.7
#define FUEL_LOW_THRESHOLD 0.7

#define TEMP_COLD_MAX_C    59
#define TEMP_NORMAL_MAX_C  95
#define TEMP_HOT_MAX_C     105

#define TEMP_COLD_MAX_F    139
#define TEMP_NORMAL_MAX_F  203
#define TEMP_HOT_MAX_F     221

/* Shared system state structure */
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

    /* ECU outputs (derived state) */
    bool engine_on;
    rpm_zone_t rpm_zone;
    temp_classification_t temp_classification;
    signal_state_t signal_state;
    bool headlight_on;

    /* Time tracking */
    time_t time_overall_start;   /* Random offset at program start */
    time_t time_trip_start;      /* When engine was last turned on */
    time_t program_start;        /* When program started */

    /* Display preference */
    bool use_celsius;
} system_state_t;

/* Global shared state - defined in system_state.c */
extern system_state_t g_state;

/* Initialize system state (call once at startup) */
void system_state_init(void);

#endif /* SYSTEM_STATE_H */
