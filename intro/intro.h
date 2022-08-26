#ifndef __INTRO_H__
#define __INTRO_H__

#include <effect.h>
#include <sync.h>

static inline short FromCurrKeyFrame(TrackT *track) {
  return frameCount - CurrKeyFrame(track);
}

#endif /* !__INTRO_H__ */
