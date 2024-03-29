#include <blitter.h>

typedef struct {
  const BitmapT *src;
  const BitmapT *dst;
  u_int start;
  u_short size;
} StateT;

static StateT state[1];

/* Supports any (x, y) and any source bitmap width. */
void BlitterOrSetup(const BitmapT *dst, u_short x, u_short y,
                    const BitmapT *src)
{
  /* Calculate real blit width. It can be greater than src->bytesPerRow! */
  u_short width = (x & 15) + src->width;
  u_short bytesPerRow = ((width + 15) & ~15) >> 3;
  u_short srcmod = src->bytesPerRow - bytesPerRow;
  u_short dstmod = dst->bytesPerRow - bytesPerRow;
  u_short bltafwm = FirstWordMask[x & 15];
  u_short bltalwm = LastWordMask[width & 15];
  u_short bltshift = rorw(x & 15, 4);

  state->src = src;
  state->dst = dst;
  state->start = ((x & ~15) >> 3) + y * dst->bytesPerRow;
  state->size = (src->height << 6) | (bytesPerRow >> 1);

  WaitBlitter();

  custom->bltcon0 = A_OR_B | (SRCA | SRCB | DEST) | bltshift;
  custom->bltcon1 = 0;
  custom->bltafwm = bltafwm;
  custom->bltalwm = bltalwm;
  custom->bltamod = srcmod;
  custom->bltbmod = dstmod;
  custom->bltdmod = dstmod;
}

void BlitterOrStart(short dstbpl, short srcbpl) {
  void *srcbpt = state->src->planes[srcbpl];
  void *dstbpt = state->dst->planes[dstbpl] + state->start;
  u_short bltsize = state->size;

  WaitBlitter();

  custom->bltapt = srcbpt;
  custom->bltbpt = dstbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}
