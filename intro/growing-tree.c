#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <gfx.h>
#include <line.h>
#include <sprite.h>
#include <stdlib.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3
#define NSPRITES 8

static CopListT *cp;
static CopInsT *bplptr[DEPTH];
static BitmapT *screen;
static CopInsT *sprptr[8];

#include "growing-tree-greets.c"
#include "data/fruit-1.c"
#include "data/fruit-2.c"
#include "data/grass-1.c"
#include "data/grass-2.c"

typedef struct Branch {
  short pos_x, pos_y; // Q12.4
  union { // Q4.12
    short word;
    char byte;
  } _vel_x;
  union { // Q4.12
    short word;
    char byte;
  } _vel_y;
  short diameter;     // Q12.4
} BranchT;

#define vel_x _vel_x.word
#define vel_y _vel_y.word
#define vel_x_b _vel_x.byte
#define vel_y_b _vel_y.byte

#define MAXBRANCHES 256

static BranchT branches[MAXBRANCHES];
static BranchT *lastBranch = branches;

typedef struct Greets {
  unsigned char x, y; // pos x,y
  unsigned char delay; // delay in frames
  unsigned char *currentDataPos; // ptr to data
} GreetsT;

static GreetsT greetsData[3];

static int mTableIdx = 0;
static int mTable[] = {
  0x74a7beec, 0xb2818113, // 1 
  0x4ffa0d80, 0x23743a06, // 2
  0x237f164b, 0x9ffa9d90, //3
  0x011bad37, 0x7a6433ee, // 4
};

static int fastrand_a = 0, fastrand_b = 0;
static inline int fastrand(void) {
  int *mAddr = &mTable[mTableIdx*2];


  // https://www.atari-forum.com/viewtopic.php?p=188000#p188000
  asm volatile("move.l (%2)+,%0\n"
               "move.l (%2),%1\n"
               "swap   %1\n"
               "add.l  %0,(%2)\n"
               "add.l  %1,-(%2)\n"
               : "=d" (fastrand_a), "=d" (fastrand_b)
               : "a" (mAddr));
  
  return fastrand_a;
}

#define random fastrand

static u_short nrPal = 0;
static void setTreePalette(void) {
  const u_short *fruit_cols;
  const PaletteT *grass_pal;
  SpriteT *grass;
  short i;
  if (nrPal) {
    fruit_cols = &fruit_2_pal.colors[1],
    grass_pal = &grass_2_pal;
    grass = grass_2;
    SetColor(0, 0x123);
    SetColor(1, 0x068);
  } else {
    fruit_cols = &fruit_1_pal.colors[1],
    grass_pal = &grass_1_pal;
    grass = grass_1;
    SetColor(0, 0xfdb);
    SetColor(1, 0x653);
  }
  for (i = 0; i < 3; i++) {
    u_short c = *fruit_cols++;
    SetColor(2 + i * 2, c);
    SetColor(3 + i * 2, c);
  }
  for (i = 16; i < 32; i += 4)
    LoadPalette(grass_pal, i);
  for (i = 0; i < NSPRITES; i++)
    CopInsSetSprite(sprptr[i], &grass[i]);
  nrPal ^= 1;
}

// format:
// x, y, [0..255], [0..255]
// delay, [0..255]
// data .. to 255,255 - end of line
// data ... to 255,255, 128, 128 - end of logo
unsigned char greetingsElude0[] = {
  10,10,
  1,
  109,31,100,29,85,27,69,27,50,30,33,36,15,46,0,57,255,255,
  62,25,57,23,59,12,55,26,51,28,48,27,49,13,46,27,41,30,35,29,31,20,31,11,36,2,37,11,36,18,28,33,23,38,16,41,9,38,7,29,11,23,18,20,22,22,20,28,14,33,6,34,255,255,
  99,22,93,23,89,23,84,20,83,14,85,10,89,10,91,14,88,18,82,18,77,14,73,13,68,14,65,18,66,22,68,25,73,23,75,18,76,9,74,0,255,255,
  128,128
};
unsigned char greetingsElude1[] = {
  180,30,
  10,
  109,31,100,29,85,27,69,27,50,30,33,36,15,46,0,57,255,255,
  62,25,57,23,59,12,55,26,51,28,48,27,49,13,46,27,41,30,35,29,31,20,31,11,36,2,37,11,36,18,28,33,23,38,16,41,9,38,7,29,11,23,18,20,22,22,20,28,14,33,6,34,255,255,
  99,22,93,23,89,23,84,20,83,14,85,10,89,10,91,14,88,18,82,18,77,14,73,13,68,14,65,18,66,22,68,25,73,23,75,18,76,9,74,0,255,255,
  128,128
};
unsigned char greetingsElude2[] = {
  140,200,
  20,
  109,31,100,29,85,27,69,27,50,30,33,36,15,46,0,57,255,255,
  62,25,57,23,59,12,55,26,51,28,48,27,49,13,46,27,41,30,35,29,31,20,31,11,36,2,37,11,36,18,28,33,23,38,16,41,9,38,7,29,11,23,18,20,22,22,20,28,14,33,6,34,255,255,
  99,22,93,23,89,23,84,20,83,14,85,10,89,10,91,14,88,18,82,18,77,14,73,13,68,14,65,18,66,22,68,25,73,23,75,18,76,9,74,0,255,255,
  128,128
};

unsigned char *greetsSet0[] = {
  greetingsElude0,
  NULL,
};

unsigned char *greetsSet1[] = {
  greetingsElude1,
  NULL,
};

unsigned char *greetsSet2[] = {
  greetingsElude2,
  NULL,
};

static int greetsIdx = 0;
static void GreetsSetTrack(short idx, unsigned char *greetzData) {
  greetsData[idx].x = *greetzData++;
  greetsData[idx].y = *greetzData++;
  greetsData[idx].delay = *greetzData++;
  greetsData[idx].currentDataPos = greetzData;
}

static void GreetsNextTrack(void) {
  GreetsSetTrack(0, greetsSet0[greetsIdx]);
  GreetsSetTrack(1, greetsSet1[greetsIdx]);
  GreetsSetTrack(2, greetsSet2[greetsIdx]);
  greetsIdx++;
}

static void Init(void) {
  short i;

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  for (i = 0; i < NSPRITES; i++) {
    short hp = X(i * 16 + (WIDTH - 16 * NSPRITES) / 2);
    SpriteUpdatePos(&grass_1[i], hp, Y(HEIGHT - grass_1_height));
    SpriteUpdatePos(&grass_2[i], hp, Y(HEIGHT - grass_2_height));
  }

  /* Move sprites into background. */
  custom->bplcon2 = BPLCON2_PF1P2;

  cp = NewCopList(50);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  CopSetupSprites(cp, sprptr);
  CopEnd(cp);

  setTreePalette();
  GreetsNextTrack();


  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE | DMAF_BLITTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE);

  DeleteBitmap(screen);
  DeleteCopList(cp);
}

static inline BranchT *NewBranch(void) {
  if (lastBranch == &branches[MAXBRANCHES]) {
    return NULL;
  } else {
    return lastBranch++;
  }
}

#define screen_bytesPerRow (WIDTH / 8)
#define fruit_bytesPerRow 2
#define fruit_width 16
#define fruit_height 16
#define fruit_bplSize (fruit_bytesPerRow * fruit_height)

static void _CopyFruit(void *srcbpt, u_short x, u_short y) {
  u_short dstmod = screen_bytesPerRow - fruit_bytesPerRow;
  u_short bltshift = rorw(x & 15, 4);
  u_short bltsize = (fruit_height << 6) | (fruit_bytesPerRow >> 1);
  void *dstbpt = screen->planes[1];
  short dstoff; 
  u_short bltcon0;

  dstoff = (x & ~15) >> 3;
  dstoff += y * screen_bytesPerRow;

  if (bltshift)
    bltsize++, dstmod -= 2;

  dstbpt += dstoff;
  bltcon0 = (SRCA | SRCB | DEST | A_OR_B) | bltshift;

  WaitBlitter();

  if (bltshift) {
    custom->bltalwm = 0;
    custom->bltamod = -2;
  } else {
    custom->bltalwm = -1;
    custom->bltamod = 0;
  }

  custom->bltbmod = dstmod;
  custom->bltdmod = dstmod;
  custom->bltcon0 = bltcon0;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltapt = srcbpt;
  custom->bltbpt = dstbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;

  dstbpt = screen->planes[2];
  dstbpt += dstoff;
  WaitBlitter();

  custom->bltapt = srcbpt + fruit_bplSize;
  custom->bltbpt = dstbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}

static void CopyFruit(u_short *fruit, u_short x, u_short y) {
  _CopyFruit(fruit, x - fruit_width / 2, y - fruit_height / 2);
}

static void DrawBranch(short x1, short y1, short x2, short y2) {
  u_char *data = screen->planes[0];
  short dx, dy, derr;

  u_short bltcon0 = BC0F_LINE_OR;
  u_short bltcon1 = LINEMODE;

  /* Always draw the line downwards. */
  if (y1 > y2) {
    swapr(x1, x2);
    swapr(y1, y2);
  }

  bltcon0 |= rorw(x1 & 15, 4);

  /* Word containing the first pixel of the line. */
  data += screen_bytesPerRow * y1;
  data += (x1 >> 3) & ~1;

  dx = x2 - x1;
  dy = y2 - y1;

  if (dx < 0) {
    dx = -dx;
    if (dx >= dy) {
      bltcon1 |= AUL | SUD;
    } else {
      bltcon1 |= SUL;
      swapr(dx, dy);
    }
  } else {
    if (dx >= dy) {
      bltcon1 |= SUD;
    } else {
      swapr(dx, dy);
    }
  }

  derr = dy + dy - dx;
  if (derr < 0)
    bltcon1 |= SIGNFLAG;

  {
    u_short bltamod = derr - dx;
    u_short bltbmod = dy + dy;
    u_short bltsize = (dx << 6) + 66;

    WaitBlitter();

    custom->bltadat = 0x8000;
    custom->bltbdat = -1;

    custom->bltcon0 = bltcon0;
    custom->bltcon1 = bltcon1;
    custom->bltafwm = -1;
    custom->bltalwm = -1;
    custom->bltcmod = screen_bytesPerRow;
    custom->bltbmod = bltbmod;
    custom->bltamod = bltamod;
    custom->bltdmod = screen_bytesPerRow;
    custom->bltcpt = data;
    custom->bltapt = (void *)(int)derr;
    custom->bltdpt = data;
    custom->bltsize = bltsize;
  }
}

static void MakeBranch(short x, short y) {
  BranchT *b = NewBranch();
  if (!b)
    return;
  b->pos_x = fx4i(x);
  b->pos_y = fx4i(y);
  b->vel_x = fx12i(0);
  b->vel_y = fx12i(-7);
  b->diameter = fx4i(20);
}

static inline void KillBranch(BranchT *b, BranchT **lastp) {
  BranchT *last = --(*lastp);
  if (b != last)
    *b = *last;
}

static bool SplitBranch(BranchT *parent, BranchT **lastp) {
  short newDiameter = normfx(mul16(parent->diameter, fx12f(0.58)));

  if (newDiameter >= fx4f(0.2)) {
    BranchT *b = NewBranch();
    if (!b)
      return true;
    b->pos_x = parent->pos_x;
    b->pos_y = parent->pos_y;
    b->vel_x = parent->vel_x;
    b->vel_y = parent->vel_y;
    b->diameter = newDiameter;
    parent->diameter = newDiameter;
    return true;
  }

  KillBranch(parent, lastp);
  return false;
}

static void DrawGreetigs(short nr) {
  unsigned char x1, x2, y1, y2;

  if (greetsData[nr].currentDataPos == NULL) {
    return;
  }

  if (greetsData[nr].delay) {
    greetsData[nr].delay--;
    return;
  }

  x1 = *greetsData[nr].currentDataPos++;
  y1 = *greetsData[nr].currentDataPos++;

  if (x1 == 128 && y1 == 128) {
    // end of logo
    greetsData[nr].currentDataPos = NULL;
    return;
  }

  x2 = *greetsData[nr].currentDataPos;
  y2 = *(greetsData[nr].currentDataPos+1);

  if (x2 == 255 && y2 == 255) {
    // to next logo branch
    greetsData[nr].currentDataPos++;
    greetsData[nr].currentDataPos++;
    return;
  }

  DrawBranch(
    greetsData[nr].x + x1, greetsData[nr].y + y1,
    greetsData[nr].x + x2, greetsData[nr].y + y2
  );
}

void GrowingTree(BranchT *branches, BranchT **lastp) {
  u_short *fruit = nrPal ? _fruit_2_bpl : _fruit_1_bpl;
  BranchT *b;

  for (b = *lastp - 1; b >= branches; b--) {
    short prev_x = b->pos_x;
    short prev_y = b->pos_y;
    short curr_x, curr_y;

    /*
     * Watch out! The code below is very brittle. You can easily generate
     * overflow that will cause branches to go in the opposite direction.
     * Code below was carefully crafted in Processing prototype using
     * lots of assertion on numeric results of each operation.
     */
    {
      short scale = (random() & 0x7ff) + 0x1000; // Q4.12
      short angle = random();
      short vx = b->vel_x;
      short vy = b->vel_y;
      short mag = abs(vx) / 4 + abs(vy) / 4;
      short vel_scale = div16(shift12(scale), mag);

      b->vel_x = normfx(COS(angle) * scale + vel_scale * vx);
      b->vel_y = normfx(SIN(angle) * scale + vel_scale * vy);

      curr_x = prev_x + b->vel_x_b;
      curr_y = prev_y + b->vel_y_b;
    }

    if (curr_x < fx4i(fruit_width / 2) ||
        curr_x >= fx4i(WIDTH - (fruit_width / 2)) ||
        curr_y < fx4i(fruit_height / 2) ||
        curr_y >= fx4i(HEIGHT - (fruit_height / 2)))
    {
      CopyFruit(fruit, prev_x >> 4, prev_y >> 4);
      KillBranch(b, lastp);
    } else {
      b->pos_x = curr_x;
      b->pos_y = curr_y;

      prev_x >>= 4;
      prev_y >>= 4;
      curr_x >>= 4;
      curr_y >>= 4;
      
      DrawBranch(prev_x, prev_y, curr_x, curr_y);

      if ((u_short)random() < (u_short)(65536 * 0.2f)) {
        if (!SplitBranch(b, lastp)) {
          CopyFruit(fruit, curr_x, curr_y);
        }
      }
    }
  }
}

PROFILE(GrowTree);

static short waitFrame = 0;

static void Render(void) {
  if (waitFrame > 0) {
    if (frameCount - waitFrame < 100) {
      TaskWaitVBlank();
      return;
    }
    waitFrame = 0;
    BitmapClear(screen);
    setTreePalette();
  }

  if (lastBranch == branches) {
    MakeBranch(WIDTH / 2, HEIGHT - fruit_height / 2 - 1);
    mTableIdx++; mTableIdx &= 3;
    fastrand_a = fastrand_b = 0;
    greetsIdx = 0;
    GreetsNextTrack();
  }

  ProfilerStart(GrowTree);
  GrowingTree(branches, &lastBranch);
  DrawGreetigs(0);
  DrawGreetigs(1);
  DrawGreetigs(2);
  ProfilerStop(GrowTree);

  if (lastBranch == branches) {
    waitFrame = frameCount;
  }

  TaskWaitVBlank();
}

EFFECT(GrowingTree, NULL, NULL, Init, Kill, Render);
