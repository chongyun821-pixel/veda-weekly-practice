#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "../server/device_interface.h"

static pthread_mutex_t buzzer_mutex = PTHREAD_MUTEX_INITIALIZER;

static int spkr_pin = -1;
static volatile int current_play_id = 0;
static int initialized = 0;

// Define default melodies statically. Terminating with 0.
static int melody0[] = {                                          /* 학교종을 연주하기 위한 계이름 */
  391, 391, 440, 440, 391, 391, 329.63, 329.63, \
  391, 391, 329.63, 329.63, 293.66, 293.66, 293.66, 0, \
  391, 391, 440, 440, 391, 391, 329.63, 329.63, \
  391, 329.63, 293.66, 329.63, 261.63, 261.63, 261.63, -1
};
static int melody1[] = { 262, 294, 330, 349, 392, 440, 494, 523, -1 }; // C4 D4 E4 F4 G4 A4 B4 C5
static int melody2[] = { 523, 494, 440, 392, 349, 330, 294, 262, -1 }; // C5 B4 A4 G4 F4 E4 D4 C4
static int melody3[] = { 330, 294, 262, 294, 330, 330, 330, -1 };       // Mary Had a Little Lamb segment

static int aranara_melody[] = {
  0, 193, 234, 217, 220, 219, 217, 217, 217, 216, 216, 217, 217, 217, 217, 219,
  219, 217, 217, 217, 217, 217, 217, 0, 247, 198, 198, 198, 198, 198, 199, 200,
  204, 203, 203, 204, 204, 206, 206, 209, 210, 212, 212, 234, 199, 220, 220, 228,
  219, 220, 220, 220, 221, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220,
  220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 212, 211, 212, 211, 211, 211,
  211, 214, 215, 215, 247, 247, 247, 247, 247, 247, 247, 247, 247, 245, 220, 221,
  221, 222, 224, 224, 225, 224, 224, 225, 245, 247, 247, 247, 247, 247, 245, 247,
  245, 243, 0, 268, 274, 272, 272, 271, 228, 237, 234, 236, 0, 132, 133, 133,
  134, 134, 135, 135, 135, 134, 134, 133, 134, 133, 134, 134, 135, 135, 135, 135,
  136, 136, 136, 136, 136, 135, 135, 135, 134, 134, 134, 178, 248, 248, 247, 247,
  247, 247, 248, 248, 248, 247, 220, 220, 221, 221, 221, 221, 221, 222, 222, 222,
  222, 222, 222, 222, 221, 221, 221, 221, 221, 221, 221, 221, 221, 221, 220, 220,
  220, 220, 221, 221, 240, 244, 245, 245, 245, 245, 245, 245, 244, 244, 244, 220,
  220, 219, 219, 217, 217, 217, 219, 219, 219, 219, 219, 219, 219, 219, 220, 220,
  220, 220, 220, 220, 220, 220, 220, 219, 219, 217, 219, 217, 217, 217, 216, 215,
  215, 214, 214, 212, 211, 211, 210, 210, 216, 219, 167, 0, 0, 0, 172, 173,
  172, 171, 184, 186, 185, 185, 185, 185, 185, 185, 185, 185, 0, 186, 186, 185,
  169, 209, 169, 209, 179, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
  232, 177, 243, 243, 241, 241, 241, 241, 243, 243, 241, 240, 236, 238, 237, 234,
  241, 234, 234, 240, 237, 237, 237, 236, 238, 240, 238, 243, 240, 240, 244, 240,
  241, 241, 238, 241, 238, 240, 244, 241, 238, 251, 165, 166, 166, 166, 166, 166,
  166, 166, 166, 166, 165, 166, 166, 166, 166, 166, 166, 0, 0, 0, 0, 140,
  140, 140, 140, 140, 140, 140, 204, 135, 226, 217, 214, 212, 211, 215, 214, 215,
  141, 142, 142, 0, 0, 0, 0, 0, 0, 0, 250, 247, 247, 247, 247, 247,
  247, 247, 248, 179, 179, 179, 179, 180, 181, 181, 181, 181, 182, 0, 0, 209,
  209, 279, 279, 279, 279, 279, 277, 368, 248, 247, 248, 247, 248, 247, 248, 247,
  248, 220, 221, 221, 221, 222, 221, 222, 221, 221, 339, 331, 335, 335, 341, 341,
  341, 349, 349, 349, 0, 0, 280, 280, 279, 214, 219, 217, 216, 217, 171, 248,
  247, 248, 166, 166, 166, 166, 166, 166, 215, 206, 211, 205, 210, 205, 216, 217,
  215, 222, 331, 331, 329, 331, 329, 329, 329, 331, 329, 0, 166, 166, 166, 165,
  165, 165, 166, 165, 245, 250, 220, 220, 220, 220, 220, 221, 222, 221, 221, 221,
  230, 167, 240, 238, 241, 166, 167, 166, 167, 166, 0, 279, 279, 279, 280, 279,
  279, 274, 272, 268, 250, 248, 248, 148, 148, 148, 148, 148, 148, 148, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 132, 167, 166, 166, 166, 167, 166, 166, 166,
  166, 166, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 253, 265,
  262, 257, 251, 254, 257, 256, 250, 220, 132, 132, 132, 132, 132, 132, 132, 132,
  132, 0, 0, 0, 148, 148, 147, 148, 147, 147, 147, 238, 237, 211, 211, 211,
  211, 212, 215, 217, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 222,
  186, 184, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 206, 205, 206,
  206, 206, 206, 205, 205, 205, 204, 212, 206, 159, 212, 205, 216, 233, 237, 241,
  233,
  -1
};
static int *playlist[] = { melody0, melody1, melody2, melody3, aranara_melody };
static const int playlist_num = sizeof(playlist) / sizeof(playlist[0]);

int device_init(char *io_buf) {
    pthread_mutex_lock(&buzzer_mutex);
    if (initialized) {
        pthread_mutex_unlock(&buzzer_mutex);
        device_cleanup();
        pthread_mutex_lock(&buzzer_mutex);
    }
    int pin = atoi(io_buf);
    if (pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Buzzer GPIO pin %d", pin);
        pthread_mutex_unlock(&buzzer_mutex);
        return -1;
    }
    static int setup_done = 0;
    if (!setup_done) {
        wiringPiSetup();
        setup_done = 1;
    }
    spkr_pin = pin;
    if (softToneCreate(spkr_pin) != 0) {
        spkr_pin = -1;
        snprintf(io_buf, BUFFER_SIZE, "FAILED: softToneCreate failed");
        pthread_mutex_unlock(&buzzer_mutex);
        return -1;
    }
    initialized = 1;
    snprintf(io_buf, BUFFER_SIZE, "DONE: Buzzer configured");
    pthread_mutex_unlock(&buzzer_mutex);
    return 0;
}

static void* play_thread_func(void *arg) {
    int play_idx = (int)(intptr_t)arg;
    
    pthread_mutex_lock(&buzzer_mutex);
    int my_play_id = current_play_id;
    pthread_mutex_unlock(&buzzer_mutex);
    
    int j = 0;
    while (1) {
        pthread_mutex_lock(&buzzer_mutex);
        if (!initialized || playlist[play_idx][j] == -1 || my_play_id != current_play_id) {
            pthread_mutex_unlock(&buzzer_mutex);
            break;
        }
        if (spkr_pin != -1) {
            softToneWrite(spkr_pin, playlist[play_idx][j]);
        }
        pthread_mutex_unlock(&buzzer_mutex);
        delay(200);
        j++;
    }
    
    pthread_mutex_lock(&buzzer_mutex);
    if (my_play_id == current_play_id && spkr_pin != -1) {
        softToneWrite(spkr_pin, 0);
    }
    pthread_mutex_unlock(&buzzer_mutex);
    return NULL;
}

int device_execute(char *io_buf) {
    pthread_mutex_lock(&buzzer_mutex);
    if (!initialized || spkr_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Buzzer GPIO pin\nRECOMMANDATION: set buzzer [GPIO Pin]");
        pthread_mutex_unlock(&buzzer_mutex);
        return -1;
    }

    int play_idx = -1;

    if (strcmp(io_buf, "OFF") == 0) {
        current_play_id++;
        if (spkr_pin != -1) {
            softToneWrite(spkr_pin, 0);
        }
        pthread_mutex_unlock(&buzzer_mutex);
        snprintf(io_buf, BUFFER_SIZE, "DONE: Buzzer turned OFF");
        return 0;
    } else if (strcmp(io_buf, "ON") == 0) {
        play_idx = 0;
    } else {
        char *endptr;
        int num = strtol(io_buf, &endptr, 10);
        if (*endptr != '\0' || num < 1 || num > playlist_num) {
            snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid option. Must be ON, OFF, or 1..%d", playlist_num);
            pthread_mutex_unlock(&buzzer_mutex);
            return -1;
        }
        play_idx = num - 1;
    }

    current_play_id++;
    
    pthread_t tid;
    if (pthread_create(&tid, NULL, play_thread_func, (void*)(intptr_t)play_idx) != 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Failed to create play thread");
        pthread_mutex_unlock(&buzzer_mutex);
        return -1;
    }
    pthread_detach(tid);

    pthread_mutex_unlock(&buzzer_mutex);
    
    snprintf(io_buf, BUFFER_SIZE, "DONE: Melody %d started playing", play_idx + 1);
    return 0;
}
    
int device_get_state(char *io_buf) {
    pthread_mutex_lock(&buzzer_mutex);
    if (!initialized || spkr_pin <= 0) {
        snprintf(io_buf, BUFFER_SIZE, "FAILED: Invalid Buzzer GPIO pin\nRECOMMANDATION: set buzzer [GPIO Pin]");
        pthread_mutex_unlock(&buzzer_mutex);
        return -1;
    }
    snprintf(io_buf, BUFFER_SIZE, "FAILED: BUZZER does not support get command");
    pthread_mutex_unlock(&buzzer_mutex);
    return -1;
}

void device_cleanup(void) {
    pthread_mutex_lock(&buzzer_mutex);
    current_play_id++;
    if (spkr_pin != -1) {
        softToneWrite(spkr_pin, 0);
        softToneStop(spkr_pin);
        spkr_pin = -1;
    }
    initialized = 0;
    pthread_mutex_unlock(&buzzer_mutex);
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