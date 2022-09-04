#include "effect.h"

#include "custom.h"
#include "copper.h"
#include "blitter.h"
#include "bitmap.h"
#include "sprite.h"
#include "fx.h"
#include <system/memory.h>

#include "data/turmite-pal.c"
#include "data/turmite-credits-1.c"

#define WIDTH 256
#define HEIGHT 256
#define DEPTH 4
#define NSTEPS (256 + 196)

#define GENERATION 1

/* don't decrease it since color's value is stored on lower 3 bits of tile */
#define STEP 8

static CopListT *cp;
static BitmapT *screen;

/* higher 5 bits store value, lower 3 bits store color information */
static u_char *board;
static u_int lookup[256][2];

static const BitmapT *turmite_credits[] = {
  &turmite_credits_1,
  NULL,
};

static void BitmapToBoard(const BitmapT *bm, u_char *board) {
  short n = WIDTH * HEIGHT / 8 - 1;
  u_char *src = bm->planes[0];
  u_int *dst = (u_int *)board;
  u_int *tab;
  
  do {
    tab = lookup[*src++];
    *dst++ = *tab++;
    *dst++ = *tab++;
  } while (--n != -1);
}

static void Load(void) {
  short i, j;
  for (i = 0; i < 256; i++) {
    u_char *tab = (u_char *)&lookup[i];
    for (j = 0; j < 8; j++)
      tab[j] = i & (1 << (7 - j)) ? 6 : 0;
  }
}

#define SOUTH 0
#define WEST 1
#define NORTH 2
#define EAST 3

/* WARNING! The order of fields in `RuleT` and `Turmite` matters
 * due to optimizations in `SimulateTurmites`. */
typedef struct Rule {
  short nstate;
  short ndir;
  char ncolor;
} RuleT;

#define RULESZ sizeof(RuleT)

typedef struct Turmite {
  u_short pos;
  short state;
  short dir;
  RuleT rules[3][2];
} TurmiteT;

/* board color is assumed to be binary */
#define RULE(nc, nd, ns)                        \
  (RuleT){ .ncolor = (nc) * RULESZ,             \
           .ndir = (nd) * 2,                    \
           .nstate = (ns) * RULESZ * 2 }
#define POS(x, y) ((y) * WIDTH + (x))

#if 0
static TurmiteT SpiralGrowth = {
  .pos = POS(128, 128),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1,  0, 1),
      RULE(1, -1, 0),
    }, {
      RULE(1, +1, 1),
      RULE(0,  0, 0),
    }
  }
};

static TurmiteT ChaoticGrowth = {
  .pos = POS(128, 128),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1, +1, 1),
      RULE(1, -1, 1),
    }, {
      RULE(1, +1, 1),
      RULE(0, +1, 0),
    }
  }
};

static TurmiteT SnowFlake = {
  .pos = POS(128, 128),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1, -1, 1),
      RULE(1, +1, 0),
    }, {
      RULE(1, +2, 1),
      RULE(1, +2, 2),
    }, {
      RULE(0, +2, 0),
      RULE(0, +2, 0),
    }
  }
};

static TurmiteT Irregular = {
  .pos = POS(128, 128),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1, +1, 0),
      RULE(1, +1, 1),
    }, {
      RULE(0, 0, 0),
      RULE(0, 0, 1),
    }
  }
};
#endif

static TurmiteT Credits = {
  .pos = POS(128,128),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(0, 1, 1),
      RULE(0, -1, 0),
    }, {
      RULE(1, -1, 1),
      RULE(1, 0, 0),
    }
  }
};

#define BPLSIZE (WIDTH * HEIGHT / 8)

#if 0
static inline void BitValue(u_char *bpl, short offset, u_char bit, u_char val) {
  if (val)
    bset(bpl + offset, bit);
  else
    bclr(bpl + offset, bit);
}

static inline void PixelValue(u_char *bpl, u_char bit, u_char val) {
  BitValue(bpl, BPLSIZE * 0, bit, val & 1);
  BitValue(bpl, BPLSIZE * 1, bit, val & 2);
  BitValue(bpl, BPLSIZE * 2, bit, val & 4);
  BitValue(bpl, BPLSIZE * 3, bit, val & 8);
}

static inline void SetPixel(u_char *bpl, u_short pos, u_char val) {
  int offset = pos >> 3;
  u_char bit = ~pos;

  bpl += offset;
  
  switch (val >> 4) {
    case 0: PixelValue(bpl, bit, 0); break;
    case 1: PixelValue(bpl, bit, 1); break;
    case 2: PixelValue(bpl, bit, 2); break;
    case 3: PixelValue(bpl, bit, 3); break;
    case 4: PixelValue(bpl, bit, 4); break;
    case 5: PixelValue(bpl, bit, 5); break;
    case 6: PixelValue(bpl, bit, 6); break;
    case 7: PixelValue(bpl, bit, 7); break;
    case 8: PixelValue(bpl, bit, 8); break;
    case 9: PixelValue(bpl, bit, 9); break;
    case 10: PixelValue(bpl, bit, 10); break;
    case 11: PixelValue(bpl, bit, 11); break;
    case 12: PixelValue(bpl, bit, 12); break;
    case 13: PixelValue(bpl, bit, 13); break;
    case 14: PixelValue(bpl, bit, 14); break;
    case 15: PixelValue(bpl, bit, 15); break;
  }
}
#else
static inline void SetPixel(u_char *bpl, u_short pos, u_char val) {
  int offset = pos >> 3;
  u_char bit = ~pos;

  bpl += offset;

  /*
   * Each block of code that sets a single pixel has exactly 16 bytes.
   * Since color is stored in `board` cell on upper 5 bits, we can choose
   * upper four bit to index one of the code blocks.
   *
   * Looking at disassembly helps to understand what really happened here.
   */

  asm volatile(
    "       lea     .end(pc),a1\n"
    "       andi.w  #0xf0,%2\n"
    "       jmp     .start(pc,%2.w)\n"

    ".start:\n"

    "       bclr    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bclr    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bclr    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bclr    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bclr    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    "       jmp     (a1)\n"

    "       bset    %1,(%0)\n"
    "       bset    %1,8192(%0)\n"
    "       bset    %1,16384(%0)\n"
    "       bset    %1,24576(%0)\n"
    ".end:\n"
    :
    : "a" (bpl), "d" (bit), "d" (val)
    : "2", "a1", "memory", "cc");
}
#endif

#if GENERATION
static u_char generation = 0;
#endif

static TurmiteT *TheTurmite =
#if GENERATION
  &Credits;
#else
  &Irregular;
#endif

#define GetPosChange(dir) (*(short *)((void *)PosChange + (dir)))

static void SimulateTurmite(TurmiteT *t asm("a2"), u_char *board asm("a3"),
                            u_char *bpl asm("a6")) {
  static const u_short PosChange[4] = {
    [SOUTH] = +WIDTH,
    [WEST] = -1,
    [NORTH] = -WIDTH,
    [EAST] = +1,
  };

  short n = NSTEPS - 1;
  short state = t->state;
  short dir = t->dir;

#if GENERATION
  u_char val = generation;
#else
  u_char val;
#endif

  do {
    int pos = t->pos;
    char col = board[pos];
    short *rule;

    {
      short offset = (col & (STEP - 1)) + state;
      rule = (void *)t->rules + offset;
    }

    state = *rule++; /* r->nstate */
    dir = (dir + (*rule++)) & 7;  /* r->ndir */
    t->pos += GetPosChange(dir);

    {
      char newcol = *(char *)rule; /* r->ncolor */

#if !GENERATION
      val = col & -STEP;
      val += STEP;
      if (val == 0)
        val -= STEP;
      newcol |= val;
#endif

      board[pos] = newcol;
    }

    SetPixel(bpl, pos, val);
  } while (--n != -1);

  t->state = state;
  t->dir = dir;

#if GENERATION
  generation += STEP;
#endif
}

static void ResetTurmite(TurmiteT *t) {
  t->pos = POS(128, 128);
  t->dir = 0;
  t->state = 0;
}

static void Init(void) {
  board = MemAlloc(WIDTH * HEIGHT, MEMF_PUBLIC|MEMF_CLEAR);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  SetupMode(MODE_LORES, DEPTH);
  LoadPalette(&turmite_pal, 0);

  EnableDMA(DMAF_BLITTER);
  BlitterCopyFastSetup(screen, 0, 0, turmite_credits[0]);
  BlitterCopyFastStart(DEPTH - 1, 0);
  BitmapToBoard(turmite_credits[0], board);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);
  CopListActivate(cp);

  ResetTurmite(TheTurmite);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER|DMAF_COPPER);
  DeleteCopList(cp);
  DeleteBitmap(screen);
  MemFree(board);
}

PROFILE(SimulateTurmite);

static void Render(void) {
  ProfilerStart(SimulateTurmite);
  SimulateTurmite(TheTurmite, board, screen->planes[0]);
  ProfilerStop(SimulateTurmite);

  TaskWaitVBlank();
}

EFFECT(Turmite, Load, NULL, Init, Kill, Render);
