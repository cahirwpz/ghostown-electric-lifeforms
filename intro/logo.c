#include <intro.h>
#include <blitter.h>
#include <copper.h>
#include <color.h>
#include <sync.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

#include "data/ghostown-logo-01.c"
#include "data/ghostown-logo-02.c"
#include "data/ghostown-logo-03.c"
#include "data/ghostown-logo-crop.c"

static CopListT *cp;
static BitmapT *screen;

static const PaletteT *ghostown_logo_pal[] = {
  NULL,
  &ghostown_logo_1_pal,
  &ghostown_logo_2_pal,
  &ghostown_logo_3_pal,
};

extern TrackT GhostownLogoPal;

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  {
    short i = 0;

    for (i = 0; i < (1 << DEPTH); i++)
      SetColor(i, 0xf00);
  }

  EnableDMA(DMAF_BLITTER);
  BitmapCopy(screen, (WIDTH - ghostown_logo_width) / 2,
             (HEIGHT - ghostown_logo_height) / 2, &ghostown_logo);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);

  TrackInit(&GhostownLogoPal);

  cp = NewCopList(40);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);

  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  DeleteCopList(cp);

  DeleteBitmap(screen);
}

static void Render(void) {
  short num = TrackValueGet(&GhostownLogoPal, frameCount);
  short frame = FromCurrKeyFrame(&GhostownLogoPal);

  if (num > 0) {
    if (frame < 16) {
      short i;

      for (i = 0; i < (1 << DEPTH); i++) {
        short prev = (num == 1) ? 0xf00 : ghostown_logo_pal[num - 1]->colors[i];
        short curr = ghostown_logo_pal[num]->colors[i];
        SetColor(i, ColorTransition(prev, curr, frame));
      }
    } else {
      LoadPalette(ghostown_logo_pal[num], 0);
    }
  }

  TaskWaitVBlank();
}

EFFECT(Logo, NULL, NULL, Init, Kill, Render);
