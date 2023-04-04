#include "data/gol-transitions.c"

extern TrackT GOLGradientTrans;
extern TrackT GOLLogoType;

static short palette_vitruvian[16] = {
   0x000,
   0x3bf,
   0x9ee,
   0x9ee,
   0xdff,
   0xdff,
   0xdff,
   0xdff,
   0xfff,
   0xfff,
   0xfff,
   0xfff,
   0xfff,
   0xfff,
   0xfff,
   0xfff,
};

static short palette_gol[16] = {0};

static short* LoadCompressedPal(short *from, short *to) {
  *to++ = *from++;
  *to++ = *from;
  *to++ = *from++;
  *to++ = *from;
  *to++ = *from;
  *to++ = *from;
  *to++ = *from++;
  return from;
}

static void TransitionPal(short *target_pal) {
    short val;
    static short phase = 0;
    static u_short *pal = gol_transitions_pixels;

    val = TrackValueGet(&GOLGradientTrans, frameCount);
    if (val)
      phase = val;
    if (phase) {
      pal = LoadCompressedPal(pal, target_pal);
      phase--;
      Log("%d\n", phase);
    }
}