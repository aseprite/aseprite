	MACRO
	RECTFILL8BEGIN
	lwz	r12,_drawing_mode[TC](rtoc)
	lwz	r9,0(r12)
	cmpli	cr0,r9,0
	beq	cr0,@ok
	b	._normal_rectfill[PR]	
@ok	
	mr	r9,r6
	cmp	cr0,r4,r9
	bng	cr0,@5
	mr	r6,r4
	mr	r4,r9
@5:
	mr	r9,r7
	cmp	cr1,r5,r9
	bng	cr1,@9
	mr	r7,r5
	mr	r5,r9
@9:
	lwz	r10,BITMAP.clip(r3)
	cmpi	cr6,r10,0
	beq	cr6,@n_clip
	lwz	r9,12(r3)
	cmp	cr7,r4,r9
	bnl	cr7,@13
	mr	r4,r9
@13:
	lwz	r9,16(r3)
	cmp	cr0,r6,r9
	blt	cr0,@15
	addi	r6,r9,-1
@15:
	cmp	cr1,r6,r4
	bltlr	cr1
@18:
	lwz	r9,20(r3)
	cmp	cr6,r5,r9
	bnl	cr6,@20
	mr	r5,r9
@20:
	lwz	r9,24(r3)
	cmp	cr7,r7,r9
	blt	cr7,@22
	addi	r7,r9,-1
@22:
	cmp	cr0,r7,r5
	bltlr	cr0
@n_clip:
	subf	r11,r5,r7
	addi	r10,r11,1
	rlwinm	r11,r5,2,0,29
	add	r11,r3,r11
	lwz	r9,64(r11)
	add	r9,r4,r9
	subf	r4,r4,r6
	addi	r4,r4,1
	lwz	r6,68(r3)
	lwz	r11,64(r3)
	subf	r6,r11,r6
	subf.	r6,r4,r6
	bne	@solid_cont
	mullw	r4,r4,r10
	addi	r10,r0,1
@solid_cont
	MAKEDOUBLE8	r8,f0,r11
	
	ENDM

	MakeFunction	_ppc_linear_rectfill8

._ppc_linear_rectfill8:
;r3	BITMAP*
;r4	x1
;r5	y1
;r6	x2
;r7	y2
;r8	color
	RECTFILL8BEGIN
._ppc_linear_rectfill8_n_cache
@solid_r_loop:
	addi	r7,r4,0
	HL8SOLID8	r9,r7,r8,f0,r3,r11
	add	r9,r9,r6
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@solid_r_loop
	blr
._ppc_linear_rectfill8_end:

	MakeFunction	_ppc_linear_rectfill8_32
._ppc_linear_rectfill8_32:
;r3	BITMAP*
;r4	x1
;r5	y1
;r6	x2
;r7	y2
;r8	color
	RECTFILL8BEGIN
	cmpli	cr0,r4,64
	blt	cr0,._ppc_linear_rectfill8_n_cache
@solid_r_loop:
	addi	r7,r4,0
	HL8SOLID32	r9,r7,r8,f0,r3,r11
	add	r9,r9,r6
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@solid_r_loop
	blr
._ppc_linear_rectfill8_32_end: