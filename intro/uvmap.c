#include <intro.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <sync.h>
#include <system/interrupt.h>
#include <system/memory.h>

#define WIDTH 160
#define HEIGHT 100
#define DEPTH 4
#define FULLPIXEL 1

extern TrackT UvmapTransition;
extern TrackT UvmapSrcTexture;
extern TrackT UvmapDstTexture;

static u_short *texFstHi, *texFstLo;
static u_short *texSndHi, *texSndLo;
static BitmapT *screen[2];
static __code u_short active = 0;
static __code short c2p_phase;
static __code void **c2p_bpl;
static CopListT *cp;
static CopInsT *bplptr[DEPTH];

#include "data/texture-inside.c"
#include "data/texture-outside.c"
#include "data/gradient.c"
#include "data/uvmap/gut-uv.c"
#include "data/uvmap/tit-uv.c"

#define UVMapRenderSize ((5 + WIDTH * HEIGHT / 32 * (8 * 8 + 2)) * 2)
void (*UVMapRender)(u_short *chunkyEnd asm("a0"),
                    u_short *texFstHi asm("a1"),
                    u_short *texFstLo asm("a2"),
                    u_short *texSndHi asm("a3"),
                    u_short *texSndLo asm("a4"));

/* [0 0 0 0 a0 a1 a2 a3] => [a0 a1 0 0 a2 a3 0 0] x 2 */
static u_short PixelHi[16] = {
  0x0000, 0x0404, 0x0808, 0x0c0c, 0x4040, 0x4444, 0x4848, 0x4c4c,
  0x8080, 0x8484, 0x8888, 0x8c8c, 0xc0c0, 0xc4c4, 0xc8c8, 0xcccc,
};

/* [0 0 0 0 b0 b1 b2 b3] => [ 0 0 b0 b1 0 0 b2 b3] x 2 */
static u_short PixelLo[16] = {
  0x0000, 0x0101, 0x0202, 0x0303, 0x1010, 0x1111, 0x1212, 0x1313, 
  0x2020, 0x2121, 0x2222, 0x2323, 0x3030, 0x3131, 0x3232, 0x3333, 
};

#define TW texture_in_width
#define TH texture_in_height

static __code short lastStep;

static void CopyTexture(const u_char *data, u_short *hi0, u_short *lo0,
                        short step)
{
  register u_short *lo1 asm("a3");
  register u_short *hi1 asm("a4");
  short n;

  if (lastStep <= step)
    return;

  {
    int start = step * TW;
    data += start;
    hi0 += start;
    lo0 += start;
    hi1 = hi0 + TW * TH;
    lo1 = lo0 + TW * TH;
  }

  n = (lastStep - step) * TW;

  while (--n >= 0) {
    int c = *data++;
    short hi = PixelHi[c];
    short lo = PixelLo[c];
    *hi0++ = hi;
    *hi1++ = hi;
    *lo0++ = lo;
    *lo1++ = lo;
  }
}

static void ScrambleUVMap(u_short *uvmap, u_char *u, u_char *v) {
  short i;

#define MAKEUV() (((*v++) << 7) | (*u++))

  for (i = 0; i < WIDTH * HEIGHT; i += 8) {
    uvmap[i + 0] = MAKEUV();
    uvmap[i + 1] = MAKEUV();
    uvmap[i + 4] = MAKEUV();
    uvmap[i + 5] = MAKEUV();
    uvmap[i + 2] = MAKEUV();
    uvmap[i + 3] = MAKEUV();
    uvmap[i + 6] = MAKEUV();
    uvmap[i + 7] = MAKEUV();
  }

#undef MAKEUV
}

static void MakeUVMapRenderCode(u_short *uvmap) {
  u_short *code = (void *)UVMapRender;
  u_short *data = uvmap + WIDTH * HEIGHT;

  /* The map is pre-scrambled to avoid one c2p pass:
   * [a b c d e f g h] => [a b e f c d g h] */
  register short n asm("d7") = WIDTH * HEIGHT / 32 - 1;

  register short m1 asm("d5") = 1;
  register short m2 asm("d6") = -2;

  *code++ = 0x48e7; *code++ = 0x3f00; /* movem.l d2-d7,-(sp) */
  data -= 4;

  do {
    short m, w, o;

    for (m = 0x0e00; m >= 0; m -= 0x0200) {
      /* 3029 xxxx | move.w xxxx(a1),d0 */
      w = *data++; o = w & m1;
      *code++ = (0x3029 | m) + (o + o);
      *code++ = w & m2;
      /* 806a yyyy | or.w   yyyy(a2),d0 */
      w = *data++; o = w & m1;
      *code++ = (0x806a | m) + (o + o);
      *code++ = w & m2;
      /* 1029 wwww | move.b wwww(a1),d0 */
      w = *data++; o = w & m1;
      *code++ = (0x1029 | m) + (o + o);
      *code++ = w & m2;
      /* 802a zzzz | or.b   zzzz(a2),d0 */
      w = *data++; o = w & m1;
      *code++ = (0x802a | m) + (o + o);
      *code++ = w & m2;
      data -= 8;
    }

    *code++ = 0x48a0; *code++ = 0xff00; /* d0-d7,-(a0) */
  } while (--n != -1);

  *code++ = 0x4cdf; *code++ = 0x00fc; /* movem.l (sp)+,d2-d7 */
  *code++ = 0x4e75; /* rts */
}

static void DeltaDecode(uint8_t *data) {
  uint8_t *left = data;
  uint8_t *curr = data + 1;
  short x, y;

  for (x = 1; x < WIDTH; x++) {
    *curr++ += *left++;
  }

  for (y = 1; y < HEIGHT; y++) {
    *curr++ += curr[-WIDTH];
    left++;
    for (x = 1; x < WIDTH; x++)
      *curr++ += *left++;
  }
}

static void Load(void) {
  static bool done = false;

  if (!done) {
    DeltaDecode(gut[0]);
    DeltaDecode(gut[1]);

    DeltaDecode(tit[0]);
    DeltaDecode(tit[1]);

    {
      short *uvmap = MemAlloc(WIDTH * HEIGHT * sizeof(short), MEMF_PUBLIC);

      ScrambleUVMap(uvmap, gut[0], gut[1]);
      memcpy(gut, uvmap, WIDTH * HEIGHT * sizeof(short));
      ScrambleUVMap(uvmap, tit[0], tit[1]);
      memcpy(tit, uvmap, WIDTH * HEIGHT * sizeof(short));

      MemFree(uvmap);
    }

    done = true;
  }
}

#define BLTSIZE ((WIDTH / 2) * HEIGHT) /* 8000 bytes */

/* If you think you can speed it up (I doubt it) please first look into
 * `c2p_2x1_4bpl_mangled_fast_blitter.py` in `prototypes/c2p`. */

static void ChunkyToPlanar(void) {
  register void **bpl asm("a0") = c2p_bpl;

  /*
   * Our chunky buffer of size (WIDTH/2, HEIGHT/2) is stored in bpl[0].
   * Each 32-bit long word of chunky buffer contains eight 4-bit pixels
   * [a b c d e f g h] in scrambled format described below.
   * Please note that a_i is the i-th least significant bit of a.
   *
   * [a0 a1 b0 b1 a2 a3 b2 b3 e0 e1 f0 f1 e2 e3 f2 f3
   *  c0 c1 d0 d1 c2 c3 d2 d3 g0 g1 h0 h1 g2 g3 h2 h3]
   *
   * So not only pixels in the texture must be scrambled, but also consecutive
   * bytes of input buffer i.e.: [a b] [e f] [c d] [g h] (see `gen-uvmap.py`).
   *
   * Chunky to planar is divided into two major steps:
   * 
   * Swap 4x2: in two consecutive 16-bit words swap diagonally two bits,
   *           i.e. [b0 b1] <-> [c0 c1], [b2 b3] <-> [c2 c3].
   * Expand 2x1: [x0 x1 ...] is translated into [x0 x0 ...] and [x1 x1 ...]
   *             and copied to corresponding bitplanes, this step effectively
   *             stretches pixels to 2x1.
   *
   * Line doubling is performed using copper. Rendered bitmap will have size
   * (WIDTH, HEIGHT/2, DEPTH) and will be placed in bpl[2] and bpl[3].
   */

  ClearIRQ(INTF_BLIT);

  switch (c2p_phase) {
    case 0:
      /* Initialize chunky to planar. */
      custom->bltamod = 2;
      custom->bltbmod = 2;
      custom->bltdmod = 0;
      custom->bltcdat = 0xF0F0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;

      /* Swap 4x2, pass 1, high-bits. */
      custom->bltapt = bpl[0];
      custom->bltbpt = bpl[0] + 2;
      custom->bltdpt = bpl[1] + BLTSIZE / 2;

      /* (a & 0xF0F0) | ((b >> 4) & ~0xF0F0) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
      custom->bltcon1 = BSHIFT(4);

    case 1:
      /* overall size: BLTSIZE / 2 bytes */
      custom->bltsize = 1 | ((BLTSIZE / 8) << 6);
      break;

    case 2:
      /* Swap 4x2, pass 2, low-bits. */
      custom->bltapt = bpl[1] - 4;
      custom->bltbpt = bpl[1] - 2;
      custom->bltdpt = bpl[1] + BLTSIZE / 2 - 2;

      /* ((a << 4) & 0xF0F0) | (b & ~0xF0F0) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(4);
      custom->bltcon1 = BLITREVERSE;

    case 3:
      /* overall size: BLTSIZE / 2 bytes */
      custom->bltsize = 1 | ((BLTSIZE / 8) << 6);
      break;

    case 4:
      custom->bltamod = 0;
      custom->bltbmod = 0;
      custom->bltdmod = 0;
      custom->bltcdat = 0xAAAA;

      custom->bltapt = bpl[1];
      custom->bltbpt = bpl[1];
      custom->bltdpt = bpl[3];

#if FULLPIXEL
      /* (a & 0xAAAA) | ((b >> 1) & ~0xAAAA) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
      custom->bltcon1 = BSHIFT(1);
#else
      /* (a & 0xAAAA) */
      custom->bltcon0 = (SRCA | DEST) | (ABC | ANBC);
      custom->bltcon1 = 0;
#endif
      /* overall size: BLTSIZE bytes */
      custom->bltsize = 4 | ((BLTSIZE / 8) << 6);
      break;

    case 5:
      custom->bltapt = bpl[1] + BLTSIZE - 2;
      custom->bltbpt = bpl[1] + BLTSIZE - 2;
      custom->bltdpt = bpl[2] + BLTSIZE - 2;
      custom->bltcdat = 0xAAAA;

#if FULLPIXEL
      /* ((a << 1) & 0xAAAA) | (b & ~0xAAAA) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(1);
      custom->bltcon1 = BLITREVERSE;
#else
      /* (a & ~0xAAAA) */
      custom->bltcon0 = (SRCA | DEST) | (ABNC | ANBNC);
      custom->bltcon1 = BLITREVERSE;
#endif
      /* overall size: BLTSIZE bytes */
      custom->bltsize = 4 | ((BLTSIZE / 8) << 6);
      break;

    case 6:
      CopInsSet32(bplptr[0], bpl[2]);
      CopInsSet32(bplptr[1], bpl[3]);
      CopInsSet32(bplptr[2], bpl[2] + BLTSIZE / 2);
      CopInsSet32(bplptr[3], bpl[3] + BLTSIZE / 2);
      break;

    default:
      break;
  }

  c2p_phase++;
}

static void MakeCopperList(CopListT *cp) {
  short *pixels = gradient_pixels;
  short i, j;

  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen[active], DEPTH);
  for (j = 0; j < 16; j++)
    CopSetColor(cp, j, *pixels++);
  for (i = 0; i < HEIGHT * 2; i++) {
    CopWaitSafe(cp, Y(i + 28), 0);
    /* Line doubling. */
    CopMove16(cp, bpl1mod, (i & 1) ? 0 : -40);
    CopMove16(cp, bpl2mod, (i & 1) ? 0 : -40);
#if !FULLPIXEL
    /* Alternating shift by one for bitplane data. */
    CopMove16(cp, bplcon1, (i & 1) ? 0x0010 : 0x0021);
#endif
    if (i % 13 == 12)
      for (j = 0; j < 16; j++)
        CopSetColor(cp, j, *pixels++);
  }
  CopEnd(cp);
}

static void Init(void) {
  screen[0] = NewBitmap(WIDTH * 2, HEIGHT * 2, DEPTH);
  screen[1] = NewBitmap(WIDTH * 2, HEIGHT * 2, DEPTH);

  texFstHi = MemAlloc(texture_out_width * texture_out_height * 4,
                      MEMF_PUBLIC|MEMF_CLEAR);
  texFstLo = MemAlloc(texture_out_width * texture_out_height * 4,
                      MEMF_PUBLIC|MEMF_CLEAR);
  texSndHi = MemAlloc(texture_in_width * texture_in_height * 4,
                      MEMF_PUBLIC|MEMF_CLEAR);
  texSndLo = MemAlloc(texture_in_width * texture_in_height * 4,
                      MEMF_PUBLIC|MEMF_CLEAR);

  EnableDMA(DMAF_BLITTER);

  BitmapClear(screen[0]);
  BitmapClear(screen[1]);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(28), WIDTH * 2, HEIGHT * 2);

  cp = NewCopList(900 + 256);
  MakeCopperList(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);

  active = 0;
  c2p_bpl = NULL;
  c2p_phase = 256;
  lastStep = TH;

  SetIntVector(INTB_BLIT, (IntHandlerT)ChunkyToPlanar, NULL);
  EnableINT(INTF_BLIT);
}

static void InitGut(void) {
  UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  MakeUVMapRenderCode((u_short *)gut);
  Init();
}

static void InitTit(void) {
  UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  MakeUVMapRenderCode((u_short *)tit);
  Init();
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER);

  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

  DeleteCopList(cp);
  MemFree(texFstHi);
  MemFree(texFstLo);
  MemFree(texSndHi);
  MemFree(texSndLo);
  MemFree(UVMapRender);

  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(UVMap);

static void Render(void) {
  int size = TW * TH;
  short fstOff = (frameCount * TW * 2 + frameCount / 2) & (size - 1);
  short sndOff = (-frameCount * TW * 2 + TH / 2 + (SIN(frameCount * 32) / 256)) & (size - 1);

  /* screen's bitplane #0 is used as a chunky buffer */
  ProfilerStart(UVMap);

  {
    short val;

    if ((val = TrackValueGet(&UvmapTransition, frameFromStart)) || lastStep) {
      short texSrcIdx = TrackValueGet(&UvmapSrcTexture, frameFromStart);
      short texDstIdx = TrackValueGet(&UvmapDstTexture, frameFromStart);
      short step = val / 2;

      CopyTexture(texSrcIdx ? texture_in_pixels : texture_out_pixels,
                  texDstIdx ? texSndHi : texFstHi,
                  texDstIdx ? texSndLo : texFstLo, step);

      lastStep = step;
    }
  }

  {
    u_short *chunky = screen[active]->planes[0] + WIDTH * HEIGHT / 2;
    u_short *fstHi = texFstHi + fstOff;
    u_short *fstLo = texFstLo + fstOff;
    u_short *sndHi = texSndHi + sndOff;
    u_short *sndLo = texSndLo + sndOff;

    (*UVMapRender)(chunky, fstHi, fstLo, sndHi, sndLo);
  }
  ProfilerStop(UVMap);

  c2p_phase = 0;
  c2p_bpl = screen[active]->planes;
  ChunkyToPlanar();
  active ^= 1;
}

EFFECT(UVGut, Load, NULL, InitGut, Kill, Render, NULL);
EFFECT(UVTit, Load, NULL, InitTit, Kill, Render, NULL);
