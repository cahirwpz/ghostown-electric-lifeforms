#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <stdlib.h>
#include <system/memory.h>
#include <fx.h>
#include <sync.h>

/* Add tile sized margins on every side to hide visual artifacts. */
#define MARGIN 32
#define WIDTH  (272 + MARGIN)
#define HEIGHT (224 + MARGIN)
#define DEPTH  4
#define COLORS 16
#define NFLOWFIELDS 5

#define TILESIZE  16
#define WTILES    (WIDTH / TILESIZE)
#define HTILES    (HEIGHT / TILESIZE)
#define NTILES    ((WTILES - 1) * (HTILES - 1))

#define TWO_PI 25736
#define PI 12868

static BitmapT *screen;
static int active = 0;
static CopListT *cp;
static CopInsT *bplptr[DEPTH + 1];
static CopInsT *palptr[COLORS];

// 1 bit version of logo for blitting
static BitmapT *logo_blit;

/* for each tile following array stores source and destination offsets relative
 * to the beginning of a bitplane */
static int tiles[NFLOWFIELDS][NTILES * 2];
static u_short current_ff = 0;

#include "data/tilemover-pal.c"
#include "data/ghostown-logo-crop.c"

extern TrackT TileMoverNumber;
extern TrackT TileMoverBlit;

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

      *tile++ = sy * WIDTH + sx; /* source tile offset */
      *tile++ = dy * WIDTH + dx; /* destination tile offset */
    }
  }
}

short ranges[NFLOWFIELDS*4] = {
  SIN_PI/3,  SIN_PI,   -SIN_PI/3, SIN_PI/12,
  -SIN_PI/2, SIN_PI/2, 0,         SIN_PI/2,
  -SIN_PI/3, SIN_PI/3, -SIN_PI/3, SIN_PI/3,
  -2*SIN_PI, 2*SIN_PI, -SIN_PI,   SIN_PI,
  -SIN_PI/4, SIN_PI/4, -SIN_PI,   SIN_PI,
};

static void BlitSimple(void *sourceA, void *sourceB,
                       void *sourceC, const BitmapT *target,
                       u_short minterms) {
  u_short bltsize = (target->height << 6) | (target->width >> 4);

  custom->bltcon0 = minterms | (SRCA | SRCB | SRCC | DEST);
  custom->bltcon1 = 0;

  custom->bltafwm = 0xFFFF;
  custom->bltalwm = 0xFFFF;
  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltapt = sourceA;
  custom->bltbpt = sourceB;
  custom->bltcpt = sourceC;
  custom->bltdpt = target->planes[0];
  custom->bltsize = bltsize;
}

static void BlitGhostown(void) {
  short i;
  short j = active;
  BlitterCopySetup(screen, (WIDTH - MARGIN - logo_blit->width) / 2 + 6,
                   (HEIGHT - MARGIN - logo_blit->height) / 2, logo_blit);
  // monkeypatch minterms to perform screen = screen | logo_blit
  custom->bltcon0 = (SRCB | SRCC | DEST) | (ABC | ANBC | ABNC);

  for (i = DEPTH - 1; i >= 0; i--) {
    BlitterCopyStart(j, 0);
    j--;
    if (j < 0)
      j += DEPTH + 1;
  }
}

static void Load(void) {
  u_short i;
  short* rangetab = ranges;
  for (i = 0; i < NFLOWFIELDS; i++) {
    short fx = *rangetab++;
    short tx = *rangetab++;
    short fy = *rangetab++;
    short ty = *rangetab++;
    CalculateTiles(tiles[i], fx, tx, fy, ty, i);
  }

  {
    u_short w = ghostown_logo.width;
    u_short h = ghostown_logo.height;

    EnableDMA(DMAF_BLITTER);

    // bitmap width aligned to word
    logo_blit = NewBitmap(w + (16 - (w % 16)), h, 1);
    BlitSimple(ghostown_logo.planes[0], ghostown_logo.planes[1], ghostown_logo.planes[2], logo_blit,
               ABC | ANBC | ABNC | ANBNC | NABC | NANBC | NABNC);

    WaitBlitter();
    DisableDMA(DMAF_BLITTER);
  }
}

static void UnLoad(void) {
  DeleteBitmap(logo_blit);
}

static void Init(void) {
  u_short i;
  u_short* color = tilemover_pal.colors;

  TrackInit(&TileMoverNumber);
  TrackInit(&TileMoverBlit);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1);

  SetupPlayfield(MODE_LORES, DEPTH, X(MARGIN), Y((256 - HEIGHT + MARGIN) / 2),
                 WIDTH - MARGIN, HEIGHT - MARGIN);
  LoadPalette(&tilemover_pal, 0);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  CopMove16(cp, bpl1mod, MARGIN / 8);
  CopMove16(cp, bpl2mod, MARGIN / 8);
  for (i = 0; i < COLORS; i++)
    palptr[i] = CopSetColor(cp, i, *color++);
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

static void MoveTiles(void *src asm("a2"), void *dst asm("a3"),
                      short xshift asm("d0"), short yshift asm("d1"),
                      int *tile asm("a4"), CustomPtrT custom_ asm("a6")) {
  register short m15 asm("d4") = 15;
  register int mask1 asm("d5") = rorl(0xffff0000, xshift);
  register int mask2 asm("d6") = rorl(0x0000ffff, xshift);
  register int offset asm("d7") = yshift * WIDTH + xshift;
  short n = NTILES - 1;

  custom_->bltadat = 0xffff;
  custom_->bltbmod = BLTMOD;
  custom_->bltcmod = BLTMOD;
  custom_->bltdmod = BLTMOD;
  custom_->bltcon0 = (SRCB | SRCC | DEST) | (ABC | ABNC | NABC | NANBC);

  do {
    register int srcoff asm("d2") = *tile++ + offset;
    register int dstoff asm("d3") = *tile++ + offset;
    register void *srcpt asm("a0") = src + (srcoff >> 3);
    register void *dstpt asm("a1") = dst + (dstoff >> 3);
    short dx = dstoff & m15;
    short sx = srcoff & m15;
    u_int mask;
    short shift;

    shift = dx - sx;
    if (shift >= 0) {
      shift = rorw(shift, 4);
      mask = mask1;
    } else {
      shift = rorw(-shift, 4) | BLITREVERSE;
      mask = mask2;

      srcpt += WIDTH * (TILESIZE - 1) / 8 + 2;
      dstpt += WIDTH * (TILESIZE - 1) / 8 + 2;
    }

    /* Comment it out if you're feeling lucky! */
    _WaitBlitter(custom_);

    {
      register volatile void *ptr asm("a5") = &custom_->bltcon1;

      *((short *)ptr)++ = shift;        // bltcon1
      *((int *)ptr)++ = mask;           // bltaltwm & bltafwm
      *((int *)ptr)++ = (int)dstpt;     // bltcpt
      *((int *)ptr)++ = (int)srcpt;     // bltbpt
      ptr += 4;                         // bltapt
      *((int *)ptr)++ = (int)dstpt;     // bltdpt
      *((short *)ptr)++ = BLTSIZE;      // bltsize
    }
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

PROFILE(TileMover);

static void Render(void) {
  current_ff = TrackValueGet(&TileMoverNumber, frameCount);

#if 0
  {
    short i;
    // replace with some interpolated palette?
    short *color = tilemover_pal.colors;
    for (i = 0; i < COLORS; i++)
      CopInsSet16(palptr[i], *color++);
  }
#endif

  if (TrackValueGet(&TileMoverBlit, frameCount))
    BlitGhostown();

  ProfilerStart(TileMover);
  if ((random() & 15) == 0)
    DrawSeed(screen->planes[active]);
  {
    short xshift = random() & (TILESIZE - 1);
    short yshift = random() & (TILESIZE - 1);
    void *src = screen->planes[active];
    void *dst;
    if (++active == DEPTH + 1)
      active = 0;
    dst = screen->planes[active];

    MoveTiles(src, dst, xshift, yshift, tiles[current_ff], custom);
  }
  ProfilerStop(TileMover);
  UpdateBitplanePointers();

  TaskWaitVBlank();
}

EFFECT(TileMover, Load, UnLoad, Init, Kill, Render);
