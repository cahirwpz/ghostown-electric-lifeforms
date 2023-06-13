#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <gfx.h>
#include <line.h>
#include <stdlib.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 2

static CopListT *cp;
static CopInsT *bplptr[DEPTH];
static BitmapT *screen;

#include "data/fruit.c"

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

//#define MAXBRANCHES 256
#define MAXBRANCHES 192

static BranchT branches[MAXBRANCHES];
static BranchT *lastBranch = branches;

static int mTable[] = {
  0x3050B28C, 0xD061A7F9,
  0x3150B28C, 0xD161A7F9,
  0x3250B28C, 0xD261A7F9,
  0x3350B28C, 0xD431A7F9
};
static int mTableIdx = 0;
//static int canSee = 1;

static inline int fastrand(void) {
//  static int m[2] = { 0x3E50B28C, 0xD461A7F9 };

  int a, b;
  int *mAddr = &mTable[mTableIdx*2];

  // if (canSee) {
  //   Log("mTableIdx = %d, mTable[0] = %x, mTable[1] = %x\n", mTableIdx, mAddr[0], mAddr[1]);
  //   canSee = 0;
  // }

  // https://www.atari-forum.com/viewtopic.php?p=188000#p188000
  asm volatile("move.l (%2)+,%0\n"
               "move.l (%2),%1\n"
               "swap   %1\n"
               "add.l  %0,(%2)\n"
               "add.l  %1,-(%2)\n"
               : "=d" (a), "=d" (b)
               : "a" (mAddr));
  
/*
    lea     m(pc),a0
    lea     4(a0),a1

    move.l  (a0),d0  ; AB
    move.l  (a1),d1  ; CD
    swap    d1       ; DC
    add.l   d1,(a0)  ; AB + DC
    add.l   d0,(a1)  ; CD + AB

*/  
  return a;
}

#define random fastrand

static u_short palette[] = {
  // pal0
  0xfdb,
  0x653,
  0xd43,
  0xd43,
  // pal1
  0x123,
  0x068,
  0x9df,
  0x9df
};

static u_short nrPal = 0;
static void setTreePalette(void) {
  u_short x = 0;
  u_short *pal = &palette[nrPal<<2];
  for (x = 0; x < 4; x++) {
    SetColor(x, *pal++);
  }
  nrPal++;
  nrPal &= 1;
}

// could be readed from group data
unsigned short greets_x = 0;
unsigned short greets_y = 0;

unsigned char greetingsElude[] = {
  // todo: start x,y here
  109,31,100,29,85,27,69,27,50,30,33,36,15,46,0,57,255,255,
  62,25,57,23,59,12,55,26,51,28,48,27,49,13,46,27,41,30,35,29,31,20,31,11,36,2,37,11,36,18,28,33,23,38,16,41,9,38,7,29,11,23,18,20,22,22,20,28,14,33,6,34,255,255,
  99,22,93,23,89,23,84,20,83,14,85,10,89,10,91,14,88,18,82,18,77,14,73,13,68,14,65,18,66,22,68,25,73,23,75,18,76,9,74,0,255,255,
  128,128
};

unsigned char *greetings[] = {
  greetingsElude,
  NULL,
};

static unsigned char *currGreetsPtr;
static int greetsIdx = 0;

static void GreetsNextTrack(void) {
  currGreetsPtr = greetings[greetsIdx];
  if (currGreetsPtr == NULL) {
    return;
  }
  greetsIdx++;
}

static void GreetsInit(void) {
  GreetsNextTrack();
  BlitterLineSetup(screen, 0, LINE_OR|LINE_SOLID);
}

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  setTreePalette();
  GreetsInit();

  cp = NewCopList(50);
  CopInit(cp);
  CopSetupBitplanes(cp, bplptr, screen, DEPTH);
  CopEnd(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_BLITTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_BLITTER | DMAF_RASTER);

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

static void _CopyFruit(u_short x, u_short y) {
  u_short dstmod = screen_bytesPerRow - fruit_bytesPerRow;
  u_short bltshift = rorw(x & 15, 4);
  u_short bltsize = (fruit_height << 6) | (fruit_bytesPerRow >> 1);
  void *srcbpt = fruit.planes[0];
  void *dstbpt = screen->planes[1];
  u_short bltcon0;

  if (bltshift)
    bltsize++, dstmod -= 2;

  dstbpt += (x & ~15) >> 3;
  dstbpt += y * screen_bytesPerRow;
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
}

static void CopyFruit(u_short x, u_short y) {
  _CopyFruit(x - fruit_width / 2, y - fruit_height / 2);
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
  if (derr < 0) {
    bltcon1 |= SIGNFLAG;
  }

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


static void DrawGreetigs(void) {
  unsigned char x1, x2, y1, y2;
  if (currGreetsPtr == NULL) {
    return;
  }
  x1 = *currGreetsPtr++;
  y1 = *currGreetsPtr++;

  x2 = *currGreetsPtr;
  y2 = *(currGreetsPtr+1);

  if (x1 == 128 && y1 == 128) {
    // end of group
    GreetsNextTrack();
    return;
  }

  if (x2 == 255 || y2 == 255) {
    // to next curve
    currGreetsPtr++; currGreetsPtr++;
    return;
  }

  //Log("x1=%d, y1=%d, x2=%d, y2=%d\n", x1, y1, x2, y2);
  //BlitterLine(x1, y1, x2, y2);
  DrawBranch(
    greets_x + x1, greets_y + y1,
    greets_x + x2, greets_y + y2
  );
}


void GrowingTree(BranchT *branches, BranchT **lastp) {
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
      CopyFruit(prev_x >> 4, prev_y >> 4);
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
          CopyFruit(curr_x, curr_y);
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
    MakeBranch((WIDTH-60) / 2, HEIGHT - fruit_height / 2 - 1);
    mTableIdx++; mTableIdx &= 3;
    //canSee++;
    greetsIdx = 0;
  }

  ProfilerStart(GrowTree);
  GrowingTree(branches, &lastBranch);

  DrawGreetigs();

  ProfilerStop(GrowTree);

  if (lastBranch == branches) {
    waitFrame = frameCount;
  }

  TaskWaitVBlank();
}

EFFECT(GrowingTree, NULL, NULL, Init, Kill, Render, NULL);
