#ifndef PTPLAYER_H
#define PTPLAYER_H

/**************************************************
 *    ----- Protracker V2.3B Playroutine -----    *
 **************************************************/

/*
  Version 6.2
  Written by Frank Wille in 2013, 2016, 2017, 2018, 2019, 2020, 2021, 2022.
*/

/* (cahir) Originally the code used some external header files.
 * I replaced them with reasonable counterparts. */

#define ASM
#define REG(r, decl) decl asm(#r)
#define BYTE char
#define UBYTE unsigned char
#define WORD short
#define UWORD unsigned short
#define LONG int
#define ULONG unsigned int
#define APTR void *

struct pt_sample {
  BYTE name[22];
  UWORD size;
  BYTE finetune;
  BYTE volume;
  UWORD loop_start;
  UWORD loop_size;
} __attribute__((packed));

struct pt_mod {
  BYTE name[20];
  struct pt_sample sample[31];
  UBYTE len;
  UBYTE restart;
  UBYTE order[128];
  UBYTE magic[4];
  ULONG pattern[0][64][4];
} __attribute__((packed));

struct mt_chan {
  UWORD n_note;
  UBYTE n_cmd;
  UBYTE n_cmdlo;
  UBYTE n_index;
  UBYTE n_sfxpri;
  UWORD n_reserved1;
  APTR n_start;
  APTR n_loopstart;
  UWORD n_length;
  UWORD n_replen;
  UWORD n_period;
  UWORD n_volume;
  APTR n_pertab;
  UWORD n_dmabit;
  UWORD n_noteoff;
  UWORD n_toneportspeed;
  UWORD n_wantedperiod;
  UWORD n_pattpos;
  UWORD n_funk;
  APTR n_wavestart;
  UWORD n_reallength;
  UWORD n_intbit;
  APTR n_sfxptr;
  UWORD n_sfxlen;
  UWORD n_sfxper;
  UWORD n_sfxvol;
  BYTE n_looped;
  BYTE n_minusft;
  BYTE n_vibratoamp;
  BYTE n_vibratospd;
  BYTE n_vibratopos;
  BYTE n_vibratoctrl;
  BYTE n_tremoloamp;
  BYTE n_tremolospd;
  BYTE n_tremolopos;
  BYTE n_tremoloctrl;
  BYTE n_gliss;
  BYTE n_sampleoffset;
  BYTE n_loopcount;
  BYTE n_funkoffset;
  BYTE n_retrigcount;
  BYTE n_freecnt;
  BYTE n_musiconly;
  BYTE n_enable;
} __attribute__((packed));

struct mt_data {
  struct mt_chan mt_chan[4];
  APTR mt_SampleStarts[31];
  struct pt_mod *mt_mod;
  APTR mt_oldLev6;
  APTR mt_timerval;
  UBYTE mt_oldtimers[4];
  APTR mt_Lev6Int;
  UWORD mt_Lev6Ena;
  UWORD mt_PatternPos;
  UWORD mt_PBreakPos;
  UBYTE mt_PosJumpFlag;
  UBYTE mt_PBreakFlag;
  UBYTE mt_Speed;
  UBYTE mt_Counter;
  UBYTE mt_SongPos;
  UBYTE mt_PattDelTime;
  UBYTE mt_PattDelTime2;
} __attribute__((packed));

/*
  mt_install_cia(a6=CUSTOM, a0=VectorBase, d0=PALflag.b)
    Install a CIA-B interrupt for calling _mt_music or mt_sfxonly.
    The music module is replayed via _mt_music when _mt_Enable is non-zero.
    Otherwise the interrupt handler calls mt_sfxonly to play sound
    effects only. VectorBase is 0 for 68000, otherwise set it to the CPU's
    VBR register. A non-zero PALflag selects PAL-clock for the CIA timers
    (NTSC otherwise).
*/

void ASM mt_install_cia(REG(a6, void *Custom),
	REG(a0, void *VectorBase), REG(d0, UBYTE PALflag));

/*
  mt_remove_cia(a6=CUSTOM)
    Remove the CIA-B music interrupt and restore the old vector.
*/

void ASM mt_remove_cia(REG(a6, void *Custom));

/*
  mt_init(a6=CUSTOM, a0=TrackerModule, a1=Samples|NULL, d0=InitialSongPos.b)
    Initialize a new module.
    Reset speed to 6, tempo to 125 and start at the given position.
    Master volume is at 64 (maximum).
    When a1 is NULL the samples are assumed to be stored after the patterns.
*/

void ASM mt_init(REG(a6, void *Custom),
	REG(a0, void *TrackerModule), REG(a1, void *Samples),
	REG(d0, UBYTE InitialSongPos));

/*
  mt_end(a6=CUSTOM)
    Stop playing current module.
*/

void ASM mt_end(REG(a6, void *Custom));

/*
  mt_soundfx(a6=CUSTOM, a0=SamplePointer,
             d0=SampleLength.w, d1=SamplePeriod.w, d2=SampleVolume.w)
    Request playing of an external sound effect on the most unused channel.
    This function is for compatibility with the old API only!
    You should call mt_playfx() instead.
*/

void ASM mt_soundfx(REG(a6, void *Custom),
	REG(a0, void *SamplePointer), REG(d0, UWORD SampleLength),
	REG(d1, UWORD SamplePeriod), REG(d2, UWORD SampleVolume));

/*
  SfxChanStatus = mt_playfx(a6=CUSTOM, a0=SfxStructurePointer)
    Request playing of a prioritized external sound effect, either on a
    fixed channel or on the most unused one.
    Structure layout of SfxStructure:
      void *sfx_ptr (pointer to sample start in Chip RAM, even address)
      WORD sfx_len  (sample length in words)
      WORD sfx_per  (hardware replay period for sample)
      WORD sfx_vol  (volume 0..64, is unaffected by the song's master volume)
      BYTE sfx_cha  (0..3 selected replay channel, -1 selects best channel)
      BYTE sfx_pri (priority, must be in the range 1..127)
    When multiple samples are assigned to the same channel the lower
    priority sample will be replaced. When priorities are the same, then
    the older sample is replaced.
    The chosen channel is blocked for music until the effect has
    completely been replayed.
    Returns pointer to SfxChanStatus when sample was scheduled for
    playing and NULL when the request was ignored.
*/

typedef struct SfxStructure
{
	void *sfx_ptr; /* pointer to sample start in Chip RAM, even address */
	UWORD sfx_len; /* sample length in words */
	UWORD sfx_per; /* hardware replay period for sample */
	UWORD sfx_vol; /* volume 0..64, is unaffected by the song's master volume */
	BYTE sfx_cha;  /* 0..3 selected replay channel, -1 selects best channel */
	BYTE sfx_pri;  /* priority, must be in the range 1..127 */
} SfxStructure;

typedef struct SfxChanStatus
{
  UWORD n_note;
  UWORD n_cmd;
  UBYTE n_index;   /* channel index 0..3 */
  UBYTE n_sfxpri;  /* sfx_pri when playing, becomes 0 when done */
  /* Rest of structure is currently not exported. Don't rely on it! */
} SfxChanStatus;

SfxChanStatus * ASM mt_playfx(REG(a6, void *Custom),
	REG(a0, SfxStructure *SfxStructurePointer));

/*
  mt_loopfx(a6=CUSTOM, a0=SfxStructurePointer)
    Request playing of a looped sound effect on a fixed channel, which
    will be blocked for music until the effect is stopped (mt_stopfx()).
    It uses the same sfx-structure as mt_playfx(), but the priority is
    ignored. A looped sound effect has always highest priority and will
    replace a previous effect on the same channel. No automatic channel
    selection possible!
    Also make sure the sample starts with a zero-word, which is used
    for idling when the effect is stopped. This word is included in the
    total length calculation, but excluded when actually playing the loop.
*/

void ASM mt_loopfx(REG(a6, void *Custom),
	REG(a0, SfxStructure *SfxStructurePointer));

/*
  mt_stopfx(a6=CUSTOM, d0=Channel.b)
    Immediately stop a currently playing sound effect on a channel (0..3)
    and make it available for music, or other effects, again. This is the
    only way to stop a looped sound effect (mt_loopfx()), besides stopping
    replay completely (mt_end()).
*/

void ASM mt_stopfx(REG(a6, void *Custom),
	REG(d0, UBYTE ChannelNo));

/*
  mt_musicmask(a6=CUSTOM, d0=ChannelMask.b)
    Set bits in the mask define which specific channels are reserved
    for music only. Set bit 0 for channel 0, ..., bit 3 for channel 3.
    When calling mt_soundfx() or mt_playfx() with automatic channel selection
    (sfx_cha=-1) then these masked channels will never be picked.
    The mask defaults to 0.
*/

void ASM mt_musicmask(REG(a6, void *Custom),
	REG(d0, UBYTE ChannelMask));

/*
  mt_mastervol(a6=CUSTOM, d0=MasterVolume.w)
    Set a master volume from 0 to 64 for all music channels.
    Note that the master volume does not affect the volume of external
    sound effects (which is desired).
*/

void ASM mt_mastervol(REG(a6, void *Custom),
	REG(d0, UWORD MasterVolume));

/*
  mt_samplevol(d0=SampleNumber.w, d1=Volume.b)
    Redefine a sample's volume. May also be done while the song is playing.
    Warning: Does not check arguments for valid range! You must have done
    mt_init() before calling this function!
    The new volume is persistent. Even when the song is restarted.
*/

void ASM mt_samplevol(REG(d0, UWORD SampleNumber), REG(d1, UBYTE Volume));

/*
  mt_channelmask(a6=CUSTOM, d0=ChannelMask.b)
    Bits cleared in the mask define which specific channels are muted
    for music replay. Clear bit 0 for channel 0, ..., bit 3 for channel 3.
    The mask defaults to all channels unmuted (bits set) and is reset to
    this state on mt_init() and mt_end().
*/

void ASM mt_channelmask(REG(a6, void *Custom),
	REG(d0, UBYTE ChannelMask));

/*
  mt_music(a6=CUSTOM)
    The replayer routine. Can be called from your own VBlank interrupt
    handler when VBLANK_MUSIC is set. Is otherwise called automatically
    by Timer-A interrupts after mt_install_cia().
*/

void ASM mt_music(REG(a6, void *Custom));

/*
  mt_Enable
    Set this byte to non-zero to play music, zero to pause playing.
    Note that you can still play sound effects, while music is stopped.
    It is set to 0 by mt_install_cia().
*/

extern UBYTE mt_Enable;

/*
  mt_E8Trigger
    This byte reflects the value of the last E8 command.
    It is reset to 0 after mt_init().
*/

extern UBYTE mt_E8Trigger;

/*
  mt_MusicChannels
    This byte defines the number of channels which should be dedicated
    for playing music. So sound effects will never use more
    than 4 - mt_MusicChannels channels at once. Defaults to 0.
*/

extern UBYTE mt_MusicChannels;

#endif
