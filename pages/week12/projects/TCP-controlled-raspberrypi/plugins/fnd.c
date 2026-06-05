#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "../server/device_interface.h"

static pthread_mutex_t fnd_mutex = PTHREAD_MUTEX_INITIALIZER;

#define FND_SEGMENTS 8

static int gpiopins[FND_SEGMENTS] = {-1,-1,-1,-1,-1,-1,-1,-1};
static volatile int current_number = 0;
static int initialized = 0;

/* Common Anode 7-segment: 0=ON, 1=OFF.  핀 순서: a,b,c,d,e,f,g,dp */
static const int number2signal[10][FND_SEGMENTS] = {
    /* a, b, c, d, e, f, g, dp */
    {0, 0, 0, 0, 0, 0, 1, 1}, // 0
    {1, 0, 0, 1, 1, 1, 1, 1}, // 1
    {0, 0, 1, 0, 0, 1, 0, 1}, // 2
    {0, 0, 0, 0, 1, 1, 0, 1}, // 3
    {1, 0, 0, 1, 1, 0, 0, 1}, // 4
    {0, 1, 0, 0, 1, 0, 0, 1}, // 5
    {0, 1, 0, 0, 0, 0, 0, 1}, // 6
    {0, 0, 0, 1, 1, 1, 1, 1}, // 7
    {0, 0, 0, 0, 0, 0, 0, 1}, // 8
    {0, 0, 0, 0, 1, 0, 0, 1}  // 9
};

int device_init(char *io_buf) {
    pthread_mutex_lock(&fnd_mutex);
    if (initialized) {
        pthread_mutex_unlock(&fnd_mutex);
        device_cleanup();
        pthread_mutex_lock(&fnd_mutex);
    }
    int pins[FND_SEGMENTS];
    int parsed = sscanf(io_buf, "%d %d %d %d %d %d %d %d",
        &pins[0], &pins[1], &pins[2], &pins[3],
        &pins[4], &pins[5], &pins[6], &pins[7]);
    if (parsed < FND_SEGMENTS) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Require %d GPIO pins (a b c d e f g dp)", FND_SEGMENTS);
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }
    for (int i = 0; i < FND_SEGMENTS; i++) {
        if (pins[i] <= 0) {
            snprintf(io_buf, BUFFER_SIZE, "FAILED: Pin %d is invalid (%d)", i + 1, pins[i]);
            pthread_mutex_unlock(&fnd_mutex);
            return -1;
        }
    }
    static int setup_done = 0;
    if (!setup_done) {
        wiringPiSetup();
        setup_done = 1;
    }
    for (int i = 0; i < FND_SEGMENTS; i++) {
        gpiopins[i] = pins[i];
        pinMode(gpiopins[i], OUTPUT);
        digitalWrite(gpiopins[i], HIGH); /* Common Anode: HIGH = OFF */
    }
    initialized = 1;
    snprintf(io_buf, BUFFER_SIZE, "DONE: FND configured");
    pthread_mutex_unlock(&fnd_mutex);
    return 0;
}

static void* fnd_thread_func(void *arg) {
    int start_val = (int)(intptr_t)arg;
    
    for (int val = start_val; val >= 0; val--) {
        pthread_mutex_lock(&fnd_mutex);
        if (!initialized) {
            pthread_mutex_unlock(&fnd_mutex);
            break;
        }
        current_number = val;
        for (int i = 0; i < FND_SEGMENTS; i++) {
            if (gpiopins[i] > 0) {
                digitalWrite(gpiopins[i], number2signal[val][i] ? HIGH : LOW);
            }
        }
        pthread_mutex_unlock(&fnd_mutex);
        delay(1000);
    }
    
    pthread_mutex_lock(&fnd_mutex);
    current_number = 0;
    pthread_mutex_unlock(&fnd_mutex);
    
    // 외부로 노출된 서버의 trigger_buzzer_on 함수 호출 (0 도달 시 부저 울림)
    extern void trigger_buzzer_on(void);
    trigger_buzzer_on();
    
    return NULL;
}

int device_execute(char *io_buf) {
    pthread_mutex_lock(&fnd_mutex);
    if (!initialized || gpiopins[0] <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: FND not initialized\nRECOMMENDATION: set fnd [a] [b] [c] [d] [e] [f] [g] [dp]");
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }

    char *endptr;
    int start_val = strtol(io_buf, &endptr, 10);
    if (*endptr != '\0' || start_val < 0 || start_val > 9) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid option %s. FND value must be between 0 and 9", io_buf);
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }

    if (current_number != 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: FND is already running (current count: %d)", current_number);
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }

    current_number = start_val;
    
    pthread_t tid;
    if (pthread_create(&tid, NULL, fnd_thread_func, (void*)(intptr_t)start_val) != 0) {
        current_number = 0;
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Failed to create FND thread");
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }
    pthread_detach(tid);
    
    pthread_mutex_unlock(&fnd_mutex);

    snprintf(io_buf, BUFFER_SIZE, "DONE: FND countdown started from %d", start_val);
    return 0;
}

int device_get_state(char *io_buf) {
    pthread_mutex_lock(&fnd_mutex);
    if (!initialized || gpiopins[0] <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: FND not initialized\nRECOMMENDATION: set fnd [a] [b] [c] [d] [e] [f] [g] [dp]");
        pthread_mutex_unlock(&fnd_mutex);
        return -1;
    }
    snprintf(io_buf, BUFFER_SIZE, "DONE: FND current count is %d", current_number);
    pthread_mutex_unlock(&fnd_mutex);
    return 0;
}

void device_cleanup(void) {
    pthread_mutex_lock(&fnd_mutex);
    current_number = 0;
    for (int i = 0; i < FND_SEGMENTS; i++) {
        if (gpiopins[i] <= 0) continue;
        digitalWrite(gpiopins[i], HIGH); /* Common Anode: HIGH = OFF */
        pinMode(gpiopins[i], INPUT);
        gpiopins[i] = -1;
    }
    initialized = 0;
    pthread_mutex_unlock(&fnd_mutex);
}

int device_main(char *io_buf) {
    char input[BUFFER_SIZE];
    strncpy(input, io_buf, BUFFER_SIZE - 1);
    input[BUFFER_SIZE - 1] = '\0';

    char cmd[32] = {0};
    char target[32] = {0};
    char args[256] = {0};

    int parsed = sscanf(input, "%s %s %[^\n]", cmd, target, args);
    if (parsed < 2) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid command format");
        return -1;
    }

    if (strcmp(cmd, "set") == 0) {
        strcpy(io_buf, args);
        return device_init(io_buf);
    } else if (strcmp(cmd, "do") == 0) {
        strcpy(io_buf, args);
        return device_execute(io_buf);
    } else if (strcmp(cmd, "get") == 0) {
        io_buf[0] = '\0';
        return device_get_state(io_buf);
    } else {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Unknown command %s", cmd);
        return -1;
    }
}
