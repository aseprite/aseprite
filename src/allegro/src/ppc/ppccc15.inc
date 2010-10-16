DST	SET	ARG1
COR	SET	ARG2
	MACRO
	PRECLEAR15
	
		lwz	r10, BITMAP.h(DST)
		rlwinm	COR, COR, 0, 17, 31
		lwz	r11, BITMAP.w(DST)
		rlwinm	r5, COR, 16, 0, 15
		lwz	r9, BITMAP._line(DST)
		add	COR, r5, COR
		lwz	r8, BITMAP._line1(DST)
		subf	r8, r9, r8
		rlwinm	r6, r11, 1, 0, 30
		subf	r8, r6, r8

		cmpi	cr0, r10, 0
		beqlr	cr0
		cmpi	cr1, r11, 0
		beqlr	cr1

		stw	COR,-8(sp)
		stw	COR,-4(sp)
		lfd	f0,-8(sp)
	ENDM

	MakeFunction	_ppc_linear_clear_to_color15
._ppc_linear_clear_to_color15:
	PRECLEAR15
@r_loop:
	rlwinm.	DST,r9,0,30,30
	beq	@pre_n_h
	sth	COR,0(r9)
	addi	r9,r9,2
@pre_n_h:
	rlwinm.	DST,r9,0,29,29
	beq	@pre_n_w
	stw	COR,0(r9)
	addi	r9,r9,4
@pre_n_w:
	mtcrf	0x01,r11
	srawi.	DST,r11,2
	mtctr	DST
@loop:
	stfd	f0,0(r9)
	addi	r9,r9,8
	bdnz	@loop

	bc	4,30,@pos_n_w
	stw	COR,0(r9)
	addi	r9,r9,4
@pos_n_w:
	bc	4,31,@pos_n_h
	sth	COR,0(r9)
	addi	r9,r9,2
@pos_n_h:
	add	r9,r9,r8
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@r_loop
@end:
	blr
._ppc_linear_clear_to_color15_end:


	MakeFunction	_ppc_linear_clear_to_color15_32
._ppc_linear_clear_to_color15_32:
	PRECLEAR15
@r_loop:
	rlwinm.	DST,r9,0,30,30
	beq	@pre_n_h
	sth	COR,0(r9)
	addi	r9,r9,2
@pre_n_h:
	rlwinm.	DST,r9,0,29,29
	beq	@pre_n_w
	stw	COR,0(r9)
	addi	r9,r9,4
@pre_n_w:
	rlwinm.	DST,r9,0,28,28
	beq	@pre_n_d
	stfd	f0,0(r9)
	addi	r9,r9,8
@pre_n_d:
	rlwinm.	DST,r9,0,27,27
	beq	@pre_n_q
	stfd	f0,0(r9)
	stfd	f0,8(r9)
	addi	r9,r9,16
@pre_n_q:
	mtcrf	0x01,r11
	srawi.	DST,r11,4
	mtctr	DST
@loop:
	dcbz	0,r9
	stfd	f0,0(r9)
	stfd	f0,8(r9)
	stfd	f0,16(r9)
	stfd	f0,24(r9)
	addi	r9,r9,32
	bdnz	@loop

	bc	4,28,@pos_n_q
	stfd	f0,0(r9)
	stfd	f0,8(r9)
	addi	r9,r9,16
@pos_n_q:
	bc	4,29,@pos_n_d
	stfd	f0,0(r9)
	addi	r9,r9,8
@pos_n_d:
	bc	4,30,@pos_n_w
	stw	COR,0(r9)
	addi	r9,r9,4
@pos_n_w:
	bc	4,31,@pos_n_h
	sth	COR,0(r9)
	addi	r9,r9,2
@pos_n_h:
	add	r9,r9,r8
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@r_loop
@end:
	blr
._ppc_linear_clear_to_color15_32_end:
