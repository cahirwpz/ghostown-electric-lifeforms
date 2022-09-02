#include "effect.h"

#include "custom.h"
#include "copper.h"
#include "blitter.h"
#include "bitmap.h"
#include "sprite.h"
#include "fx.h"

#include "data/bar.c"
#include "data/bar-2.c"
#include "data/colors.c"
#include "data/stripes.c"

#define bar_bplmod ((bar_width - WIDTH) / 8 - 2)

#define WIDTH (320 - 32)
#define HEIGHT 256
#define DEPTH 4
#define NSPRITES 8

#define BELOW 0
#define ABOVE BPLCON2_PF2P2

#define O0 0
#define O1 56
#define O2 112
#define O3 172
#define O4 224

#define STRIPES 5
#define BARS 4

typedef struct State {
  CopInsT *sprite;
  /* two halves of the screen: 4 bitplane pointers and bplcon1 */
  CopInsT *bar[4];
  /* for each line five horizontal positions */
  u_short *stripes[HEIGHT];
} StateT;

static int active = 1;
static CopListT *cp[2];
static short sintab8[128 * 4];
static StateT state[2];

/* These numbers must be odd due to optimizations. */
static char StripePhase[STRIPES] = { 4, 24, 16, 8, 12 };
static char StripePhaseIncr[STRIPES] = { 8, -10, 14, -6, 6 };

static inline void CopSpriteSetHP(CopListT *cp, short n) {
  CopMove16(cp, spr[n * 2 + 0].pos, 0);
  CopMove16(cp, spr[n * 2 + 1].pos, 0);
}

static void MakeCopperList(CopListT *cp, StateT *state) {
  short b, y;

  CopInit(cp);

  /* Setup initial bitplane pointers. */
  CopMove32(cp, bplpt[0], bar.planes[0]);
  CopMove32(cp, bplpt[1], bar.planes[1]);
  CopMove32(cp, bplpt[2], bar.planes[2]);
  CopMove32(cp, bplpt[3], bar.planes[3]);
  CopMove16(cp, bplcon1, 0);

  /* Move back bitplane pointers to repeat the line. */
  CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
  CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);

  /* Load default sprite settings */
  CopMove32(cp, sprpt[0], &stripes0_sprdat); /* up */
  CopMove32(cp, sprpt[1], &stripes1_sprdat);
  CopMove32(cp, sprpt[2], &stripes2_sprdat); /* down */
  CopMove32(cp, sprpt[3], &stripes3_sprdat);
  CopMove32(cp, sprpt[4], &stripes0_sprdat); /* up */
  CopMove32(cp, sprpt[5], &stripes1_sprdat);
  CopMove32(cp, sprpt[6], &stripes2_sprdat); /* down */
  CopMove32(cp, sprpt[7], &stripes3_sprdat);

  CopWait(cp, Y(-1), 0);

  state->sprite = cp->curr;
  CopMove32(cp, sprpt[0], stripes0_sprdat.data); /* up */
  CopMove32(cp, sprpt[1], stripes1_sprdat.data);
  CopMove32(cp, sprpt[2], stripes2_sprdat.data); /* down */
  CopMove32(cp, sprpt[3], stripes3_sprdat.data);
  CopMove32(cp, sprpt[4], stripes0_sprdat.data); /* up */
  CopMove32(cp, sprpt[5], stripes1_sprdat.data);
  CopMove32(cp, sprpt[6], stripes2_sprdat.data); /* down */
  CopMove32(cp, sprpt[7], stripes3_sprdat.data);

  for (y = 0, b = 0; y < HEIGHT; y++) {
    short vp = Y(y);
    short my = y & 63;

    CopWaitSafe(cp, vp, 0);

    if (my == 0) {
      state->bar[y >> 6] = cp->curr;
      CopMove32(cp, bplpt[0], NULL);
      CopMove32(cp, bplpt[1], NULL);
      CopMove32(cp, bplpt[2], NULL);
      CopMove32(cp, bplpt[3], NULL);
      CopMove16(cp, bplcon1, 0);
    } else if (my == 8) {
      if (y & 64) {
        CopLoadPal(cp, &bar_pal, 0);
      } else {
        CopLoadPal(cp, &bar2_pal, 0);
      }
    } else if (my == 16) {
      /* Advance bitplane pointers to display consecutive lines. */
      CopMove16(cp, bpl1mod, bar_bplmod);
      CopMove16(cp, bpl2mod, bar_bplmod);
    } else if (my == 49) {
      /* Move back bitplane pointers to repeat the line. */
      CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
      CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);
    }

    {
      short p0, p1;

      if (y & 64) {
        p0 = BELOW, p1 = ABOVE;
      } else {
        p0 = ABOVE, p1 = BELOW;
      }

      CopWait(cp, vp, HP(O0) + 2);
      state->stripes[y] = (u_short *)cp->curr + 1;
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O1) + 2);
      CopSpriteSetHP(cp, 1);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O2) + 2);
      CopSpriteSetHP(cp, 2);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O3) + 2);
      CopSpriteSetHP(cp, 3);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O4) + 2);
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
    }
  }

  CopEnd(cp);
}

static void UpdateBarState(StateT *state) {
  short w = (bar_width - WIDTH) / 2;
  short f = frameCount * 16;
  short bx = w + normfx(SIN(f) * w);
  short shift, offset, i;

  for (i = 0; i < BARS; i++) {
    CopInsT *ins = state->bar[i];

    offset = (bx >> 3) & -2;
    shift = ~bx & 15;

    if (i & 1)
      offset += bar_bytesPerRow * 33;

    CopInsSet32(&ins[0], bar.planes[0] + offset);
    CopInsSet32(&ins[2], bar.planes[1] + offset);
    CopInsSet32(&ins[4], bar.planes[2] + offset);
    CopInsSet32(&ins[6], bar.planes[3] + offset);
    CopInsSet16(&ins[8], (shift << 4) | shift);

    f += SIN_HALF_PI;
    bx = w + normfx(SIN(f) * w);
  }
}

static void UpdateSpriteState(StateT *state) {
  CopInsT *ins = state->sprite;
  int fu = frameCount & 63;
  int fd = (~frameCount) & 63;

  CopInsSet32(ins + 0, stripes0_sprdat.data + fu); /* up */
  CopInsSet32(ins + 2, stripes1_sprdat.data + fu);
  CopInsSet32(ins + 4, stripes2_sprdat.data + fd); /* down */
  CopInsSet32(ins + 6, stripes3_sprdat.data + fd);
  CopInsSet32(ins + 8, stripes0_sprdat.data + fu); /* up */
  CopInsSet32(ins + 10, stripes1_sprdat.data + fu);
  CopInsSet32(ins + 12, stripes2_sprdat.data + fd); /* down */
  CopInsSet32(ins + 14, stripes3_sprdat.data + fd);
}

#define HPOFF(x) HP(x + 32)

static void UpdateStripeState(StateT *state) {
  static const char offset[STRIPES] = {
    HPOFF(O0), HPOFF(O1), HPOFF(O2), HPOFF(O3), HPOFF(O4) };
  u_char *phasep = StripePhase;
  char *incrp = StripePhaseIncr;
  const char *offsetp = offset;
  short i;

  for (i = 0; i < STRIPES * 8; i += 8) {
    u_int phase = (u_char)*phasep;
    u_short **stripesp = state->stripes;
    short *st = &sintab8[phase];
    short hp_off = *offsetp++;
    short n = HEIGHT / 8 - 1;

    (*phasep++) += (*incrp++);

    do {
      u_short *ins;
      short hp;

#define BODY()                  \
      ins = (*stripesp++) + i;  \
      hp = hp_off + (*st++);    \
      ins[0] = hp;              \
      ins[2] = hp + 8;

      BODY(); BODY(); BODY(); BODY();
      BODY(); BODY(); BODY(); BODY();
    } while (--n != -1);
  }
}

static void MakeSinTab8(void) {
  int i, j;

  for (i = 0, j = 0; i < 128; i++, j += 32)
    sintab8[i] = (sintab[j] + 512) >> 10;

  memcpy(&sintab8[128], &sintab8[0], 128 * sizeof(u_short));
  memcpy(&sintab8[256], &sintab8[0], 256 * sizeof(u_short));
}

#define COPLIST_SIZE (HEIGHT * 22 + 100)

static void Load(void) {
  MakeSinTab8();
}

static void Init(void) {
  SetupDisplayWindow(MODE_LORES, X(16), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(0), WIDTH + 16);
  SetupMode(MODE_LORES, DEPTH);
  LoadPalette(&bar_pal, 0);
  LoadPalette(&stripes_pal, 16);

  /* Place sprites 0-3 above playfield, and 4-7 below playfield. */
  custom->bplcon2 = BPLCON2_PF2PRI | BPLCON2_PF2P1 | BPLCON2_PF1P1;

  SpriteUpdatePos(&stripes0, X(0), Y(0));
  SpriteUpdatePos(&stripes1, X(0), Y(0));
  SpriteUpdatePos(&stripes2, X(0), Y(0));
  SpriteUpdatePos(&stripes3, X(0), Y(0));

  cp[0] = NewCopList(COPLIST_SIZE);
  cp[1] = NewCopList(COPLIST_SIZE);

  MakeCopperList(cp[0], &state[0]);
  MakeCopperList(cp[1], &state[1]);

  UpdateBarState(&state[0]);
  UpdateSpriteState(&state[0]);
  UpdateStripeState(&state[0]);

  CopListActivate(cp[0]);

  Log("CopperList: %ld instructions left\n",
      COPLIST_SIZE - (cp[0]->curr - cp[0]->entry));
  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER | DMAF_COPPER);
  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
  ResetSprites();
}

PROFILE(UpdateStripeState);

static void Render(void) {
  UpdateBarState(&state[active]);
  UpdateSpriteState(&state[active]);

  ProfilerStart(UpdateStripeState);
  UpdateStripeState(&state[active]);
  ProfilerStop(UpdateStripeState);

  CopListRun(cp[active]);
  WaitVBlank();
  active ^= 1;
}

EFFECT(Weave, Load, NULL, Init, Kill, Render);
