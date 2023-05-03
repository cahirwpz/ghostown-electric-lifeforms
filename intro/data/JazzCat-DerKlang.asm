        section '.bsschip', bss
        xdef _Samples

_Samples:
        ds.w AK_SMP_LEN/2

        section '.data', data
        xdef _ExtSamples

_ExtSamples:
        incbin  'data/JazzCat-DerKlang.raw'

        section '.text', code
        xdef _AK_Generate
        xdef _AK_Progress
        xdef _AK_ProgressLen

_AK_Generate:
        movem.l d2-d7/a2-a6,-(sp)
        lea     _Samples,a0
        lea     _ExtSamples,a2
        lea     _AK_Progress(pc),a3
        bsr     AK_Generate
        movem.l (sp)+,d2-d7/a2-a6
        rts

_AK_Progress:
        ds.l    1

_AK_ProgressLen:
        dc.l    AK_FINE_PROGRESS_LEN

;----------------------------------------------------------------------------
;
; Generated with Aklang2Asm V1.1, by Dan/Lemon. 2021-2022.
;
; Based on Alcatraz Amigaklang rendering core. (c) Jochen 'Virgill' Feldkötter 2020.
;
; What's new in V1.1?
; - Instance offsets fixed in ADSR operator
; - Incorrect shift direction fixed in OnePoleFilter operator
; - Loop Generator now correctly interleaved with instrument generation
; - Fine progress includes loop generation, and new AK_FINE_PROGRESS_LEN added
; - Reverb large buffer instance offsets were wrong, causing potential buffer overrun
;
; Call 'AK_Generate' with the following registers set:
; a0 = Sample Buffer Start Address
; a1 = 36864 Bytes Temporary Work Buffer Address (can be freed after sample rendering complete)
; a2 = External Samples Address (need not be in chip memory, and can be freed after sample rendering complete)
; a3 = Rendering Progress Address (2 modes available... see below)
;
; AK_FINE_PROGRESS equ 0 = rendering progress as a byte (current instrument number)
; AK_FINE_PROGRESS equ 1 = rendering progress as a long (current sample byte)
;
;----------------------------------------------------------------------------

AK_USE_PROGRESS			equ 1
AK_FINE_PROGRESS		equ 1
AK_FINE_PROGRESS_LEN	equ 243144
AK_SMP_LEN				equ 177446
AK_EXT_SMP_LEN			equ 6762

AK_Generate:

				lea		AK_Vars(pc),a5

				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						move.b	#-1,(a3)
					else
						move.l	#0,(a3)
					endif
				endif

				; Create sample & external sample base addresses
				lea		AK_SmpLen(a5),a6
				lea		AK_SmpAddr(a5),a4
				move.l	a0,d0
				moveq	#31-1,d7
.SmpAdrLoop		move.l	d0,(a4)+
				add.l	(a6)+,d0
				dbra	d7,.SmpAdrLoop
				move.l	a2,d0
				moveq	#8-1,d7
.ExtSmpAdrLoop	move.l	d0,(a4)+
				add.l	(a6)+,d0
				dbra	d7,.ExtSmpAdrLoop

				; Convert external samples from stored deltas
				move.l	a2,a6
				move.w	#AK_EXT_SMP_LEN-1,d7
				moveq	#0,d0
.DeltaLoop		add.b	(a6),d0
				move.b	d0,(a6)+
				dbra	d7,.DeltaLoop

;----------------------------------------------------------------------------
; Instrument 1 - leider
;----------------------------------------------------------------------------

				moveq	#9,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst1Loop
				; v3 = osc_tri(0, 2020, 52)
				add.w	#2020,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d2
				bge.s	.TriNoInvert_1_1
				not.w	d2
.TriNoInvert_1_1
				sub.w	#16384,d2
				add.w	d2,d2
				muls	#52,d2
				asr.l	#7,d2

				; v1 = osc_saw(1, 2000, 0)
				add.w	#2000,AK_OpInstance+2(a5)
				move.w	AK_OpInstance+2(a5),d0
				muls	#0,d0
				asr.l	#7,d0

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_1_3
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_1_3

				; v3 = osc_saw(3, 2010, 60)
				add.w	#2010,AK_OpInstance+4(a5)
				move.w	AK_OpInstance+4(a5),d2
				muls	#60,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_1_5
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_1_5

				; v3 = osc_saw(5, 1990, 0)
				add.w	#1990,AK_OpInstance+6(a5)
				move.w	AK_OpInstance+6(a5),d2
				muls	#0,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_1_7
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_1_7

				; v1 = sv_flt_n(7, v1, 107, 77, 0)
				move.w	AK_OpInstance+AK_BPF+8(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#107,d5
				move.w	AK_OpInstance+AK_LPF+8(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_1_8
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_1_8
				move.w	d4,AK_OpInstance+AK_LPF+8(a5)
				muls	#77,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_1_8
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_1_8
.NoClampMaxHPF_1_8
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_1_8
				move.w	#-32768,d5
.NoClampMinHPF_1_8
				move.w	d5,AK_OpInstance+AK_HPF+8(a5)
				asr.w	#7,d5
				muls	#107,d5
				add.w	AK_OpInstance+AK_BPF+8(a5),d5
				bvc.s	.NoClampBPF_1_8
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_1_8
				move.w	d5,AK_OpInstance+AK_BPF+8(a5)
				move.w	AK_OpInstance+AK_LPF+8(a5),d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+0(a5),d7
				blt		.Inst1Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 1 - Loop Generator (Offset: 9016 Length: 8578
;----------------------------------------------------------------------------

				move.l	#8578,d7
				move.l	AK_SmpAddr+0(a5),a0
				lea		9016(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_0
				moveq	#0,d0
.LoopGenVC_0
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_0
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_0

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 2 - Jazzy_CH
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst2Loop
				; v1 = chordgen(0, 0, 5, 0, 0, 127)
				move.l	AK_SmpAddr+0(a5),a4
				move.b	(a4,d7.l),d6
				ext.w	d6
				add.w	#127,a4
				moveq	#0,d4
				move.w	AK_OpInstance+AK_CHORD1+0(a5),d4
				add.l	#87552,AK_OpInstance+AK_CHORD1+0(a5)
				move.b	(a4,d4.l),d5
				ext.w	d5
				add.w	d5,d6
				move.w	#255,d5
				cmp.w	d5,d6
				blt.s	.NoClampMaxChord_2_1
				move.w	d5,d6
				bra.s	.NoClampMinChord_2_1
.NoClampMaxChord_2_1
				not.w	d5
				cmp.w	d5,d6
				bge.s	.NoClampMinChord_2_1
				move.w	d5,d6
.NoClampMinChord_2_1
				asl.w	#7,d6
				move.w	d6,d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+4(a5),d7
				blt		.Inst2Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 2 - Loop Generator (Offset: 6144 Length: 3532
;----------------------------------------------------------------------------

				move.l	#3532,d7
				move.l	AK_SmpAddr+4(a5),a0
				lea		6144(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_1
				moveq	#0,d0
.LoopGenVC_1
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_1
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_1

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 3 - Instrument_3
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst3Loop
				; v1 = chordgen(0, 0, 4, 0, 0, 100)
				move.l	AK_SmpAddr+0(a5),a4
				move.b	(a4,d7.l),d6
				ext.w	d6
				add.w	#100,a4
				moveq	#0,d4
				move.w	AK_OpInstance+AK_CHORD1+0(a5),d4
				add.l	#82432,AK_OpInstance+AK_CHORD1+0(a5)
				move.b	(a4,d4.l),d5
				ext.w	d5
				add.w	d5,d6
				move.w	#255,d5
				cmp.w	d5,d6
				blt.s	.NoClampMaxChord_3_1
				move.w	d5,d6
				bra.s	.NoClampMinChord_3_1
.NoClampMaxChord_3_1
				not.w	d5
				cmp.w	d5,d6
				bge.s	.NoClampMinChord_3_1
				move.w	d5,d6
.NoClampMinChord_3_1
				asl.w	#7,d6
				move.w	d6,d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+8(a5),d7
				blt		.Inst3Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 3 - Loop Generator (Offset: 6144 Length: 6144
;----------------------------------------------------------------------------

				move.l	#6144,d7
				move.l	AK_SmpAddr+8(a5),a0
				lea		6144(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_2
				moveq	#0,d0
.LoopGenVC_2
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_2
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_2

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 4 - Instrument_4
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst4Loop
				; v1 = chordgen(0, 0, 3, 0, 0, 100)
				move.l	AK_SmpAddr+0(a5),a4
				move.b	(a4,d7.l),d6
				ext.w	d6
				add.w	#100,a4
				moveq	#0,d4
				move.w	AK_OpInstance+AK_CHORD1+0(a5),d4
				add.l	#77824,AK_OpInstance+AK_CHORD1+0(a5)
				move.b	(a4,d4.l),d5
				ext.w	d5
				add.w	d5,d6
				move.w	#255,d5
				cmp.w	d5,d6
				blt.s	.NoClampMaxChord_4_1
				move.w	d5,d6
				bra.s	.NoClampMinChord_4_1
.NoClampMaxChord_4_1
				not.w	d5
				cmp.w	d5,d6
				bge.s	.NoClampMinChord_4_1
				move.w	d5,d6
.NoClampMinChord_4_1
				asl.w	#7,d6
				move.w	d6,d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+12(a5),d7
				blt		.Inst4Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 4 - Loop Generator (Offset: 6144 Length: 6144
;----------------------------------------------------------------------------

				move.l	#6144,d7
				move.l	AK_SmpAddr+12(a5),a0
				lea		6144(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_3
				moveq	#0,d0
.LoopGenVC_3
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_3
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_3

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 5 - Instrument_5
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst5Loop
				; v1 = chordgen(0, 0, 2, 5, 0, 100)
				move.l	AK_SmpAddr+0(a5),a4
				move.b	(a4,d7.l),d6
				ext.w	d6
				add.w	#100,a4
				moveq	#0,d4
				move.w	AK_OpInstance+AK_CHORD1+0(a5),d4
				add.l	#73472,AK_OpInstance+AK_CHORD1+0(a5)
				move.b	(a4,d4.l),d5
				ext.w	d5
				add.w	d5,d6
				move.w	AK_OpInstance+AK_CHORD2+0(a5),d4
				add.l	#87552,AK_OpInstance+AK_CHORD2+0(a5)
				move.b	(a4,d4.l),d5
				ext.w	d5
				add.w	d5,d6
				move.w	#255,d5
				cmp.w	d5,d6
				blt.s	.NoClampMaxChord_5_1
				move.w	d5,d6
				bra.s	.NoClampMinChord_5_1
.NoClampMaxChord_5_1
				not.w	d5
				cmp.w	d5,d6
				bge.s	.NoClampMinChord_5_1
				move.w	d5,d6
.NoClampMinChord_5_1
				asl.w	#7,d6
				move.w	d6,d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+16(a5),d7
				blt		.Inst5Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 5 - Loop Generator (Offset: 6784 Length: 5504
;----------------------------------------------------------------------------

				move.l	#5504,d7
				move.l	AK_SmpAddr+16(a5),a0
				lea		6784(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_4
				moveq	#0,d0
.LoopGenVC_4
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_4
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_4

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Instrument 11 - kixx
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst11Loop
				; v1 = imported_sample(smp,6)
				moveq	#0,d0
				cmp.l	AK_ExtSmpLen+24(a5),d7
				bge.s	.NoClone_11_1
				move.l	AK_ExtSmpAddr+24(a5),a4
				move.b	(a4,d7.l),d0
				asl.w	#8,d0
.NoClone_11_1

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+40(a5),d7
				blt		.Inst11Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 11 - Loop Generator (Offset: 1692 Length: 68
;----------------------------------------------------------------------------

				move.l	#68,d7
				move.l	AK_SmpAddr+40(a5),a0
				lea		1692(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_10
				moveq	#0,d0
.LoopGenVC_10
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_10
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_10

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 12 - bassstab
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst12Loop
				; v1 = clone(smp,10, 0)
				moveq	#0,d0
				cmp.l	AK_SmpLen+40(a5),d7
				bge.s	.NoClone_12_1
				move.l	AK_SmpAddr+40(a5),a4
				move.b	(a4,d7.l),d0
				asl.w	#8,d0
.NoClone_12_1

				; v1 = cmb_flt_n(1, v1, 194, 127, 128)
				move.l	a1,a4
				move.w	AK_OpInstance+0(a5),d5
				move.w	(a4,d5.w),d4
				muls	#127,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.CombAddNoClamp_12_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.CombAddNoClamp_12_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#194<<1,d5
				blt.s	.NoCombReset_12_2
				moveq	#0,d5
.NoCombReset_12_2
				move.w  d5,AK_OpInstance+0(a5)
				move.w	d4,d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+44(a5),d7
				blt		.Inst12Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 12 - Loop Generator (Offset: 4566 Length: 2912
;----------------------------------------------------------------------------

				move.l	#2912,d7
				move.l	AK_SmpAddr+44(a5),a0
				lea		4566(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_11
				moveq	#0,d0
.LoopGenVC_11
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_11
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_11

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Instrument 15 - noisia
;----------------------------------------------------------------------------

				moveq	#1,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst15Loop
				; v2 = osc_noise(87)
				move.l	AK_NoiseSeeds+0(a5),d4
				move.l	AK_NoiseSeeds+4(a5),d5
				eor.l	d5,d4
				move.l	d4,AK_NoiseSeeds+0(a5)
				add.l	d5,AK_NoiseSeeds+8(a5)
				add.l	d4,AK_NoiseSeeds+4(a5)
				move.w	AK_NoiseSeeds+10(a5),d1
				muls	#87,d1
				asr.l	#7,d1

				; v1 = sv_flt_n(1, v1, v1, 94, 1)
				move.w	AK_OpInstance+AK_BPF+0(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d0,d5
				move.w	AK_OpInstance+AK_LPF+0(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_15_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_15_2
				move.w	d4,AK_OpInstance+AK_LPF+0(a5)
				muls	#94,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_15_2
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_15_2
.NoClampMaxHPF_15_2
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_15_2
				move.w	#-32768,d5
.NoClampMinHPF_15_2
				move.w	d5,AK_OpInstance+AK_HPF+0(a5)
				asr.w	#7,d5
				muls	d0,d5
				add.w	AK_OpInstance+AK_BPF+0(a5),d5
				bvc.s	.NoClampBPF_15_2
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_15_2
				move.w	d5,AK_OpInstance+AK_BPF+0(a5)
				move.w	AK_OpInstance+AK_HPF+0(a5),d0

				; v1 = add(v1, v2)
				add.w	d1,d0
				bvc.s	.AddNoClamp_15_3
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_15_3

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+56(a5),d7
				blt		.Inst15Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 15 - Loop Generator (Offset: 4178 Length: 4180
;----------------------------------------------------------------------------

				move.l	#4180,d7
				move.l	AK_SmpAddr+56(a5),a0
				lea		4178(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_14
				moveq	#0,d0
.LoopGenVC_14
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_14
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_14

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 16 - 0B_bass2
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst16Loop
				; v2 = envd(0, 11, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#524288,d5
				bgt.s   .EnvDNoSustain_16_1
				moveq	#0,d5
.EnvDNoSustain_16_1
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v4 = mul(v2, 64)
				move.w	d1,d3
				muls	#64,d3
				add.l	d3,d3
				swap	d3

				; v3 = osc_saw(2, 200, 127)
				add.w	#200,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = osc_saw(3, 400, 127)
				add.w	#400,AK_OpInstance+2(a5)
				move.w	AK_OpInstance+2(a5),d0
				muls	#127,d0
				asr.l	#7,d0

				; v1 = add(v3, v1)
				add.w	d2,d0
				bvc.s	.AddNoClamp_16_5
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_16_5

				; v3 = osc_saw(5, 402, 127)
				add.w	#402,AK_OpInstance+4(a5)
				move.w	AK_OpInstance+4(a5),d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_16_7
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_16_7

				; v1 = sv_flt_n(7, v1, v4, 16, 0)
				move.w	AK_OpInstance+AK_BPF+6(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d3,d5
				move.w	AK_OpInstance+AK_LPF+6(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_16_8
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_16_8
				move.w	d4,AK_OpInstance+AK_LPF+6(a5)
				asl.w	#4,d6
				ext.l	d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_16_8
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_16_8
.NoClampMaxHPF_16_8
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_16_8
				move.w	#-32768,d5
.NoClampMinHPF_16_8
				move.w	d5,AK_OpInstance+AK_HPF+6(a5)
				asr.w	#7,d5
				muls	d3,d5
				add.w	AK_OpInstance+AK_BPF+6(a5),d5
				bvc.s	.NoClampBPF_16_8
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_16_8
				move.w	d5,AK_OpInstance+AK_BPF+6(a5)
				move.w	AK_OpInstance+AK_LPF+6(a5),d0

				; v3 = osc_tri(8, 200, 127)
				add.w	#200,AK_OpInstance+12(a5)
				move.w	AK_OpInstance+12(a5),d2
				bge.s	.TriNoInvert_16_9
				not.w	d2
.TriNoInvert_16_9
				sub.w	#16384,d2
				add.w	d2,d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_16_10
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_16_10

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+60(a5),d7
				blt		.Inst16Loop

;----------------------------------------------------------------------------
; Instrument 17 - 0C_bass3
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst17Loop
				; v2 = envd(0, 11, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#524288,d5
				bgt.s   .EnvDNoSustain_17_1
				moveq	#0,d5
.EnvDNoSustain_17_1
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v4 = mul(v2, 32)
				move.w	d1,d3
				muls	#32,d3
				add.l	d3,d3
				swap	d3

				; v3 = osc_saw(2, 200, 127)
				add.w	#200,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = osc_saw(3, 402, 127)
				add.w	#402,AK_OpInstance+2(a5)
				move.w	AK_OpInstance+2(a5),d0
				muls	#127,d0
				asr.l	#7,d0

				; v1 = add(v3, v1)
				add.w	d2,d0
				bvc.s	.AddNoClamp_17_5
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_17_5

				; v3 = osc_saw(5, 398, 127)
				add.w	#398,AK_OpInstance+4(a5)
				move.w	AK_OpInstance+4(a5),d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_17_7
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_17_7

				; v1 = sv_flt_n(7, v1, v4, 16, 0)
				move.w	AK_OpInstance+AK_BPF+6(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d3,d5
				move.w	AK_OpInstance+AK_LPF+6(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_17_8
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_17_8
				move.w	d4,AK_OpInstance+AK_LPF+6(a5)
				asl.w	#4,d6
				ext.l	d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_17_8
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_17_8
.NoClampMaxHPF_17_8
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_17_8
				move.w	#-32768,d5
.NoClampMinHPF_17_8
				move.w	d5,AK_OpInstance+AK_HPF+6(a5)
				asr.w	#7,d5
				muls	d3,d5
				add.w	AK_OpInstance+AK_BPF+6(a5),d5
				bvc.s	.NoClampBPF_17_8
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_17_8
				move.w	d5,AK_OpInstance+AK_BPF+6(a5)
				move.w	AK_OpInstance+AK_LPF+6(a5),d0

				; v3 = osc_tri(8, 200, 127)
				add.w	#200,AK_OpInstance+12(a5)
				move.w	AK_OpInstance+12(a5),d2
				bge.s	.TriNoInvert_17_9
				not.w	d2
.TriNoInvert_17_9
				sub.w	#16384,d2
				add.w	d2,d2
				muls	#127,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_17_10
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_17_10

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+64(a5),d7
				blt		.Inst17Loop

;----------------------------------------------------------------------------
; Instrument 18 - TBlow
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst18Loop
				; v1 = osc_saw(0, 256, 58)
				add.w	#256,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#58,d0
				asr.l	#7,d0

				; v2 = envd(1, 2, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#8388352,d5
				bgt.s   .EnvDNoSustain_18_2
				moveq	#0,d5
.EnvDNoSustain_18_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v2 = add(v2, -9023)
				add.w	#-9023,d1
				bvc.s	.AddNoClamp_18_3
				spl		d1
				ext.w	d1
				eor.w	#$7fff,d1
.AddNoClamp_18_3

				; v2 = ctrl(v2)
				moveq	#9,d4
				asr.w	d4,d1
				add.w	#64,d1

				; v3 = enva(4, 6, 0, 127)
				move.l	AK_OpInstance+2(a5),d5
				move.l	d5,d2
				swap	d2
				add.l	#1677568,d5
				bvc.s   .EnvANoMax_18_5
				move.l	#32767<<16,d5
.EnvANoMax_18_5
				move.l	d5,AK_OpInstance+2(a5)
				muls	#127,d2
				asr.l	#7,d2

				; v3 = add(v3, -31818)
				add.w	#-31818,d2
				bvc.s	.AddNoClamp_18_6
				spl		d2
				ext.w	d2
				eor.w	#$7fff,d2
.AddNoClamp_18_6

				; v3 = ctrl(v3)
				moveq	#9,d4
				asr.w	d4,d2
				add.w	#64,d2

				; v1 = sv_flt_n(7, v1, v2, v3, 0)
				move.w	AK_OpInstance+AK_BPF+6(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d1,d5
				move.w	AK_OpInstance+AK_LPF+6(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_18_8
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_18_8
				move.w	d4,AK_OpInstance+AK_LPF+6(a5)
				move.w	d2,d5
				and.w	#255,d5
				muls	d5,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_18_8
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_18_8
.NoClampMaxHPF_18_8
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_18_8
				move.w	#-32768,d5
.NoClampMinHPF_18_8
				move.w	d5,AK_OpInstance+AK_HPF+6(a5)
				asr.w	#7,d5
				muls	d1,d5
				add.w	AK_OpInstance+AK_BPF+6(a5),d5
				bvc.s	.NoClampBPF_18_8
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_18_8
				move.w	d5,AK_OpInstance+AK_BPF+6(a5)
				move.w	AK_OpInstance+AK_LPF+6(a5),d0

				; v2 = envd(8, 12, 0, 127)
				move.l	AK_EnvDValue+4(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#441344,d5
				bgt.s   .EnvDNoSustain_18_9
				moveq	#0,d5
.EnvDNoSustain_18_9
				move.l	d5,AK_EnvDValue+4(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+68(a5),d7
				blt		.Inst18Loop

;----------------------------------------------------------------------------
; Instrument 19 - TBcutoffincrease
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst19Loop
				; v1 = osc_saw(0, 256, 58)
				add.w	#256,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#58,d0
				asr.l	#7,d0

				; v2 = envd(1, 2, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#8388352,d5
				bgt.s   .EnvDNoSustain_19_2
				moveq	#0,d5
.EnvDNoSustain_19_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v2 = add(v2, 5223)
				add.w	#5223,d1
				bvc.s	.AddNoClamp_19_3
				spl		d1
				ext.w	d1
				eor.w	#$7fff,d1
.AddNoClamp_19_3

				; v2 = ctrl(v2)
				moveq	#9,d4
				asr.w	d4,d1
				add.w	#64,d1

				; v3 = enva(4, 6, 0, 127)
				move.l	AK_OpInstance+2(a5),d5
				move.l	d5,d2
				swap	d2
				add.l	#1677568,d5
				bvc.s   .EnvANoMax_19_5
				move.l	#32767<<16,d5
.EnvANoMax_19_5
				move.l	d5,AK_OpInstance+2(a5)
				muls	#127,d2
				asr.l	#7,d2

				; v3 = add(v3, -31818)
				add.w	#-31818,d2
				bvc.s	.AddNoClamp_19_6
				spl		d2
				ext.w	d2
				eor.w	#$7fff,d2
.AddNoClamp_19_6

				; v3 = ctrl(v3)
				moveq	#9,d4
				asr.w	d4,d2
				add.w	#64,d2

				; v1 = sv_flt_n(7, v1, v2, v3, 0)
				move.w	AK_OpInstance+AK_BPF+6(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d1,d5
				move.w	AK_OpInstance+AK_LPF+6(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_19_8
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_19_8
				move.w	d4,AK_OpInstance+AK_LPF+6(a5)
				move.w	d2,d5
				and.w	#255,d5
				muls	d5,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_19_8
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_19_8
.NoClampMaxHPF_19_8
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_19_8
				move.w	#-32768,d5
.NoClampMinHPF_19_8
				move.w	d5,AK_OpInstance+AK_HPF+6(a5)
				asr.w	#7,d5
				muls	d1,d5
				add.w	AK_OpInstance+AK_BPF+6(a5),d5
				bvc.s	.NoClampBPF_19_8
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_19_8
				move.w	d5,AK_OpInstance+AK_BPF+6(a5)
				move.w	AK_OpInstance+AK_LPF+6(a5),d0

				; v2 = envd(8, 12, 0, 127)
				move.l	AK_EnvDValue+4(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#441344,d5
				bgt.s   .EnvDNoSustain_19_9
				moveq	#0,d5
.EnvDNoSustain_19_9
				move.l	d5,AK_EnvDValue+4(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+72(a5),d7
				blt		.Inst19Loop

;----------------------------------------------------------------------------
; Instrument 20 - reverbedlead
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst20Loop
				; v1 = osc_saw(0, 1024, 98)
				add.w	#1024,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#98,d0
				asr.l	#7,d0

				; v2 = envd(1, 9, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#762368,d5
				bgt.s   .EnvDNoSustain_20_2
				moveq	#0,d5
.EnvDNoSustain_20_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v2 = reverb(v1, 117, 20)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+2(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_20_4_0
				moveq	#0,d5
.NoReverbReset_20_4_0
				move.w  d5,AK_OpInstance+2(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+4(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_20_4_1
				moveq	#0,d5
.NoReverbReset_20_4_1
				move.w  d5,AK_OpInstance+4(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+6(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_20_4_2
				moveq	#0,d5
.NoReverbReset_20_4_2
				move.w  d5,AK_OpInstance+6(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_20_4_3
				moveq	#0,d5
.NoReverbReset_20_4_3
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_20_4_4
				moveq	#0,d5
.NoReverbReset_20_4_4
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_20_4_5
				moveq	#0,d5
.NoReverbReset_20_4_5
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_20_4_6
				moveq	#0,d5
.NoReverbReset_20_4_6
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#117,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_20_4_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_20_4_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_20_4_7
				moveq	#0,d5
.NoReverbReset_20_4_7
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				muls	#20,d7
				asr.l	#7,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_20_4
				move.w	#32767,d7
				bra.s	.NoReverbMin_20_4
.NoReverbMax_20_4
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_20_4
				move.w	#-32768,d7
.NoReverbMin_20_4
				move.w	d7,d1
				move.l	(sp)+,d7

				; v1 = add(v1, v2)
				add.w	d1,d0
				bvc.s	.AddNoClamp_20_5
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_20_5

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+76(a5),d7
				blt		.Inst20Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 20 - Loop Generator (Offset: 5608 Length: 3188
;----------------------------------------------------------------------------

				move.l	#3188,d7
				move.l	AK_SmpAddr+76(a5),a0
				lea		5608(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_19
				moveq	#0,d0
.LoopGenVC_19
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_19
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_19

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 21 - Instrument_21
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst21Loop
				; v1 = imported_sample(smp,1)
				moveq	#0,d0
				cmp.l	AK_ExtSmpLen+4(a5),d7
				bge.s	.NoClone_21_1
				move.l	AK_ExtSmpAddr+4(a5),a4
				move.b	(a4,d7.l),d0
				asl.w	#8,d0
.NoClone_21_1

				; v1 = reverb(v1, 68, 37)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+0(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_21_2_0
				moveq	#0,d5
.NoReverbReset_21_2_0
				move.w  d5,AK_OpInstance+0(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+2(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_21_2_1
				moveq	#0,d5
.NoReverbReset_21_2_1
				move.w  d5,AK_OpInstance+2(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+4(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_21_2_2
				moveq	#0,d5
.NoReverbReset_21_2_2
				move.w  d5,AK_OpInstance+4(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+6(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_21_2_3
				moveq	#0,d5
.NoReverbReset_21_2_3
				move.w  d5,AK_OpInstance+6(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_21_2_4
				moveq	#0,d5
.NoReverbReset_21_2_4
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_21_2_5
				moveq	#0,d5
.NoReverbReset_21_2_5
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_21_2_6
				moveq	#0,d5
.NoReverbReset_21_2_6
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#68,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_21_2_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_21_2_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_21_2_7
				moveq	#0,d5
.NoReverbReset_21_2_7
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				muls	#37,d7
				asr.l	#7,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_21_2
				move.w	#32767,d7
				bra.s	.NoReverbMin_21_2
.NoReverbMax_21_2
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_21_2
				move.w	#-32768,d7
.NoReverbMin_21_2
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+80(a5),d7
				blt		.Inst21Loop

;----------------------------------------------------------------------------
; Instrument 22 - fairlightbass
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst22Loop
				; v1 = osc_saw(0, 670, 90)
				add.w	#670,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#90,d0
				asr.l	#7,d0

				; v2 = osc_tri(1, 670, 127)
				add.w	#670,AK_OpInstance+2(a5)
				move.w	AK_OpInstance+2(a5),d1
				bge.s	.TriNoInvert_22_2
				not.w	d1
.TriNoInvert_22_2
				sub.w	#16384,d1
				add.w	d1,d1
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v2 = osc_saw(3, 676, 52)
				add.w	#676,AK_OpInstance+4(a5)
				move.w	AK_OpInstance+4(a5),d1
				muls	#52,d1
				asr.l	#7,d1

				; v1 = add(v1, v2)
				add.w	d1,d0
				bvc.s	.AddNoClamp_22_5
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_22_5

				; v3 = osc_sine(5, 4, 88)
				add.w	#4,AK_OpInstance+6(a5)
				move.w	AK_OpInstance+6(a5),d2
				sub.w	#16384,d2
				move.w	d2,d5
				bge.s	.SineNoAbs_22_6
				neg.w	d5
.SineNoAbs_22_6
				move.w	#32767,d6
				sub.w	d5,d6
				muls	d6,d2
				swap	d2
				asl.w	#3,d2
				muls	#88,d2
				asr.l	#7,d2

				; v3 = ctrl(v3)
				moveq	#9,d4
				asr.w	d4,d2
				add.w	#64,d2

				; v3 = add(v3, -19)
				add.w	#-19,d2
				bvc.s	.AddNoClamp_22_8
				spl		d2
				ext.w	d2
				eor.w	#$7fff,d2
.AddNoClamp_22_8

				; v1 = sv_flt_n(8, v1, v3, 127, 0)
				move.w	AK_OpInstance+AK_BPF+8(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	d2,d5
				move.w	AK_OpInstance+AK_LPF+8(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_22_9
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_22_9
				move.w	d4,AK_OpInstance+AK_LPF+8(a5)
				muls	#127,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_22_9
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_22_9
.NoClampMaxHPF_22_9
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_22_9
				move.w	#-32768,d5
.NoClampMinHPF_22_9
				move.w	d5,AK_OpInstance+AK_HPF+8(a5)
				asr.w	#7,d5
				muls	d2,d5
				add.w	AK_OpInstance+AK_BPF+8(a5),d5
				bvc.s	.NoClampBPF_22_9
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_22_9
				move.w	d5,AK_OpInstance+AK_BPF+8(a5)
				move.w	AK_OpInstance+AK_LPF+8(a5),d0

				; v2 = osc_tri(9, 672, 46)
				add.w	#672,AK_OpInstance+14(a5)
				move.w	AK_OpInstance+14(a5),d1
				bge.s	.TriNoInvert_22_10
				not.w	d1
.TriNoInvert_22_10
				sub.w	#16384,d1
				add.w	d1,d1
				muls	#46,d1
				asr.l	#7,d1

				; v1 = add(v1, v2)
				add.w	d1,d0
				bvc.s	.AddNoClamp_22_11
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_22_11

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+84(a5),d7
				blt		.Inst22Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 22 - Loop Generator (Offset: 6158 Length: 6158
;----------------------------------------------------------------------------

				move.l	#6158,d7
				move.l	AK_SmpAddr+84(a5),a0
				lea		6158(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_21
				moveq	#0,d0
.LoopGenVC_21
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_21
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_21

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Empty Instrument
;----------------------------------------------------------------------------

				addq.w	#2,a0
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					else
						addq.l	#2,(a3)
					endif
				endif

;----------------------------------------------------------------------------
; Instrument 25 - Instrument_25
;----------------------------------------------------------------------------

				moveq	#0,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst25Loop
				; v1 = osc_saw(0, 1310, 92)
				add.w	#1310,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#92,d0
				asr.l	#7,d0

				; v2 = envd(1, 8, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#931840,d5
				bgt.s   .EnvDNoSustain_25_2
				moveq	#0,d5
.EnvDNoSustain_25_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v1 = sv_flt_n(3, v1, 127, 53, 0)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#127,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_25_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_25_4
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				muls	#53,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_25_4
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_25_4
.NoClampMaxHPF_25_4
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_25_4
				move.w	#-32768,d5
.NoClampMinHPF_25_4
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				muls	#127,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_25_4
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_25_4
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_LPF+2(a5),d0

				; v1 = reverb(v1, 109, 32)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_25_5_0
				moveq	#0,d5
.NoReverbReset_25_5_0
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_25_5_1
				moveq	#0,d5
.NoReverbReset_25_5_1
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_25_5_2
				moveq	#0,d5
.NoReverbReset_25_5_2
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_25_5_3
				moveq	#0,d5
.NoReverbReset_25_5_3
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_25_5_4
				moveq	#0,d5
.NoReverbReset_25_5_4
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_25_5_5
				moveq	#0,d5
.NoReverbReset_25_5_5
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_25_5_6
				moveq	#0,d5
.NoReverbReset_25_5_6
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_25_5_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_25_5_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_25_5_7
				moveq	#0,d5
.NoReverbReset_25_5_7
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_25_5
				move.w	#32767,d7
				bra.s	.NoReverbMin_25_5
.NoReverbMax_25_5
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_25_5
				move.w	#-32768,d7
.NoReverbMin_25_5
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+96(a5),d7
				blt		.Inst25Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 25 - Loop Generator (Offset: 4178 Length: 4180
;----------------------------------------------------------------------------

				move.l	#4180,d7
				move.l	AK_SmpAddr+96(a5),a0
				lea		4178(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_24
				moveq	#0,d0
.LoopGenVC_24
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_24
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_24

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 26 - Instrument_26
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst26Loop
				; v1 = osc_saw(0, 1310, 92)
				add.w	#1310,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#92,d0
				asr.l	#7,d0

				; v2 = envd(1, 8, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#931840,d5
				bgt.s   .EnvDNoSustain_26_2
				moveq	#0,d5
.EnvDNoSustain_26_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v1 = sv_flt_n(3, v1, 92, 53, 0)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#92,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_26_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_26_4
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				muls	#53,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_26_4
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_26_4
.NoClampMaxHPF_26_4
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_26_4
				move.w	#-32768,d5
.NoClampMinHPF_26_4
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				muls	#92,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_26_4
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_26_4
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_LPF+2(a5),d0

				; v1 = reverb(v1, 109, 32)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_26_5_0
				moveq	#0,d5
.NoReverbReset_26_5_0
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_26_5_1
				moveq	#0,d5
.NoReverbReset_26_5_1
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_26_5_2
				moveq	#0,d5
.NoReverbReset_26_5_2
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_26_5_3
				moveq	#0,d5
.NoReverbReset_26_5_3
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_26_5_4
				moveq	#0,d5
.NoReverbReset_26_5_4
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_26_5_5
				moveq	#0,d5
.NoReverbReset_26_5_5
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_26_5_6
				moveq	#0,d5
.NoReverbReset_26_5_6
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_26_5_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_26_5_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_26_5_7
				moveq	#0,d5
.NoReverbReset_26_5_7
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_26_5
				move.w	#32767,d7
				bra.s	.NoReverbMin_26_5
.NoReverbMax_26_5
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_26_5
				move.w	#-32768,d7
.NoReverbMin_26_5
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+100(a5),d7
				blt		.Inst26Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 26 - Loop Generator (Offset: 3808 Length: 2790
;----------------------------------------------------------------------------

				move.l	#2790,d7
				move.l	AK_SmpAddr+100(a5),a0
				lea		3808(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_25
				moveq	#0,d0
.LoopGenVC_25
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_25
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_25

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 27 - Instrument_27
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst27Loop
				; v1 = osc_saw(0, 1310, 92)
				add.w	#1310,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#92,d0
				asr.l	#7,d0

				; v2 = envd(1, 8, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#931840,d5
				bgt.s   .EnvDNoSustain_27_2
				moveq	#0,d5
.EnvDNoSustain_27_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v1 = sv_flt_n(3, v1, 61, 53, 0)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#61,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_27_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_27_4
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				muls	#53,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_27_4
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_27_4
.NoClampMaxHPF_27_4
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_27_4
				move.w	#-32768,d5
.NoClampMinHPF_27_4
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				muls	#61,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_27_4
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_27_4
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_LPF+2(a5),d0

				; v1 = reverb(v1, 109, 32)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_27_5_0
				moveq	#0,d5
.NoReverbReset_27_5_0
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_27_5_1
				moveq	#0,d5
.NoReverbReset_27_5_1
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_27_5_2
				moveq	#0,d5
.NoReverbReset_27_5_2
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_27_5_3
				moveq	#0,d5
.NoReverbReset_27_5_3
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_27_5_4
				moveq	#0,d5
.NoReverbReset_27_5_4
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_27_5_5
				moveq	#0,d5
.NoReverbReset_27_5_5
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_27_5_6
				moveq	#0,d5
.NoReverbReset_27_5_6
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_27_5_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_27_5_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_27_5_7
				moveq	#0,d5
.NoReverbReset_27_5_7
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_27_5
				move.w	#32767,d7
				bra.s	.NoReverbMin_27_5
.NoReverbMax_27_5
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_27_5
				move.w	#-32768,d7
.NoReverbMin_27_5
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+104(a5),d7
				blt		.Inst27Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 27 - Loop Generator (Offset: 4178 Length: 4180
;----------------------------------------------------------------------------

				move.l	#4180,d7
				move.l	AK_SmpAddr+104(a5),a0
				lea		4178(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_26
				moveq	#0,d0
.LoopGenVC_26
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_26
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_26

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 28 - Instrument_28
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst28Loop
				; v1 = osc_saw(0, 1310, 92)
				add.w	#1310,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#92,d0
				asr.l	#7,d0

				; v2 = envd(1, 8, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#931840,d5
				bgt.s   .EnvDNoSustain_28_2
				moveq	#0,d5
.EnvDNoSustain_28_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v1 = sv_flt_n(3, v1, 33, 53, 0)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#33,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_28_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_28_4
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				muls	#53,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_28_4
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_28_4
.NoClampMaxHPF_28_4
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_28_4
				move.w	#-32768,d5
.NoClampMinHPF_28_4
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				muls	#33,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_28_4
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_28_4
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_LPF+2(a5),d0

				; v1 = reverb(v1, 109, 32)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_28_5_0
				moveq	#0,d5
.NoReverbReset_28_5_0
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_28_5_1
				moveq	#0,d5
.NoReverbReset_28_5_1
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_28_5_2
				moveq	#0,d5
.NoReverbReset_28_5_2
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_28_5_3
				moveq	#0,d5
.NoReverbReset_28_5_3
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_28_5_4
				moveq	#0,d5
.NoReverbReset_28_5_4
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_28_5_5
				moveq	#0,d5
.NoReverbReset_28_5_5
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_28_5_6
				moveq	#0,d5
.NoReverbReset_28_5_6
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_28_5_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_28_5_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_28_5_7
				moveq	#0,d5
.NoReverbReset_28_5_7
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_28_5
				move.w	#32767,d7
				bra.s	.NoReverbMin_28_5
.NoReverbMax_28_5
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_28_5
				move.w	#-32768,d7
.NoReverbMin_28_5
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+108(a5),d7
				blt		.Inst28Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 28 - Loop Generator (Offset: 4178 Length: 4180
;----------------------------------------------------------------------------

				move.l	#4180,d7
				move.l	AK_SmpAddr+108(a5),a0
				lea		4178(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_27
				moveq	#0,d0
.LoopGenVC_27
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_27
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_27

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 29 - Instrument_29
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst29Loop
				; v1 = osc_saw(0, 1310, 92)
				add.w	#1310,AK_OpInstance+0(a5)
				move.w	AK_OpInstance+0(a5),d0
				muls	#92,d0
				asr.l	#7,d0

				; v2 = envd(1, 8, 0, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d1
				swap	d1
				sub.l	#931840,d5
				bgt.s   .EnvDNoSustain_29_2
				moveq	#0,d5
.EnvDNoSustain_29_2
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d1
				asr.l	#7,d1

				; v1 = mul(v1, v2)
				muls	d1,d0
				add.l	d0,d0
				swap	d0

				; v1 = sv_flt_n(3, v1, 17, 75, 0)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#17,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_29_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_29_4
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				muls	#75,d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_29_4
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_29_4
.NoClampMaxHPF_29_4
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_29_4
				move.w	#-32768,d5
.NoClampMinHPF_29_4
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				muls	#17,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_29_4
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_29_4
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_LPF+2(a5),d0

				; v1 = reverb(v1, 109, 32)
				move.l	d7,-(sp)
				sub.l	a6,a6
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_29_5_0
				moveq	#0,d5
.NoReverbReset_29_5_0
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_29_5_1
				moveq	#0,d5
.NoReverbReset_29_5_1
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_29_5_2
				moveq	#0,d5
.NoReverbReset_29_5_2
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_29_5_3
				moveq	#0,d5
.NoReverbReset_29_5_3
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_29_5_4
				moveq	#0,d5
.NoReverbReset_29_5_4
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_29_5_5
				moveq	#0,d5
.NoReverbReset_29_5_5
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_29_5_6
				moveq	#0,d5
.NoReverbReset_29_5_6
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#109,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_29_5_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_29_5_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_29_5_7
				moveq	#0,d5
.NoReverbReset_29_5_7
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				asr.w	#2,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_29_5
				move.w	#32767,d7
				bra.s	.NoReverbMin_29_5
.NoReverbMax_29_5
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_29_5
				move.w	#-32768,d7
.NoReverbMin_29_5
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+112(a5),d7
				blt		.Inst29Loop

				movem.l a0-a1,-(sp)	;Stash sample base address & large buffer address for loop generator

;----------------------------------------------------------------------------
; Instrument 29 - Loop Generator (Offset: 3958 Length: 3960
;----------------------------------------------------------------------------

				move.l	#3960,d7
				move.l	AK_SmpAddr+112(a5),a0
				lea		3958(a0),a0
				move.l	a0,a1
				sub.l	d7,a1
				moveq	#0,d4
				move.l	#32767<<8,d5
				move.l	d5,d0
				divs	d7,d0
				bvc.s	.LoopGenVC_28
				moveq	#0,d0
.LoopGenVC_28
				moveq	#0,d6
				move.w	d0,d6
.LoopGen_28
				move.l	d4,d2
				asr.l	#8,d2
				move.l	d5,d3
				asr.l	#8,d3
				move.b	(a0),d0
				move.b	(a1)+,d1
				ext.w	d0
				ext.w	d1
				muls	d3,d0
				muls	d2,d1
				add.l	d1,d0
				add.l	d0,d0
				swap	d0
				move.b	d0,(a0)+
				add.l	d6,d4
				sub.l	d6,d5

				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif

				subq.l	#1,d7
				bne.s	.LoopGen_28

				movem.l (sp)+,a0-a1	;Restore sample base address & large buffer address after loop generator

;----------------------------------------------------------------------------
; Instrument 30 - snareyeah
;----------------------------------------------------------------------------

				moveq	#8,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst30Loop
				; v3 = envd(0, 7, 2, 127)
				move.l	AK_EnvDValue+0(a5),d5
				move.l	d5,d2
				swap	d2
				sub.l	#1198336,d5
				cmp.l	#33554432,d5
				bgt.s   .EnvDNoSustain_30_1
				move.l	#33554432,d5
.EnvDNoSustain_30_1
				move.l	d5,AK_EnvDValue+0(a5)
				muls	#127,d2
				asr.l	#7,d2

				; v3 = mul(v3, 128)
				muls	#128,d2
				add.l	d2,d2
				swap	d2

				; v4 = envd(2, 8, 10, 127)
				move.l	AK_EnvDValue+4(a5),d5
				move.l	d5,d3
				swap	d3
				sub.l	#931840,d5
				cmp.l	#167772160,d5
				bgt.s   .EnvDNoSustain_30_3
				move.l	#167772160,d5
.EnvDNoSustain_30_3
				move.l	d5,AK_EnvDValue+4(a5)
				muls	#127,d3
				asr.l	#7,d3

				; v1 = osc_saw(3, v2, v3)
				add.w	d1,AK_OpInstance+0(a5)
				move.w	d2,d4
				and.w	#255,d4
				move.w	AK_OpInstance+0(a5),d0
				muls	d4,d0
				asr.l	#7,d0

				; v1 = sv_flt_n(4, v1, 16, 4, 1)
				move.w	AK_OpInstance+AK_BPF+2(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				asl.w	#4,d5
				move.w	AK_OpInstance+AK_LPF+2(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_30_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_30_5
				move.w	d4,AK_OpInstance+AK_LPF+2(a5)
				asl.w	#2,d6
				ext.l	d6
				move.w	d0,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_30_5
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_30_5
.NoClampMaxHPF_30_5
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_30_5
				move.w	#-32768,d5
.NoClampMinHPF_30_5
				move.w	d5,AK_OpInstance+AK_HPF+2(a5)
				asr.w	#7,d5
				asl.w	#4,d5
				add.w	AK_OpInstance+AK_BPF+2(a5),d5
				bvc.s	.NoClampBPF_30_5
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_30_5
				move.w	d5,AK_OpInstance+AK_BPF+2(a5)
				move.w	AK_OpInstance+AK_HPF+2(a5),d0

				; v3 = cmb_flt_n(5, v1, 148, 59, 42)
				move.l	a1,a4
				move.w	AK_OpInstance+8(a5),d5
				move.w	(a4,d5.w),d4
				muls	#59,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.CombAddNoClamp_30_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.CombAddNoClamp_30_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#148<<1,d5
				blt.s	.NoCombReset_30_6
				moveq	#0,d5
.NoCombReset_30_6
				move.w  d5,AK_OpInstance+8(a5)
				move.w	d4,d2
				muls	#42,d2
				asr.l	#7,d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_30_7
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_30_7

				; v2 = add(v1, -12823)
				move.w	d0,d1
				add.w	#-12823,d1
				bvc.s	.AddNoClamp_30_8
				spl		d1
				ext.w	d1
				eor.w	#$7fff,d1
.AddNoClamp_30_8

				; v1 = mul(v1, v4)
				muls	d3,d0
				add.l	d0,d0
				swap	d0

				; v1 = reverb(v1, 98, 46)
				move.l	d7,-(sp)
				sub.l	a6,a6
				lea		4096(a1),a4
				move.w	AK_OpInstance+10(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_0
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_0
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#557<<1,d5
				ble.s	.NoReverbReset_30_10_0
				moveq	#0,d5
.NoReverbReset_30_10_0
				move.w  d5,AK_OpInstance+10(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		8192(a1),a4
				move.w	AK_OpInstance+12(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_1
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_1
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#593<<1,d5
				ble.s	.NoReverbReset_30_10_1
				moveq	#0,d5
.NoReverbReset_30_10_1
				move.w  d5,AK_OpInstance+12(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		12288(a1),a4
				move.w	AK_OpInstance+14(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_2
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#641<<1,d5
				ble.s	.NoReverbReset_30_10_2
				moveq	#0,d5
.NoReverbReset_30_10_2
				move.w  d5,AK_OpInstance+14(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		16384(a1),a4
				move.w	AK_OpInstance+16(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_3
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_3
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#677<<1,d5
				ble.s	.NoReverbReset_30_10_3
				moveq	#0,d5
.NoReverbReset_30_10_3
				move.w  d5,AK_OpInstance+16(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		20480(a1),a4
				move.w	AK_OpInstance+18(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_4
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_4
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#709<<1,d5
				ble.s	.NoReverbReset_30_10_4
				moveq	#0,d5
.NoReverbReset_30_10_4
				move.w  d5,AK_OpInstance+18(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		24576(a1),a4
				move.w	AK_OpInstance+20(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_5
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_5
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#743<<1,d5
				ble.s	.NoReverbReset_30_10_5
				moveq	#0,d5
.NoReverbReset_30_10_5
				move.w  d5,AK_OpInstance+20(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				lea		28672(a1),a4
				move.w	AK_OpInstance+22(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_6
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_6
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#787<<1,d5
				ble.s	.NoReverbReset_30_10_6
				moveq	#0,d5
.NoReverbReset_30_10_6
				move.w  d5,AK_OpInstance+22(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				move.l	a1,a4
				add.l	#32768,a4
				move.w	AK_OpInstance+24(a5),d5
				move.w	(a4,d5.w),d4
				muls	#98,d4
				asr.l	#7,d4
				add.w	d0,d4
				bvc.s	.ReverbAddNoClamp_30_10_7
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.ReverbAddNoClamp_30_10_7
				move.w	d4,(a4,d5.w)
				addq.w	#2,d5
				cmp.w	#809<<1,d5
				ble.s	.NoReverbReset_30_10_7
				moveq	#0,d5
.NoReverbReset_30_10_7
				move.w  d5,AK_OpInstance+24(a5)
				move.w	d4,d7
				muls	#46,d7
				asr.l	#7,d7
				add.w	d7,a6
				move.l	a6,d7
				cmp.l	#32767,d7
				ble.s	.NoReverbMax_30_10
				move.w	#32767,d7
				bra.s	.NoReverbMin_30_10
.NoReverbMax_30_10
				cmp.l	#-32768,d7
				bge.s	.NoReverbMin_30_10
				move.w	#-32768,d7
.NoReverbMin_30_10
				move.w	d7,d0
				move.l	(sp)+,d7

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+116(a5),d7
				blt		.Inst30Loop

;----------------------------------------------------------------------------
; Instrument 31 - hi-hat
;----------------------------------------------------------------------------

				moveq	#9,d0
				bsr		AK_ResetVars
				moveq	#0,d7
				ifne	AK_USE_PROGRESS
					ifeq	AK_FINE_PROGRESS
						addq.b	#1,(a3)
					endif
				endif
.Inst31Loop
				; v3 = osc_noise(98)
				move.l	AK_NoiseSeeds+0(a5),d4
				move.l	AK_NoiseSeeds+4(a5),d5
				eor.l	d5,d4
				move.l	d4,AK_NoiseSeeds+0(a5)
				add.l	d5,AK_NoiseSeeds+8(a5)
				add.l	d4,AK_NoiseSeeds+4(a5)
				move.w	AK_NoiseSeeds+10(a5),d2
				muls	#98,d2
				asr.l	#7,d2

				; v3 = sv_flt_n(1, v3, 127, 127, 1)
				move.w	AK_OpInstance+AK_BPF+0(a5),d5
				asr.w	#7,d5
				move.w	d5,d6
				muls	#127,d5
				move.w	AK_OpInstance+AK_LPF+0(a5),d4
				add.w	d5,d4
				bvc.s	.NoClampLPF_31_2
				spl		d4
				ext.w	d4
				eor.w	#$7fff,d4
.NoClampLPF_31_2
				move.w	d4,AK_OpInstance+AK_LPF+0(a5)
				muls	#127,d6
				move.w	d2,d5
				ext.l	d5
				ext.l	d4
				sub.l	d4,d5
				sub.l	d6,d5
				cmp.l	#32767,d5
				ble.s	.NoClampMaxHPF_31_2
				move.w	#32767,d5
				bra.s	.NoClampMinHPF_31_2
.NoClampMaxHPF_31_2
				cmp.l	#-32768,d5
				bge.s	.NoClampMinHPF_31_2
				move.w	#-32768,d5
.NoClampMinHPF_31_2
				move.w	d5,AK_OpInstance+AK_HPF+0(a5)
				asr.w	#7,d5
				muls	#127,d5
				add.w	AK_OpInstance+AK_BPF+0(a5),d5
				bvc.s	.NoClampBPF_31_2
				spl		d5
				ext.w	d5
				eor.w	#$7fff,d5
.NoClampBPF_31_2
				move.w	d5,AK_OpInstance+AK_BPF+0(a5)
				move.w	AK_OpInstance+AK_HPF+0(a5),d2

				; v1 = add(v1, v3)
				add.w	d2,d0
				bvc.s	.AddNoClamp_31_3
				spl		d0
				ext.w	d0
				eor.w	#$7fff,d0
.AddNoClamp_31_3

				asr.w	#8,d0
				move.b	d0,(a0)+
				ifne	AK_USE_PROGRESS
					ifne	AK_FINE_PROGRESS
						addq.l	#1,(a3)
					endif
				endif
				addq.l	#1,d7
				cmp.l	AK_SmpLen+120(a5),d7
				blt		.Inst31Loop


;----------------------------------------------------------------------------

				; Clear first 2 bytes of each sample
				lea		AK_SmpAddr(a5),a6
				moveq	#0,d0
				moveq	#31-1,d7
.SmpClrLoop		move.l	(a6)+,a4
				move.b	d0,(a4)+
				move.b	d0,(a4)+
				dbra	d7,.SmpClrLoop

				rts

;----------------------------------------------------------------------------

AK_ResetVars:
				moveq   #0,d1
				moveq   #0,d2
				moveq   #0,d3
				move.w  d0,d7
				beq.s	.NoClearDelay
				lsl.w	#8,d7
				subq.w	#1,d7
				move.l  a1,a6
.ClearDelayLoop
				move.l  d1,(a6)+
				move.l  d1,(a6)+
				move.l  d1,(a6)+
				move.l  d1,(a6)+
				dbra	d7,.ClearDelayLoop
.NoClearDelay
				moveq   #0,d0
				lea		AK_OpInstance(a5),a6
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l	d0,(a6)+
				move.l  #32767<<16,d6
				move.l	d6,(a6)+
				move.l	d6,(a6)+
				rts

;----------------------------------------------------------------------------

				rsreset
AK_LPF			rs.w	1
AK_HPF			rs.w	1
AK_BPF			rs.w	1
				rsreset
AK_CHORD1		rs.l	1
AK_CHORD2		rs.l	1
AK_CHORD3		rs.l	1
				rsreset
AK_SmpLen		rs.l	31
AK_ExtSmpLen	rs.l	8
AK_NoiseSeeds	rs.l	3
AK_SmpAddr		rs.l	31
AK_ExtSmpAddr	rs.l	8
AK_OpInstance	rs.w    14
AK_EnvDValue	rs.l	2
AK_VarSize		rs.w	0

AK_Vars:
				dc.l	$000044ba		; Instrument 1 Length 
				dc.l	$000025cc		; Instrument 2 Length 
				dc.l	$00003000		; Instrument 3 Length 
				dc.l	$00003000		; Instrument 4 Length 
				dc.l	$00003000		; Instrument 5 Length 
				dc.l	$00000002		; Instrument 6 Length 
				dc.l	$00000002		; Instrument 7 Length 
				dc.l	$00000002		; Instrument 8 Length 
				dc.l	$00000002		; Instrument 9 Length 
				dc.l	$00000002		; Instrument 10 Length 
				dc.l	$000006e0		; Instrument 11 Length 
				dc.l	$00001d36		; Instrument 12 Length 
				dc.l	$00000002		; Instrument 13 Length 
				dc.l	$00000002		; Instrument 14 Length 
				dc.l	$000020a6		; Instrument 15 Length 
				dc.l	$00001000		; Instrument 16 Length 
				dc.l	$00001000		; Instrument 17 Length 
				dc.l	$000012e2		; Instrument 18 Length 
				dc.l	$000012e2		; Instrument 19 Length 
				dc.l	$0000225c		; Instrument 20 Length 
				dc.l	$0000225c		; Instrument 21 Length 
				dc.l	$0000301c		; Instrument 22 Length 
				dc.l	$00000002		; Instrument 23 Length 
				dc.l	$00000002		; Instrument 24 Length 
				dc.l	$000020a6		; Instrument 25 Length 
				dc.l	$000019c6		; Instrument 26 Length 
				dc.l	$000020a6		; Instrument 27 Length 
				dc.l	$000020a6		; Instrument 28 Length 
				dc.l	$00001eee		; Instrument 29 Length 
				dc.l	$00001b7e		; Instrument 30 Length 
				dc.l	$00000516		; Instrument 31 Length 
				dc.l	$00000000		; External Sample 1 Length 
				dc.l	$00001592		; External Sample 2 Length 
				dc.l	$00000000		; External Sample 3 Length 
				dc.l	$00000000		; External Sample 4 Length 
				dc.l	$00000000		; External Sample 5 Length 
				dc.l	$00000000		; External Sample 6 Length 
				dc.l	$000004d8		; External Sample 7 Length 
				dc.l	$00000000		; External Sample 8 Length 
				dc.l	$67452301		; AK_NoiseSeed1
				dc.l	$efcdab89		; AK_NoiseSeed2
				dc.l	$00000000		; AK_NoiseSeed3
				ds.b	AK_VarSize-AK_SmpAddr

;----------------------------------------------------------------------------
