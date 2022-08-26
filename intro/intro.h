#ifndef __INTRO_H__
#define __INTRO_H__

#include <effect.h>
#include <sync.h>

static inline short FromCurrKeyFrame(TrackT *track) {
  return frameCount - CurrKeyFrame(track);
}

static inline short TillNextKeyFrame(TrackT *track) {
  return NextKeyFrame(track) - frameCount;
}

#endif /* !__INTRO_H__ */
