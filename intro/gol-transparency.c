extern TrackT GOLCellColor;
extern TrackT GOLLogoColor;
extern TrackT GOLLogoFade;

static void ColorFadingStep(short *pal) {
    short i;
    short s = TrackValueGet(&GOLLogoFade, frameCount);
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

    for (i = 0; i < 8; i++) {
      CopInsSet16(palptr + i, ColorTransition(pal[i], 0x000, cell_col));
      CopInsSet16(palptr + i + 8, ColorTransition(pal[i], 0xfff, logo_col));
    }
}