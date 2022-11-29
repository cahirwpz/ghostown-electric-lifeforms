#include "debug.h"

#if !defined(UAE) && DEBUGOUT > 0
#include <stdarg.h>
#include <stdio.h>

#if DEBUGOUT == 1
#include <system/cia.h>

extern void DPutChar(void *ptr, char data);

#define PUTCHAR DPutChar
#define AUXDATA ciab
#endif 

#if DEBUGOUT == 2
#include <custom.h>

extern void KPutChar(void *ptr, char data);

#define PUTCHAR KPutChar
#define AUXDATA custom
#endif

void Log(const char *format, ...) {
  va_list args;

  va_start(args, format);
  kvprintf(PUTCHAR, (void *)AUXDATA, format, args);

  va_end(args);
}

__noreturn void Panic(const char *format, ...) {
  va_list args;

  va_start(args, format);
  kvprintf(PUTCHAR, (void *)AUXDATA, format, args);
  va_end(args);

  PANIC();
  for (;;);
}
#endif
