//

static int hashTable[] = {
  0x011bad37, 0x7a6433ee, // 3
  0x4ffa0d80, 0x23743a06, // 1
  0x273f164b, 0x9ffa9d90, // 2
  0x74a7beec, 0xb2818113, // 4 
};

// format:
// start_pos_x, start_pos_y, [0..255], [0..255]
// delay, [0..255]
// data [x1, y1, [[x1/2, y1/2], x2, y2] to 255,255 - end of line
// data ... to 255,255, 128, 128 - end of logo

// ALTAIR
static unsigned char grAltair[] = {
14,44,
0,
66,11,62,11,58,15,52,18,43,19,36,24,31,28,25,32,21,33,16,31,11,25,9,20,8,11,7,20,7,27,5,34,3,38,1,37,0,34,2,30,5,27,9,24,19,21,255,255,
18,5,19,15,20,23,22,26,24,25,25,23,26,19,27,14,27,2,28,20,30,22,32,21,34,16,255,255,
35,12,35,18,37,19,38,18,38,13,38,8,36,9,35,11,255,255,
39,13,40,16,42,17,44,15,44,11,45,4,46,12,49,14,50,13,50,5,51,4,52,7,52,12,52,4,53,2,54,1,57,2,255,255,
23,9,27,7,31,5,255,255,
45,0,45,2,255,255,
128,128
};

// APPENDIX
static unsigned char grAppendix[] = {
103,147,
0,
76,23,73,21,69,19,64,19,59,20,54,23,45,28,35,36,21,46,10,54,6,54,4,51,3,47,4,42,5,35,6,24,10,39,12,42,13,43,15,43,17,41,17,25,19,24,20,25,20,29,19,31,17,33,255,255,
25,37,25,22,26,21,28,22,28,27,27,28,25,29,255,255,
1,45,0,44,0,42,2,40,6,38,13,35,21,32,27,28,31,24,34,21,34,18,33,17,31,18,31,19,31,22,32,25,34,26,36,25,37,22,38,17,39,16,40,17,40,23,39,24,255,255,
40,16,43,15,44,16,44,18,45,20,46,20,47,19,48,17,255,255,
51,8,48,8,48,11,48,16,49,17,51,18,51,17,51,8,50,0,255,255,
51,11,53,14,54,15,55,15,56,14,57,11,57,4,58,12,59,13,60,13,62,11,63,8,64,5,65,3,67,0,255,255,
57,0,58,2,255,255,
60,1,62,2,63,5,64,7,66,9,68,9,70,8,70,6,255,255,
128,128
};
// ARTWAY
static unsigned char grArtway[] = {
222,11,
0,
22,26,23,27,23,29,22,29,20,28,18,26,18,25,18,11,19,7,20,5,22,5,23,8,24,13,26,20,28,23,31,27,255,255,
0,39,6,31,10,25,17,19,24,15,27,13,28,13,30,15,30,19,31,14,33,13,34,11,255,255,
34,4,34,16,35,18,36,19,38,19,39,17,40,15,41,13,255,255,
30,10,42,4,255,255,
41,9,41,13,42,13,43,12,43,9,44,13,45,13,45,7,46,10,48,11,49,11,50,10,51,9,51,7,50,6,49,7,49,10,51,10,53,9,54,8,55,4,55,8,57,8,58,8,58,7,59,3,61,6,62,8,63,12,63,18,62,21,60,23,57,23,55,21,55,18,57,13,59,9,61,6,64,3,67,1,71,0,75,1,77,2,255,255,
128,128
};

// ATTENTION WHORE
static unsigned char grAW[] = {
228,8,
0,
12,73,11,64,8,57,4,51,1,44,0,36,3,29,5,24,9,18,11,11,13,5,16,0,19,0,21,1,21,6,20,15,20,24,21,29,23,31,26,30,255,255,
3,16,13,15,22,13,30,11,39,9,255,255,
33,6,30,13,28,18,27,22,26,26,26,29,28,30,30,30,32,28,37,23,38,22,39,24,38,28,40,20,42,18,44,17,45,18,46,23,47,25,49,25,51,23,52,21,255,255,
35,36,34,40,35,44,36,43,38,39,38,43,39,45,40,44,42,40,43,35,44,39,46,40,52,22,46,45,50,39,51,37,53,36,55,37,55,40,55,42,55,43,57,41,60,43,62,42,63,40,62,38,60,38,58,39,58,41,255,255,
64,40,67,39,68,37,68,45,70,38,72,37,73,37,76,39,79,40,80,38,79,35,77,36,76,41,78,43,81,43,84,40,85,38,255,255,
128,128
};
// CAPSULE
static unsigned char grCapsule[] = {
155,155,
0,
0,45,3,40,6,36,11,32,16,29,20,28,255,255,
28,8,27,7,24,7,22,8,20,10,18,13,20,15,23,16,27,15,32,13,37,8,40,4,40,1,38,0,32,0,22,4,16,7,10,14,7,18,6,23,6,26,8,28,10,29,15,29,23,28,29,26,34,23,38,21,41,20,255,255,
48,21,49,18,48,16,45,15,44,16,43,17,42,20,44,21,47,22,51,22,53,21,255,255,
53,23,59,23,60,21,60,19,59,18,56,18,51,24,44,31,255,255,
71,22,70,20,67,20,66,22,67,23,69,24,70,25,69,26,66,26,64,25,60,24,255,255,
71,26,72,27,74,27,78,23,74,28,76,29,78,29,83,25,79,29,80,31,83,31,87,30,93,28,97,25,99,23,98,21,96,21,94,22,91,24,86,28,83,32,84,34,87,35,91,34,96,33,100,32,103,30,102,28,99,28,97,30,96,31,97,35,100,36,104,36,108,35,111,32,255,255,
128,128
};
// DEKADENCE
static unsigned char grDekadence[] = {
49,184,
0,
106,41,99,34,90,30,83,27,75,25,66,24,60,26,53,28,47,31,41,32,32,31,29,29,27,24,27,16,29,8,30,1,255,255,
2,9,3,5,7,1,13,0,16,4,17,9,17,17,15,26,12,30,8,34,2,34,0,32,0,28,3,22,8,16,13,12,18,8,22,7,24,9,255,255,
6,9,4,23,255,255,
18,18,19,20,21,21,23,19,23,17,22,16,20,16,20,18,20,21,23,22,25,21,29,19,32,15,35,14,255,255,
28,20,30,22,33,22,36,20,255,255,
40,19,40,17,38,16,36,17,36,19,37,21,40,20,43,19,46,16,255,255,
52,12,49,12,46,15,46,18,47,19,50,19,51,17,52,10,54,2,255,255,
52,16,54,17,56,17,58,16,58,14,57,13,55,13,55,14,55,16,55,19,58,19,60,17,61,15,62,14,63,15,63,19,64,13,66,12,67,17,69,18,70,18,73,15,255,255,
77,13,77,11,75,11,73,14,73,18,75,19,77,19,80,17,82,15,84,14,85,12,84,10,82,10,81,12,81,14,83,17,84,17,86,17,88,16,255,255,
128,128
};

// DESIRE
static unsigned char grDesire[] = {
14,36,
0,
89,23,86,3,83,11,79,15,75,16,70,13,70,8,71,3,74,0,77,0,78,1,78,5,74,8,71,9,66,5,65,2,63,1,61,4,59,10,56,17,55,15,55,11,56,7,52,18,49,22,47,23,46,22,45,21,46,9,42,17,39,21,35,26,33,30,33,36,34,37,37,36,38,32,38,27,34,22,31,18,31,13,34,10,36,10,38,13,255,255,
31,20,30,27,27,35,24,38,21,37,19,33,18,26,19,21,21,18,23,18,25,19,25,23,21,29,18,35,17,41,17,48,16,49,11,42,8,32,6,27,3,27,1,30,0,36,1,40,5,49,7,51,8,49,8,42,6,22,6,14,8,8,255,255,
128,128
};
// DREAMWEB
static unsigned char grDreamweb[] = {
21,186,
0,
17,4,13,12,9,20,7,25,3,27,0,24,0,21,4,18,9,18,12,24,15,30,17,30,19,26,19,15,255,255,
17,17,21,14,24,14,26,16,26,18,24,21,22,22,19,22,23,24,26,26,30,24,36,21,38,19,38,17,34,17,32,20,33,26,36,28,41,26,46,23,49,21,51,17,49,13,46,13,44,16,43,20,48,23,53,23,56,20,59,18,61,19,61,24,64,20,68,16,69,18,67,25,68,27,71,26,71,28,69,34,70,37,73,34,76,30,78,34,82,37,84,35,86,32,87,28,88,25,92,28,99,26,102,25,105,22,104,19,100,19,96,22,95,25,96,29,99,31,105,30,111,26,116,21,123,12,126,6,127,3,125,0,121,6,118,14,115,25,116,29,119,30,122,27,123,24,121,21,117,21,255,255,
120,30,126,30,133,32,137,38,255,255,
128,128
};
// ELUDE
static unsigned char grElude[] = {
203,24,
0,
0,8,7,13,14,14,21,14,27,10,27,7,25,5,21,6,17,10,15,13,14,19,16,22,19,23,29,20,35,16,43,12,51,6,52,3,50,0,46,3,41,8,37,13,33,19,32,25,35,27,40,25,45,22,52,18,55,15,47,22,48,25,50,27,56,26,60,23,64,21,66,19,59,26,56,30,57,33,61,34,64,33,255,255,
79,31,79,27,77,25,73,25,68,29,65,33,65,36,68,37,72,35,77,32,94,16,78,33,74,39,75,41,78,43,83,43,89,40,92,37,91,34,85,36,81,40,78,46,77,51,80,54,86,52,90,48,93,44,255,255,
128,128
};

// EMPTY PLACEHOLDER
static unsigned char emptyPlaceholder[] = {
0,0,
1,
0,0, 255,255,
128,128
};

// TO BE CONTINUED :)
static unsigned char grTBC[] = {
44,180,
0,
0,11,23,11,255,255,
19,3,8,19,4,25,4,29,7,29,12,27,17,24,21,21,255,255,
27,19,26,18,24,18,22,20,22,24,24,25,27,23,27,20,30,22,33,23,35,23,38,21,41,21,46,24,49,26,52,27,54,26,56,24,67,2,56,25,59,24,62,22,65,20,64,18,60,17,255,255,
61,24,66,23,70,21,75,21,76,17,73,17,71,19,71,23,74,25,78,25,83,22,90,19,96,18,101,21,104,25,106,30,255,255,
113,36,114,33,118,31,122,30,126,27,128,24,255,255,
133,18,133,17,131,16,129,17,128,20,129,24,131,25,136,22,138,20,255,255,
143,19,142,17,139,17,138,22,139,23,141,23,144,19,147,20,149,22,150,17,151,20,153,17,156,16,157,19,157,24,159,24,163,20,255,255,
172,1,168,8,164,20,163,24,163,28,166,29,170,26,178,16,176,20,178,23,181,23,185,17,184,26,186,19,190,16,192,18,192,23,193,24,196,24,198,18,198,24,203,24,205,22,205,17,208,22,214,22,216,20,216,18,213,18,211,20,211,24,213,26,217,26,222,23,255,255,
227,16,224,16,222,18,223,23,228,24,255,255,
232,0,229,6,227,14,227,20,228,24,232,27,255,255,
158,10,179,8,255,255,
128,128
};

static unsigned char *greetsSet0[] = {
  grAltair,
  grAW,
  grDesire,
  emptyPlaceholder,
  NULL,
};

static unsigned char *greetsSet1[] = {
  grAppendix,
  grCapsule,
  grDreamweb,
  emptyPlaceholder,
  NULL,
};

static unsigned char *greetsSet2[] = {
  grArtway,
  grDekadence,
  grElude,
  grTBC,
  NULL,
};

