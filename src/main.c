/**
 * Names: Yaroslav Trach, Aiden Sheehy, Murat Yildiz
 * Course: CSC 220
 * Instructor: Dr. Kancharla
 * Project: Motorcycle Dashboard - Phase 1
 * File: main.c
 * Date: 03/24/2026
 *
 * Description:
 * This file is the main entry point for the BAZOOKI OS motorcycle simulation.
 * Its job is to initialize the shared system state, start all subsystem threads,
 * and keep the program running until the user interrupts it. It coordinates
 * the engine, motion, fuel, ECU, and dashboard subsystems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include "system_state.h"
#include "subsystems.h"
#include "log.h"

/* Global flag used to keep the program running */
static volatile int g_running = 1;


/*
 * Signal handler:
 * This function runs when the user presses Ctrl+C.
 * It changes the running flag so the main loop can stop.
 */
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}


int main(void) {
    pthread_t engine_tid, motion_tid, fuel_tid, ecu_tid, dashboard_tid;

    /* Enable proper display of special UTF-8 characters in the terminal */
    setlocale(LC_CTYPE, "");

    /* Seed the random number generator for engine RPM variation */
    srand((unsigned)time(NULL));

    /* Initialize the shared system state before threads start running */
    system_state_init();

#ifdef ENABLE_LOG
    /*
     * Logging goes to a file by default so lines are not erased by the
     * dashboard full-screen refresh. Use BAZOOKI_LOG_FILE=- for stderr.
     */
    {
        const char *path = getenv("BAZOOKI_LOG_FILE");
        if (path && strcmp(path, "-") == 0) {
            log_init(NULL);
        } else if (path && path[0] != '\0') {
            log_init(path);
            fprintf(stderr, "BAZOOKI OS: logging to %s (try: tail -f %s)\n", path, path);
        } else {
            log_init("bazooki_os.log");
            fprintf(stderr,
                "BAZOOKI OS: logging to bazooki_os.log (another terminal: tail -f bazooki_os.log)\n");
        }
    }
#endif

    /* Register Ctrl+C handler so the program can stop when interrupted */
    signal(SIGINT, signal_handler);

    /* Create subsystem threads */

    /* Engine thread handles RPM and temperature simulation */
    if (pthread_create(&engine_tid, NULL, engine_thread, NULL) != 0) {
        perror("pthread_create engine");
        return 1;
    }

    /* Motion thread handles speed and distance updates */
    if (pthread_create(&motion_tid, NULL, motion_thread, NULL) != 0) {
        perror("pthread_create motion");
        return 1;
    }

    /* Fuel thread handles fuel consumption over time */
    if (pthread_create(&fuel_tid, NULL, fuel_thread, NULL) != 0) {
        perror("pthread_create fuel");
        return 1;
    }

    /* ECU thread updates derived values such as RPM zone and temperature class */
    if (pthread_create(&ecu_tid, NULL, ecu_thread, NULL) != 0) {
        perror("pthread_create ecu");
        return 1;
    }

    /* Dashboard thread continuously prints the formatted dashboard */
    if (pthread_create(&dashboard_tid, NULL, dashboard_thread, NULL) != 0) {
        perror("pthread_create dashboard");
        return 1;
    }

    /* 
     * Main loop:
     * The program stays alive while all subsystem threads run in parallel.
     * It exits only when the user presses Ctrl+C.
     */
    while (g_running) {
        sleep(1);
    }

    /* 
     * In Phase 1, graceful shutdown is not implemented.
     * The threads are not joined. The process simply ends when interrupted.
     */
    return 0;
}
