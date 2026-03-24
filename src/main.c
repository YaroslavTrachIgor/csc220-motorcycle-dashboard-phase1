/**
 * main.c - BAZOOKI OS Entry Point
 * Launches all subsystem threads and runs the motorcycle dashboard.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "system_state.h"
#include "subsystems.h"

static volatile int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

int main(void) {
    pthread_t engine_tid, motion_tid, fuel_tid, ecu_tid, dashboard_tid;

    srand((unsigned)time(NULL));
    system_state_init();

    signal(SIGINT, signal_handler);

    /* Create subsystem threads */
    if (pthread_create(&engine_tid, NULL, engine_thread, NULL) != 0) {
        perror("pthread_create engine");
        return 1;
    }
    if (pthread_create(&motion_tid, NULL, motion_thread, NULL) != 0) {
        perror("pthread_create motion");
        return 1;
    }
    if (pthread_create(&fuel_tid, NULL, fuel_thread, NULL) != 0) {
        perror("pthread_create fuel");
        return 1;
    }
    if (pthread_create(&ecu_tid, NULL, ecu_thread, NULL) != 0) {
        perror("pthread_create ecu");
        return 1;
    }
    if (pthread_create(&dashboard_tid, NULL, dashboard_thread, NULL) != 0) {
        perror("pthread_create dashboard");
        return 1;
    }

    /* Run until interrupted */
    while (g_running) {
        sleep(1);
    }

    /* Note: In Phase 1 we don't implement graceful shutdown.
       Threads are not joined; process exits on Ctrl+C. */
    return 0;
}
