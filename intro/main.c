#include <custom.h>
#include <effect.h>
#include <cinter.h>
#include <sync.h>
#include <system/interrupt.h>
#include <system/task.h>
#include <system/memory.h>

#define _SYSTEM
#include <system/cia.h>

static CinterPlayerT CinterPlayer[1];
extern u_char CinterModule[];
extern u_char CinterSamples[];

extern EffectT LogoEffect;
extern EffectT WeaveEffect;
extern EffectT TextScrollEffect;
extern EffectT TurmiteEffect;

#include "data/intro.c"

static EffectT *AllEffects[] = {
  &LogoEffect,
  &WeaveEffect,
  &TextScrollEffect,
  &TurmiteEffect,
  NULL,
};

static int CinterMusic(CinterPlayerT *player) {
  CinterPlay1(player);
  CinterPlay2(player);
  return 0;
}

INTSERVER(CinterMusicServer, 10, (IntFuncT)CinterMusic, CinterPlayer);

static void LoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectLoad(effect);
  }
}

static void UnLoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectUnLoad(effect);
  }
}

int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  CinterInit(CinterModule, CinterSamples, CinterPlayer);

  TrackInit(&EffectNumber);
  LoadEffects(AllEffects);

  EnableDMA(DMAF_AUDIO);
  AddIntServer(INTB_VERTB, CinterMusicServer);

  /* Reset frame counter and wait for all time actions to finish. */
  SetFrameCounter(0);
  frameCount = 0;

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0)
        EffectKill(AllEffects[prev]);
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
    }

    lastFrameCount = ReadFrameCounter();

    {
      EffectT *effect = AllEffects[curr];
      int t = ReadFrameCounter();
      frameCount = t;
      if (effect->Render)
        effect->Render();
      lastFrameCount = t;
    }

    prev = curr;
  }

  RemIntServer(INTB_VERTB, CinterMusicServer);
  DisableDMA(DMAF_AUDIO);

  UnLoadEffects(AllEffects);

  return 0;
}
