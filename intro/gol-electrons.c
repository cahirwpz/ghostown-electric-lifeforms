// frame at which to spawn specific electron next - counting from stepCount
static short next_spawn[128];

extern TrackT WireworldSpawnMask;
extern TrackT WireworldMinDelay;
extern TrackT WireworldSpawnNow;

#define ELPOS(x, y) \
  ((x) + EXT_WIDTH_LEFT + (((y) + EXT_HEIGHT_TOP) * EXT_BOARD_WIDTH))

#include "data/wireworld-vitruvian-electrons.c"
#include "data/wireworld-fullscreen-electrons.c"

static const ElectronArrayT *cur_electrons;

static void InitSpawnFrames(const ElectronArrayT *electrons, short spawn_mask) {
  short i;
  short *spawn = next_spawn;
  for (i = 0; i < electrons->num_electrons; i++)
    *spawn++ = stepCount + (spawn_mask & random());
}

static void SpawnElectrons(const ElectronArrayT *electrons,
                           BitmapT *board_heads, BitmapT *board_tails)
{
  u_char *bpl_heads = board_heads->planes[0];
  u_char *bpl_tails = board_tails->planes[0];
  short *pts = electrons->points;
  short *spawn = next_spawn;
  short n = electrons->num_electrons - 1;
  short spawn_mask = TrackValueGet(&WireworldSpawnMask, frameCount);
  short min_delay = TrackValueGet(&WireworldMinDelay, frameCount);
  short manual_override = TrackValueGet(&WireworldSpawnNow, frameCount);

  if (n < 0)
    return;
  do {
    if (manual_override || *spawn <= stepCount) {
      short posh = (*pts++);
      short post = posh + (*pts++);
      bset(bpl_heads + (posh >> 3), ~posh);
      bset(bpl_tails + (post >> 3), ~post);
      *spawn++ += (random() & spawn_mask) + min_delay;
    } else {
      pts += 4;
      spawn++;
    }
  } while (--n != -1);
}

