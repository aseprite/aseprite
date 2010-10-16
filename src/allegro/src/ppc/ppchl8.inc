	MakeFunction      _ppc_linear_hline8
._ppc_linear_hline8:
;BITMAP *	r3
;x1	r4
;y	r5
;x2	r6
;color	r7
	lwz	r9, _drawing_mode[TC](rtoc)
	lwz	r11, 0(r9)
	cmpli	cr0, r11, DRAW_MODE_MAX_SUPORTED
	ble	cr0, @ok
	b	._linear_hline8[PR]
@ok	cmp	cr0, r4, r6
	bng	cr0,@n_swap_x1_x2
	mr	r9, r6
	mr	r6, r4
	mr	r4, r9
@n_swap_x1_x2
	lwz	r8, BITMAP.clip(r3)
	cmpi	cr1, r8, 0
	beq	cr1,@n_clip
	lwz	r8, BITMAP.ct(r3)
	lwz	r9, BITMAP.cb(r3)
	cmp	cr6, r5, r8
	cmp	cr7, r5, r9
	bltlr	cr6
	bgelr	cr7

	lwz	r9, BITMAP.cl(r3)
	lwz	r8, BITMAP.cr(r3)

	cmp	cr0, r4, r9
	cmp	cr1, r6, r8
	
	bnl	cr0,@n_clip_x1
	addi	r4, r9, 0
@n_clip_x1:
	blt	cr1,@n_clip_x2
	addi	r6, r8,-1
@n_clip_x2:
	cmp	cr6, r6, r4
	bltlr	cr6
@n_clip:
	sub	r12,r6,r4
	addi	r12,r12,1
	rlwinm	r7, r7, 0, 24, 31
._ppc_linear_n_cache:
	rlwinm	r8, r5, 2, 0, 29
	add	r8, r3, r8
	lwz	r10, 64(r8)
	add	r9, r10, r4
	cmpli	cr7, r11,DRAW_MODE_COPY_PATTERN
	bge	cr7,@pat_transfer
	cmpli	cr0, r11,DRAW_MODE_SOLID
	MAKEDOUBLE8	r7,f0,r11
                  bne	cr0,@xor
	HL8SOLID8	r9,r12,r7,f0,r3,r11
	blr
@xor:
	rlwinm.	r11, r9, 0, 30, 31
	beq	@xor_pre_end
	li	r4,4
	sub	r3,r4,r11
	sub.	r12,r12,r3
	rlwinm	r10, r9, 0, 0, 29
	bge	@xor_n_too_small
	addi	r4,r12,4
	rlwinm	r4,r4,3,0,28
	slw	r7,r7,r4
	li	r12,0
@xor_n_too_small:
	lwz	r4,0(r10)
	rlwinm	r3,r11,3,0,28
	srw	r5,r7,r3
	xor	r6,r4,r5
	stw	r6,0(r10)
	addi	r9,r10,4
@xor_pre_end:
	srawi.	r3,r12,2
	rlwinm	r11, r12, 0, 30, 31
	addi              r9,r9,-4
	beq	@xor_pos
	mtctr             r3
@xor_loop:
	lwzu	r3, 4(r9)
	xor	r4, r3,r7
	stw	r4, 0(r9)
	bdnz              @xor_loop
@xor_pos:
	cmpi	cr6, r11, 0
	li	r5,4
	beqlr	cr6
	lwzu	r4,4(r9)
	sub	r3,r5,r11
	rlwinm	r3,r3,3,0,28
	slw	r5,r7,r3
	xor	r6,r4,r5
	stw	r6,0(r9)
@xor_end:
	blr	

@trans:
	lwz	r11, color_map[TC](rtoc)
	lwz	r10, 0(r11)
	rlwinm	r3, r7, 8, 16, 23
	add	r8, r10, r3
	rlwinm.	r11, r9, 0, 30, 31
	beq	@trans_pre_end
	li	r3,4
	addi	r9,r9,-1
	sub	r3,r3,r11
	cmp	cr6,r3,r12
	blt	cr6,@trans_pre_loop0
	mr	r3,r12
@trans_pre_loop0:
	mtctr	r3
	sub	r12,r12,r3
@trans_pre_loop:
	lbzu	r4, 1(r9)
	lbzx	r5, r8, r4
	stb	r5, 0(r9)
	bdnz              @trans_pre_loop
	addi	r9,r9,1
@trans_pre_end:
	srawi.	r3,r12,2
	rlwinm	r11, r12, 0, 30, 31
	beq	@trans_pos
	addi              r9,r9,-4
	mtctr             r3
@trans_loop:
	lwzu	r7, 4(r9)
	rlwinm	r3,r7,0,24,31
	rlwinm	r4,r7,8,24,31
	rlwinm	r5,r7,16,24,31
	rlwinm	r6,r7,24,24,31
	lbzx	r3,r8,r3
	lbzx	r4,r8,r4
	lbzx	r5,r8,r5
	lbzx	r6,r8,r6
	rlwinm	r4,r4,24,0,31
	rlwinm	r5,r5,16,0,31
	rlwinm	r6,r6, 8,0,31
	add	r7,r3,r4
	add	r5,r5,r6
	add	r3,r7,r5
	stw	r3, 0(r9)

	bdnz              @trans_loop
	addi	r9,r9,4
@trans_pos:
	cmpi	cr6,r11, 0
	beq	cr6,@trans_end
	addi              r9,r9,-1
	mtctr	r11
@trans_pos_loop:
	lbzu	r4, 1(r9)
	lbzx	r5, r8, r4
	stb	r5, 0(r9)
	bdnz	@trans_pos_loop
@trans_end:
	blr	


@pat_transfer:
	cmpli	cr7, r11,DRAW_MODE_TRANS
	beq	cr7,@trans	
	cmpli	cr6, r11,DRAW_MODE_SOLID_PATTERN

	lwz	r11, _drawing_y_anchor[TC](rtoc)
	lwz	r3, _drawing_y_mask[TC](rtoc)
	lwz	r10,0(r11)
	lwz	r11,0(r3)
	add	r8,r10,r5
	and	r8,r8,r11
	lwz	r5, _drawing_pattern[TC](rtoc)
	lwz	r11, _drawing_x_anchor[TC](rtoc)
	lwz	r3, _drawing_x_mask[TC](rtoc)
	rlwinm	r8,r8,2,0,29
	lwz	r5,0(r5)
	add	r5,r5,r8
	lwz	r8,64(r5)
	
	lwz	r10,0(r11)
	lwz	r6,0(r3)


	add	r10,r10,r4
	
	beq	cr6,@solid_pat
	bgt	cr6,@mask_pat	
@copy_pat:
	rlwinm.	r11, r9, 0, 30, 31
	beq	@copy_pat_pre_end
	li	r3,4
	addi	r9,r9,-1
	sub	r3,r3,r11
	cmp	cr6,r3,r12
	blt	cr6,@copy_pat_pre_loop0
	mr	r3,r12
@copy_pat_pre_loop0
	mtctr	r3
	sub	r12,r12,r3
@copy_pat_pre_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	addi	r10,r10,1
	stbu	r3,1(r9)
	bdnz	@copy_pat_pre_loop
	addi	r9,r9,1
@copy_pat_pre_end:
	srawi.	r3,r12,2
	rlwinm	r11, r12, 0, 30, 31
	beq	@copy_pat_pos
	addi              r9,r9,-4
	mtctr             r3
@copy_pat_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	addi	r10,r10,1
	and	r10,r10,r6
	rlwinm	r3, r3, 24, 0, 31	
	lbzx	r4,r10,r8
	addi	r10,r10,1
	and	r10,r10,r6
	rlwinm	r4, r4, 16, 0, 31	
	lbzx	r5,r10,r8
	addi	r10,r10,1
	and	r10,r10,r6
	add	r3,r3,r4
	rlwinm	r5, r5, 8, 0, 31	
	lbzx	r7,r10,r8
	addi	r10,r10,1
	add	r3,r3,r5
	add	r3,r3,r7
	stwu	r3,4(r9)
	bdnz              @copy_pat_loop
	addi	r9,r9,4
@copy_pat_pos:
	cmpi	cr6,r11, 0
	beqlr	cr6
	addi              r9,r9,-1
	mtctr	r11
@copy_pat_pos_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	addi	r10,r10,1
	stbu	r3,1(r9)
	bdnz	@copy_pat_pos_loop
@copy_pat_end:
	blr	

@solid_pat:
	rlwinm.	r11, r9, 0, 30, 31
	beq	@solid_pat_pre_end
	li	r3,4
	addi	r9,r9,-1
	sub	r3,r3,r11
	cmp	cr6,r3,r12
	blt	cr6,@solid_pat_pre_loop0
	mr	r3,r12
@solid_pat_pre_loop0
	mtctr	r3
	sub	r12,r12,r3
@solid_pat_pre_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	cmpi	cr7,r3,0
	addi	r10,r10,1
	beq	cr7,@solid_pat_pre_1
	mr	r3,r7
@solid_pat_pre_1
	stbu	r3,1(r9)
	bdnz	@solid_pat_pre_loop
	addi	r9,r9,1
@solid_pat_pre_end:
	srawi.	r3,r12,2
	rlwinm	r11, r12, 0, 30, 31
	beq	@solid_pat_pos
	addi              r9,r9,-4
	mtctr             r3
@solid_pat_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	addi	r10,r10,1
	cmpi	cr7, r3,0
	and	r10,r10,r6
	lbzx	r4,r10,r8
	beq	cr7,@solid_pat_1
	rlwinm	r3, r7, 24, 0, 31	
@solid_pat_1
	addi	r10,r10,1
	cmpi	cr7, r4,0
	and	r10,r10,r6
	lbzx	r5,r10,r8
	beq	cr7,@solid_pat_2
	rlwinm	r4, r7, 16, 0, 31
	or	r3,r3,r4
@solid_pat_2
	addi	r10,r10,1
	cmpi	cr7, r5,0
	and	r10,r10,r6
	lbzx	r4,r10,r8
	beq	cr7,@solid_pat_3
	rlwinm	r5, r7, 8, 0, 31	
	or	r3,r3,r5
@solid_pat_3
	cmpi	cr7, r4,0
	addi	r10,r10,1
	beq	cr7,@solid_pat_4
	or	r3,r3,r7
@solid_pat_4
	stwu	r3,4(r9)
	bdnz              @solid_pat_loop
	addi	r9,r9,4
@solid_pat_pos:
	cmpi	cr6,r11, 0
	beqlr	cr6
	addi              r9,r9,-1
	mtctr	r11
@solid_pat_pos_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	cmpi	cr7,r3,0
	addi	r10,r10,1
	beq	cr7,@solid_pat_pos_1
	mr	r3,r7
@solid_pat_pos_1	
	stbu	r3,1(r9)
	bdnz	@solid_pat_pos_loop
@solid_pat_end:
	blr	


@mask_pat
	mtctr	r12
@mask_pat_loop:
	and	r10,r10,r6
	lbzx	r3,r10,r8
	cmpi	cr7,r3,0
	addi	r10,r10,1
	beq	cr7,@mask_pat_1
	stb	r7,0(r9)
@mask_pat_1	
	addi	r9,r9,1
	bdnz	@mask_pat_loop
	blr	
._ppc_linear_hline8_end:


	MakeFunction      _ppc_linear_hline8_32
._ppc_linear_hline8_32:
;BITMAP *	r3
;x1	r4
;y	r5
;x2	r6
;color	r7
;cache optimised only solid mode on memory cacheble
	lwz	r9, _drawing_mode[TC](rtoc)
	lwz	r11, 0(r9)
	cmpli	cr0, r11, DRAW_MODE_SOLID
	beq	cr0, @ok
	b	._ppc_linear_hline8
@ok	cmp	cr0, r4, r6
	bng	cr0,@n_swap_x1_x2
	mr	r9, r6
	mr	r6, r4
	mr	r4, r9
@n_swap_x1_x2
	lwz	r8, BITMAP.clip(r3)
	cmpi	cr1, r8, 0
	beq	cr1,@n_clip
	lwz	r8, BITMAP.ct(r3)
	lwz	r9, BITMAP.cb(r3)
	cmp	cr6, r5, r8
	cmp	cr7, r5, r9
	bltlr	cr6
	bgelr	cr7

	lwz	r9, BITMAP.cl(r3)
	lwz	r8, BITMAP.cr(r3)

	cmp	cr0, r4, r9
	cmp	cr1, r6, r8
	
	bnl	cr0,@n_clip_x1
	addi	r4, r9, 0
@n_clip_x1:
	blt	cr1,@n_clip_x2
	addi	r6, r8,-1
@n_clip_x2:
	cmp	cr6, r6, r4
	bltlr	cr6
@n_clip:
	sub	r12,r6,r4
	addi	r12,r12,1
	cmpli	cr0,r12,64
	rlwinm	r7, r7, 0, 24, 31
	
	blt	cr0,._ppc_linear_n_cache
	
	rlwinm	r8, r5, 2, 0, 29
	add	r8, r3, r8
	lwz	r10, 64(r8)
	add	r9, r10, r4

	MAKEDOUBLE8	r7,f0,r11
	HL8SOLID32	r9,r12,r7,f0,r3,r11
	
	blr
._ppc_linear_hline8_32_end: