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

extern u_char PtModule[];
extern u_char PtModuleStart[];
extern u_char PtModuleSize[];

static BitmapT *screen;
static CopListT *cp;
static ConsoleT console;

/* Extra variables to enhance replayer functionality */
static bool stopped = true;


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

  // AddIntServer(INTB_VERTB, CinterMusicServer);

  ConsoleSetCursor(&console, 0, 0);
  ConsolePutStr(&console, 
                "Pause (SPACE) -10s (LEFT) +10s (RIGHT)\n"
                "Exit (ESC)\n");

  stopped = false;
}

static void Kill(void) {
  // RemIntServer(INTB_VERTB, CinterMusicServer);

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
    stopped ^= 1;
    if (stopped)
      DisableDMA(DMAF_AUDIO);
  }

  if (ev.key.code == KEY_LEFT) {
  }

  if (ev.key.code == KEY_RIGHT) {
  }

  return true;
}

EFFECT(PlayProtracker, Load, UnLoad, Init, Kill, Render);
