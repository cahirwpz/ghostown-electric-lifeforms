#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <sprite.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

#include "data/electric-lifeforms.c"

static CopListT *cp;

static void Init(void) {
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadPalette(&logo_pal, 0);

  cp = NewCopList(40);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, &logo, DEPTH);
  CopEnd(cp);

  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  DeleteCopList(cp);
}

static void Render(void) {
  TaskWaitVBlank();
}

EFFECT(Logo, NULL, NULL, Init, Kill, Render);
