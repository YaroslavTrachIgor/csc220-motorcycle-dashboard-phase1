/**
 * Motorcycle Dashboard — Phase III: threads, keyboard input, graceful shutdown.
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
#include "input.h"
#include "log.h"

static void signal_handler(int sig) {
    (void)sig;
    g_shutdown_request = 1;
}

static void wake_all_blocked_threads(void) {
    pthread_cond_broadcast(&cond_engine_run);
    pthread_mutex_lock(&mtx_ecu);
    pthread_cond_broadcast(&cond_ecu);
    pthread_mutex_unlock(&mtx_ecu);
}

static void init_from_command_line(char *argv[]) {
    int rpm = atoi(argv[1]);
    int engine_state = atoi(argv[2]);
    int speed = atoi(argv[3]);
    int fuel_level = atoi(argv[4]);
    char accel_mode = argv[5][0];
    float accel_rate = DEFAULT_ACCEL_RATE;
    float decel_rate = DEFAULT_DECEL_RATE;

    if (argv[6] != NULL && argv[7] != NULL) {
        float ar = (float)strtod(argv[6], NULL);
        float dr = (float)strtod(argv[7], NULL);
        if (ar > 0.0f) {
            accel_rate = ar;
        }
        if (dr > 0.0f) {
            decel_rate = dr;
        }
    }

    system_state_init_from_args(rpm, engine_state, speed, fuel_level, accel_mode,
                                accel_rate, decel_rate);
}

static void init_system_state(int argc, char *argv[]) {
    if (argc >= 8) {
        init_from_command_line(argv);
    } else if (argc >= 6) {
        char *extended_argv[8];
        for (int i = 0; i < 6; i++) {
            extended_argv[i] = argv[i];
        }
        char buf_a[32];
        char buf_d[32];
        snprintf(buf_a, sizeof buf_a, "%f", (double)DEFAULT_ACCEL_RATE);
        snprintf(buf_d, sizeof buf_d, "%f", (double)DEFAULT_DECEL_RATE);
        extended_argv[6] = buf_a;
        extended_argv[7] = buf_d;
        init_from_command_line(extended_argv);
    } else {
        system_state_init();
    }
}

static void destroy_sync_primitives(void) {
    pthread_mutex_destroy(&mtx_engine);
    pthread_mutex_destroy(&mtx_motion);
    pthread_mutex_destroy(&mtx_fuel);
    pthread_mutex_destroy(&mtx_ecu);
    pthread_cond_destroy(&cond_engine_run);
    pthread_cond_destroy(&cond_ecu);
}

int main(int argc, char *argv[]) {
    pthread_t engine_tid, motion_tid, fuel_tid, ecu_tid, dashboard_tid, input_tid;

    setlocale(LC_CTYPE, "");
    srand((unsigned)time(NULL));

    init_system_state(argc, argv);

#ifdef ENABLE_LOG
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

    if (!input_terminal_setup()) {
        fprintf(stderr, "BAZOOKI OS: stdin is not a TTY — keyboard controls disabled; use Ctrl+C to exit.\n");
    }

    signal(SIGINT, signal_handler);

    if (pthread_create(&engine_tid, NULL, engine_thread, NULL) != 0) {
        perror("pthread_create engine");
        input_terminal_restore();
        return 1;
    }
    if (pthread_create(&motion_tid, NULL, motion_thread, NULL) != 0) {
        perror("pthread_create motion");
        g_shutdown_request = 1;
        wake_all_blocked_threads();
        pthread_join(engine_tid, NULL);
        input_terminal_restore();
        return 1;
    }
    if (pthread_create(&fuel_tid, NULL, fuel_thread, NULL) != 0) {
        perror("pthread_create fuel");
        g_shutdown_request = 1;
        wake_all_blocked_threads();
        pthread_join(motion_tid, NULL);
        pthread_join(engine_tid, NULL);
        input_terminal_restore();
        return 1;
    }
    if (pthread_create(&ecu_tid, NULL, ecu_thread, NULL) != 0) {
        perror("pthread_create ecu");
        g_shutdown_request = 1;
        wake_all_blocked_threads();
        pthread_join(fuel_tid, NULL);
        pthread_join(motion_tid, NULL);
        pthread_join(engine_tid, NULL);
        input_terminal_restore();
        return 1;
    }
    if (pthread_create(&dashboard_tid, NULL, dashboard_thread, NULL) != 0) {
        perror("pthread_create dashboard");
        g_shutdown_request = 1;
        wake_all_blocked_threads();
        pthread_join(ecu_tid, NULL);
        pthread_join(fuel_tid, NULL);
        pthread_join(motion_tid, NULL);
        pthread_join(engine_tid, NULL);
        input_terminal_restore();
        return 1;
    }
    if (pthread_create(&input_tid, NULL, input_thread, NULL) != 0) {
        perror("pthread_create input");
        g_shutdown_request = 1;
        wake_all_blocked_threads();
        pthread_join(dashboard_tid, NULL);
        pthread_join(ecu_tid, NULL);
        pthread_join(fuel_tid, NULL);
        pthread_join(motion_tid, NULL);
        pthread_join(engine_tid, NULL);
        input_terminal_restore();
        return 1;
    }

    while (!g_shutdown_request) {
        sleep(1);
    }

    wake_all_blocked_threads();

    pthread_join(input_tid, NULL);
    pthread_join(dashboard_tid, NULL);
    pthread_join(ecu_tid, NULL);
    pthread_join(fuel_tid, NULL);
    pthread_join(motion_tid, NULL);
    pthread_join(engine_tid, NULL);

    input_terminal_restore();

#ifdef ENABLE_LOG
    log_close();
#endif

    destroy_sync_primitives();

    return 0;
}
