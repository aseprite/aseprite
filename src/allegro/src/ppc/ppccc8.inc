	MACRO
	CLEARBEGIN
		lwz	r10, 4(r3)
		lwz	r8, 0(r3)
		lwz	r9, 64(r3)
		lwz	r6, 68(r3)
		subf	r6, r9, r6
		subf.	r6, r8, r6
		cmpi	cr0, r10, 0
		beqlr	cr0
		bne	@_cont
		mullw	r7,r7,r10
		addi	r10,0,1	
@_cont	
		MAKEDOUBLE8	r4,f0,r11
	ENDM
	
	MakeFunction	_ppc_linear_clear_to_color8
._ppc_linear_clear_to_color8:
;BITMAP *	r3
;color	r4
	CLEARBEGIN
@_r_loop:
	addi	r7,r8,0
	HL8SOLID8	r9,r7,r4,f0,r3,r11
	add	r9,r9,r6
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@_r_loop
	blr
._ppc_linear_clear_to_color8end:

	MakeFunction	_ppc_linear_clear_to_color8_32
._ppc_linear_clear_to_color8_32:
;BITMAP *	r3
;color	r4
;Cache optimized code should be used only with memory cacheble
	CLEARBEGIN
@_r_loop:
	addi	r7,r8,0
	HL8SOLID32	r9,r7,r4,f0,r3,r11
	add	r9,r9,r6
	addi	r10,r10,-1
	cmpi	cr2,r10,0
	bne	cr2,@_r_loop
	blr
._ppc_linear_clear_to_color8_32_end: