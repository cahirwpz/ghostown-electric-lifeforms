#ifndef __INTRO_H__
#define __INTRO_H__

#include <palette.h>
#include <sync.h>
#include <effect.h>

short UpdateFrameCount(void);

static inline short FromCurrKeyFrame(TrackT *track) {
  return frameCount - CurrKeyFrame(track);
}

static inline short TillNextKeyFrame(TrackT *track) {
  return NextKeyFrame(track) - frameCount;
}

void FadeBlack(const PaletteT *pal, u_int start, short step);

#endif /* !__INTRO_H__ */
