/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard — Phase II
 * File: subsystems.h
 * Date: 03/24/2026 (Phase I); Phase II — 04/15/2026
 *
 * Description:
 * Declarations for pthread entry points. Implementations use mutex-protected
 * critical sections on `g_state`, condition variables for engine-on gating
 * (motion, fuel) and ECU wakeup, and documented lock order — see system_state.h.
 */

#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

/* Engine: RPM/temperature; owns engine_on transitions and cond_engine_run broadcast */
void *engine_thread(void *arg);

/* Motion: speed and odometer; waits on cond_engine_run until engine_on */
void *motion_thread(void *arg);

/* Fuel: consumption; waits on cond_engine_run; lock order engine->motion->fuel */
void *fuel_thread(void *arg);

/* ECU: classifications, signal demo, trip reset rule; waits on cond_ecu (timed) */
void *ecu_thread(void *arg);

/* Dashboard: snapshot under all subsystem locks, then render (no simulation) */
void *dashboard_thread(void *arg);

#endif /* SUBSYSTEMS_H */
