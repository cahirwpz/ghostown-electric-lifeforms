#ifndef __PALETTE_H__
#define __PALETTE_H__

#include "common.h"
#include "custom.h"

typedef struct Palette {
  u_short *colors;
  short count;
} PaletteT;

void LoadColorArray(const u_short *colors, short count, int start);

#define LoadColors(colors, start) \
  LoadColorArray((colors), nitems(colors), (start))

#define LoadPalette(pal, start) \
  LoadColorArray((pal)->colors, (pal)->count, (start))

static inline void SetColor(u_short i, u_short rgb) {
  custom->color[i] = rgb;
}

#endif
