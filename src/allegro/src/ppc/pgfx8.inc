	MACRO	;Get 4 copies of in &DSTF from &SRC 
	MAKEDOUBLE8	&SRC,&DSTF,&TMP1
		rlwinm	&SRC,&SRC,0,24,31
		rlwinm	&TMP1,&SRC,8,16,23
		or	&SRC,&SRC,&TMP1
		rlwinm	&TMP1,&SRC,16,0,15
		or	&SRC,&SRC,&TMP1

		stw	&SRC,-8(sp)
		stw	&SRC,-4(sp)
		lfd	&DSTF,-8(sp)
	ENDM

	MACRO
	HL8SOLID8	&ADDR,&W,&SRCW,&SRCD,&TMP1,&TMP2
		rlwinm.	&TMP2,&ADDR,0,29,31
		beq	@align
		li	&TMP1,8
		sub	&TMP1,&TMP1,&TMP2
		cmp	cr7,&TMP1,&W
		blt	cr7,@not_too_small
		mr	&TMP1,&W
@not_too_small
		mtcrf	0x03,&TMP1
		sub	&W,&W,&TMP1
@row_loop:
		bc	4,31,@pre_n_b
		stb	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,1
@pre_n_b:
		bc	4,30,@pre_n_h
		sth	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,2
@pre_n_h:
		bc	4,29,@pre_n_w
		stw	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,4
@pre_n_w:
@align:
		srawi.	&TMP1,&W,3
		mtcrf	0x03,&W
		mtctr	&TMP1
		beq	@pos
@loop:
		stfd	&SRCD,0(&ADDR)
		addi	&ADDR,&ADDR,8
		bdnz	@loop
@pos:
		bc	4,29,@pos_n_w
		stw	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,4
@pos_n_w:
		bc	4,30,@pos_n_h
		sth	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,2
@pos_n_h:
		bc	4,31,@pos_n_b
		stb	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,1
@pos_n_b:
	ENDM

	MACRO
	HL8SOLID32	&ADDR,&W,&SRCW,&SRCD,&TMP1,&TMP2
		rlwinm.	&TMP2,&ADDR,0,27,31
		beq	@align
		li	&TMP1,32
		sub	&TMP1,&TMP1,&TMP2
		cmp	cr7,&TMP1,&W
		blt	cr7,@not_too_small
		mr	&TMP1,&W
@not_too_small
		mtcrf	0x03,&TMP1
		sub	&W,&W,&TMP1
@row_loop:
		bc	4,31,@pre_n_b
		stb	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,1
@pre_n_b:
		bc	4,30,@pre_n_h
		sth	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,2
@pre_n_h:
		bc	4,29,@pre_n_w
		stw	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,4
@pre_n_w:
		bc	4,28,@pre_n_d
		stfd	&SRCD,0(&ADDR)
		addi	&ADDR,&ADDR,8
@pre_n_d:
		bc	4,27,@pre_n_q
		stfd	&SRCD,0(&ADDR)
		stfd	&SRCD,8(&ADDR)
		addi	&ADDR,&ADDR,16
@pre_n_q:
@align:
		srawi.	&TMP1,&W,5
		mtcrf	0x03,&W
		mtctr	&TMP1
		beq	@pos
@loop:
		dcbz	0,&ADDR
		stfd	&SRCD,0(&ADDR)
		stfd	&SRCD,8(&ADDR)
		stfd	&SRCD,16(&ADDR)
		stfd	&SRCD,24(&ADDR)
		addi	&ADDR,&ADDR,32
		bdnz	@loop
@pos:
		bc	4,27,@pos_n_q
		stfd	&SRCD,0(&ADDR)
		stfd	&SRCD,8(&ADDR)
		addi	&ADDR,&ADDR,16
@pos_n_q:
		bc	4,28,@pos_n_d
		stfd	&SRCD,0(&ADDR)
		addi	&ADDR,&ADDR,8
@pos_n_d:
		bc	4,29,@pos_n_w
		stw	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,4
@pos_n_w:
		bc	4,30,@pos_n_h
		sth	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,2
@pos_n_h:
		bc	4,31,@pos_n_b
		stb	&SRCW,0(&ADDR)
		addi	&ADDR,&ADDR,1
@pos_n_b:
	ENDM
