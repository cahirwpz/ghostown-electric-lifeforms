// frame at which to spawn specific electron next - counting from stepCount
static short next_spawn[128];

static const ElectronArrayT *cur_electrons;

static void InitSpawnFrames(const ElectronArrayT *electrons) {
  short *spawn = next_spawn;
  short i;

  for (i = 0; i < electrons->num_electrons; i++)
    *spawn++ = stepCount + (random() & RAND_SPAWN_MASK);
}

static void SpawnElectrons(const ElectronArrayT *electrons,
                           BitmapT *board_heads, BitmapT *board_tails)
{
  u_char *bpl_heads = board_heads->planes[0];
  u_char *bpl_tails = board_tails->planes[0];
  short *pts = (short *)electrons->points;
  short *spawn = next_spawn;
  short n = electrons->num_electrons - 1;
  if (n < 0)
    return;
  do {
    if (*spawn <= stepCount) {
      short hx = (*pts++) + EXT_WIDTH_LEFT;
      short hy = (*pts++) + EXT_HEIGHT_TOP;
      short tx = (*pts++) + EXT_WIDTH_LEFT;
      short ty = (*pts++) + EXT_HEIGHT_TOP;
      int posh = EXT_BOARD_WIDTH * hy + hx;
      int post = EXT_BOARD_WIDTH * ty + tx;
      bset(bpl_heads + (posh >> 3), ~posh);
      bset(bpl_tails + (post >> 3), ~post);
      *spawn++ += (random() & RAND_SPAWN_MASK) + RAND_SPAWN_MIN_DELAY;
    } else {
      pts += 4;
      spawn++;
    }
  } while (--n != -1);
}

