#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct PinConfig {
  int led_out[1];
  int cds_in[1];
  int sda;
  int sdc;
};

extern struct PinConfig pin;

void cds_function(int* run) {
    int pin_num = pin.cds_in[0];
    wiringPiSetupGpio();
    pinMode (pin_num, INPUT);
    while(*run){
        if(digitalRead(pin_num) == 0)
            printf("어둡습니다.\n");
        else
            printf("밝습니다.\n");
        delay(1000);
    }
}