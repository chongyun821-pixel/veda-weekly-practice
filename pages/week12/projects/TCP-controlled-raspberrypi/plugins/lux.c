#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../server/device_interface.h"

static pthread_mutex_t lux_mutex = PTHREAD_MUTEX_INITIALIZER;

static int lux_pin = -1;
static int initialized = 0;

int device_init(char *io_buf) {
    if (initialized) {
        device_cleanup();
    }
    int pin = atoi(io_buf);
    if (pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Lux GPIO pin\nRECOMMANDATION: set lux [GPIO Pin]");
        return -1;
    }
    static int setup_done = 0;
    if (!setup_done) {
        wiringPiSetup();
        setup_done = 1;
    }
    lux_pin = pin;
    pinMode(lux_pin, INPUT);
    initialized = 1;
    snprintf(io_buf, BUFFER_SIZE, "DONE: Lux configured");
    return 0;
}

int device_execute(char *io_buf) {
    if (!initialized || lux_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Lux GPIO pin\nRECOMMANDATION: set lux [GPIO Pin]");
        return -1;
    }
    snprintf(io_buf, BUFFER_SIZE, "FAILED: lux has no option 'do'");
    return -1;
}

int device_get_state(char *io_buf) {
    if (!initialized || lux_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Lux GPIO pin\nRECOMMANDATION: set lux [GPIO Pin]");
        return -1;
    }
    int val = digitalRead(lux_pin);
    snprintf(io_buf, BUFFER_SIZE, "DONE : Lux value is %s", val ? "HIGH" : "LOW");
    return 0;
}

void device_cleanup(void) {
    if (lux_pin != -1) {
        pullUpDnControl(lux_pin, PUD_OFF);
        lux_pin = -1;
    }
    initialized = 0;
}

int device_main(char *io_buf) {
    char input[BUFFER_SIZE];
    pthread_mutex_lock(&lux_mutex);
    strncpy(input, io_buf, BUFFER_SIZE - 1);
    input[BUFFER_SIZE - 1] = '\0';

    char cmd[32] = {0};
    char target[32] = {0};
    char args[256] = {0};

    int parsed = sscanf(input, "%s %s %[^\n]", cmd, target, args);
    if (parsed < 2) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid command format");
        pthread_mutex_unlock(&lux_mutex);
        return -1;
    }

    int ret = 0;
    if (strcmp(cmd, "set") == 0) {
        strcpy(io_buf, args);
        ret = device_init(io_buf);
    } else if (strcmp(cmd, "do") == 0) {
        strcpy(io_buf, args);
        ret = device_execute(io_buf);
    } else if (strcmp(cmd, "get") == 0) {
        io_buf[0] = '\0';
        ret = device_get_state(io_buf);
    } else {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Unknown command %s", cmd);
        ret = -1;
    }
    pthread_mutex_unlock(&lux_mutex);
    return ret;
}
