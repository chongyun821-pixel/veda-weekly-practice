
#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// LED Pin - wiringPi pin 0 is BCM_GPIO 17.

struct PinConfig {
  int led_out[1];
  int cds_in[1];
  int sda;
  int sdc;
};

extern struct PinConfig pin;

void led_function() {
  int pin_num = pin.led_out[0];
  wiringPiSetupGpio();
  pinMode (pin_num, OUTPUT);
  char cmd[20];
loop: 
  printf("미천한 종이 주인님의 명령을 받듭니다 : ");
  scanf(" %s", cmd);
  if (strcmp(cmd, "ON") == 0)
    digitalWrite(pin_num, HIGH);
  else if (strcmp(cmd, "OFF") == 0)
    digitalWrite(pin_num, LOW);
  else if(strcmp(cmd, "EXIT") == 0) goto exit;
  else
    puts("어리석은 종은 주인님의 말씀을 이해할 수 없습니다.\n사용 예 : ON/OFF/EXIT\n");
  goto loop;
exit:
  return;
}
