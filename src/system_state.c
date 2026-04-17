#include "system_state.h"
#include <pthread.h>
#include <time.h>

/* ===== GLOBAL STATE DEFINITIONS ===== */

system_state_t g_state;

/* Mutexes */
pthread_mutex_t mtx_engine = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_motion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_fuel   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_ecu    = PTHREAD_MUTEX_INITIALIZER;

/* Condition variables */
pthread_cond_t cond_engine_run = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_ecu        = PTHREAD_COND_INITIALIZER;

/*
 * Wakes the ECU thread after producer threads update values that the ECU
 * depends on. This allows the ECU to immediately re-evaluate system rules.
 */
void sync_notify_ecu(void) {
    pthread_mutex_lock(&mtx_ecu);
    pthread_cond_signal(&cond_ecu);
    pthread_mutex_unlock(&mtx_ecu);
}

/*
 * Initializes the shared system state at program startup.
 * This sets the default values used by the motorcycle simulation before
 * any subsystem threads begin running.
 */
void system_state_init(void) {
    g_state.rpm = 0;
    g_state.engine_temp_celsius = 25.0f;

    g_state.speed = 0;
    g_state.total_distance = 0.0;
    g_state.trip_distance = 0.0;

    g_state.fuel_gallons = FUEL_MAX_GALLONS;

    g_state.engine_on = false;
    g_state.rpm_zone = RPM_ZONE_IDLE;
    g_state.temp_classification = TEMP_COLD;
    g_state.signal_state = SIGNAL_OFF;
    g_state.headlight_on = false;

    g_state.time_overall_start = time(NULL);
    g_state.time_trip_start = time(NULL);
    g_state.program_start = time(NULL);

    g_state.use_celsius = true;
}

/*
 * Initializes the shared system state using command-line arguments.
 * This Phase II helper first applies the normal default initialization,
 * then overrides the startup values with the user-provided values.
 * This keeps the logic isolated so it can be removed in Phase III.
 */
void system_state_init_from_args(int rpm, int engine_state, int speed, int fuel_level, char accel_mode) {
    /* Start with the default initialization */
    system_state_init();

    /* Override engine-related startup values */
    pthread_mutex_lock(&mtx_engine);
    g_state.rpm = rpm;
    g_state.engine_on = (engine_state != 0);
    pthread_mutex_unlock(&mtx_engine);

    /* Override motion-related startup values */
    pthread_mutex_lock(&mtx_motion);
    g_state.speed = speed;
    pthread_mutex_unlock(&mtx_motion);

    /* Override fuel-related startup values
     * If your assignment uses percentage in Phase II, convert it to gallons.
     */
    pthread_mutex_lock(&mtx_fuel);
    g_state.fuel_gallons = ((float)fuel_level / 100.0f) * FUEL_MAX_GALLONS;
    pthread_mutex_unlock(&mtx_fuel);

    /* Notify waiting subsystems if the engine starts ON */
    if (g_state.engine_on) {
        pthread_cond_broadcast(&cond_engine_run);
    }

    /* Wake ECU so it can immediately evaluate rules from the initialized state */
    sync_notify_ecu();

    /* The accel/decel mode is accepted from the command line for Phase II.
     * Store it in shared state if your design includes a field for it.
     * If no shared field exists yet, this argument can still be used later
     * by the motion subsystem after you add support for it there.
     */
    (void)accel_mode;
}