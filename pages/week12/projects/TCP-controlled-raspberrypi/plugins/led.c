#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../server/device_interface.h"

static pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;

static int led_pin = -1;
static int current_duty = 0; /* 현재 PWM 듀티 사이클 기억 (0-100) */
static int initialized = 0;

int device_init(char *io_buf) {
    if (initialized) {
        device_cleanup();
    }
    int pin = atoi(io_buf);
    if (pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid LED GPIO pin %d", pin);
        return -1;
    }
    static int setup_done = 0;
    if (!setup_done) {
        wiringPiSetup();
        setup_done = 1;
    }
    led_pin = pin;
    if (softPwmCreate(led_pin, 0, 100) != 0) {
        led_pin = -1;
        snprintf(io_buf, BUFFER_SIZE, "FAILED: softPwmCreate failed");
        return -1;
    }
    initialized = 1;
    current_duty = 0;
    snprintf(io_buf, BUFFER_SIZE, "DONE: LED configured");
    return 0;
}

int device_execute(char *io_buf) {
    if (!initialized || led_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid LED GPIO pin\nRECOMMANDATION: set led [GPIO Pin]");
        return -1;
    }

    int duty = 0;

    if (strcmp(io_buf, "ON") == 0 || strcmp(io_buf, "HI") == 0 || strcmp(io_buf, "HIGH") == 0) {
        duty = 100;
    } else if (strcmp(io_buf, "MID") == 0 || strcmp(io_buf, "MED") == 0) {
        duty = 50;
    } else if (strcmp(io_buf, "LOW") == 0) {
        duty = 10;
    } else if (strcmp(io_buf, "OFF") == 0) {
        duty = 0;
    } else {
        char *endptr;
        duty = strtol(io_buf, &endptr, 10);
        if (*endptr != '\0' || duty < 0 || duty > 100) {
            snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid LED value %s. Must be ON/OFF/HI/MID/LOW or 0..100", io_buf);
            return -1;
        }
    }

    softPwmWrite(led_pin, duty);
    current_duty = duty;

    snprintf(io_buf, BUFFER_SIZE, "DONE: LED brightness set to %d%%", duty);
    return 0;
}

int device_get_state(char *io_buf) {
    if (!initialized || led_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid LED GPIO pin\nRECOMMANDATION: set led [GPIO Pin]");
        return -1;
    }
    snprintf(io_buf, BUFFER_SIZE, "DONE: LED brightness is %d%%", current_duty);
    return 0;
}

void device_cleanup(void) {
    if (led_pin != -1) {
        softPwmWrite(led_pin, 0);
        current_duty = 0;
        delay(10);
        led_pin = -1;
    }
    initialized = 0;
}

int device_main(char *io_buf) {
    char input[BUFFER_SIZE];
    pthread_mutex_lock(&led_mutex);
    strncpy(input, io_buf, BUFFER_SIZE - 1);
    input[BUFFER_SIZE - 1] = '\0';

    char cmd[32] = {0};
    char target[32] = {0};
    char args[256] = {0};

    int parsed = sscanf(input, "%s %s %[^\n]", cmd, target, args);
    if (parsed < 2) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid command format");
        pthread_mutex_unlock(&led_mutex);
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
    pthread_mutex_unlock(&led_mutex);
    return ret;
}
