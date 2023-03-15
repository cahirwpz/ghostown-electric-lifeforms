#include <bitmap.h>
#include <blitter.h>
#include <copper.h>
#include <effect.h>
#include <fx.h>
#include <sprite.h>
#include <pixmap.h>
#include <color.h>
#include <sync.h>
#include <system/interrupt.h>
#include <system/memory.h>
#include <stdlib.h>

// This effect calculates Conway's game of life (with the classic rules: live
// cells with <2 or >3 neighbours die, live cells with 2-3 neighbouring cells
// live to the next generation and dead cells with 3 neighbours become alive).
// If you need more introductory information check out
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life
//
// It works the following way - on each iteration we start with a board (a 1-bit
// bitmap) that is the current game state. On this bitmap, 1's represent alive
// cells, 0's represent dead cells. We perform several blits: first on this
// bitmap, producing some intermediate results and then on those intermediate
// results as well, extensively making use of the blitter's function generator.
// Minterms for the function generator, the order of blits and input sources
// were picked in such a way, that the end result calculates the next board
// state. Then the board gets its pixels horizontally and vertically doubled -
// horizontally by the CPU (using lookup table), vertically by the copper (by
// line doubling).  Previously calculated game states (with pixels already
// doubled) are kept in a circular buffer and displayed on separate bitplanes
// with dimmer colors (the dimmer the color, the more time has passed since cell
// on that square died). This process is repeated indefinitely.

// max amount boards for current board + intermediate results
#define BOARD_COUNT 12

#define DISP_WIDTH 320
#define DISP_HEIGHT 256

// these are max values for all game of life-based effects
// for dynamically changing the amount kept previous states
// see variable prev_states_depth
#define DISP_DEPTH 4
#define PREV_STATES_DEPTH (DISP_DEPTH + 1)
#define COLORS 32

#define EXT_WIDTH_LEFT 16
#define EXT_WIDTH_RIGHT 16
#define EXT_HEIGHT_TOP 1
#define EXT_HEIGHT_BOTTOM 1
#define EXT_BOARD_MODULO (EXT_WIDTH_LEFT / 8 + EXT_WIDTH_RIGHT / 8)

#define BOARD_WIDTH (DISP_WIDTH / 2)
#define BOARD_HEIGHT (DISP_HEIGHT / 2)
#define EXT_BOARD_WIDTH (BOARD_WIDTH + EXT_WIDTH_LEFT + EXT_WIDTH_RIGHT)
#define EXT_BOARD_HEIGHT (BOARD_HEIGHT + EXT_HEIGHT_TOP + EXT_HEIGHT_BOTTOM)
#define BOARD_DEPTH 1

#define BLTSIZE ((BOARD_HEIGHT << 6) | ((EXT_BOARD_WIDTH / 16)))

// "EXT_BOARD" is the area on which the game of life is calculated
// "BOARD" is the area which will actually be displayed (size before pixel
// doubling). Various constants are best described using a drawing (all shown
// constants are in pixels, drawing not to scale):
//
// -----------------------------------------------------------------------------
// |                                     ^                                     |
// |                                     | EXT_HEIGHT_TOP                      |
// |                                     v                                     |
// |                    -----------------------------------                    |
// |                    |           BOARD_WIDTH     ^     |                    |
// |                    |<--------------------------|---->|                    |
// |                    |                           |     |                    |
// |<------------------>|              BOARD_HEIGHT |     |<------------------>|
// |   EXT_WIDTH_LEFT   |                           |     |   EXT_WIDTH_RIGHT  |
// |                    |                           |     |                    |
// |                    |                           v     |                    |
// |                    -----------------------------------                    |
// |                                     ^                                     |
// |                                     | EXT_HEIGHT_BOTTOM                   |
// |                                     v                                     |
// -----------------------------------------------------------------------------
//

#define RAND_SPAWN_MASK 0xf
#define RAND_SPAWN_MIN_DELAY 8
#define NUM_SCENES 4

#define DEBUG_KBD 0

#if DEBUG_KBD
#include <system/event.h>
#include <system/keyboard.h>
#endif

#include "data/cell-gradient.c"
#include "data/wireworld-vitruvian.c"
#include "data/wireworld-vitruvian-electrons.c"
#include "data/wireworld-fullscreen.c"
#include "data/wireworld-fullscreen-electrons.c"
#include "data/chip.c"

extern TrackT GOLPaletteH;
extern TrackT GOLPaletteS;
extern TrackT GOLPaletteV;
extern TrackT GOLGame;
extern TrackT WireworldDisplayBg;
extern TrackT WireworldBg;

static CopListT *cp;
static BitmapT *current_board;

// current board = boards[0], the rest is intermediate results
static BitmapT *boards[BOARD_COUNT];

#include "games.c"
#include "chip-pal-rotate.c"

// pointers to copper instructions, for rewriting bitplane pointers
static CopInsT *bplptr[DISP_DEPTH];

// pointers to copper instructions, for setting colors
static CopInsT *palptr;

static CopInsT *sprptr[8];

// circular buffer of previous game states as they would be rendered (with
// horizontally doubled pixels)
static BitmapT *prev_states[PREV_STATES_DEPTH];

// states_head % PREV_STATES_DEPTH points to the newest (currently being
// pixel-doubled, not displayed yet) game state in prev_states
static u_short states_head = 0;

// phase (0-8) of blitter calculations
static u_short phase = 0;

// like frameCount, but counts game of life generations (sim steps)
static u_short stepCount = 0;

// are we running wireworld?
static bool wireworld = false;

static short prev_states_depth = PREV_STATES_DEPTH;

// frame at which to spawn specific electron next - counting from stepCount
static short next_spawn[128];

static const ElectronArrayT *cur_electrons;

static const GameDefinitionT *current_game;

static const GameDefinitionT *games[] = {
  &classic_gol, &coagulation,   &maze,       &diamoeba,
  &stains,      &day_and_night, &three_four,
};

static PaletteT palette_vitruvian = {
  .count = 16,
  .colors =
    {
      0x000, // 0000
      0x006, // 0001
      0x026, // 0010
      0x026, // 0011
      0x05B, // 0100
      0x05B, // 0101
      0x05B, // 0110
      0x05B, // 0111
      0x09F, // 1000
      0x09F, // 1001
      0x09F, // 1010
      0x09F, // 1011
      0x09F, // 1100
      0x09F, // 1101
      0x09F, // 1110
      0x09F, // 1111
    },
};

static PaletteT palette_pcb = {
  .count = 16,
  .colors =
    {
      0x000, // 0000
      0x000, // 0001
      0x000, // 0010
      0x000, // 0011
      0x000, // 0100
      0x000, // 0101
      0x000, // 0110
      0x000, // 0111
      0x006, // 1000
      0x009, // 1001
      0x01c, // 1010
      0x01c, // 1011
      0x03f, // 1100
      0x03f, // 1101
      0x03f, // 1110
      0x03f, // 1111
    },
};

typedef struct PalState {
  u_short *dynamic;
  u_short *cur;
  u_short phase;
} PalStateT;

static PalStateT pal;

// Used by CPU to quickly transform 1x1 pixels into 2x1 pixels.
static u_short double_pixels[256];

static void MakeDoublePixels(void) {
  u_short *data = double_pixels;
  u_short w = 0;
  short i;

  for (i = 0; i < 256; i++) {
    *data++ = w;
    w |= 0xAAAA; /* let carry propagate through odd bits */
    w++;
    w &= 0x5555;
    w |= w << 1;
  }
}

// setup blitter to calculate a function of three horizontally adjacent lit
// pixels the setup in this blit is as follows (what data each channel sees):
//            -1                 0                 1                 2
// C: [---------------] [---------------] [c--------------] [---------------]
//            -1                 0                 1                 2
// A: [---------------] [--------------a] [a--------------] [---------------]
//                                     |   ^
//                                     >>>>^
//                         'a' gets shifted one to the right
//
//             0                 1                 2                 3
// B: [---------------] [-b-------------] [b--------------] [---------------]
//                        |                ^
//                        >>>>>>>>>>>>>>>>>^
//                'b' starts one word later and gets shifted 15 to the right
//
// Thus a, b and c are lined up properly to perform boolean function on them
//
static void BlitAdjacentHorizontal(const BitmapT *sourceA,
                                   __unused const BitmapT *sourceB,
                                   __unused const BitmapT *sourceC,
                                   const BitmapT *target, u_short minterms) {
  void *srcCenter = sourceA->planes[0] + sourceA->bytesPerRow;    // C channel
  void *srcRight = sourceA->planes[0] + sourceA->bytesPerRow + 2; // B channel
  void *srcLeft = sourceA->planes[0] + sourceA->bytesPerRow;      // A channel

  custom->bltcon0 = ASHIFT(1) | minterms | (SRCA | SRCB | SRCC | DEST);
  custom->bltcon1 = BSHIFT(15);

  custom->bltcpt = srcCenter;
  custom->bltbpt = srcRight;
  custom->bltapt = srcLeft;
  custom->bltdpt = target->planes[0] + target->bytesPerRow;
  custom->bltsize = BLTSIZE;
}

// setup blitter to calculate a function of three vertically adjacent lit pixels
static void BlitAdjacentVertical(const BitmapT *sourceA,
                                 __unused const BitmapT *sourceB,
                                 __unused const BitmapT *sourceC,
                                 const BitmapT *target, u_short minterms) {
  void *srcCenter = sourceA->planes[0] + sourceA->bytesPerRow;   // C channel
  void *srcUp = sourceA->planes[0];                              // A channel
  void *srcDown = sourceA->planes[0] + 2 * sourceA->bytesPerRow; // B channel

  custom->bltcon0 = minterms | (SRCA | SRCB | SRCC | DEST);
  custom->bltcon1 = 0;

  custom->bltapt = srcUp;
  custom->bltbpt = srcDown;
  custom->bltcpt = srcCenter;
  custom->bltdpt = target->planes[0] + target->bytesPerRow;
  custom->bltsize = BLTSIZE;
}

// setup blitter for a standard blit without shifts
static void BlitFunc(const BitmapT *sourceA, const BitmapT *sourceB,
                     const BitmapT *sourceC, const BitmapT *target,
                     u_short minterms) {
  custom->bltcon0 = minterms | (SRCA | SRCB | SRCC | DEST);
  custom->bltcon1 = 0;

  custom->bltapt = sourceA->planes[0] + sourceA->bytesPerRow;
  custom->bltbpt = sourceB->planes[0] + sourceB->bytesPerRow;
  custom->bltcpt = sourceC->planes[0] + sourceC->bytesPerRow;
  custom->bltdpt = target->planes[0] + target->bytesPerRow;
  custom->bltsize = BLTSIZE;
}

static void InitSpawnFrames(const ElectronArrayT *electrons) {
  short i;
  for (i = 0; i < electrons->num_electrons; i++)
    next_spawn[i] = stepCount + (random() & RAND_SPAWN_MASK);
}

static void SpawnElectrons(const ElectronArrayT *electrons, short board_heads,
                           short board_tails) {
  u_char *bpl_heads = boards[board_heads]->planes[0];
  u_char *bpl_tails = boards[board_tails]->planes[0];
  short *pts = (short *)electrons->points;
  short n = electrons->num_electrons - 1;
  if (n < 0)
    return;
  do {
    if (next_spawn[n] <= stepCount) {
      short hx = (*pts++) + EXT_WIDTH_LEFT;
      short hy = (*pts++) + EXT_HEIGHT_TOP;
      short tx = (*pts++) + EXT_WIDTH_LEFT;
      short ty = (*pts++) + EXT_HEIGHT_TOP;
      int posh = EXT_BOARD_WIDTH * hy + hx;
      int post = EXT_BOARD_WIDTH * ty + tx;
      bset(bpl_heads + (posh >> 3), ~posh);
      bset(bpl_tails + (post >> 3), ~post);
      next_spawn[n] += (random() & RAND_SPAWN_MASK) + RAND_SPAWN_MIN_DELAY;
    } else {
      pts += 4;
    }
  } while (--n != -1);
}

// This is a bit hacky way to reuse the current interface for blitter phases
// to do something after the last blit has been completed. For technical
// reasons wireworld implementation requires 2 separate games to be ran
// in alternating fashion, and the switch between them must be done precisely
// when the board for a game finishes being calculated or things will break
// in ways undebuggable by single-stepping the game state
static void WireworldSwitch(__unused const BitmapT *sourceA,
                            __unused const BitmapT *sourceB,
                            __unused const BitmapT *sourceC,
                            __unused const BitmapT *target,
                            __unused u_short minterms) {
  // which wireworld game (0-1) phase are we on
  static short wireworld_step = 0;

  current_board = boards[wireworld_step];
  current_game = wireworlds[wireworld_step];
  wireworld_step ^= 1;

  // set pixels on correct board
  SpawnElectrons(cur_electrons, wireworld_step ^ 1, wireworld_step);
}

static void (*PixelDouble)(u_char *source asm("a0"), u_short *target asm("a1"),
                           u_short *lut asm("a2"));

#define PixelDoubleSize \
  ((BOARD_WIDTH / 8) * BOARD_HEIGHT * 10 + BOARD_HEIGHT * 2 + 6)

// doubles pixels horizontally
static void MakePixelDoublingCode(const BitmapT *bitmap) {
  u_short x;
  u_short y;
  u_short *code = (void *)PixelDouble;

  *code++ = 0x7200 | (EXT_BOARD_MODULO & 0xFF); // moveq #EXT_BOARD_MODULO,d1
  for (y = EXT_HEIGHT_TOP; y < bitmap->height - EXT_HEIGHT_BOTTOM; y++) {
    for (x = EXT_WIDTH_LEFT / 8; x < bitmap->bytesPerRow - EXT_WIDTH_RIGHT / 8;
         x++) {
      *code++ = 0x7000; // moveq.l #0,d0           # 4
      *code++ = 0x1018; // move.b (a0)+,d0         # 8
      *code++ = 0xd040; // add.w  d0,d0            # 4
      *code++ = 0x32f2;
      *code++ = 0x0000; // move.w (a2,d0.w),(a1)+  # 18
      // perform a lookup in the pixel doubling lookup table
      // (e.g. 00100110 -> 0000110000111100)
      // *double_target++ = double_pixels[*double_src++];
    }
    *code++ = 0xD1C1; // adda.l d1,a0
    // double_src += EXT_BOARD_MODULO & 0xFF;
    // bitmap modulo - skip the extra EXT_BOARD_MODULO bytes on the edges
    // (EXT_WIDTH_LEFT/8 bytes on the left, EXT_WIDTH_RIGHT/8 bytes on the right
    // on the next row)
  }
  *code++ = 0x4e75; // rts
}

static void MakeCopperList(CopListT *cp) {
  u_short i;

  CopInit(cp);
  // initially previous states are empty
  // save addresses of these instructions to change bitplane
  // order when new state gets generated
  for (i = 0; i < DISP_DEPTH; i++)
    bplptr[i] = CopMove32(cp, bplpt[i], prev_states[i]->planes[0]);

  CopSetupSprites(cp, sprptr);
  for (i = 0; i < 8; i++) {
    SpriteT *spr = &wireworld_chip[i];
    SpriteUpdatePos(spr, X(DISP_WIDTH/2 + (i/2)*16 - 32), Y(DISP_HEIGHT/2 - spr->height/2));
    if (TrackValueGet(&WireworldBg, frameCount) == 1) {
      CopInsSetSprite(sprptr[i], spr);
    } else {
      CopInsSetSprite(sprptr[i], NULL);
    }
  }

  palptr = CopSetColor(cp, 0, 0);
  for (i = 1; i < COLORS; i++)
    CopSetColor(cp, i, 0);

  for (i = 1; i <= DISP_HEIGHT; i += 2) {
    // vertical pixel doubling
    CopMove16(cp, bpl1mod, -prev_states[0]->bytesPerRow);
    CopMove16(cp, bpl2mod, -prev_states[0]->bytesPerRow);
    CopWaitSafe(cp, Y(i), 0);
    CopMove16(cp, bpl1mod, 0);
    CopMove16(cp, bpl2mod, 0);
    CopWaitSafe(cp, Y(i + 1), 0);
  }
  CopEnd(cp);
}

static void UpdateBitplanePointers(void) {
  BitmapT *cur;
  u_short i;
  u_short last = states_head + 1;
  for (i = 0; i < prev_states_depth - 1; i++) {
    // update bitplane order: (states_head + 2) % prev_states_depth iterates
    // from the oldest+1 (to facilitate double buffering; truly oldest state is
    // the one we won't display as it's gonna be a buffer for the next game
    // state) to newest game state, so 0th bitplane displays the oldest+1 state
    // and (prev_states_depth-1)'th bitplane displays the newest state
    last++;
    if (last >= prev_states_depth)
      last -= prev_states_depth;
    cur = prev_states[last];
    CopInsSet32(bplptr[i], cur->planes[0]);
  }
}

static void ChangePalette(const u_short *pal) {
  register short i asm("d6");
  register short j asm("d7");

  // unrolling this loop gives worse performance for some reason
  // increment `pal` on every power of 2, essentially setting
  // 4 consecutive colors from `pal` to 1, 2, 4 and 8 colors respectively
  for (i = 1; i < COLORS;) {
    short next_i = i + i;
    CopInsT *ins = &palptr[i];
    for (j = i; j < next_i; j++)
      CopInsSet16(ins++, *pal);
    i = next_i;
    pal++;
  }
}

static void CyclePalette(PalStateT *pal) {
  u_short *end = (u_short *)(pal->dynamic) + 1024 - 4;

  ChangePalette(pal->cur);

  if (pal->phase)
    pal->cur -= 4;
  else
    pal->cur += 4;

  if (pal->cur >= end || pal->cur <= pal->dynamic) {
    pal->phase ^= 1;
    if (pal->phase)
      pal->cur -= 4;
    else
      pal->cur += 4;
  }
}

static void GameOfLife(void *boards) {
  ClearIRQ(INTF_BLIT);
  if (phase < current_game->num_phases) {
    const BlitterPhaseT *p = &current_game->phases[phase];
    p->blitfunc(*(const BitmapT **)(boards + p->srca),
                *(const BitmapT **)(boards + p->srcb),
                *(const BitmapT **)(boards + p->srcc),
                *(const BitmapT **)(boards + p->dst), p->minterm);
  }
  phase++;
}

static short scene_count = 0;

static void Load(void) {
  static bool loaded = false;

  if (loaded)
    return;

  TrackInit(&GOLGame);
  TrackInit(&WireworldDisplayBg);
  TrackInit(&WireworldBg);
  TrackInit(&GOLPaletteH);
  TrackInit(&GOLPaletteS);
  TrackInit(&GOLPaletteV);

  loaded = true;
}

static void SharedPreInit(void) {
  static bool allocated = false;

  short i;

  scene_count++;
  if (!allocated) {
    for (i = 0; i < BOARD_COUNT; i++)
      boards[i] = NewBitmap(EXT_BOARD_WIDTH, EXT_BOARD_HEIGHT, BOARD_DEPTH);

    for (i = 0; i < PREV_STATES_DEPTH; i++) {
      // only needs half the vertical resolution, other half
      // achieved via copper line doubling
      prev_states[i] = NewBitmap(DISP_WIDTH, DISP_HEIGHT / 2, BOARD_DEPTH);
    }

    MakeDoublePixels();
    PixelDouble = MemAlloc(PixelDoubleSize, MEMF_PUBLIC);
    MakePixelDoublingCode(boards[0]);

    pal.dynamic = MemAlloc(4 * 256 * sizeof(u_short), MEMF_PUBLIC);
    for (i = 0; i < 256; i++) {
      u_char h = TrackValueGet(&GOLPaletteH, i);
      u_char s = TrackValueGet(&GOLPaletteS, i);
      u_char v = TrackValueGet(&GOLPaletteV, i);
      pal.dynamic[4 * i] = HsvToRgb(h, s, v / 8);
      pal.dynamic[4 * i + 1] = HsvToRgb(h, s, v / 4);
      pal.dynamic[4 * i + 2] = HsvToRgb(h, s, v / 2);
      pal.dynamic[4 * i + 3] = HsvToRgb(h, s, v);
    }
    pal.cur = pal.dynamic;

    allocated = true;
  }

  SetupPlayfield(MODE_LORES, DISP_DEPTH, X(0), Y(0), DISP_WIDTH, DISP_HEIGHT);

  cp = NewCopList(850);
  MakeCopperList(cp);
  CopListActivate(cp);

#if DEBUG_KBD
  KeyboardInit();
#endif

  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE | DMAF_COPPER);

  current_board = boards[0];
  for (i = 0; i < PREV_STATES_DEPTH; i++) {
    BitmapClear(prev_states[i]);
  }
  stepCount = 0;
}

static void SharedPostInit(void) {
  // This took good part of my sanity do debug
  // Wait until the clearing is complete, otherwise we would overwrite
  // the bits we're setting below *after* they're set
  WaitBlitter();
  ClearIRQ(INTF_BLIT);

  custom->bltafwm = 0xFFFF;
  custom->bltalwm = 0xFFFF;
  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  SetIntVector(INTB_BLIT, (IntHandlerT)GameOfLife, boards);
  EnableINT(INTF_BLIT);
}

static void InitWireworld(void) {
  short i;
  const BitmapT *desired_bg;
  const PaletteT *pal;
  short display_bg = TrackValueGet(&WireworldDisplayBg, frameCount);
  short bg_idx = TrackValueGet(&WireworldBg, frameCount);

  current_game = &wireworld1;
  wireworld = true;
  prev_states_depth = display_bg ? 4 : 5;
  desired_bg = bg_idx ? &wireworld_pcb : &wireworld_vitruvian;
  cur_electrons = bg_idx ? &pcb_electrons : &vitruvian_electrons;
  pal = bg_idx ? &palette_pcb : &palette_vitruvian;

  SharedPreInit();
  for (i = 0; i < pal->count; i++)
    CopInsSet16(&palptr[i], pal->colors[i]);
  InitSpawnFrames(cur_electrons);

  if (display_bg) {
    BitmapT *tmp = NewBitmap(EXT_BOARD_WIDTH, EXT_BOARD_HEIGHT, BOARD_DEPTH);
    BitmapCopy(tmp, 0, 0, desired_bg);
    WaitBlitter();
    PixelDouble(tmp->planes[0], prev_states[4]->planes[0], double_pixels);
    DeleteBitmap(tmp);
    CopInsSet32(bplptr[3], prev_states[4]->planes[0]);
  }

  // board 11 is special in case of wireworld - it contains the electron paths
  BitmapCopy(boards[11], EXT_WIDTH_LEFT, EXT_HEIGHT_TOP, desired_bg);

  // electron heads/tails in case of wireworld
  BitmapClear(boards[0]);
  BitmapClear(boards[1]);

  SharedPostInit();
}

static void InitGameOfLife(void) {
  current_game = games[0];
  wireworld = false;
  prev_states_depth = 5;

  SharedPreInit();

  BitmapClear(boards[0]);
  BitmapCopy(boards[0], EXT_WIDTH_LEFT, EXT_HEIGHT_TOP, &wireworld_pcb);

  SharedPostInit();
}

static void Kill(void) {
  short i;
  DisableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE | DMAF_COPPER);
  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

#if DEBUG_KBD
  KeyboardKill();
#endif

  // last time we show the effect - deallocate
  if (scene_count == NUM_SCENES) {
    for (i = 0; i < BOARD_COUNT; i++)
      DeleteBitmap(boards[i]);

    for (i = 0; i < PREV_STATES_DEPTH; i++)
      DeleteBitmap(prev_states[i]);

    MemFree(PixelDouble);
    MemFree(pal.dynamic);
  }

  DeleteCopList(cp);
}

PROFILE(GOLStep)

static void GolStep(void) {
  void *dst = prev_states[states_head]->planes[0];
  void *src =
    current_board->planes[0] + EXT_BOARD_WIDTH / 8 + EXT_WIDTH_LEFT / 8;

  ProfilerStart(GOLStep);
  if (!wireworld)
    current_game = games[TrackValueGet(&GOLGame, frameCount)];
  WaitBlitter();
  PixelDouble(src, dst, double_pixels);
  UpdateBitplanePointers();
  states_head++;
  if (states_head >= prev_states_depth)
    states_head -= prev_states_depth;
  phase = 0;
  GameOfLife(boards);
  if (wireworld) {
    SpriteCyclePal(&palptr[16], rot, 3);
  } else {
    CyclePalette(&pal);
  }

  stepCount++;

  ProfilerStop(GOLStep);
}

#if DEBUG_KBD
static bool run_continous = 1;

static bool HandleEvent(void) {
  EventT ev[1];

  if (!PopEvent(ev))
    return true;

  if (ev->type == EV_KEY && !(ev->key.modifier & MOD_PRESSED)) {
    if (ev->key.code == KEY_ESCAPE)
      return false;
    else if (ev->key.code == KEY_SPACE)
      GolStep();
    else if (ev->key.code == KEY_C)
      run_continous ^= 1;
  }

  return true;
}

static void Render(void) {
  if (run_continous)
    GolStep();
  exitLoop = !HandleEvent();
}
#else
static void Render(void) {
  GolStep();
}
#endif

EFFECT(Wireworld, Load, NULL, InitWireworld, Kill, Render);
EFFECT(GameOfLife, Load, NULL, InitGameOfLife, Kill, Render);
