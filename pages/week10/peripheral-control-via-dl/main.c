#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "pin_manage.h"

void * led_thread(void *arg) {
  printf("주인님께서 너그럽게도 이 부족한 종에게 LED제어용 GPIO핀 %d번을 맡겨주셨습니다.\n", pin.led_out[0]);
  void *handle;
  void (*function_pointer)();
  char *error;

  handle = dlopen("./libcontrol.so", RTLD_LAZY);
  if(!handle){
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  dlerror(); // clear error

  char * func_name = "led_function";
  function_pointer = dlsym(handle, func_name);
  error = dlerror();
  if (error != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
  }    
  function_pointer();
  dlclose(handle);

  return NULL;
}
void * cds_thread(void *arg){
  int *run = (int *)arg;
  printf("주인님께서 너그럽게도 이 부족한 종에게 CDS제어용 GPIO핀 %d번을 맡겨주셨습니다.\n", pin.cds_in[0]);
  void *handle;
  void (*function_pointer)(int*);
  char *error;

  handle = dlopen("./libcontrol.so", RTLD_LAZY);
  if(!handle){
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  dlerror(); // clear error

  char * func_name = "cds_function";
  function_pointer = dlsym(handle, func_name);
  error = dlerror();
  if (error != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
  }    
  function_pointer(run);
  dlclose(handle);
  return NULL;
}

void * cds_adc_thread(void *arg) {
  int *args = (int *)arg;
  printf("주인님께서 너그럽게도 이 부족한 종에게 CDS ADC제어용 I2C SDA핀 %d번을 맡겨주셨습니다.\n", pin.sda);
  void *handle;
  void (*function_pointer)(int*);
  char *error;

  handle = dlopen("./libcontrol.so", RTLD_LAZY);
  if(!handle){
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  dlerror(); // clear error

  char * func_name = "cds_adc_function";
  function_pointer = dlsym(handle, func_name);
  error = dlerror();
  if (error != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
  }    
  function_pointer(args);
  dlclose(handle);
  return NULL;
}

int main (int argc, char * argv[])
{
  int mode = 1;
  int threshold = 180;
  load_yaml("pinnum.yaml");
  printf("주인님의 뜻을 받들어 미천한 종이 명을 수행하겠습니다.\n");
  
  char menu = ' ';
  int running = 1;
  pthread_t tid;

  while(running) {
    printf("\n-----Main Menu-----\n");
    printf("1: LED\n");
    printf("2: CDS (디지털)\n");
    printf("3: CDS (아날로그)\n");
    printf("0: 종료\n");
    printf("메뉴를 선택해주시옵소서->: ");
    scanf(" %c", &menu);

    switch (menu) {
      case '1':
        if (pthread_create(&tid, NULL, led_thread, NULL) != 0) {
          perror("pthread_create");
          exit(1);
        }
        pthread_join(tid, NULL);
        break;
      case '2':
        {
          int digital_run = 1;
          if (pthread_create(&tid, NULL, cds_thread, &digital_run) != 0) {
            perror("pthread_create");
            exit(1);
          }
          printf("\n스레드 동작 중이옵니다. (종료하려면 아무 문자를 입력하고 엔터를 누르시옵소서...)\n");
          char dummy[50];
          scanf("%s", dummy);
          digital_run = 0;
          pthread_join(tid, NULL);
        }
        break;
      case '3':
        {
          int cds_args[2] = { 1, 180 };
          if (pthread_create(&tid, NULL, cds_adc_thread, cds_args) != 0) {
            perror("pthread_create");
            exit(1);
          }
          while(1) {
            int input_val;
            printf("\n새로운 문턱값을 입력해주시옵소서 (종료하려면 -1) : ");
            if (scanf("%d", &input_val) == 1 && input_val >= -1 && input_val <= 255) {
              if (input_val == -1) {
                cds_args[0] = 0;
                break;
              }
              cds_args[1] = input_val;
              printf("문턱값이 %d(으)로 변경되었사옵니다.\n", cds_args[1]);
            } else {
              while (getchar() != '\n');
              printf("어리석은 종이 이해할 수 없는 숫자이옵니다.\n");
            }
          }
          pthread_join(tid, NULL);
        }
        break;
      case '0':
        printf("주인님의 뜻에 따라 물러가겠사옵니다...\n");
        running = 0;
        break;
      default:
        printf("잘못된 입력이옵니다. 다시 선택해주시옵소서.\n");
        break;
    }
  }
  return 0 ;
}
