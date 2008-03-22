/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define ARG_BACK	8(%ebp)
#define ARG_FRONT	12(%ebp)
#define ARG_OPACITY	16(%ebp)

.text
	.align	4

/* int _rgba_blend_normal_sse(int back, int front, int opacity) */
.globl __rgba_blend_normal_sse
	.def	__rgba_blend_normal_sse;	.scl	2;	.type	32;	.endef
__rgba_blend_normal_sse:
	
#if 0 /* TODO en esta alternativa (no optimizada) el -56(%ebp) no se
	utiliza, debemos disminuir a -64 el stack */
	
	/* we create the new stack frame ... */
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$68, %esp		/* with 17 dwords as local variables */

	/* if ((back & 0xff000000) != 0) goto L1 */
	movl	ARG_BACK, %eax
	andl	$-16777216, %eax
	testl	%eax, %eax
	jne	L1

	/* then ((back & 0xff000000) == 0) */
	movl	ARG_FRONT, %ebx
	andl	$16777215, %ebx
	movl	ARG_OPACITY, %edx
	/* ecx = _rgba_geta(front) */
	movl	ARG_FRONT, %eax
	sarl	$24, %eax		/* alpha channel of ARG_FRONT */
	movzbl	%al, %ecx		/* al = (eax & 0xff) */
	call	@_int_mult_fast@8       /* _int_mult_fast(%ecx, %edx) */
	sall	$24, %eax		/* eax <<= _rgba_a_shift */
	orl	%ebx, %eax		/* eax |= ebx */
	jmp	L3

L1:
	/* if ((front & 0xff000000) != 0) goto L2 */
	movl	ARG_FRONT, %eax
	andl	$-16777216, %eax
	testl	%eax, %eax
	jne	L2

	/* then ((front & 0xff000000) == 0) */
	movl	ARG_BACK, %eax		/* return ARG_BACK */
	jmp	L3

L2:
	movzbl	ARG_BACK,%eax
	movl	%eax, -8(%ebp)
	movl	ARG_BACK, %eax
	sarl	$8, %eax
	andl	$255, %eax
	movl	%eax, -12(%ebp)
	movl	ARG_BACK, %eax
	sarl	$16, %eax
	andl	$255, %eax
	movl	%eax, -16(%ebp)
	movl	ARG_BACK, %eax
	sarl	$24, %eax
	andl	$255, %eax
	movl	%eax, -20(%ebp)
	movzbl	ARG_FRONT,%eax
	movl	%eax, -24(%ebp)
	movl	ARG_FRONT, %eax
	sarl	$8, %eax
	andl	$255, %eax
	movl	%eax, -28(%ebp)
	movl	ARG_FRONT, %eax
	sarl	$16, %eax
	andl	$255, %eax
	movl	%eax, -32(%ebp)
	movl	ARG_FRONT, %eax
	sarl	$24, %eax
	andl	$255, %eax
	movl	%eax, -36(%ebp)
	movl	ARG_OPACITY, %edx
	movl	-36(%ebp), %ecx
	call	@_int_mult_fast@8
	movl	%eax, -36(%ebp)
	movl	-36(%ebp), %eax
	movl	-20(%ebp), %ebx
	addl	%eax, %ebx
	movl	-36(%ebp), %edx
	movl	-20(%ebp), %ecx
	call	@_int_mult_fast@8
	subl	%eax, %ebx
	movl	%ebx, %eax
	movl	%eax, -52(%ebp)
	movl	-8(%ebp), %edx
	movl	-24(%ebp), %eax
	subl	%edx, %eax
	movl	%eax, %edx
	imull	-36(%ebp), %edx
	leal	-52(%ebp), %eax
	movl	%eax, -60(%ebp)
	movl	%edx, %eax
	movl	-60(%ebp), %ecx
	cltd
	idivl	(%ecx)
	movl	%eax, -60(%ebp)
	movl	-60(%ebp), %eax
	addl	-8(%ebp), %eax
	movl	%eax, -40(%ebp)
	movl	-12(%ebp), %edx
	movl	-28(%ebp), %eax
	subl	%edx, %eax
	movl	%eax, %edx
	imull	-36(%ebp), %edx
	leal	-52(%ebp), %eax
	movl	%eax, -60(%ebp)
	movl	%edx, %eax
	movl	-60(%ebp), %ecx
	cltd
	idivl	(%ecx)
	movl	%eax, -60(%ebp)
	movl	-60(%ebp), %eax
	addl	-12(%ebp), %eax
	movl	%eax, -44(%ebp)
	movl	-16(%ebp), %edx
	movl	-32(%ebp), %eax
	subl	%edx, %eax
	movl	%eax, %edx
	imull	-36(%ebp), %edx
	leal	-52(%ebp), %eax
	movl	%eax, -60(%ebp)
	movl	%edx, %eax
	movl	-60(%ebp), %ecx
	cltd
	idivl	(%ecx)
	movl	%eax, -60(%ebp)
	movl	-60(%ebp), %eax
	addl	-16(%ebp), %eax
	movl	%eax, -48(%ebp)
	movl	-44(%ebp), %eax
	sall	$8, %eax
	orl	-40(%ebp), %eax
	movl	-48(%ebp), %edx
	sall	$16, %edx
	orl	%edx, %eax
	movl	-52(%ebp), %edx
	sall	$24, %edx
	orl	%edx, %eax

L3:
	/* restore the old stack frame */
	addl	$68, %esp
	popl	%ebx
	popl	%ebp
	ret

#endif

#if 1 /* TODO add SSE support (the routine is called "_sse" but
	doesn't use any SSE at the moment :) */

#define LOCAL_OLD_EBX	-12(%ebp)
#define LOCAL_OLD_ESI	-8(%ebp)
#define LOCAL_OLD_EDI	-4(%ebp)

	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp

	/*
	 * edx = back
	 * eax = front
	 * ebx = opacity
	 */

	movl	ARG_BACK, %edx
	movl	ARG_FRONT, %eax
	movl	%ebx, LOCAL_OLD_EBX /* it seems that it's ~1 msec faster if we put it here */
	movl	ARG_OPACITY, %ebx
	movl	%esi, LOCAL_OLD_ESI
	movl	%edi, LOCAL_OLD_EDI

	/* if ((back & 0xff000000) != 0) then goto L2 */
	testl	$0xff000000, %edx
	jne	L2

	/* else ((back & 0xff000000) == 0) */
	movl	%eax, %esi		/* esi = front */
	andl	$0xffffff, %esi		/* esi &= 0xffffff */
	shrl	$24, %eax		/* eax = front(eax) >> 24 = alpha */
	imull	%ebx, %eax		/* eax *= opacity(ebx) */
	leal	128(%eax), %edx
	movl	%edx, %eax
	sarl	$8, %eax
	addl	%edx, %eax
	sarl	$8, %eax
	sall	$24, %eax
	orl	%eax, %esi
L1:
	movl	%esi, %eax
	movl	LOCAL_OLD_EBX, %ebx
	movl	LOCAL_OLD_ESI, %esi
	movl	LOCAL_OLD_EDI, %edi
	movl	%ebp, %esp
	popl	%ebp
	ret
L2:
	testl	$-16777216, %eax
	movl	%edx, %esi
	je	L1
	movl	%edx, %esi
	movzbl	%dl,%ecx
	sarl	$16, %esi
	movl	%ecx, -16(%ebp)
	movzbl	%dh, %edi
	andl	$255, %esi
	movl	%edi, -20(%ebp)
	movl	%eax, %ecx
	movzbl	%al,%edi
	movl	%esi, -24(%ebp)
	movzbl	%ah, %esi
	shrl	$24, %eax
	imull	%ebx, %eax
	sarl	$16, %ecx
	andl	$255, %ecx
	movl	%ecx, -28(%ebp)
	shrl	$24, %edx
	subl	$-128, %eax
	movl	%eax, %ecx
	sarl	$8, %ecx
	addl	%eax, %ecx
	sarl	$8, %ecx
	leal	(%edx,%ecx), %ebx
	imull	%ecx, %edx
	subl	$-128, %edx
	movl	%edx, %eax
	sarl	$8, %eax
	addl	%edx, %eax
	sarl	$8, %eax
	subl	%eax, %ebx
	movl	-16(%ebp), %eax
	subl	%eax, %edi
	imull	%ecx, %edi
	movl	%edi, %eax
	cltd
	movl	-20(%ebp), %edi
	idivl	%ebx
	movl	-16(%ebp), %edx
	subl	%edi, %esi
	movl	%eax, -40(%ebp)
	imull	%ecx, %esi
	addl	%edx, %eax
	movl	%eax, -32(%ebp)
	movl	%esi, %eax
	cltd
	idivl	%ebx
	movl	-20(%ebp), %edx
	movl	%eax, %esi
	movl	-24(%ebp), %eax
	addl	%edx, %esi
	subl	%eax, -28(%ebp)
	movl	-28(%ebp), %edi
	imull	%edi, %ecx
	movl	%ecx, -28(%ebp)
	movl	%ecx, %eax
	cltd
	idivl	%ebx
	movl	-32(%ebp), %ecx
	movl	%eax, %edx
	movl	-24(%ebp), %eax
	sall	$8, %esi
	orl	%ecx, %esi
	sall	$24, %ebx
	leal	(%edx,%eax), %edi
	sall	$16, %edi
	orl	%edi, %esi
	orl	%ebx, %esi

	movl	%esi, %eax
	movl	LOCAL_OLD_EBX, %ebx
	movl	LOCAL_OLD_ESI, %esi
	movl	LOCAL_OLD_EDI, %edi
	movl	%ebp, %esp
	popl	%ebp
	ret

#endif

