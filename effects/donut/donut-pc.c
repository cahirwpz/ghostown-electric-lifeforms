#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "donut.h"

int main(void) {
  for (;;) {
    CalcDonut();

    for (int k = 0; (W * H + 1) > k; k++)
      putchar(k % W ? b[k] : 10);
    usleep(15000);
    printf("\x1b[%dA", H + 1);
  }
  return 0;
}
