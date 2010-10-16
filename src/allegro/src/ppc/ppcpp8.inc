DMOD	SET	r7
	MakeFunction	_ppc_linear_putpixel8
._ppc_linear_putpixel8:
;BITMAP *	r3
;x	r4
;y	r5
;color	r6
	lwz	DMOD,_drawing_mode[TC](rtoc)
	lwz	DMOD,0(DMOD)
	cmpli	cr0,DMOD,DRAW_MODE_TRANS
	ble	cr0,@OK
	b	._linear_putpixel8[PR]
@OK:
	lwz	r8,BITMAP.clip(r3)
	cmpi	cr0,r8,0
	beq	cr0,@no_clip
	lwz	r9,BITMAP.ct(r3)
	lwz	r10,BITMAP.cb(r3)
	cmp	cr1,r5,r9
	cmp	cr6,r5,r10
	bltlr	cr1
	bgelr	cr6
	lwz	r11,BITMAP.cl(r3)
	lwz	r12,BITMAP.cr(r3)
	cmp	cr0,r4,r12
	cmp	cr7,r4,r11
	bgelr	cr0
	bltlr	cr7
@no_clip:
	rlwinm	r10,r5,2,0,29
	add	r10,r3,r10
	lwz	r8,64(r10)
	cmpli	cr1,DMOD,DRAW_MODE_XOR
	add	r9,r8,r4
	bge	cr1,@n_solid
	stb	r6,0(r9)
	blr
@n_solid:
	bne	cr1,@n_xor
	lbz	r3,0(r9)
	xor	r4,r6,r3
	stb	r4,0(r9)
	blr
@n_xor:
	cmpli	cr7,DMOD,DRAW_MODE_TRANS
	bne	cr7,@n_trans
	lwz	r12,color_map[TC](rtoc)
	lwz	r10,0(r12)
	lbz	r11,0(r9)
	add	r10,r10,r11
	rlwinm	r8,r6,8,16,23
	lbzx	r3,r10,r8
	stb	r3,0(r9)
	blr
@n_trans:
	lwz	r12,_drawing_y_anchor[TC](rtoc)
	lwz	r11,0(r12)
	subf	r11,r11,r5
	lwz	r10,_drawing_y_mask[TC](rtoc)
	lwz	r12,0(r10)
	and	r11,r11,r12
	rlwinm	r11,r11,2,0,29
	lwz	r10,_drawing_pattern[TC](rtoc)
	lwz	r12,0(r10)
	add	r11,r12,r11
	lwz	r10,64(r11)
	lwz	r12,_drawing_x_anchor[TC](rtoc)
	lwz	r11,0(r12)
	subf	r11,r11,r4
	lwz	r12,_drawing_x_mask[TC](rtoc)
	lwz	r4,0(r12)
	and	r11,r11,r4
	lbzx	r8,r10,r11
	cmpi	cr0,DMOD,DRAW_MODE_COPY_PATTERN
	bne	cr0,@n_cpy_pat
	stb	r8,0(r9)
	blr
@n_cpy_pat:
	cmpi	cr1,DMOD,DRAW_MODE_MASKED_PATTERN
	cmpi	cr6,r8,0
	bne	cr1,@40
	bne	cr6,@37
	addi	r6,r0,0
@37:
	stb	r6,0(r9)
	blr
@40:
	beqlr	cr6
	stb	r6,0(r9)
	blr
._ppc_linear_putpixel8_end:
