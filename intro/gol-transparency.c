extern TrackT GOLCellColor;
extern TrackT GOLLogoColor;
extern TrackT GOLLogoSin;

static void ColorFadingStep(void) {
    short i;
    short s = TrackValueGet(&GOLLogoSin, frameCount);
    short cell_col = 0;
    short logo_col = 0;

    switch(TrackValueGet(&GOLCellColor, frameCount)) {
        case 0: cell_col = 0; break;
        case 1: cell_col = s; break;
        case 2: cell_col = 15-s; break;
        case 3: cell_col = 15; break;
    }

    switch(TrackValueGet(&GOLLogoColor, frameCount)) {
        case 0: logo_col = 0; break;
        case 1: logo_col = s; break;
        case 2: logo_col = 15-s; break;
        case 3: logo_col = 15; break;
    }

    for (i = 0; i < 8; i++)
      CopInsSet16(palptr + i, ColorTransition(palette_vitruvian.colors[i], 0x000, cell_col));
    for (i = 0; i < 8; i++)
      CopInsSet16(palptr + i + 8, ColorTransition(palette_vitruvian.colors[i & 7], 0xfff, logo_col));
}