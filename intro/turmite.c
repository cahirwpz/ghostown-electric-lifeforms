#include "effect.h"

#include "custom.h"
#include "copper.h"
#include "blitter.h"
#include "bitmap.h"
#include "sprite.h"
#include "fx.h"
#include <sync.h>
#include <system/memory.h>
#include <system/interrupt.h>

#include "data/turmite-pal-1.c"
#include "data/turmite-pal-1-dark.c"
#include "data/turmite-pal-1-light.c"

#include "data/turmite-pal-2.c"
#include "data/turmite-pal-2-dark.c"
#include "data/turmite-pal-2-light.c"

#include "data/turmite-pal-3.c"
#include "data/turmite-pal-3-dark.c"
#include "data/turmite-pal-3-light.c"

#include "data/turmite-credits-1.c"
#include "data/turmite-credits-2.c"
#include "data/turmite-credits-3.c"

#define WIDTH 256
#define HEIGHT 256
#define DEPTH 4
#define NSTEPS (256 + 256 + 80)

#define GENERATION 1

/* don't decrease it since color's value is stored on lower 3 bits of tile */
#define STEP 8

static CopListT *cp;
static BitmapT *screen;

/* higher 5 bits store value, lower 3 bits store color information */
static u_char *board;
static u_int lookup[256][2];

extern TrackT TurmiteBoard;
extern TrackT TurmitePal;

static short activeBoard = 1;
static short lightLevel = 0;

static const BitmapT *turmite_credits[] = {
  NULL,
  &turmite_credits_1,
  &turmite_credits_2,
  &turmite_credits_3,
};

typedef const PaletteT *TurmitePalT[4];

static TurmitePalT turmite_palettes = {
  NULL,
  &turmite_pal_1,
  &turmite_pal_2,
  &turmite_pal_3,
};

static TurmitePalT turmite_pal1 = {
  NULL,
  &turmite_pal_1_light,
  &turmite_pal_1,
  &turmite_pal_1_dark,
};

static TurmitePalT turmite_pal2 = {
  NULL,
  &turmite_pal_2_light,
  &turmite_pal_2,
  &turmite_pal_2_dark,
};

static TurmitePalT turmite_pal3 = {
  NULL,
  &turmite_pal_3_light,
  &turmite_pal_3,
  &turmite_pal_3_dark,
};

static TurmitePalT *active_pal = &turmite_pal1;

static const short blip_sequence[] = {
  0, 2, 1, 1, 1, 2, 2, 2, 3, 3, 3
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

static TurmiteT SquarePattern = {
  .pos = POS(60,60),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1 ,1 ,1),
      RULE(1 ,1 ,0)
    },
    {
      RULE(0 ,1 ,0),
      RULE(1 ,0 ,1)
    },
  }
};

static TurmiteT SquarePattern2 = {
  .pos = POS(160,160),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1 ,1 ,1),
      RULE(1 ,0 ,0)
    },
    {
      RULE(1 ,0 ,0),
      RULE(0 ,0 ,1)
    },
  }
};

static TurmiteT AroundLetters = {
  .pos = POS(60,60),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(0 ,1 ,1),
      RULE(1 ,1 ,1)
    },
    {
      RULE(0 ,-1 ,2),
      RULE(0 ,-1 ,0)
    },
    {
      RULE(1 ,1 ,2),
      RULE(1 ,-1 ,0)
    }
  }
};

static TurmiteT AroundLetters2 = {
  .pos = POS(160,160),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1 ,-1 ,1),
      RULE(0, -1, 0),
    }, 
    {
      RULE(1, 1, 1),
      RULE(1, 1, 0),
    }
  }
};

static TurmiteT Spiral = {
  .pos = POS(60,60),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1 ,1 ,1),
      RULE(0 ,1 ,1)
    },
    {
      RULE(1 ,0 ,0),
      RULE(1 ,0 ,1)
    },
  }
};

static TurmiteT Spiral2 = {
  .pos = POS(160,160),
  .dir = 0,
  .state = 0,
  .rules = {
    {
      RULE(1 ,1 ,0),
      RULE(1 ,1 ,1)
    },
    {
      RULE(0 ,0 ,0),
      RULE(0 ,0 ,1)
    },
  }
};

static TurmiteT *turmite_types[4][2] = {
  { NULL, NULL },
  { &SquarePattern, &SquarePattern2 },
  { &AroundLetters, &AroundLetters2 },
  { &Spiral,  &Spiral2 },
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
}
#endif

#if GENERATION
static u_char generation = 0;
static short gen_inc = 0;
#endif

static TurmiteT *TheTurmite =
#if GENERATION
  &SquarePattern;
#else
  &Irregular;
#endif

static TurmiteT *TheTurmite2 = &SquarePattern2;

static void SimulateTurmite(TurmiteT *t asm("a2"), u_char *board asm("a3"),
                            u_char *bpl asm("a6")) {
  static const u_short PosChange[4] = {
    [SOUTH] = +WIDTH,
    [WEST] = -1,
    [NORTH] = -WIDTH,
    [EAST] = +1,
  };

  short n = NSTEPS - 1;
  u_int pos = t->pos;
  short state = t->state;
  short dir = t->dir;
  void **label = &&end;
  register char m7 asm("d7") = 7;

#if GENERATION
  u_short val = generation & 0xf0;
#else
  u_char val;
#endif

  do {
    char *colp = &board[pos];
    char col = *colp;
    short offset = (col & m7) + state;
    short *rule = (void *)t->rules + offset;

    state = *rule++; /* r->nstate */
    dir = (dir + (*rule++)) & m7;  /* r->ndir */

    {
      char newcol = *(char *)rule; /* r->ncolor */

#if !GENERATION
      val = col & -STEP;
      val += STEP;
      if (val == 0)
        val -= STEP;
      newcol |= val;
#endif

      *colp = newcol;
    }

    /*
     * Each block of code that sets a single pixel has exactly 16 bytes.
     * Since color is stored in `board` cell on upper 5 bits, we can choose
     * upper four bit to index one of the code blocks.
     *
     * Looking at disassembly helps to understand what really happened here.
     */
    asm volatile(
      "       move.w  %1,d0\n"
      "       lsr.w   #3,d0\n"
      "       lea     (%0,d0.w),a0\n"
      "       move.w  %1,d0\n"
      "       not.w   d0\n"
      "       jmp     .start(pc,%2.w)\n"

      ".start:\n"

      "       bclr    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bclr    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bclr    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bclr    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bclr    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      "       jmp     (%3)\n"

      "       bset    d0,(a0)\n"
      "       bset    d0,8192(a0)\n"
      "       bset    d0,16384(a0)\n"
      "       bset    d0,24576(a0)\n"
      :
      : "a" (bpl), "d" (pos), "d" (val), "a" (label)
      : "d0", "a0", "a1", "memory", "cc");

end:

    // pos = (u_short)(pos + *(u_short *)((void *)PosChange + dir));
    asm volatile(
      "       add.w (%1,%2.w),%0\n"
      : "+d" (pos)
      : "a" (PosChange), "d" (dir));
  } while (--n != -1);

  t->pos = pos;
  t->state = state;
  t->dir = dir;

#if GENERATION
 if (generation == 0) {
    gen_inc = STEP;
  }
  if (generation == 248) {
    gen_inc = -STEP;
  }
  if (gen_inc) {
    generation -= STEP;
  } else {
    generation += STEP;
  }
#endif
}

static void ResetTurmite(TurmiteT *t, u_short pos) {
  t->pos = pos;
  t->dir = 0;
  t->state = 0;
}

static void ChooseTurmiteBoard(short i) {
  activeBoard = i;
  BitmapClear(screen);
  LoadPalette(turmite_palettes[i], 0);
  BlitterCopyFastSetup(screen, 0, 0, turmite_credits[i]);
  BlitterCopyFastStart(DEPTH - 1, 0);
  BitmapToBoard(turmite_credits[i], board);
  TheTurmite = turmite_types[i][0];
  TheTurmite2 = turmite_types[i][1];
  ResetTurmite(TheTurmite, POS(60, 60));
  ResetTurmite(TheTurmite2, POS(160, 160));
  if (i == 1) {
    active_pal = &turmite_pal1;
  } else if (i == 2) {
    active_pal = &turmite_pal2;
  } else if (i == 3) {
    active_pal = &turmite_pal3;
  }
}

static int PaletteBlip(void) {
  if (lightLevel) {
    short ll = blip_sequence[lightLevel];
    LoadPalette((*active_pal)[ll], 0);
    lightLevel--;
  }
  return 0;
}

INTSERVER(BlipPaletteInterrupt, 0, (IntFuncT)PaletteBlip, NULL);

static void Init(void) {
  board = MemAlloc(WIDTH * HEIGHT, MEMF_PUBLIC|MEMF_CLEAR);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  SetupMode(MODE_LORES, DEPTH);
  LoadPalette(&turmite_pal_1, 0);

  TrackInit(&TurmiteBoard);
  TrackInit(&TurmitePal);
  
  EnableDMA(DMAF_BLITTER);
  ChooseTurmiteBoard(1);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);

  AddIntServer(INTB_VERTB, BlipPaletteInterrupt);
}

static void Kill(void) {
  RemIntServer(INTB_VERTB, BlipPaletteInterrupt);
  DisableDMA(DMAF_RASTER|DMAF_COPPER);
  DeleteCopList(cp);
  DeleteBitmap(screen);
  MemFree(board);
}

PROFILE(SimulateTurmite);

static void Render(void) {
  short val, valPal;
  if ((valPal = TrackValueGet(&TurmitePal, frameFromStart)))
    lightLevel = valPal;

  if ((val = TrackValueGet(&TurmiteBoard, frameFromStart)))
    ChooseTurmiteBoard(val);

  ProfilerStart(SimulateTurmite);
  SimulateTurmite(TheTurmite, board, screen->planes[0]);
  SimulateTurmite(TheTurmite2, board, screen->planes[0]);
  ProfilerStop(SimulateTurmite);

  TaskWaitVBlank();
}

EFFECT(Turmite, Load, NULL, Init, Kill, Render);
