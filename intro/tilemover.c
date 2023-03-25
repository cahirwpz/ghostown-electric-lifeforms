#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <stdlib.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <fx.h>
#include <sync.h>

/*
 * Display window position within render buffer is always constant.
 * Target window changes position every frame and it always covers display
 * window, hence an extra tile around display window is needed to accommodate
 * for target window jitter. For each tile in target window we calculate source
 * tile position based on target tile position and a X/Y offset (in [-15..15]
 * range) fetched from vector field. Since resulting position can lie outside
 * target window we need to surround it by clean tiles. In the end we need an
 * extra layer of tiles around target window.
 */
#define TILESIZE 16
#define S_WIDTH 272
#define S_HEIGHT 224
#define DEPTH 4
#define COLORS 16
#define NFLOWFIELDS 8

#define MARGIN (2 * TILESIZE)
#define WIDTH (S_WIDTH + MARGIN * 2)
#define HEIGHT (S_HEIGHT + MARGIN * 2)

/* Tiles have to cover target window, i.e. display window plus one tile. */
#define WTILES (S_WIDTH / TILESIZE + 1)
#define HTILES (S_HEIGHT / TILESIZE + 1)
#define NTILES (WTILES * HTILES)

#define TWO_PI 25736
#define PI 12868

static __code BitmapT *screen;
static __code int active = 0;
static __code short prev_active = 0;
static CopListT *cp;
static CopInsT *bplptr[DEPTH + 1];

/* 1 bit version of logo for blitting */
static BitmapT *logo_blit;

/* for each tile following array stores source and destination offsets relative
 * to the beginning of a bitplane */
static short tiles[NFLOWFIELDS][NTILES];

#include "data/tilemover-bg-pal.c"
#include "data/tilemover-windmills.c"
#include "data/tilemover-wave.c"
#include "data/tilemover-drops.c"
#include "data/tilemover-block.c"

#include "data/pal-blue.c"
#include "data/pal-green.c"
#include "data/pal-red.c"

typedef const PaletteT *TilemoverPalT[8];

static TilemoverPalT tilemover_palettes = {
  NULL,
  &pal_blue,    // static
  &pal_blue,    // kitchen sink
  &pal_blue,    // soundwave
  &pal_red,     // windmills 
  &pal_green,   // rolling tube
  &pal_blue,     // funky soundwave
  &pal_blue,    // static
};

static __code short lightLevel = 0;
static const short blip_sequence[] = {
  0, 0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1
};

extern const BitmapT ghostown_logo;
extern TrackT TileMoverNumber;
extern TrackT TileMoverBlit;
extern TrackT TileMoverBgBlip;

/* fx, tx, fy, ty are arguments in pi-space (pi = 0x800, as in fx.h) */
static void CalculateTiles(short *tile, short range[4], u_short field_idx) {
  short fx = *range++;
  short tx = *range++;
  short fy = *range++;
  short ty = *range++;
  short x, y;
  short px, py;
  short px_real, py_real;

  /*
   * convert x and y ranges from pi-space to Q4.12 space
   * (pi = 12868 = 3.1416015625 in Q4.12)
   */
  short fx_real = normfx((int)(fx) * (int)(TWO_PI));
  short tx_real = normfx((int)(tx) * (int)(TWO_PI));
  short fy_real = normfx((int)(fy) * (int)(TWO_PI));
  short ty_real = normfx((int)(ty) * (int)(TWO_PI));

  short px_step = (tx - fx) / WTILES;
  short px_real_step = (tx_real - fx_real) / WTILES;
  short py_step = (ty - fy) / HTILES;
  short py_real_step = (ty_real - fy_real) / HTILES;

  for (y = 0, py = fy, py_real = fy_real; y < HTILES;
       y++, py += py_step, py_real += py_real_step) {
    /*
     * multiplication could overflow
     * bare formulas:
     * short py = fy + ((int)(y) * (int)(ty - fy)) / (short)HTILES;
     * short py_real =
     *   fy_real + ((int)(y) * (int)(ty_real - fy_real)) / (short)HTILES;
     */

    for (x = 0, px = fx, px_real = fx_real; x < WTILES;
         x++, px += px_step, px_real += px_real_step) {
      /*
       * bare formulas:
       * short px = fx + ((int)(x) * (int)(tx - fx)) / (short)WTILES;
       * short px_real =
       *   fx_real + ((int)(x) * (int)(tx_real - fx_real)) / (short)WTILES;
       */
      short vx = 0;
      short vy = 0;
      int mag =
        isqrt((int)px_real * (int)px_real + (int)py_real * (int)py_real);
      //int mag_sin = div16((int)(mag << 16), TWO_PI) >> 4;

      switch (field_idx) {
        // STATIC
        case 1:
          vx = 0;
          vy = 0;
          break;
        // KITCHEN SINK
        case 2:
          /* {-PI/4, PI/4, -PI, PI} */
          vx = py >> 10;
          vy = px >> 10;
          break;
        // SOUND WAVE
        case 3:
          /* -SIN_PI/2, SIN_PI/2, 0, SIN_PI/2 */
          vx = min(py_real, normfx(px_real * py_real)) >> 9;
          vy = mag >> 9;
          break;
        // WINDMILLS
        case 4:
          /*
           * -SIN_PI/3, SIN_PI/3, -SIN_PI/3, SIN_PI/3
           * this is also good with range:
           * SIN_PI/12, SIN_PI/4, -SIN_PI/12, SIN_PI/24
           */
          vx = SIN(py * 6) >> 10;
          vy = SIN(px * 6) >> 10;
          break;
        // ROLLING TUBE
        case 5:
          /* {-PI / 4, PI / 4, -PI, PI} */
          vx = SIN(px) >> 10;
          vy = COS(px) >> 15;
          break;
        // FUNKY SOUND WAVE
        case 6:
          /* {-2*SIN_PI, 2*SIN_PI, -SIN_PI, SIN_PI} */
          vx = px >> 9;
          vy = COS(px + 127) >> 9;
          break;
        // SLOW KITCHEN SINK
        case 7:
          /* {-PI/4, PI/4, -PI, PI} */
          vx = py >> 11;
          vy = px >> 11;
          break;
        default:
      }

      // Log("(%d %d): %d %d %d %d\n", px_real, py_real, mag, mag_sin, vx, vy);

      vx = -vx;

      /* source tile offset */
      *tile++ = vy * WIDTH + vx;
    }
  }
}

static short ranges[NFLOWFIELDS][4] = {
  {0, 0, 0, 0},                               // unused
  {0, 0, 0, 0},                               // static
  {-PI/4, PI/4, -PI, PI},                     // kitchen sink
  {-SIN_PI/2, SIN_PI/2, 0, SIN_PI/2},         // sound wave
  {-SIN_PI/3, SIN_PI/3, -SIN_PI/3, SIN_PI/3}, // windmills
  {-PI/4, PI/4, -PI, PI},                     // rolling tube
  {-2*SIN_PI, 2*SIN_PI, -SIN_PI, SIN_PI},     // funky soundwave
  {-PI/4, PI/4, -PI, PI},                     // slow kitchen sink
};

static void BlitSimple(void *sourceA, void *sourceB, void *sourceC,
                       const BitmapT *target, u_short minterms) {
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

static void BlitBitmap(short x, short y, const BitmapT *blit) {
  short i;
  short j = active;
  BlitterCopySetup(screen, MARGIN + x, MARGIN + y, blit);
  /* monkeypatch minterms to perform screen1 = screen1 | blit */
  custom->bltcon0 = (SRCB | SRCC | DEST) | (ABC | ANBC | ABNC);

  for (i = DEPTH - 1; i >= 0; i--) {
    BlitterCopyStart(j, 0);
    j--;
    if (j < 0)
      j += DEPTH + 1;
  }
}

static void UpdateBitplanePointers(void) {
  int offset = (MARGIN + WIDTH * MARGIN) / 8;
  short i;
  short j = active;

  for (i = DEPTH - 1; i >= 0; i--) {
    CopInsSet32(bplptr[i], screen->planes[j] + offset);
    j--;
    if (j < 0)
      j += DEPTH + 1;
  }
}

static void Load(void) {
  short i;

  for (i = 0; i < NFLOWFIELDS; i++)
    CalculateTiles(tiles[i], ranges[i], i);

  EnableDMA(DMAF_BLITTER);
  /* bitmap width aligned to word */
  logo_blit = NewBitmap(ghostown_logo.width, ghostown_logo.height, 1);
  BlitSimple(ghostown_logo.planes[0], ghostown_logo.planes[1],
             ghostown_logo.planes[2], logo_blit,
             ABC | ANBC | ABNC | ANBNC | NABC | NANBC | NABNC); 
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);
}

static void UnLoad(void) {
  DeleteBitmap(logo_blit);
}

static int BgBlip(void) {
  if (lightLevel) {
    SetColor(0, tilemover_bg_pal.colors[blip_sequence[lightLevel]]);
    lightLevel--;
  }
  return 0;
}

INTSERVER(BlipBackgroundInterrupt, 0, (IntFuncT)BgBlip, NULL);

extern void KillLogo(void);

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1);  
  EnableDMA(DMAF_BLITTER);
  BlitBitmap(S_WIDTH / 2 - 96 - 6, S_HEIGHT / 2 - 66, logo_blit);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);
  SetupPlayfield(MODE_LORES, DEPTH, X(MARGIN), Y((256 - S_HEIGHT) / 2),
                 S_WIDTH, S_HEIGHT);
  LoadPalette(&pal_blue, 0);

  KillLogo();

  TrackInit(&TileMoverNumber);
  TrackInit(&TileMoverBlit);
  TrackInit(&TileMoverBgBlip);

  cp = NewCopList(100);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  CopMove16(cp, bpl1mod, (WIDTH - S_WIDTH) / 8);
  CopMove16(cp, bpl2mod, (WIDTH - S_WIDTH) / 8);
  CopEnd(cp);
 
  CopListActivate(cp);
  
  UpdateBitplanePointers();
  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);

  AddIntServer(INTB_VERTB, BlipBackgroundInterrupt);
}

static void Kill(void) {
  RemIntServer(INTB_VERTB, BlipBackgroundInterrupt);
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
  DeleteBitmap(screen);
  DeleteCopList(cp);
}

#define BLTMOD (WIDTH / 8 - TILESIZE / 8 - 2)
#define BLTSIZE ((TILESIZE << 6) | ((TILESIZE + 16) >> 4))

static void MoveTiles(void *src asm("a2"), void *dst asm("a3"),
                      short xshift asm("d0"), short yshift asm("d1"),
                      short *tile asm("a4"), CustomPtrT custom_ asm("a6")) {
  register int mask1 asm("d4") = rorl(0xffff0000, xshift);
  register int mask2 asm("d5") = rorl(0x0000ffff, xshift);
  int offset = yshift * WIDTH + xshift;
  short dx = offset & 15;
  register short m asm("d6") = HTILES - 1;

  src += (offset & -16) >> 3;
  dst += (offset & -16) >> 3;

  _WaitBlitter(custom_);

  custom_->bltadat = 0xffff;
  custom_->bltbmod = BLTMOD;
  custom_->bltcmod = BLTMOD;
  custom_->bltdmod = BLTMOD;
  custom_->bltcon0 = (SRCB | SRCC | DEST) | (ABC | ABNC | NABC | NANBC);

  do {
    register short n asm("d7") = WTILES - 1;

    do {
      void *dstpt = dst;
      short srcoff = *tile++ + dx;
      void *srcpt = src + (srcoff >> 3);
      short sx = srcoff & 15;
      short shift = sx - dx;
      u_int mask;

      if (shift < 0) {
        shift = rorw(-shift, 4);
        mask = mask1;
      } else {
        srcpt += (WIDTH * (TILESIZE - 1) + TILESIZE) / 8;
        dstpt += (WIDTH * (TILESIZE - 1) + TILESIZE) / 8;

        shift = rorw(shift, 4) | BLITREVERSE;
        mask = mask2;
      }

      {
        register volatile void *ptr asm("a5") = &custom_->bltcon1;

        /* Comment it out if you're feeling lucky! */
        _WaitBlitter(custom_);

        *((short *)ptr)++ = shift;    // bltcon1
        *((int *)ptr)++ = mask;       // bltaltwm & bltafwm
        *((int *)ptr)++ = (int)dstpt; // bltcpt
        *((int *)ptr)++ = (int)srcpt; // bltbpt
        ptr += 4;                     // bltapt
        *((int *)ptr)++ = (int)dstpt; // bltdpt
        *((short *)ptr)++ = BLTSIZE;  // bltsize
      }

      src += TILESIZE / 8;
      dst += TILESIZE / 8;
    } while (--n != -1);

    src += (WIDTH * (TILESIZE - 1) + 3 * TILESIZE) / 8;
    dst += (WIDTH * (TILESIZE - 1) + 3 * TILESIZE) / 8;
  } while (--m != -1);
}

static void BlitShape(short val) {
  short i;

  switch (val) {
    case 2: /* Noise for kitchen sink */
      BlitBitmap(170, 1, &tilemover_block);
      break;

    case 3: /* Pseudo soundwave */
      BlitBitmap(10, 170, &tilemover_wave);
      break;

    case 4: /* Windmills */
      for (i = 0; i < 6; i++)
        BlitBitmap(1 + (random() & 230), 1 + (random() & 170),
                   &tilemover_windmills);
      break;

    case 5: /* Tube with stardrops */
      for (i = 0; i < 3; i++)
        BlitBitmap(165 + (random() & 10), random() & 170, &tilemover_drops);
      for (i = 0; i < 3; i++)
        BlitBitmap(105 + (random() & 9), random() & 100, &tilemover_drops);
      break;
  }
}

PROFILE(TileMover);

static void Render(void) {
  short current_ff = TrackValueGet(&TileMoverNumber, frameCount);
  short current_blip = TrackValueGet(&TileMoverBgBlip, frameCount);
  short val;
  
  if (current_ff != prev_active) {
    prev_active = current_ff;
    LoadPalette(tilemover_palettes[current_ff], 0);
  }

  if (current_blip)
    lightLevel = current_blip;

  if ((val = TrackValueGet(&TileMoverBlit, frameCount)))
    BlitShape(val);

  ProfilerStart(TileMover);
  {
    short xshift = random() & (TILESIZE - 1);
    short yshift = random() & (TILESIZE - 1);
    void *src = screen->planes[active];
    void *dst;
    if (++active == DEPTH + 1)
      active = 0;
    dst = screen->planes[active];

    /* Clear margin around destination display window,
     * to avoid moving garbage while rendering next frame. */
    BlitterClearArea(screen, active,
                     (&(Area2D){TILESIZE, TILESIZE, TILESIZE, S_HEIGHT}));
    BlitterClearArea(screen, active,
                     (&(Area2D){WIDTH - MARGIN, TILESIZE, TILESIZE, S_HEIGHT}));
    BlitterClearArea(screen, active,
                     (&(Area2D){MARGIN, TILESIZE, S_WIDTH, TILESIZE}));
    BlitterClearArea(screen, active,
                     (&(Area2D){MARGIN, HEIGHT - MARGIN, S_WIDTH, TILESIZE}));

    /* Move to target window origin minus offset. */
    src += (WIDTH * TILESIZE + TILESIZE) / 8;
    dst += (WIDTH * TILESIZE + TILESIZE) / 8;

    MoveTiles(src, dst, xshift, yshift, tiles[current_ff], custom);
  }
  ProfilerStop(TileMover);
  UpdateBitplanePointers();

  TaskWaitVBlank();
}

EFFECT(TileMover, Load, UnLoad, Init, Kill, Render);
