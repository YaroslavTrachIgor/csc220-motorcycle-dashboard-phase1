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

/* Engine: RPM/temperature; engine_on from ignition/kill/refuel (Phase III) */
void *engine_thread(void *arg);

/* Motion: speed/distance; Phase III W/S/C via pending steps */
void *motion_thread(void *arg);

/* Fuel: consumption; refuel timer; empty-tank kill */
void *fuel_thread(void *arg);

/* ECU: classifications, driver signals (A/D/Z), rule enforcement */
void *ecu_thread(void *arg);

/* Dashboard: snapshot under all subsystem locks (Phase III timer display) */
void *dashboard_thread(void *arg);

#endif /* SUBSYSTEMS_H */
