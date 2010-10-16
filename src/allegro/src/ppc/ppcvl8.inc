	MakeFunction      _ppc_linear_vline8
._ppc_linear_vline8:
;BITMAP *	r3
;x	r4
;y1	r5
;y2	r6
;color	r7
	lwz	r9, _drawing_mode[TC](rtoc)
	lwz	r9, 0(r9)
	cmpli	cr0, r9, DRAW_MODE_MAX_SUPORTED
	ble	cr0, ._ppc_linear_vline8_ok
	b	._linear_vline8[PR]
._ppc_linear_vline8_ok:
	cmp	cr0, r5, r6
	mr	r10, r6
	bng	cr0,.vl8_5
	mr	r6, r5
	mr	r5, r10
.vl8_5:
	lwz	r8, BITMAP.clip(r3)
	cmpi	cr0, r8, 0
	beq	cr0,.vl8_no_clip
	lwz	r8, BITMAP.cl(r3)
	cmp	cr0, r4, r8
	bltlr	cr0
.vl8_10:
	lwz	r8, BITMAP.cr(r3)
	cmp	cr0, r4, r8
	bgelr	cr0
.vl8_13:
	lwz	r10, BITMAP.ct(r3)
	lwz	r8, BITMAP.cb(r3)
	cmp	cr0, r5, r10
	cmp	cr1, r6, r8
	bnl	cr0,.vl8_15
	mr	r5, r10
.vl8_15:
	blt	cr1,.vl8_17
	addi	r6, r8, -1
.vl8_17:
	cmp	cr0, r6, r5
	bltlr	cr0
.vl8_no_clip:
	stmw	r27, -20(sp)
	rlwinm	r7,r7,0,24,31
	stwu	sp, -96(sp)
	rlwinm	r12, r5, 2, 0, 29
	add	r12, r3, r12
	lwz	r31, 68(r3)
	subf	r11, r5, r6
	lwz	r30, 64(r12)
	cmpi	cr0, r9, DRAW_MODE_SOLID
	lwz	r12, 64(r3)
	addi	r10, r11, 1
	subf	r31, r12, r31
	add	r8, r30, r4
	bne	cr0,.vl8_30
	subf	r9, r31, r8
	mtctr	r10
.vl8_solid_loop:
	stbux	r7, r9, r31
	bdnz	.vl8_solid_loop
	addi	sp, sp, 96
	lmw	r27, -20(sp)
	blr
.vl8_30:
	cmpi	cr0, r9, DRAW_MODE_XOR
	bne	cr0,.vl8_38
	subf	r8, r31, r8
	mtctr	r10
.vl8_xor_loop:
	lbzux	r3, r8, r31
	xor	r4, r3, r7
	stb	r4, 0(r8)
	bdnz	.vl8_xor_loop
	addi	sp, sp, 96
	lmw	r27, -20(sp)
	blr
.vl8_38:
	cmpi	cr0, r9, 5
	bne	cr0,.vl8_patern_mode
	lwz	r3, color_map[TC](rtoc)
	subf	r9, r31, r8
	lwz	r4, 0(r3)
	rlwinm	r5, r7, 8, 16, 23
	add	r8, r4, r5
	mtctr	r10
.vl8_trans_loop:
	lbzux	r3, r9, r31
	lbzx	r4, r3, r8
	stb	r4, 0(r9)
	bdnz	.vl8_trans_loop
	addi	sp, sp, 96
	lmw	r27, -20(sp)
	blr
.vl8_patern_mode:
	lwz	r12, _drawing_pattern[TC](rtoc)
	lwz	r12, 0(r12)
	rlwinm	r11, r6, 2, 0, 29
	add	r11, r3, r11
	lwz	r30, 64(r11)
	add	r6, r4, r30
	lwz	r10, _drawing_y_anchor[TC](rtoc)
	lwz	r30, 0(r10)
	lwz	r11, _drawing_x_mask[TC](rtoc)
	lwz	r11, 0(r11)
	lwz	r3, _drawing_y_mask[TC](rtoc)
	lwz	r10, 0(r3)
	subf	r30, r30, r5
	rlwinm	r28, r10, 2, 0, 29
	lwz	r3, _drawing_x_anchor[TC](rtoc)
	lwz	r3, 0(r3)
	and	r30, r10, r30
	lwz	r27, 64(r12)
	rlwinm	r30, r30, 2, 0, 29
	lwz	r10, 68(r12)
	addi	r12, r12, 64
	subf	r10, r27, r10
	lwzx	r29, r12, r28
	subf	r3, r3, r4
	lwzx	r30, r12, r30
	and	r12, r3, r11

	add	r27, r12, r27
	add	r29, r12, r29
	add	r28, r12, r30
	cmpi	cr0, r9, DRAW_MODE_COPY_PATTERN
	bne	cr0,.vl8_63

	subf	r9, r31, r8
	subf	r8, r10, r28
	subf	r29, r10, r29
.vl8_copy_pat_loop:
	lbzux	r3, r8, r10
	stbux	r3, r9, r31
	cmpl	cr0, r8, r29
	cmpl	cr1, r9, r6
	bng	cr0,.vl8_copy_pat_1
	subf	r8, r10, r27
.vl8_copy_pat_1:
	bng	cr1,.vl8_copy_pat_loop
	b	.vl8_end

.vl8_63:
	cmpi	cr0, r9, DRAW_MODE_SOLID_PATTERN
	bne	cr0,.vl8_80

	subf	r9, r31, r8
	subf	r8, r10, r28
	subf	r29, r10, r29

.vl8_solid_pat_loop:
	lbzux	r3, r8, r10
	cmpi	cr0, r3, 0
	beq	cr0,.L64
	mr	r3,r7
.L64:
	stbux	r3, r9, r31
	cmpl	cr0, r8, r29
	cmpl	cr1, r9, r6
	bng	cr0,.vl8_solid_pat_1
	subf	r8, r10, r27
.vl8_solid_pat_1:
	bng	cr1,.vl8_solid_pat_loop
	b	.vl8_end

.vl8_80:
	mr	r9,r8
	subf	r8, r10, r28
	subf	r29, r10, r29
.vl8_mask_pat_loop:
	lbzux	r3, r8, r10
	cmpi	cr0, r3, 0
	beq	cr0,.vl8_mask_pat_no_write
	stb	r7, 0(r9)
.vl8_mask_pat_no_write
	cmpl	cr0, r8, r29
	cmpl	cr1, r9, r6
	add	r9, r9, r31
	bng	cr0,.vl8_mask_pat_no_reinit
	subf	r8, r10, r27
.vl8_mask_pat_no_reinit:
	bng	cr1,.vl8_mask_pat_loop
.vl8_end:
	addi	sp, sp, 96
	lmw	r27, -20(sp)
	blr