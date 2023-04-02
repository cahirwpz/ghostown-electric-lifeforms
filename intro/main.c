#include <custom.h>
#include <effect.h>
#include <ptplayer.h>
#include <color.h>
#include <copper.h>
#include <palette.h>
#include <sync.h>
#include <c2p_1x1_4.h>
#include <system/task.h>
#include <system/interrupt.h>

#include "intro.h"

#define _SYSTEM
#include <system/memory.h>
#include <system/cia.h>

extern u_char Module[];
extern u_char Samples[];

extern EffectT LogoEffect;
extern EffectT WeaveEffect;
extern EffectT TextScrollEffect;
extern EffectT TurmiteEffect;
extern EffectT GrowingTreeEffect;
extern EffectT TileMoverEffect;
extern EffectT SeaAnemoneEffect;
extern EffectT WireworldEffect;
extern EffectT GameOfLifeEffect;
extern EffectT VitruvianEffect;
extern EffectT UVGutEffect;
extern EffectT UVTitEffect;

short frameFromStart;
short frameTillEnd;

#include "data/intro.c"

static EffectT *AllEffects[] = {
  &LogoEffect,
  &TextScrollEffect,
  &WeaveEffect,
  &TurmiteEffect,
  &GrowingTreeEffect,
  &TileMoverEffect,
  &SeaAnemoneEffect,
  &WireworldEffect,
  &GameOfLifeEffect,
  &VitruvianEffect,
  &UVGutEffect,
  &UVTitEffect,
  NULL,
};

static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d FAST: %d\n", MemAvail(MEMF_CHIP), MemAvail(MEMF_FAST));
}

void PixmapToBitmap(BitmapT *bm, short width, short height, short depth,
                    void *pixels)
{
  short bytesPerRow = ((short)(width + 15) & ~15) / 8;
  int bplSize = bytesPerRow * height;

  bm->width = width;
  bm->height = height;
  bm->depth = depth;
  bm->bytesPerRow = bytesPerRow;
  bm->bplSize = bplSize;
  bm->flags = BM_DISPLAYABLE | BM_STATIC;

  BitmapSetPointers(bm, pixels);

  {
    void *planes = MemAlloc(bplSize * 4, MEMF_PUBLIC);
    c2p_1x1_4(pixels, planes, width, height, bplSize);
    memcpy(pixels, planes, bplSize * 4);
    MemFree(planes);
  }
}

static void LoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectLoad(effect);
    if (effect->Load)
      ShowMemStats();
  }
}

static void UnLoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectUnLoad(effect);
  }
}

void FadeBlack(const PaletteT *pal, u_int start, short step) {
  volatile short *reg = &custom->color[start];
  const short *col = pal->colors;
  short n = pal->count;
  
  if (step < 0)
    step = 0;
  if (step > 15)
    step = 15;

  while (--n >= 0) {
    short to = *col++;

    short r = ((to >> 4) & 0xf0) | step;
    short g = (to & 0xf0) | step;
    short b = ((to << 4) & 0xf0) | step;

    r = colortab[r];
    g = colortab[g];
    b = colortab[b];
    
    *reg++ = (r << 4) | g | (b >> 4);
  }
}

short UpdateFrameCount(void) {
  short t = ReadFrameCounter();
  frameCount = t;
  frameFromStart = t - CurrKeyFrame(&EffectNumber);
  frameTillEnd = NextKeyFrame(&EffectNumber) - t;
  return t;
}

static volatile EffectFuncT VBlankHandler = NULL;

static int VBlankISR(void) {
  if (VBlankHandler)
    VBlankHandler();
  return 0;
}

INTSERVER(VBlankInterrupt, 0, (IntFuncT)VBlankISR, NULL);

#define SYNCPOS(pos) (((((pos) & 0xff00) >> 2) | ((pos) & 0x3f)) * 3)

static void RunEffects(void) {
  /* Set the beginning of intro. Useful for effect synchronization! */
  short pos = 0;

  frameCount = SYNCPOS(pos);
  SetFrameCounter(frameCount);
  PtSongPos = pos >> 8;
  PtPatternPos = (pos & 0x3f) << 4;
  PtEnable = -1;

  AddIntServer(INTB_VERTB, VBlankInterrupt);

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0) {
        VBlankHandler = NULL;
        EffectKill(AllEffects[prev]);
        ShowMemStats();
      }
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
      VBlankHandler = AllEffects[curr]->VBlank;
      ShowMemStats();
      Log("[Effect] Transition to %s took %d frames!\n",
          AllEffects[curr]->name, ReadFrameCounter() - lastFrameCount);
      lastFrameCount = ReadFrameCounter() - 1;
    }

    {
      EffectT *effect = AllEffects[curr];
      short t = UpdateFrameCount();
      if (effect->Render)
        effect->Render();
      lastFrameCount = t;
    }

    prev = curr;
  }

  RemIntServer(INTB_VERTB, VBlankInterrupt);
}

extern void InitSamples(void);

int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  InitSamples();
  PtInstallCIA();
  PtInit(Module, Samples, 0);

  TrackInit(&EffectNumber);
  LoadEffects(AllEffects);

  RunEffects();

  PtEnd();
  PtRemoveCIA();

  UnLoadEffects(AllEffects);

  return 0;
}
