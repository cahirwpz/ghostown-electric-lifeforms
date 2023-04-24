#include <intro.h>
#include <blitter.h>
#include <copper.h>
#include <color.h>
#include <sync.h>
#include <types.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

#include "data/ghostown-logo-01.c"
#include "data/ghostown-logo-02.c"
#include "data/ghostown-logo-03.c"
#include "data/ghostown-logo-crop.c"

static __code bool cleanup;
static CopListT *cp;
static BitmapT *screen;
BitmapT ghostown_logo;

static const PaletteT *ghostown_logo_pal[] = {
  NULL,
  &ghostown_logo_1_pal,
  &ghostown_logo_2_pal,
  &ghostown_logo_3_pal,
};

extern TrackT GhostownLogoPal;

static void Load(void) {
  static __code short done = false;

  if (!done) {
    PixmapToBitmap(&ghostown_logo, ghostown_logo_width, ghostown_logo_height, 3,
                   ghostown_logo_pixels);
    done = true;
  }
}

static void SharedInit(bool out) {
  short i;

  cleanup = true;

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  EnableDMA(DMAF_BLITTER);
  BitmapCopy(screen, (WIDTH - ghostown_logo_width) / 2,
             (HEIGHT - ghostown_logo_height) / 2, &ghostown_logo);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);

  cp = NewCopList(40);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);

  CopListActivate(cp);

  for (i = 0; i < (1 << DEPTH); i++)
    SetColor(i, out ? ghostown_logo_3_pal.colors[i]
                    : ghostown_logo_1_pal.colors[0]);

  EnableDMA(DMAF_RASTER);
}

static void InitIn(void) {
  SharedInit(0);
}

static void InitOut(void) {
  SharedInit(1);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER);
  DeleteCopList(cp);

  DeleteBitmap(screen);
}

void KillLogo(void) {
  static __code bool enabled = false;

  if (enabled) {
    Kill();
  } else {
    enabled = true;
  }
}

static void Render(short out) {
  short num;

  if ((num = TrackValueGet(&GhostownLogoPal, frameCount))) {
    short frame = (out ? TillNextKeyFrame : FromCurrKeyFrame)(&GhostownLogoPal);

    if (frame < 16) {
      short i;

      for (i = 0; i < (1 << DEPTH); i++) {
        short prev = (num == 1) ? ghostown_logo_1_pal.colors[0]
                                : ghostown_logo_pal[num - 1]->colors[i];
        short curr = ghostown_logo_pal[num]->colors[i];
        SetColor(i, ColorTransition(prev, curr, frame));
      }
    } else {
      LoadPalette(ghostown_logo_pal[num], 0);
    }
  }

  TaskWaitVBlank();
}

static void RenderIn(void) {
  Render(0);
}

static void RenderOut(void) {
  Render(1);
}

EFFECT(LogoIn, Load, NULL, InitIn, NULL, RenderIn, NULL);
EFFECT(LogoOut, Load, NULL, InitOut, Kill, RenderOut, NULL);
