#include <custom.h>
#include <effect.h>
#include <ptplayer.h>
#include <sync.h>
#include <system/task.h>

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

#define SYNCPOS(pos) (((((pos) & 0xff00) >> 2) | ((pos) & 0x3f)) * 6)

static void RunEffects(void) {
  /* Set the beginning of intro. Useful for effect synchronization! */
  short pos = 0;

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
    }

    lastFrameCount = ReadFrameCounter() - 1;

    {
      EffectT *effect = AllEffects[curr];
      short t = ReadFrameCounter();
      frameCount = t;
      frameFromStart = t - CurrKeyFrame(&EffectNumber);
      frameTillEnd = NextKeyFrame(&EffectNumber) - t;
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
