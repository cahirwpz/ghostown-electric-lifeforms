#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <color.h>
#include <fx.h>
#include <gfx.h>
#include <line.h>
#include <stdlib.h>
#include <sync.h>
#include <system/memory.h>
#include <system/interrupt.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 4

#define GRADIENTL 33

#define DIAMETER 32
#define NARMS 15 /* must be power of two minus one */

static CopListT *cp0;
static CopListT *cp1;
static CopInsT *bplptr[DEPTH];
static BitmapT *screen;

#include "data/anemone-pal-1.c"
#include "data/anemone-pal-1-dark.c"
#include "data/anemone-pal-1-light.c"

#include "data/anemone-pal-2.c"
#include "data/anemone-pal-2-dark.c"
#include "data/anemone-pal-2-light.c"

#include "data/anemone-pal-3.c"
#include "data/anemone-pal-3-dark.c"
#include "data/anemone-pal-3-light.c"

#include "data/anemone-gradient.c"

#include "data/circles.c"

static const BitmapT *circles[DIAMETER / 2] = {
  &circle1,
  &circle2,
  &circle3,
  &circle4,
  &circle5,
  &circle6,
  &circle7,
  &circle8,
  &circle9,
  &circle10,
  &circle11,
  &circle12,
  &circle13,
  &circle14,
  &circle15,
  &circle16
};

static short lightLevel = 0;
static short gradientLevel = 15;

extern TrackT SeaAnemoneVariant;
extern TrackT SeaAnemonePal;
extern TrackT SeaAnemonePalPulse;
extern TrackT SeaAnemoneGradient;

typedef const PaletteT *SeaAnemonePalT[4];

static SeaAnemonePalT sea_anemone_palettes = {
  NULL, 
  &anemone_pal_1,
  &anemone_pal_2,
  &anemone_pal_3,
};

static SeaAnemonePalT anemone1_pal = {
  NULL,
  &anemone_pal_1_light,
  &anemone_pal_1,
  &anemone_pal_1_dark,
};

static SeaAnemonePalT anemone2_pal = {
  NULL,
  &anemone_pal_2_light,
  &anemone_pal_2,
  &anemone_pal_2_dark,
};

static SeaAnemonePalT anemone3_pal = {
  NULL,
  &anemone_pal_3_light,
  &anemone_pal_3,
  &anemone_pal_3_dark,
};

static SeaAnemonePalT *sea_anemone_pal[4] = {
  NULL,
  &anemone1_pal,
  &anemone2_pal,
  &anemone3_pal,
};

static SeaAnemonePalT *active_pal = &anemone1_pal;

static const short blip_sequence[] = {
  0,
  2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
};

short gradient[GRADIENTL * 2] = {
  0, 1,
  4, 4,
  8, 8,
  12, 12,
  27, 13,
  34, 12,
  36, 13,
  39, 12,
  40, 11,
  42, 12,
  43, 11,
  46, 10,
  47, 11,
  48, 10,
  50, 11,
  51, 10,
  55, 9,
  60, 8,
  70, 7,
  77, 6,
  78, 7,
  79, 6,
  86, 5,
  87, 6,
  88, 5,
  95, 4,
  97, 5,
  98, 4,
  104, 3,
  113, 2,
  118, 1,
  127, 1,
};



bool gradient_ascending = false;
bool gradient_active = false;

static inline int fastrand(void) {
  static int m[2] = { 0x3E50B28C, 0xD461A7F9 };

  int a, b;

  // https://www.atari-forum.com/viewtopic.php?p=188000#p188000
  asm volatile("move.l (%2)+,%0\n"
               "move.l (%2),%1\n"
               "swap   %1\n"
               "add.l  %0,(%2)\n"
               "add.l  %1,-(%2)\n"
               : "=d" (a), "=d" (b)
               : "a" (m));
  
  return a;
}

#define random fastrand

typedef struct Arm {
  short diameter;
  union { // Q4.12
    short word;
    char byte;
  } _vel_x;
  union { // Q4.12
    short word;
    char byte;
  } _vel_y;
  short pos_x, pos_y; // Q12.4
  short angle;
  short pad[2];
} ArmT;

#define vel_x _vel_x.word
#define vel_y _vel_y.word
#define vel_x_b _vel_x.byte
#define vel_y_b _vel_y.byte

typedef struct ArmQueue {
  short head; // points to first unused entry
  short tail; // points to the last element
  ArmT elems[NARMS + 1]; // contains one empty guard element
} ArmQueueT;

static ArmQueueT AnemoneArms;

static inline ArmT *ArmGet(ArmQueueT *arms, short i) {
  return &arms->elems[i & NARMS];
}

static inline ArmT *ArmFirst(ArmQueueT *arms) {
  return ArmGet(arms, arms->head - 1);
}

static inline ArmT *ArmLast(ArmQueueT *arms) {
  return ArmGet(arms, arms->tail);
}

static inline ArmT *ArmPrev(ArmQueueT *arms, ArmT *arm) {
  arm--;
  return arm < arms->elems ? &arms->elems[NARMS] : arm;
}

static inline void ArmsReset(ArmQueueT *arms) {
  arms->head = arms->tail = 0;
}

static inline bool ArmsNonEmpty(ArmQueueT *arms) {
  return arms->head != arms->tail;
}

static inline bool ArmsFull(ArmQueueT *arms) {
  return ((arms->head + 1) & NARMS) == arms->tail;
}

static short ArmVariant = 0;

static void MakeArm(ArmQueueT *arms, ArmT *arm) {
  u_int arand = random();

  if (ArmVariant == 1) {
    arm->pos_x = fx4i((arand & 255) + 64);
    if (arms->head % 2 == 0) {
      arm->pos_y = fx4i(DIAMETER);
    } else {
      arm->pos_y = fx4i(HEIGHT - DIAMETER);
    }
  } else if (ArmVariant == 2) {
    arm->angle = arand;
    arm->pos_x = fx4i(WIDTH / 2) + (short)((COS(arm->angle) * 120) >> 8);
    arm->pos_y = fx4i(HEIGHT - 60) + (short)((SIN(arm->angle) * 50) >> 8);
  } else if (ArmVariant == 3 || ArmVariant == 4) {
    arm->angle = arand;
    arm->pos_x = fx4i(WIDTH / 2) + (short)((COS(arm->angle) * 60) >> 8);
    arm->pos_y = fx4i(HEIGHT / 2) + (short)((SIN(arm->angle) * 60) >> 8);
  }

  arm->vel_x = 0;
  arm->vel_y = 0;
  arm->diameter = mod16(random() & 0x7fff, DIAMETER / 3) + DIAMETER * 2 / 3;
}

static void ArmsAdd(ArmQueueT *arms, ArmT *arm) {
  short prev, curr;

  if (ArmsFull(arms))
    return;

  prev = (arms->head - 1) & NARMS;
  curr = arms->head;

  while (curr != arms->tail && arm->diameter <= arms->elems[prev].diameter) {
    arms->elems[curr] = arms->elems[prev];      
    curr = prev;
    prev = (prev - 1) & NARMS;
  }

  arms->elems[curr] = *arm;
  arms->head = (arms->head + 1) & NARMS;
}

static inline void ArmsPop(ArmQueueT *arms) {
  arms->tail = (arms->tail + 1) & NARMS;
}

static void ArmMove(ArmT *arm, short angle) {
  short vx = arm->vel_x;
  short vy = arm->vel_y;
  int magSq;
  int diameter;

  vx += COS(angle) >> 1;
  vy += SIN(angle) >> 1;

  magSq = vx * vx + vy * vy; // Q8.24
  diameter = arm->diameter << 24;

  if (magSq > diameter) {
    // short scale = div16(diameter, normfx(magSq));
    // `magSq` does get truncated if converted to 16-bit,
    // so it's not eligible for 16-bit division.
    short scale = diameter / (magSq >> 12);
    vx = normfx(vx * scale);
    vy = normfx(vy * scale);
  }

  arm->vel_x = vx;
  arm->vel_y = vy;

  arm->pos_x += arm->vel_x_b;
  arm->pos_y += arm->vel_y_b;

  arm->diameter--;
}

static int PaletteBlip(void) {
  if (lightLevel) {
    LoadPalette((*active_pal)[blip_sequence[lightLevel]], 0);
    lightLevel--;
  }
  if (gradientLevel > 0 && gradient_ascending) {
    gradientLevel--;
    if (gradientLevel == 0) {
      gradientLevel = 15;
      gradient_active = false;
    }
  }
  if (gradientLevel <= 15 && !gradient_ascending) {
    gradientLevel++;
    if (gradientLevel > 15) {
      gradient_ascending = true;
      gradientLevel = 0;
    }
  }

  return 0;
}

INTSERVER(PulsatePaletteInterrupt, 0, (IntFuncT)PaletteBlip, NULL);

static void MakeCopperList(CopListT *cp) {
  short j, ee, aa;
  short *ptr = gradient;
  short *ptr1 = gradient + (GRADIENTL * 2);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  if (gradient_active) {
  for(j=1;j<GRADIENTL;j++) {
    CopWaitSafe(cp, Y(*(ptr++)), 0);
    if(!gradient_ascending)
      CopSetColor(cp, 0, ColorTransition(anemone_gradient.colors[*(ptr++)], 0x001, gradientLevel));
    else
      CopSetColor(cp, 0, ColorTransition(0x001, anemone_gradient.colors[*(ptr++)], gradientLevel));
  }
  ptr = gradient;
  for(j=1;j<GRADIENTL;j++) {
    ee = *(ptr1--);
    ee = *(ptr1--);
    aa = Y(HEIGHT/2 + *(ptr++));
    CopWaitSafe(cp, aa, 0);
    if(!gradient_ascending)
      CopSetColor(cp, 0, ColorTransition(anemone_gradient.colors[ee], 0, gradientLevel));
    else
      CopSetColor(cp, 0, ColorTransition(0, anemone_gradient.colors[ee], gradientLevel));

    *ptr = *ptr++;
  }
  }
  CopEnd(cp);
}

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT * 4, DEPTH);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadPalette(&anemone_pal_1, 0);

  cp0 = NewCopList(140);
  cp1 = NewCopList(140);
  MakeCopperList(cp0);
  CopListActivate(cp0);

  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);

  TrackInit(&SeaAnemoneVariant);
  TrackInit(&SeaAnemonePal);
  TrackInit(&SeaAnemonePalPulse);
  TrackInit(&SeaAnemoneGradient);
  ArmsReset(&AnemoneArms);

  /* Moved from DrawCircle, since we use only one type of blit. */
  {
    WaitBlitter();
    custom->bltcon1 = 0;
    custom->bltafwm = -1;
  }

  AddIntServer(INTB_VERTB, PulsatePaletteInterrupt);
}

static void Kill(void) {
  RemIntServer(INTB_VERTB, PulsatePaletteInterrupt);
  DisableDMA(DMAF_COPPER | DMAF_BLITTER | DMAF_RASTER | DMAF_BLITHOG);

  DeleteBitmap(screen);
  DeleteCopList(cp0);
  DeleteCopList(cp1);
}

#define screen_bytesPerRow (WIDTH / 8)

static void DrawCircle(const BitmapT *circle, short x, short y, short c,
                       int vShift)
{
  u_short dstmod = screen_bytesPerRow - circle->bytesPerRow;
  u_short bltshift = rorw(x & 15, 4);
  u_short bltsize = (circle->height << 6) | (circle->bytesPerRow >> 1);
  void *srcbpt = circle->planes[0];
  void **dstbpts = screen->planes;
  int start;

  start = (y + vShift) * screen_bytesPerRow;
  start += (x & ~15) >> 3;

  WaitBlitter();

  if (bltshift) {
    bltsize++, dstmod -= 2; 

    custom->bltalwm = 0;
    custom->bltamod = -2;
  } else {
    custom->bltalwm = -1;
    custom->bltamod = 0;
  }

  custom->bltbmod = dstmod;
  custom->bltdmod = dstmod;
  
  {
    short n = DEPTH - 1;

    do {
      void *dstbpt = (*dstbpts++) + start;
      u_short bltcon0 = bltshift;

      if (c & 1) {
        bltcon0 |= SRCA | SRCB | DEST | A_OR_B;
      } else {
        bltcon0 |= SRCA | SRCB | DEST | NOT_A_AND_B;
      }

      WaitBlitter();

      custom->bltcon0 = bltcon0;
      custom->bltapt = srcbpt;
      custom->bltbpt = dstbpt;
      custom->bltdpt = dstbpt;
      custom->bltsize = bltsize;

      c >>= 1;
    } while (--n != -1);
  }
}

static void SeaAnemone(ArmQueueT *arms, int vShift) {
  static ArmT arm;

  MakeArm(arms, &arm);
  ArmsAdd(arms, &arm);

  if (ArmsNonEmpty(arms)) {
    ArmT *curr = ArmFirst(arms);
    ArmT *last = ArmLast(arms);

    while (true) {
      short angle;

      {
        short qy = curr->pos_y >> 4;
        short arand = random();

        if (ArmVariant == 1) {
          angle = arand & 0x7ff;
          if (qy > HEIGHT / 2)
            angle += 0x800;
        } else if (ArmVariant == 3) {
          angle = ((arand & 0x7ff) - 0x400) + curr->angle;
        } else if (ArmVariant == 4) {
          angle = (arand & 0x7ff) + curr->angle;
        } else {
          angle = (arand & 0x7ff) + 0x800;
        }
      }

      ArmMove(curr, angle);

      if (curr->diameter > 1) {
        short d = curr->diameter;
        short r = d / 2;
        short x = (curr->pos_x >> 4) - r;
        short y = (curr->pos_y >> 4) - r;
        if ((x < 0) || (y < 0) || (x >= WIDTH - d) || (y >= HEIGHT - d))
          continue;
        if (r < 16)
          DrawCircle(circles[r - 1], x, y, 16 - r, vShift);
      }
      if (curr == last)
        break;
      curr = ArmPrev(arms, curr);
    }

    while (ArmsNonEmpty(arms) && ArmLast(arms)->diameter < 1) {
      ArmsPop(arms);
    }
  }
}

PROFILE(SeaAnemone);

static void Render(void) {
  short vShift = 0;
  int lineOffset = 0;
  short val, valPal, valGradient;

  if ((val = TrackValueGet(&SeaAnemoneVariant, frameFromStart))) { 
    BitmapClear(screen);
    ArmsReset(&AnemoneArms);
    ArmVariant = val;
  }

  if ((val = TrackValueGet(&SeaAnemonePal, frameFromStart))) {
    LoadPalette(sea_anemone_palettes[val], 0);
    active_pal = sea_anemone_pal[val];
  }

  // Set the light level (for palette modification)
  if ((valPal = TrackValueGet(&SeaAnemonePalPulse, frameFromStart)))
    lightLevel = valPal;

  if ((valGradient = TrackValueGet(&SeaAnemoneGradient, frameFromStart))) {
    gradientLevel = 0;
    gradient_ascending = false;
    gradient_active = true;
  }

  ProfilerStart(SeaAnemone);

  MakeCopperList(cp1);
  // Scroll the screen vertically
  if (ArmVariant == 4) {
    vShift = frameCount % (HEIGHT * 3);
    lineOffset = vShift * screen_bytesPerRow;

    CopInsSet32(bplptr[0], screen->planes[0] + lineOffset);
    CopInsSet32(bplptr[1], screen->planes[1] + lineOffset);
    CopInsSet32(bplptr[2], screen->planes[2] + lineOffset);
    CopInsSet32(bplptr[3], screen->planes[3] + lineOffset);
  }

  SeaAnemone(&AnemoneArms, vShift);
  CopListRun(cp1); 
  ProfilerStop(SeaAnemone);

  TaskWaitVBlank();
  swapr(cp0, cp1);
}

EFFECT(SeaAnemone, NULL, NULL, Init, Kill, Render);
