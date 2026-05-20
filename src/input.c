/**
 * Phase III: dedicated input thread — termios + select/read, no echo.
 */

#define _POSIX_C_SOURCE 200809L

#include "input.h"
#include "system_state.h"
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static struct termios g_termios_saved;
static bool g_termios_saved_valid;

bool input_terminal_setup(void) {
    if (!isatty(STDIN_FILENO)) {
        return false;
    }
    if (tcgetattr(STDIN_FILENO, &g_termios_saved) != 0) {
        return false;
    }
    g_termios_saved_valid = true;

    struct termios raw = g_termios_saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        g_termios_saved_valid = false;
        return false;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    return true;
}

void input_terminal_restore(void) {
    if (g_termios_saved_valid) {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &g_termios_saved);
        g_termios_saved_valid = false;
    }
    if (isatty(STDIN_FILENO)) {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags >= 0) {
            (void)fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        }
    }
}

static void handle_refuel(void) {
    pthread_mutex_lock(&mtx_engine);
    bool eng = g_state.engine_on;
    pthread_mutex_unlock(&mtx_engine);
    if (eng) {
        return;
    }

    pthread_mutex_lock(&mtx_motion);
    int spd = g_state.speed;
    pthread_mutex_unlock(&mtx_motion);
    if (spd != 0) {
        return;
    }

    pthread_mutex_lock(&mtx_fuel);
    if (g_state.refueling_active) {
        pthread_mutex_unlock(&mtx_fuel);
        return;
    }
    g_state.refueling_active = true;
    g_state.refuel_deadline = time(NULL) + (time_t)REFUEL_DURATION_SEC;
    pthread_mutex_unlock(&mtx_fuel);
}

static void handle_char(unsigned char c) {
    int ch = toupper((int)c);

    switch (ch) {
    case 'Q':
        g_shutdown_request = 1;
        return;

    case 'K':
        system_engine_kill();
        return;

    case 'I':
        system_engine_ignite();
        return;

    case 'F':
        handle_refuel();
        return;

    case 'W': {
        pthread_mutex_lock(&mtx_engine);
        bool on = g_state.engine_on;
        pthread_mutex_unlock(&mtx_engine);
        if (!on) {
            return;
        }
        pthread_mutex_lock(&mtx_fuel);
        bool ref = g_state.refueling_active;
        pthread_mutex_unlock(&mtx_fuel);
        if (ref) {
            return;
        }
        pthread_mutex_lock(&mtx_motion);
        g_state.cruise_active = false;
        g_state.pending_accel_steps++;
        pthread_mutex_unlock(&mtx_motion);
        return;
    }

    case 'S':
        pthread_mutex_lock(&mtx_motion);
        g_state.cruise_active = false;
        g_state.pending_decel_steps++;
        pthread_mutex_unlock(&mtx_motion);
        return;

    case 'C':
        pthread_mutex_lock(&mtx_engine);
        if (!g_state.engine_on) {
            pthread_mutex_unlock(&mtx_engine);
            return;
        }
        pthread_mutex_unlock(&mtx_engine);
        pthread_mutex_lock(&mtx_motion);
        g_state.cruise_active = true;
        g_state.pending_accel_steps = 0;
        g_state.pending_decel_steps = 0;
        pthread_mutex_unlock(&mtx_motion);
        return;

    case 'A':
        pthread_mutex_lock(&mtx_ecu);
        g_state.signal_left_on = !g_state.signal_left_on;
        pthread_mutex_unlock(&mtx_ecu);
        sync_notify_ecu();
        return;

    case 'D':
        pthread_mutex_lock(&mtx_ecu);
        g_state.signal_right_on = !g_state.signal_right_on;
        pthread_mutex_unlock(&mtx_ecu);
        sync_notify_ecu();
        return;

    case 'Z':
        pthread_mutex_lock(&mtx_ecu);
        g_state.hazard_on = !g_state.hazard_on;
        pthread_mutex_unlock(&mtx_ecu);
        sync_notify_ecu();
        return;

    case 'H':
        pthread_mutex_lock(&mtx_ecu);
        g_state.headlight_on = !g_state.headlight_on;
        pthread_mutex_unlock(&mtx_ecu);
        sync_notify_ecu();
        return;

    default:
        return;
    }
}

void *input_thread(void *arg) {
    (void)arg;

    while (!g_shutdown_request) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int sel = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
        if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if (sel > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
            unsigned char buf[64];
            for (;;) {
                ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }
                if (n == 0) {
                    break;
                }
                for (ssize_t i = 0; i < n; i++) {
                    handle_char(buf[i]);
                    if (g_shutdown_request) {
                        return NULL;
                    }
                }
            }
        }
    }

    return NULL;
}
