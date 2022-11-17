#include <effect.h>
#include <blitter.h>
#include <ptplayer.h>
#include <console.h>
#include <copper.h>
#include <system/event.h>
#include <system/interrupt.h>
#include <system/keyboard.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 1

#include "data/lat2-08.c"

extern u_char binary_data_virgill_colombia_mod_start[];
#define module ((void *)binary_data_virgill_colombia_mod_start)

static BitmapT *screen;
static CopListT *cp;
static ConsoleT console;

/* Extra variables to enhance replayer functionality */
static bool stopped = true;

/* Code copied from `main-executable.c` from AmigaKlang. */
static void pt_install_cia(void) {
  register int _d0 asm("d0") = 1;
  register const void* _a0 asm("a0") = (void *)0;
  register const void* _a6 asm("a6") = (void *)0xdff000;

  asm volatile ("movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
                "jsr _mt_install_cia\n"
                "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6"
                :
                : "r" (_d0), "r" (_a0), "r" (_a6)
                : "cc", "memory");
}

static void pt_init(void *ModAdr, void *SmpAdr) {
  register int _d0 __asm("d0") = 0;
  register const void* _a0 asm("a0") = ModAdr;
  register const void* _a1 asm("a1") = (void *)SmpAdr;
  register const void* _a6 asm("a6") = (void *)0xdff000;

  asm volatile ("movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
                "jsr _mt_init\n"
                "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6"
                :
                : "r" (_d0), "r" (_a0), "r" (_a1), "r" (_a6)
                : "cc", "memory");
}

static void pt_remove_cia(void) {
  register const void* _a6 asm("a6") = (void *)0xdff000;

  asm volatile ("movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
                "jsr _mt_remove_cia\n"
                "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6"
                :
                : "r" (_a6)
                : "cc", "memory");
}

static void pt_end(void) {
  register const void *_a6 asm("a6") = (void *)0xdff000;

  asm volatile ("movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
                "jsr _mt_end\n"
                "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6"
                :
                : "r" (_a6)
                : "cc", "memory");
}

static void Load(void) {
}

static void UnLoad(void) {
}

static void Init(void) {
  KeyboardInit();

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  SetColor(0, 0x000);
  SetColor(1, 0xfff);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);

  ConsoleInit(&console, &latin2, screen);

  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);

  ConsoleSetCursor(&console, 0, 0);
  ConsolePutStr(&console, "Initializing Protracker replayer... please wait!\n");

  pt_install_cia();
  pt_init(module, 0);
  mt_Enable = 1;

  ConsoleSetCursor(&console, 0, 0);
  ConsolePutStr(&console, 
                "Pause (SPACE) -10s (LEFT) +10s (RIGHT)\n"
                "Exit (ESC)\n");

  stopped = false;
}

static void Kill(void) {
  pt_end();
  pt_remove_cia();

  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_AUDIO);

  DeleteCopList(cp);
  DeleteBitmap(screen);
}

static bool HandleEvent(void);

static void Render(void) {
  TaskWaitVBlank();

  exitLoop = !HandleEvent();
}

static bool HandleEvent(void) {
  EventT ev;

  if (!PopEvent(&ev))
    return true;

  if (ev.type == EV_MOUSE)
    return true;

  if (ev.key.modifier & MOD_PRESSED)
    return true;

  if (ev.key.code == KEY_ESCAPE)
    return false;

  if (ev.key.code == KEY_SPACE) {
    mt_Enable ^= 1;
    if (!mt_Enable)
      DisableDMA(DMAF_AUDIO);
  }

  if (ev.key.code == KEY_LEFT) {
  }

  if (ev.key.code == KEY_RIGHT) {
  }

  return true;
}

EFFECT(PlayProtracker, Load, UnLoad, Init, Kill, Render);
