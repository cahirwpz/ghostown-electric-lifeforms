#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <stdlib.h>
#include <system/memory.h>

/* Add tile sized margins on every side to hide visual artifacts. */
#define MARGIN 32
#define WIDTH  (256 + MARGIN)
#define HEIGHT (224 + MARGIN)
#define DEPTH  4

#define TILESIZE  16
#define WTILES    (WIDTH / TILESIZE)
#define HTILES    (HEIGHT / TILESIZE)
#define NTILES    ((WTILES - 1) * (HTILES - 1))
#define ROTATION  1
#define ZOOM      1

static BitmapT *screen;
static int active = 0;
static CopListT *cp;
static CopInsT *bplptr[DEPTH + 1];

/* for each tile following array stores source and destination offsets relative
 * to the beginning of a bitplane */
static int tiles[NTILES * 2];

#include "data/tilemover-pal.c"

static void CalculateTiles(int *tile, short rotation, short zoom) { 
  short x, y;
  short dx, dy;

  for (y = 0, dy = 0; y < HTILES - 1; y++, dy += TILESIZE) {
    short yo = (HTILES - 1 - y) - TILESIZE / 2;
    for (x = 0, dx = 0; x < WTILES - 1; x++, dx += TILESIZE) {
      short xo = x - TILESIZE / 2;
      short sx = dx;
      short sy = dy;

      if (rotation > 0) {
        sx += yo;
        sy += xo;
      } else if (rotation < 0) {
        sx -= yo;
        sy -= xo;
      }

      if (zoom > 0) {
        sx -= xo;
        sy += yo;
      } else if (zoom < 0) {
        sx += xo;
        sy -= yo;
      }

      *tile++ = sy * WIDTH + sx; /* source tile offset */
      *tile++ = dy * WIDTH + dx; /* destination tile offset */
    }
  }
}

static void Init(void) {
  CalculateTiles(tiles, ROTATION, ZOOM);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1);

  SetupPlayfield(MODE_LORES, DEPTH, X(MARGIN), Y((256 - HEIGHT + MARGIN) / 2),
                 WIDTH - MARGIN, HEIGHT - MARGIN);
  LoadPalette(&tilemover_pal, 0);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  CopMove16(cp, bpl1mod, MARGIN / 8);
  CopMove16(cp, bpl2mod, MARGIN / 8);
  CopEnd(cp);

  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
  DeleteBitmap(screen);
  DeleteCopList(cp);
}

static void DrawSeed(u_char *bpl) {
  short y = HEIGHT / 2 - TILESIZE / 2;
  short x = WIDTH / 2 - TILESIZE / 2;
  int offset = (y * WIDTH + x) / 8;
  short n = 8 - 1;

  do {
    bpl[offset] = random();
    offset += WIDTH / 8;
  } while (--n >= 0);
}

#define BLTMOD (WIDTH / 8 - TILESIZE / 8 - 2)
#define BLTSIZE ((TILESIZE << 6) | ((TILESIZE + 16) >> 4))

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

  offset = yshift * WIDTH + xshift;

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

      srcpt += WIDTH * (TILESIZE - 1) / 8 + 2;
      dstpt += WIDTH * (TILESIZE - 1) / 8 + 2;
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
  int offset = (TILESIZE + WIDTH * TILESIZE) / 8;
  short i;
  short j = active;
  for (i = DEPTH - 1; i >= 0; i--) {
    CopInsSet32(bplptr[i], screen->planes[j] + offset);
    j--;
    if (j < 0)
      j += DEPTH + 1;
  }
}

PROFILE(TileZoomer);

static void Render(void) {
  static short rotation = 0;
  static short zoom = 0;
  ProfilerStart(TileZoomer);
  if ((frameCount & 63) < 2) {
    rotation = random() % 2;
    zoom = random() % 2;
    if (rotation == 0)
      rotation = -1;
    CalculateTiles(tiles, rotation, zoom);
  }
  if (zoom && ((random() & 3) == 0))
    DrawSeed(screen->planes[active]);
  {
    short xshift = random() & (TILESIZE - 1);
    short yshift = random() & (TILESIZE - 1);
    void *src = screen->planes[active];
    void *dst;
    if (++active == DEPTH + 1)
      active = 0;
    dst = screen->planes[active];

    MoveTiles(src, dst, xshift, yshift);
  }
  UpdateBitplanePointers();
  ProfilerStop(TileZoomer);

  TaskWaitVBlank();
}

EFFECT(TileMover, NULL, NULL, Init, Kill, Render);
