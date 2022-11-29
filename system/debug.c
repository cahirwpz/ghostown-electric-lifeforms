#include "debug.h"

#ifndef UAE
#include <stdarg.h>
#include <stdio.h>

#define PARPORT 0

#if PARPORT
#include <system/cia.h>

extern void DPutChar(void *ptr, char data);
#else
#include <custom.h>

static void KPutChar(void *ptr, char data) {
  CustomPtrT _custom = ptr;

  while (!(_custom->serdatr & SERDATF_TBE));
  _custom->serdat = data | 0x100;

  if (data == '\n') {
    while (!(_custom->serdatr & SERDATF_TBE));
    _custom->serdat = '\r' | 0x100;
  }
}
#endif

void Log(const char *format, ...) {
  va_list args;

  va_start(args, format);
#if PARPORT
  kvprintf(DPutChar, (void *)ciab, format, args);
#else
  kvprintf(KPutChar, (void *)custom, format, args);
#endif

  va_end(args);
}

__noreturn void Panic(const char *format, ...) {
  va_list args;

  va_start(args, format);
#if PARPORT
  kvprintf(DPutChar, (void *)ciab, format, args);
#else
  kvprintf(KPutChar, (void *)custom, format, args);
#endif
  va_end(args);

  PANIC();
  for (;;);
}
#endif
