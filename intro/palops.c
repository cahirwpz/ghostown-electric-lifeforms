#include "palops.h"
#include <color.h>

extern short frameFromStart;
extern short frameTillEnd;

static void FadeBlack(PaletteT *pal, CopInsT *ins, short step) {
  short n = pal->count;
  char *c = (u_char *)pal->colors;

  if (step < 0)
    step = 0;
  if (step > 15)
    step = 15;

  while (--n >= 0) {
    short r = *c++;
    short g = *c++;
    short b = *c++;

    r = (r & 0xf0) | step;
    g = (g & 0xf0) | step;
    b = (b & 0xf0) | step;

    r = colortab[r];
    g = colortab[g];
    b = colortab[b];
    
    CopInsSet16(ins++, (r << 4) | g | (b >> 4));
  }
}

void FadeIn(PaletteT *pal, CopInsT *ins) {
  FadeBlack(pal, ins, frameFromStart);
}

void FadeOut(PaletteT *pal, CopInsT *ins) {
  FadeBlack(pal, ins, frameTillEnd);
}

