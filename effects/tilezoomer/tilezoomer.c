#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <stdlib.h>
#include <system/memory.h>
#include <fx.h>
#include <gfx.h>

#define MOTIONBLUR 1

/* Add tile sized margins on every side to hide visual artifacts. */
#define MARGIN  32
#define WIDTH   (256 + MARGIN)
#define HEIGHT  (224 + MARGIN)

#if MOTIONBLUR
#define DEPTH   1
#define SHADOW  4
#else
#define DEPTH   2
#define SHADOW  0
#endif

#define TILESIZE  16
#define WTILES    (WIDTH / TILESIZE)
#define HTILES    (HEIGHT / TILESIZE)
#define NTILES    ((WTILES - 1) * (HTILES - 1))
#define ROTATION  1
#define ZOOM      1

#define TWO_PI 25736
#define PI 12868

static BitmapT *screen0;
#if !MOTIONBLUR
static BitmapT *screen1;
#endif
static int active = 0;
static CopListT *cp;
static CopInsPairT *bplptr;

/* for each tile following array stores source and destination offsets relative
 * to the beginning of a bitplane */
static int tiles[NTILES * 2];

#include "data/tilezoomer-pal.c"

// fx, tx, fy, ty are arguments in pi-space (pi = 0x800, as in fx.h)
void CalculateTiles(int *tile, short fx, short tx, short fy, short ty, u_short field_idx) { 
  short x, y;
  short dx, dy;
  short px, py;
  short px_real, py_real;

  // convert x and y ranges from pi-space to 4.12 space (pi = 12868 = 3.1416015625 in 4.12)
  short fx_real = normfx((int)(fx) * (int)(TWO_PI));
  short tx_real = normfx((int)(tx) * (int)(TWO_PI));
  short fy_real = normfx((int)(fy) * (int)(TWO_PI));
  short ty_real = normfx((int)(ty) * (int)(TWO_PI));

  short px_step = (tx - fx) / (WTILES - 1);
  short px_real_step = (tx_real - fx_real) / (WTILES - 1);
  short py_step = (ty - fy) / (HTILES - 1);
  short py_real_step = (ty_real - fy_real) / (HTILES - 1);

  for (y = 0, dy = 0, py = fy, py_real = fy_real; y < HTILES - 1; y++, dy += TILESIZE, py += py_step, py_real += py_real_step) {
    // multiplication could overflow
    // bare formulas:
    // short py = fy + ((int)(y) * (int)(ty - fy)) / (short)(HTILES - 1);
    // short py_real = fy_real + ((int)(y) * (int)(ty_real - fy_real)) / (short)(HTILES - 1);

    for (x = 0, dx = 0, px = fx, px_real = fx_real; x < WTILES - 1; x++, dx += TILESIZE, px += px_step, px_real += px_real_step) {
      // bare formulas:
      // short px = fx + ((int)(x) * (int)(tx - fx)) / (short)(WTILES - 1);
      // short px_real = fx_real + ((int)(x) * (int)(tx_real - fx_real)) / (short)(WTILES - 1);

      short sx = dx;
      short sy = dy;
      short vx = 0;
      short vy = 0;
      int mag = isqrt((int)px_real*(int)px_real + (int)py_real*(int)py_real);
      int mag_sin = div16((int)(mag << 16), TWO_PI) >> 4;

      switch (field_idx) {
        case 0:
          // SIN_PI/3, SIN_PI, -SIN_PI/3, SIN_PI/12
          vx = (SIN(mag_sin)) >> 8;
          vy = ((COS(mag_sin)) >> 8) - (py_real >> 9);
          break;
        case 1:
          // -SIN_PI/2, SIN_PI/2, 0, SIN_PI/2,
          vx = min(py_real, normfx(px_real * py_real)) >> 9;
          vy = mag >> 9;
          break;
        case 2:
          // -SIN_PI/3, SIN_PI/3, -SIN_PI/3, SIN_PI/3
          // this is also good with range: SIN_PI/12, SIN_PI/4, -SIN_PI/12, SIN_PI/24
          vx = (SIN(py*6)/2) >> 9;
          vy = (SIN(px*6)/2) >> 9;
          break;
        case 3:
          // -2 * SIN_PI, 2 * SIN_PI, -SIN_PI, SIN_PI
          vx = (SIN(py) / 2) >> 9;
          vy = 0;
          break;
        case 4:
          // -SIN_PI / 4, SIN_PI / 4, -SIN_PI, SIN_PI,
          vx = ((COS(px) / 2) + 127) >> 8;
          vy = (px_real + 127) >> 8;
          break;
        default:
      }

      //Log("(%d %d): %d %d %d %d\n", px_real, py_real, mag, mag_sin, vx, vy);

      vx = -vx;
      sx += vx;
      sy += vy;

      *tile++ = sy * WIDTH * DEPTH + sx; /* source tile offset */
      *tile++ = dy * WIDTH * DEPTH + dx; /* destination tile offset */
    }
  }
}

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100);
#if MOTIONBLUR
  bplptr = CopSetupBitplanes(cp, screen0, SHADOW);
  CopMove16(cp, bpl1mod, MARGIN / 8);
  CopMove16(cp, bpl2mod, MARGIN / 8);
#else
  bplptr = CopSetupBitplanes(cp, screen0, DEPTH);
  /* Screen bitplanes are interleaved! */
  CopMove16(cp, bpl1mod, (WIDTH * (DEPTH - 1) + MARGIN) / 8);
  CopMove16(cp, bpl2mod, (WIDTH * (DEPTH - 1) + MARGIN) / 8);
#endif
  return CopListFinish(cp);
}

static void Init(void) {
  CalculateTiles(tiles, SIN_PI/3, SIN_PI, -SIN_PI/3, SIN_PI/12, 0);

#if MOTIONBLUR
  screen0 = NewBitmap(WIDTH, HEIGHT, DEPTH + SHADOW, BM_CLEAR);
#else
  screen0 = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR | BM_INTERLEAVED);
  screen1 = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR | BM_INTERLEAVED);
#endif

#if MOTIONBLUR
  SetupPlayfield(MODE_LORES, SHADOW, X(MARGIN), Y((256 - HEIGHT + MARGIN) / 2),
                 WIDTH - MARGIN, HEIGHT - MARGIN);
  LoadColors(tilezoomer_colors, 0);
#else
  SetupPlayfield(MODE_LORES, DEPTH, X(MARGIN), Y((256 - HEIGHT + MARGIN) / 2),
                 WIDTH - MARGIN, HEIGHT - MARGIN);
  SetColor(0, 0x000);
  SetColor(1, 0x44f);
  SetColor(2, 0x88f);
  SetColor(3, 0xccf);
#endif

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DeleteBitmap(screen0);
#if !MOTIONBLUR
  DeleteBitmap(screen1);
#endif
  DeleteCopList(cp);
}

static void DrawSeed(u_char *bpl) {
  short y = HEIGHT / (random() % 8 + 1) - TILESIZE / 2;
  short x = WIDTH / (random() % 8 + 1) - TILESIZE / 2;
  int offset = (y * WIDTH * DEPTH + x) / 8;
  short n = DEPTH * 8 - 1;

  do {
    bpl[offset] = random();
    offset += WIDTH / 8;
  } while (--n >= 0);
}

#define BLTMOD (WIDTH / 8 - TILESIZE / 8 - 2)
#define BLTSIZE ((TILESIZE * DEPTH << 6) | ((TILESIZE + 16) >> 4))

static void MoveTiles(void *src, void *dst, short xshift, short yshift) {
  short n = NTILES - 1;
  int *tile = tiles;
  int offset;

  register volatile void *bltptr asm("a4") = &custom->bltcon1;

  custom->bltadat = 0xffff;
  custom->bltbmod = BLTMOD;
  custom->bltcmod = BLTMOD;
  custom->bltdmod = BLTMOD;
  custom->bltcon0 = (SRCB | SRCC | DEST) | (ABC | ABNC | NABC | NANBC);

  offset = yshift * WIDTH * DEPTH + xshift;

  do {
    int srcoff = *tile++ + offset;
    int dstoff = *tile++ + offset;
    register void *srcpt asm("a2") = src + ((srcoff >> 3) & ~1);
    register void *dstpt asm("a3") = dst + ((dstoff >> 3) & ~1);
    short sx = srcoff;
    short dx = dstoff;
    u_short bltcon1;
    u_int mask;
    short shift;

    sx &= 15; dx &= 15;

    shift = dx - sx;
    if (shift >= 0) {
      mask = 0xffff0000;
      bltcon1 = rorw(shift, 4);
    } else {
      mask = 0x0000ffff;
      bltcon1 = rorw(-shift, 4) | BLITREVERSE;

      srcpt += WIDTH * DEPTH * (TILESIZE - 1) / 8 + 2;
      dstpt += WIDTH * DEPTH * (TILESIZE - 1) / 8 + 2;
    }

    mask = rorl(mask, dx);

#if 1
    {
      volatile void *ptr = bltptr;

      WaitBlitter();

      *((short *)ptr)++ = bltcon1;
      *((int *)ptr)++ = mask;
      *((int *)ptr)++ = (int)dstpt;
      *((int *)ptr)++ = (int)srcpt;
      ptr += 4;
      *((int *)ptr)++ = (int)dstpt;
      *((short *)ptr)++ = BLTSIZE;
    }
#else
    WaitBlitter();

    custom->bltcon1 = bltcon1;
    custom->bltalwm = mask;
    custom->bltafwm = swap16(mask);
    custom->bltcpt = dstpt;
    custom->bltbpt = srcpt;
    custom->bltdpt = dstpt;
    custom->bltsize = BLTSIZE;
#endif
  } while (--n >= 0);
}

static void UpdateBitplanePointers(void) {
  int offset = (TILESIZE + WIDTH * DEPTH * TILESIZE) / 8;
  short i;
#if MOTIONBLUR
  short j = active;
  for (i = SHADOW - 1; i >= 0; i--) {
    CopInsSet32(&bplptr[i], screen0->planes[j] + offset);
    j--;
    if (j < 0)
      j += DEPTH + SHADOW;
  }
#else
  for (i = 0; i < DEPTH; i++)
    CopInsSet32(&bplptr[i], screen1->planes[i] + offset);
#endif
}

PROFILE(TileZoomer);

static void Render(void) {
  ProfilerStart(TileZoomer);
  if ((random() & 15) == 0)
    DrawSeed(screen0->planes[active]);
  {
    short xshift = random() & (TILESIZE - 1);
    short yshift = random() & (TILESIZE - 1);
    void *src = screen0->planes[active];
    void *dst;
#if MOTIONBLUR
    if (++active == DEPTH + SHADOW)
      active = 0;
    dst = screen0->planes[active];
#else
    dst = screen1->planes[0];
#endif

    MoveTiles(src, dst, xshift, yshift);
  }
  UpdateBitplanePointers();
  ProfilerStop(TileZoomer);

  TaskWaitVBlank();
#if !MOTIONBLUR
  swapr(screen0, screen1);
#endif
}

EFFECT(TileZoomer, NULL, NULL, Init, Kill, Render, NULL);
