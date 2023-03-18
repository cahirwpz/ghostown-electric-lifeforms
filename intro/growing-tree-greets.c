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
 0,0,
 10,
 80,55,76,55,72,59,66,62,57,63,50,68,45,72,39,76,35,77,30,75,25,69,23,64,22,55,21,64,21,71,19,78,17,82,15,81,14,78,16,74,19,71,23,68,33,65, 255, 255,
 32,49,33,59,34,67,36,70,38,69,39,67,40,63,41,58,41,46,42,64,44,66,46,65,48,60,255,255,
 37,53,41,51,45,49,255,255,
 49,56,49,62,51,63,52,62,52,57,52,53,50,53,255,255,
 53,57,54,60,56,61,58,59,58,55,59,48,60,56,63,58,64,57,64,51,65,48,66,51,66,56,66,48,67,46,68,45,71,46,255,255,
 59,44,59,45,255,255,
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

