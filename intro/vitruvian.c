#include <intro.h>
#include <blitter.h>
#include <copper.h>
#include <color.h>
#include <sync.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

#include "data/electric-lifeforms-01.c"
#include "data/electric-lifeforms-02.c"

static CopListT *cp;
static BitmapT *screen;
static BitmapT electric_lifeforms;

static const u_short *electric_lifeforms_colors[] = {
  NULL,
  electric_lifeforms_1_colors,
  electric_lifeforms_2_colors,
};

extern TrackT ElectricLifeformsLogoPal;

static void Load(void) {
  PixmapToBitmap(&electric_lifeforms, electric_lifeforms_width,
                 electric_lifeforms_height, 3, electric_lifeforms_pixels);
}

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  memcpy(screen->planes[0], electric_lifeforms.planes[0],
         WIDTH * HEIGHT * DEPTH / 8);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  {
    short i = 0;

    for (i = 0; i < (1 << DEPTH); i++)
      SetColor(i, electric_lifeforms_1_colors[0]);
  }

  cp = NewCopList(40);
  CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  DeleteCopList(cp);

  DeleteBitmap(screen);
}

static bool initialFadeIn = true;

static void Render(void) {
  short num = TrackValueGet(&ElectricLifeformsLogoPal, frameCount);
  short frame = FromCurrKeyFrame(&ElectricLifeformsLogoPal);

  if (num > 0) {
    if (frame < 16) {
      short i;
      
      // Initially, we want to fade in the full palette.
      // Then, only flicker the right side of the bitmap.
      for (i = initialFadeIn ? 0 : 5; i < (1 << DEPTH); i++) {
        short prev = (num == 1) ? electric_lifeforms_1_colors[0]
          : electric_lifeforms_colors[num - 1][i];
        short curr = electric_lifeforms_colors[num][i];
        SetColor(i, ColorTransition(prev, curr, frame));
      }
    } else {
      LoadColorArray(electric_lifeforms_colors[num],
                     electric_lifeforms_1_colors_count, 0);
      initialFadeIn = false;
    }
  }

  TaskWaitVBlank();
}

EFFECT(Vitruvian, Load, NULL, Init, Kill, Render, NULL);
