//

static int hashTable[] = {
  0x74a7beec, 0xb2818113, // 1 
  0x4ffa0d80, 0x23743a06, // 2
  0x273f164b, 0x9ffa9d90, // 3
  0x011bad37, 0x7a6433ee, // 4
};

// format:
// start_pos_x, start_pos_y, [0..255], [0..255]
// delay, [0..255]
// data ... to 255,255 - end of line
// data ... to 255,255, 128, 128 - end of logo

// ALTAIR
static unsigned char grAltair[] = {
  10,10,
  1,
  5,5, 255,255,
 128,128
};

// APPENDIX
static unsigned char grAppendix[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// ARTWAY
static unsigned char grArtway[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};

// ATTENTION WHORE
static unsigned char grAW[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// CAPSULE
static unsigned char grCapsule[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// DEKADENCE
static unsigned char grDekadence[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};

// DESIRE
static unsigned char grDesire[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// DREAMWEB
static unsigned char grDreamweb[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// ELUDE
static unsigned char grElude[] = {
  10,10,
  1,
  109,31,100,29,85,27,69,27,50,30,33,36,15,46,0,57,255,255,
  62,25,57,23,59,12,55,26,51,28,48,27,49,13,46,27,41,30,35,29,31,20,31,11,36,2,37,11,36,18,28,33,23,38,16,41,9,38,7,29,11,23,18,20,22,22,20,28,14,33,6,34,255,255,
  99,22,93,23,89,23,84,20,83,14,85,10,89,10,91,14,88,18,82,18,77,14,73,13,68,14,65,18,66,22,68,25,73,23,75,18,76,9,74,0,255,255,
  128,128
};

// FOCUS DESIGN
static unsigned char grFD[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// FUTURIS
static unsigned char grFututis[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};
// TO BE CONTINUED :)
static unsigned char grTBC[] = {
  10,10,
  1,
  5,5, 255,255,
  128,128
};

static unsigned char *greetsSet0[] = {
  grAltair,
  grAW,
  grDesire,
  grFD,
  NULL,
};

static unsigned char *greetsSet1[] = {
  grAppendix,
  grCapsule,
  grDreamweb,
  grFututis,
  NULL,
};

static unsigned char *greetsSet2[] = {
  grArtway,
  grDekadence,
  grElude,
  grTBC,
  NULL,
};

