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

extern u_char Module[];
extern u_char Samples[];

static BitmapT *screen;
static CopListT *cp;
static ConsoleT console;

extern struct mt_data mt_data;

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

extern u_int AK_Progress;
void AK_Generate(void *TmpBuf asm("a1"));

static void Load(void) {
  void *TmpBuf = MemAlloc(32768, MEMF_PUBLIC);
  AK_Generate(TmpBuf);
  MemFree(TmpBuf);
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
  ConsolePutStr(&console, "Initializing Protracker replayer...!\n");

  pt_install_cia();
  pt_init(Module, Samples);
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

static const char hex[16] = "0123456789ABCDEF";

static inline void NumToHex(char *str, u_int n, short l) {
  do {
    str[--l] = hex[n & 15];
    n >>= 4;
    if (n == 0)
      break;
  } while (l > 0);
}

static void NoteToHex(char *str, u_short note, u_short cmd) {
  /* instrument number */
  NumToHex(&str[0], ((note >> 8) & 0xf0) | ((cmd >> 12) & 15), 2);
  /* note period */
  NumToHex(&str[2], note & 0xfff, 3);
  /* effect command */
  NumToHex(&str[5], cmd & 0xfff, 3);
}

static void Render(void) {
  struct pt_mod *mod = mt_data.mt_mod;
  short songPos = mt_data.mt_SongPos;
  short pattNum = mod->order[songPos];
  short pattPos = mt_data.mt_PatternPos >> 4;
  u_int *pattern = (void *)mod->pattern[pattNum];
  short i;

  ConsoleSetCursor(&console, 0, 3);
  ConsolePrint(&console, "Playing \"%s\"\n\n", mod->name);
  ConsolePrint(&console, "Song position: %d -> %d\n\n", songPos, pattNum);

  for (i = pattPos - 4; i < pattPos + 4; i++) {
    static char buf[41];

    memset(buf, ' ', sizeof(buf));
    buf[40] = '\0';

    if ((i >= 0) && (i < 64)) {
      u_short *row = (void *)&pattern[4 * i];
      NumToHex(&buf[0], i, 2);
      buf[2] = ':';
      NoteToHex(&buf[3], row[0], row[1]);
      buf[11] = '|';
      NoteToHex(&buf[12], row[2], row[3]);
      buf[20] = '|';
      NoteToHex(&buf[21], row[4], row[5]);
      buf[29] = '|';
      NoteToHex(&buf[30], row[6], row[7]);
      buf[38] = '|';
    }

    ConsolePutStr(&console, buf);
  }

  ConsolePutChar(&console, '\n');

  for (i = 0; i < 4; i++) {
    struct mt_chan *chan = &mt_data.mt_chan[i];
    ConsolePrint(&console, "CH%d: %08x %04x %08x %04x\n",
                 i, (u_int)chan->n_start, chan->n_length,
                 (u_int)chan->n_loopstart, chan->n_replen);
  }

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
