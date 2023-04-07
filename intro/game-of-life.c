#include <intro.h>
#include <bitmap.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <sprite.h>
#include <pixmap.h>
#include <color.h>
#include <sync.h>
#include <system/interrupt.h>
#include <system/memory.h>
#include <stdlib.h>

#define DEBUG_KBD 0

#if DEBUG_KBD
#include <system/event.h>
#include <system/keyboard.h>
#endif

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

#define NUM_SCENES 4
#define TILES_N (40 * 32)

#include "data/wireworld-vitruvian.c"
#include "data/wireworld-fullscreen.c"
#include "data/chip.c"
#include "data/wireworld-pcb-pal.c"

#include "bitmaps.h"

extern TrackT GOLGame;
extern TrackT WireworldDisplayBg;
extern TrackT WireworldBg;
extern TrackT WireworldFadeIn;

static CopListT *cp;
static BitmapT *current_board;

// current board = boards[0], the rest is intermediate results
static BitmapT *boards[BOARD_COUNT];

// pointers to copper instructions, for rewriting bitplane pointers
static CopInsT *bplptr[DISP_DEPTH];

// pointers to copper instructions, for setting colors
static CopInsT *palptr;

static CopInsT *sprptr[8];

// circular buffer of previous game states as they would be rendered (with
// horizontally doubled pixels)
static BitmapT *prev_states[PREV_STATES_DEPTH];

static BitmapT *background[2];

static u_char tileShow[TILES_N] = {0};

// states_head % PREV_STATES_DEPTH points to the newest (currently being
// pixel-doubled, not displayed yet) game state in prev_states
static __code short states_head = 0;

// phase (0-8) of blitter calculations
static __code volatile short phase = 0;

// like frameCount, but counts game of life generations (sim steps)
static __code short stepCount = 0;

// are we running wireworld?
static __code bool wireworld = false;

static __code short prev_states_depth = PREV_STATES_DEPTH;

#include "gol-cycling.c"
#include "gol-doubling.c"
#include "gol-games.c"
#include "gol-electrons.c"
#include "gol-palette.c"
#include "gol-transparency.c"

static const GameDefinitionT *current_game;

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

void MakeCopperList(CopListT *cp) {
  short i;
  short *color = wireworld_pcb_pal_pixels;
  short display_bg = TrackValueGet(&WireworldDisplayBg, frameCount);
  short pal_start = display_bg ? 8 : 0;

  CopInit(cp);
  // initially previous states are empty
  // save addresses of these instructions to change bitplane
  // order when new state gets generated
  for (i = 0; i < DISP_DEPTH; i++)
    bplptr[i] = CopMove32(cp, bplpt[i], prev_states[i]->planes[0]);

  CopSetupSprites(cp, sprptr);
  for (i = 0; i < 8; i++) {
    SpriteT *spr = &wireworld_chip[i];
    SpriteUpdatePos(spr, X(DISP_WIDTH / 2 + (i / 2) * 16 - 33),
                    Y(DISP_HEIGHT / 2 - spr->height / 2));
    if (wireworld && TrackValueGet(&WireworldBg, frameCount) == 1) {
      CopInsSetSprite(sprptr[i], spr);
    } else {
      CopInsSet32(sprptr[i], NullSprData);
    }
  }

  palptr = CopSetColor(cp, 0, 0);
  for (i = 1; i < 32; i++)
    CopSetColor(cp, i, 0);
  // CopLoadPal(cp, &wireworld_chip_pal, 16);

  for (i = 1; i <= DISP_HEIGHT; i += 2) {
    // vertical pixel doubling
    if (wireworld && (i - 1) % 8 == 0) {
      // if we're displaying background set its colors without any electrons
      if (display_bg)
        CopSetColor(cp, pal_start, *color++);
      // else background should be black

      // if we're displaying background set its colors with electrons
      // if we're not displaying background set first half of the color palette
      CopSetColor(cp, pal_start + 1, *color++);
      CopSetColor(cp, pal_start + 2, *color);
      CopSetColor(cp, pal_start + 3, *color++);
      CopSetColor(cp, pal_start + 4, *color);
      CopSetColor(cp, pal_start + 5, *color);
      CopSetColor(cp, pal_start + 6, *color);
      CopSetColor(cp, pal_start + 7, *color++);

      // if we're not displaying background set second half of the color palette
      if (!display_bg) {
        short j;
        for (j = pal_start + 8; j < 16; j++)
          CopSetColor(cp, j, *color);
        color++;
      }
    }
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
  short i;
  short last = states_head + 1;
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
  states_head++;
  if (states_head >= prev_states_depth)
    states_head -= prev_states_depth;
}

static void BlitGameOfLife(void *boards, short phase) {
  const BlitterPhaseT *p = &current_game->phases[phase];
  p->blitfunc(*(const BitmapT **)(boards + p->srca),
              *(const BitmapT **)(boards + p->srcb),
              *(const BitmapT **)(boards + p->srcc),
              *(const BitmapT **)(boards + p->dst), p->minterm);
}

static void GameOfLife(void *boards) {
  // run all blits except the last one
  if (phase < current_game->num_phases - 1)
    BlitGameOfLife(boards, phase);
  // always increment phase (used for detecting when we finish blits)
  phase++;
}

static short scene_count = 0;

static void LoadBackground(const BitmapT *bg, u_short x, u_short y, short idx) {
  BitmapT *tmp = NewBitmapCustom(EXT_BOARD_WIDTH, EXT_BOARD_HEIGHT, BOARD_DEPTH,
                                 BM_DISPLAYABLE);
  BitmapClear(tmp); // comment for cool effects!
  BitmapCopy(tmp, x, y, bg);
  WaitBlitter();
  PixelDouble(tmp->planes[0], background[idx]->planes[0], double_pixels);
  DeleteBitmap(tmp);
}

static void InitWireworldFadein(void) {
  short x, y;
  for (x = 0; x < 40; x++) {
    for (y = 0; y < 32; y++) {
      short dx = x - 20;
      short dy = y - 16;
      tileShow[y * 40 + x] = isqrt(dx * dx + dy * dy);
    }
  }
}

static inline void CopyBlock(short pos) {
  // overall this is cheaper than keeping track of separate x and y because it
  // only happens 1280 times (40 * 32)
  short start = (pos / 40) * 160 + (pos % 40);
  u_char *src = (u_char *)(background[0]->planes[0] + start);
  u_char *dst = (u_char *)(background[1]->planes[0] + start);
  *dst = *src;
  dst += 40;
  src += 40;
  *dst = *src;
  dst += 40;
  src += 40;
  *dst = *src;
  dst += 40;
  src += 40;
  *dst = *src;
}

static inline void BoardFadeIn(void) {
  u_char *tile = tileShow;
  short n = TILES_N - 1;
  do {
    *tile = (*tile)--;
    if (*tile == 0)
      CopyBlock(n);
    tile++;
  } while (--n != -1);
}

static inline void ChipFadeIn(short phase) {
  short i;
  for (i = 16; i < 32; i++)
    CopInsSet16(
      palptr + i,
      ColorTransition(0x000, wireworld_chip_pal.colors[i - 16], 15 - phase));
}

static void SharedPreInit(void) {
  static bool allocated = false;

  short i;

  scene_count++;
  if (!allocated) {
    MakeDoublePixels();
    PixelDouble = MemAlloc(PixelDoubleSize, MEMF_PUBLIC);
    MakePixelDoublingCode();

    allocated = true;
  }

  for (i = 0; i < BOARD_COUNT; i++)
    boards[i] = NewBitmapCustom(EXT_BOARD_WIDTH, EXT_BOARD_HEIGHT, BOARD_DEPTH,
                                BM_DISPLAYABLE);

  for (i = 0; i < PREV_STATES_DEPTH; i++) {
    // only needs half the vertical resolution, other half
    // achieved via copper line doubling
    prev_states[i] =
      NewBitmapCustom(DISP_WIDTH, DISP_HEIGHT / 2, BOARD_DEPTH, BM_DISPLAYABLE);
  }

  for (i = 0; i < 2; i++)
    background[i] =
      NewBitmapCustom(DISP_WIDTH, DISP_HEIGHT / 2, BOARD_DEPTH, BM_DISPLAYABLE);

  states_head = 0;
  current_board = boards[0];
  stepCount = 0;

  EnableDMA(DMAF_BLITTER);

  for (i = 0; i < PREV_STATES_DEPTH; i++)
    BitmapClear(prev_states[i]);
  // these two bitmaps need to be cleared to prevent graphical bugs
  // ideally only their borders need to be cleared but this is simpler
  BitmapClear(boards[2]);
  BitmapClear(boards[3]);

#if DEBUG_KBD
  KeyboardInit();
#endif

  SetupPlayfield(MODE_LORES, DISP_DEPTH, X(0), Y(0), DISP_WIDTH, DISP_HEIGHT);

  cp = NewCopList(1310);
  MakeCopperList(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
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

  phase = 0;

  SetIntVector(INTB_BLIT, (IntHandlerT)GameOfLife, boards);
  EnableINT(INTF_BLIT);
}

static void InitWireworld(void) {
  const BitmapT *desired_bg;
  short display_bg = TrackValueGet(&WireworldDisplayBg, frameCount);
  short bg_idx = TrackValueGet(&WireworldBg, frameCount);

  current_game = &wireworlds[1];
  wireworld = true;
  prev_states_depth = display_bg ? 4 : 5;
  desired_bg = bg_idx ? &wireworld_pcb : &wireworld_vitruvian;
  cur_electrons = bg_idx ? &pcb_electrons : &vitruvian_electrons;

  SharedPreInit();
  InitSpawnFrames(cur_electrons);

  if (display_bg) {
    LoadBackground(desired_bg, 0, 0, 0);
    BitmapClear(background[1]); // important!
    WaitBlitter();
    CopInsSet32(bplptr[3], background[1]->planes[0]);
    InitWireworldFadein();
  }

  // board 11 is special in case of wireworld - it contains the electron paths
  BitmapCopy(boards[11], EXT_WIDTH_LEFT, EXT_HEIGHT_TOP, desired_bg);

  // electron heads/tails in case of wireworld
  BitmapClear(boards[0]);
  BitmapClear(boards[1]);

  SharedPostInit();
}

static void InitGameOfLife(void) {
  short i;
  short display_bg = TrackValueGet(&WireworldDisplayBg, frameCount);
  current_game = &games[0];
  wireworld = false;
  prev_states_depth = 4;

  SharedPreInit();

  TransitionPal(palette_gol + 1);
  for (i = 0; i < 16; i++)
    CopInsSet16(&palptr[i], palette_gol[i]);

  LoadBackground(&electric_logo, 0, 32, 0);
  LoadBackground(&lifeforms_logo, 0, 32, 1);

  BitmapClear(boards[0]);
  BitmapCopy(boards[0], EXT_WIDTH_LEFT, EXT_HEIGHT_TOP,
             display_bg ? &wireworld_pcb : &wireworld_vitruvian);

  SharedPostInit();
}

static void Kill(void) {
  short i;

  ResetSprites();
  DisableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE | DMAF_COPPER);
  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

#if DEBUG_KBD
  KeyboardKill();
#endif

  for (i = 0; i < BOARD_COUNT; i++)
    DeleteBitmap(boards[i]);

  for (i = 0; i < PREV_STATES_DEPTH; i++)
    DeleteBitmap(prev_states[i]);

  for (i = 0; i < 2; i++)
    DeleteBitmap(background[i]);

  // last time we show the effect - deallocate
  if (scene_count == NUM_SCENES)
    MemFree(PixelDouble);

  DeleteCopList(cp);
}

PROFILE(GOLStep)

static void GolStep(void) {
  void *dst = prev_states[states_head]->planes[0];
  void *src =
    current_board->planes[0] + EXT_BOARD_WIDTH / 8 + EXT_WIDTH_LEFT / 8;

  ProfilerStart(GOLStep);
  if (!wireworld)
    current_game = &games[TrackValueGet(&GOLGame, frameCount)];
  if (wireworld) {
    // For technical reasons wireworld implementation requires 2 separate games
    // to be ran in alternating fashion, and the switch between them must be
    // done precisely when the board for a game finishes being calculated or
    // things will break in ways undebuggable by single-stepping the game state

    // which wireworld game (0-1) phase are we on
    static __code short wireworld_step = 0;

    // if we're displaying background (wireworld chip variant)
    if (prev_states_depth == 4) {
      short val = TrackValueGet(&WireworldFadeIn, frameCount);
      // board fade-in
      if (val & 0xff00) {
        BoardFadeIn();
        return;
      }
      // chip fade-in
      else if (val & 0xff) {
        ChipFadeIn(val);
        return;
      }
    }

    current_board = boards[wireworld_step];
    current_game = &wireworlds[wireworld_step];
    wireworld_step ^= 1;

    // set pixels on correct board
    SpawnElectrons(cur_electrons, boards[wireworld_step ^ 1],
                   boards[wireworld_step]);
  }

  // ----- PIXELDOUBLE-BLITTER SYNCHRONIZATION -----
  // run all blits except the last
  GameOfLife(boards);
  // run PixelDouble in parallel
  PixelDouble(src, dst, double_pixels);
  // wait for all blits except the last to finish
  while (phase < current_game->num_phases)
    continue;
  // run the last blit
  BlitGameOfLife(boards, current_game->num_phases - 1);
  // wait for the last blit to finish
  while (phase <= current_game->num_phases)
    continue;
  // reset phase counter
  phase = 0;
  // ----- PIXELDOUBLE-BLITTER SYNCHRONIZATION -----

  UpdateBitplanePointers();
  if (wireworld) {
    const short cycling_len =
      sizeof(wireworld_chip_cycling) / sizeof(wireworld_chip_cycling[0]);
    if (TrackValueGet(&WireworldFadeIn, frameCount) == 0)
      ColorCyclingStep(&palptr[16], wireworld_chip_cycling, cycling_len,
                       &wireworld_chip_pal);
  } else {
    short logo_idx = TrackValueGet(&GOLLogoType, frameCount);
    CopInsSet32(bplptr[3], background[logo_idx]->planes[0]);
    TransitionPal(palette_gol + 1);
    ColorFadingStep(palette_gol);
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

EFFECT(Wireworld, NULL, NULL, InitWireworld, Kill, Render, NULL);
EFFECT(GameOfLife, NULL, NULL, InitGameOfLife, Kill, Render, NULL);
