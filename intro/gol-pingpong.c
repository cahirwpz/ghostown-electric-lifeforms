typedef struct ColorPingPong {
  short cur, dir;
  u_short palette[256][4];
} ColorPingPongT;

static ColorPingPongT *pingpong;

static void ColorPingPongStep(CopInsT *palptr, ColorPingPongT *pp) {
  u_short *pal = pp->palette[pp->cur];
  short i;

  // unrolling this loop gives worse performance for some reason
  // increment `pal` on every power of 2, essentially setting
  // 4 consecutive colors from `pal` to 1, 2, 4 and 8 colors respectively
  for (i = 1; i < COLORS;) {
    short j, next_i = i << 1;
    CopInsT *ins = &palptr[i];
    for (j = i; j < next_i; j++)
      CopInsSet16(ins++, *pal);
    i = next_i;
    pal++;
  }

  pp->cur += pp->dir;

  if (pp->cur == 0 && pp->dir < 0)
    pp->dir = -pp->dir;
  if (pp->cur == 255 && pp->dir > 0)
    pp->dir = -pp->dir;
}

static void MakeColorPingPong(ColorPingPongT *pp) {
  u_short *pal = (u_short *)pp->palette;
  short i;

  pp->cur = 0;
  pp->dir = 1;

  for (i = 0; i < 256; i++) {
    short h = TrackValueGet(&GOLPaletteH, i);
    short s = TrackValueGet(&GOLPaletteS, i);
    short v = TrackValueGet(&GOLPaletteV, i);

    *pal++ = HsvToRgb(h, s, v / 8);
    *pal++ = HsvToRgb(h, s, v / 4);
    *pal++ = HsvToRgb(h, s, v / 2);
    *pal++ = HsvToRgb(h, s, v);
  }
}
