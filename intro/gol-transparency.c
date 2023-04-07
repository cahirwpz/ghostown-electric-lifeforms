extern TrackT GOLCellColor;
extern TrackT GOLLogoColor;
extern TrackT GOLLogoFade;

static inline u_short _ColorTransition(u_short from, u_short to, u_short step) {
  short r = (from & 0xf00) | ((to >> 4) & 0x0f0) | step;
  short g = ((from << 4) & 0xf00) | (to & 0x0f0) | step;
  short b = ((from << 8) & 0xf00) | ((to << 4) & 0x0f0) | step;

  r = colortab[r];
  g = colortab[g];
  b = colortab[b];

  return (r << 4) | g | (b >> 4);
}


static void ColorFadingStep(short *pal) {
  short i;
  short s = TrackValueGet(&GOLLogoFade, frameCount);
  short cell_col = 0;
  short logo_col = 0;

  switch (TrackValueGet(&GOLCellColor, frameCount)) {
    case 0: cell_col = 0; break;
    case 1: cell_col = s; break;
    case 2: cell_col = 15 - s; break;
    case 3: cell_col = 15; break;
  }

  switch (TrackValueGet(&GOLLogoColor, frameCount)) {
    case 0: logo_col = 0; break;
    case 1: logo_col = s; break;
    case 2: logo_col = 15 - s; break;
    case 3: logo_col = 15; break;
  }

  for (i = 0; i < 8; i++) {
    CopInsSet16(palptr + i, _ColorTransition(pal[i], 0x000, cell_col));
    CopInsSet16(palptr + i + 8, _ColorTransition(pal[i], 0xfff, logo_col));
  }
}
