#include "data/gol-transitions.c"

extern TrackT GOLGradientTrans;
extern TrackT GOLLogoType;

static short palette_gol[16] = {0};

static short *LoadCompressedPal(short *from, short *to) {
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
  u_short val, start, steps;
  static u_short *pal;
  static short phase = 0;

  val = TrackValueGet(&GOLGradientTrans, frameCount);
  start = (val & 0xff) * gol_transitions_width;
  steps = (val & 0xff00) >> 8;
  if (steps > 0) {
    phase = steps;
    pal = gol_transitions_pixels + start;
  }
  if (phase) {
    pal = LoadCompressedPal(pal, target_pal);
    phase--;
  }
}