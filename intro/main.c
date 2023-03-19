#include <custom.h>
#include <effect.h>
#include <ptplayer.h>
#include <color.h>
#include <copper.h>
#include <palette.h>
#include <sync.h>
#include <system/task.h>
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
extern EffectT UVMapEffect;

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
  &UVMapEffect,
  NULL,
};

static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d FAST: %d\n", MemAvail(MEMF_CHIP), MemAvail(MEMF_FAST));
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

static void FadeBlack(const PaletteT *pal, short step) {
  const short *col = pal->colors;
  volatile short *reg = custom->color;
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

static void CopFadeBlack(const PaletteT *pal, CopInsT *ins, short step) {
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
    
    CopInsSet16(ins++, (r << 4) | g | (b >> 4));
  }
}

void CopFadeIn(const PaletteT *pal, CopInsT *ins) {
  CopFadeBlack(pal, ins, frameFromStart);
}

void CopFadeOut(const PaletteT *pal, CopInsT *ins) {
  CopFadeBlack(pal, ins, frameTillEnd);
}

void FadeIn(const PaletteT *pal, short step) {
  FadeBlack(pal, step);
}

void FadeOut(const PaletteT *pal, short step) {
  FadeBlack(pal, step);
}

short UpdateFrameCount(void) {
  short t = ReadFrameCounter();
  frameCount = t;
  frameFromStart = t - CurrKeyFrame(&EffectNumber);
  frameTillEnd = NextKeyFrame(&EffectNumber) - t;
  return t;
}

#define SYNCPOS(pos) (((((pos) & 0xff00) >> 2) | ((pos) & 0x3f)) * 6)

static void RunEffects(void) {
  /* Set the beginning of intro. Useful for effect synchronization! */
  short pos = 0x800;

  frameCount = SYNCPOS(pos);
  SetFrameCounter(frameCount);
  PtData.mt_SongPos = pos >> 8;
  PtData.mt_PatternPos = (pos & 0x3f) << 4;
  PtEnable = -1;

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0) {
        EffectKill(AllEffects[prev]);
        ShowMemStats();
      }
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
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
