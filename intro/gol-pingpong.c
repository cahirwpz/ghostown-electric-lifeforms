typedef struct ColorPingPong {
  short cur, dir;
  u_short palette[256][4];
} ColorPingPongT;

static ColorPingPongT *pingpong;

static void ColorPingPongStep(ColorPingPongT *pp) {
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
  short i;

  for (i = 0; i < 256; i++) {
    u_char h = TrackValueGet(&GOLPaletteH, i);
    u_char s = TrackValueGet(&GOLPaletteS, i);
    u_char v = TrackValueGet(&GOLPaletteV, i);

    pp->palette[i][0] = HsvToRgb(h, s, v / 8);
    pp->palette[i][1] = HsvToRgb(h, s, v / 4);
    pp->palette[i][2] = HsvToRgb(h, s, v / 2);
    pp->palette[i][3] = HsvToRgb(h, s, v);
  }

  pp->cur = 0;
  pp->dir = 1;
}
