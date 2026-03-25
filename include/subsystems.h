/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: subsystems.h
 * Date: 03/24/2026
 *
 * Description:
 * This header file contains the function declarations for all subsystem
 * threads used in the BAZOOKI OS motorcycle simulation. Each subsystem
 * runs in its own pthread and is responsible for a specific part of the
 * simulation, such as engine behavior, motion, fuel usage, ECU updates,
 * and dashboard display.
 */

#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

/*
 * Thread entry point declarations.
 * Each of these functions is designed to run in its own pthread
 * and continuously update or display part of the system.
 */

/* Simulates engine RPM and engine temperature */
void *engine_thread(void *arg);

/* Simulates motorcycle speed and distance */
void *motion_thread(void *arg);

/* Simulates fuel consumption over time */
void *fuel_thread(void *arg);

/* Updates derived system values such as RPM zone and temperature class */
void *ecu_thread(void *arg);

/* Displays the formatted dashboard output in the terminal */
void *dashboard_thread(void *arg);

#endif /* SUBSYSTEMS_H */
