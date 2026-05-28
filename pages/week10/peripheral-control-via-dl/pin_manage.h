#ifndef PINMANAGE
#define PINMANAGE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PinConfig {
  int led_out[1];
  int cds_in[1];
  int sda;
  int sdc;
} pin;

void load_yaml(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Failed to open pinnum.yaml");
    return;
  }

  char line[256];
  char section[64] = "";

  while (fgets(line, sizeof(line), file)) {
    // Remove trailing newline/carriage return
    line[strcspn(line, "\r\n")] = '\0';

    // Ignore comments and empty lines
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '#') continue;

    // Check if this line is a section header (e.g. "led_in:")
    if (strstr(p, ":") != NULL && strchr(p, ' ') == NULL) {
      char *colon = strchr(p, ':');
      *colon = '\0';
      strncpy(section, p, sizeof(section) - 1);
      section[sizeof(section) - 1] = '\0';
      continue;
    }

    // Check for key-value pairs (e.g., "  1: 17")
    char *colon = strchr(p, ':');
    if (colon != NULL) {
      *colon = '\0';
      char *key_str = p;
      char *val_str = colon + 1;

      // trim
      while (*key_str == ' ' || *key_str == '\t') key_str++;
      while (*val_str == ' ' || *val_str == '\t') val_str++;

      int key = atoi(key_str);
      int val = atoi(val_str);

      if (strcmp(section, "led_out") == 0) {
        if (key == 1) {
          pin.led_out[0] = val;
        }
      } else if (strcmp(section, "cds_in") == 0) {
        if (key == 1) {
          pin.cds_in[0] = val;
        }
      }
      else if(strcmp(section, "sda") == 0){
        if(key == 1){
          pin.sda = val;
        }
      }
      else if(strcmp(section, "sdc") == 0){
        if(key == 1){
          pin.sdc = val;
        }
      }
    }
  }
  fclose(file);
}

#endif // PINMANAGE