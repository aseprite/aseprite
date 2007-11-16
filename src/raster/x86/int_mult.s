/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

.text
	.align 4

/* Function: int _int_mult (int a, int b) */
.globl _int_mult
/*	.type	 _int_mult,@function */
_int_mult:
	pushl %ebp
	movl %esp,%ebp
	
 	movl $0, %eax
 	movb 8(%ebp), %al	/* mov al, a */
	imul 12(%ebp), %ax	/* mul b */
	addw $0x80, %ax		/* add ax, 0x80 */
	addb %ah, %al           /* add al, ah */
	adcb $0, %ah		/* adc ah, 0 */
/* 	movb %ah, c.5 		/\* mov c, ah *\/ */
	shrl $8, %eax
	
	movl %ebp,%esp
	popl %ebp
	ret
