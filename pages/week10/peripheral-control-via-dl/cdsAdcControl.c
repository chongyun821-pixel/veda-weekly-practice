#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPiI2C.h>

struct PinConfig {
  int led_out[1];
  int cds_in[1];
  int sda;
  int sdc;
};

extern struct PinConfig pin;

void cds_adc_function(int* args) {
    int* run = &args[0];
    int* threshold = &args[1];
    int fd;
    int a2dChannel = 0;
    int a2dVal;

    if((fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48)) < 0)
        fprintf(stderr, "무능한 종을 죽여주옵소서!\n");

    while (*run) {
        wiringPiI2CWrite(fd, 0x00 | a2dChannel);
        wiringPiI2CRead(fd); // dummy read
        a2dVal = wiringPiI2CRead(fd);

        if(a2dVal < *threshold)
            printf("어둡습니다. (현재 밝기: %d, 문턱값: %d)\n", a2dVal, *threshold);
        else
            printf("밝습니다. (현재 밝기: %d, 문턱값: %d)\n", a2dVal, *threshold);
        delay(1000);
    }
}
