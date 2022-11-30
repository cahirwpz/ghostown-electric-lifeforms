#include "effect.h"
#include "console.h"
#include "copper.h"
#include <system/event.h>
#include <system/keyboard.h>
#include <system/file.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 1

#include "data/drdos8x8.c"

static BitmapT *screen;
static CopListT *cp;
static ConsoleT console;

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);
  cp = NewCopList(100);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  SetColor(0, 0x000);
  SetColor(1, 0xfff);

  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);

  ConsoleInit(&console, &drdos8x8, screen);

  KeyboardInit();
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  KeyboardKill();

  DeleteCopList(cp);
  DeleteBitmap(screen);
}

static bool HandleEvent(void) {
  EventT ev;

  if (!PopEvent(&ev))
    return true;

  if (ev.type != EV_KEY)
    return true;

  if (ev.key.modifier & MOD_PRESSED)
    return true;

  if (ev.key.code == KEY_ESCAPE)
    return false;

  ConsoleDrawCursor(&console);

  ConsoleDrawCursor(&console);

  return true;
}

#include "donut.h"

static void Render(void) {
  int k;

  ConsoleSetCursor(&console, 0, 0);

  CalcDonut();

  for (k = 0; (W * H + 1) > k; k++)
    ConsolePutChar(&console, k % W ? b[k] : 10);

  exitLoop = !HandleEvent();
}

EFFECT(KbdTest, NULL, NULL, Init, Kill, Render);
