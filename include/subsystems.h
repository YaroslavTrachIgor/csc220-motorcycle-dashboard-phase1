/**
 * subsystems.h - Subsystem thread declarations for BAZOOKI OS
 */

#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

/* Thread entry points - each runs in its own pthread */
void *engine_thread(void *arg);
void *motion_thread(void *arg);
void *fuel_thread(void *arg);
void *ecu_thread(void *arg);
void *dashboard_thread(void *arg);

#endif /* SUBSYSTEMS_H */
