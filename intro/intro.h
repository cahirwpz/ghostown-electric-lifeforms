#ifndef __INTRO_H__
#define __INTRO_H__

#include <bitmap.h>
#include <palette.h>
#include <sync.h>
#include <effect.h>

extern BitmapT ghostown_logo;

short UpdateFrameCount(void);

static inline short FromCurrKeyFrame(TrackT *track) {
  return frameCount - CurrKeyFrame(track);
}

static inline short TillNextKeyFrame(TrackT *track) {
  return NextKeyFrame(track) - frameCount;
}

void FadeBlack(const PaletteT *pal, u_int start, short step);

void PixmapToBitmap(BitmapT *bm, int width, int height, int depth,
                    void *pixels);

#endif /* !__INTRO_H__ */
