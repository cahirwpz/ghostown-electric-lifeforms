#ifndef __INTRO_H__
#define __INTRO_H__

#include <palette.h>
#include <copper.h>
#include <sync.h>
#include <effect.h>

void FadeIn(const PaletteT *pal, short step);
void FadeOut(const PaletteT *pal, short step);

void CopFadeIn(const PaletteT *pal, CopInsT *ins);
void CopFadeOut(const PaletteT *pal, CopInsT *ins);

short UpdateFrameCount(void);

static inline short FromCurrKeyFrame(TrackT *track) {
  return frameCount - CurrKeyFrame(track);
}

static inline short TillNextKeyFrame(TrackT *track) {
  return NextKeyFrame(track) - frameCount;
}

#endif /* !__INTRO_H__ */
