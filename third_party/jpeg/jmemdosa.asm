;
; jmemdosa.asm
;
; Copyright (C) 1992, Thomas G. Lane.
; This file is part of the Independent JPEG Group's software.
; For conditions of distribution and use, see the accompanying README file.
;
; This file contains low-level interface routines to support the MS-DOS
; backing store manager (jmemdos.c).  Routines are provided to access disk
; files through direct DOS calls, and to access XMS and EMS drivers.
;
; This file should assemble with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).  If you haven't got
; a compatible assembler, better fall back to jmemansi.c or jmemname.c.
;
; To minimize dependence on the C compiler's register usage conventions,
; we save and restore all 8086 registers, even though most compilers only
; require SI,DI,DS to be preserved.  Also, we use only 16-bit-wide return
; values, which everybody returns in AX.
;
; Based on code contributed by Ge' Weijers.
;

JMEMDOSA_TXT	segment byte public 'CODE'

		assume	cs:JMEMDOSA_TXT

		public	_jdos_open
		public	_jdos_close
		public	_jdos_seek
		public	_jdos_read
		public	_jdos_write
		public	_jxms_getdriver
		public	_jxms_calldriver
		public	_jems_available
		public	_jems_calldriver

;
; short far jdos_open (short far * handle, char far * filename)
;
; Create and open a temporary file
;
_jdos_open	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	cx,0			; normal file attributes
		lds	dx,dword ptr [bp+10]	; get filename pointer
		mov	ah,3ch			; create file
		int	21h
		jc	open_err		; if failed, return error code
		lds	bx,dword ptr [bp+6]	; get handle pointer
		mov	word ptr [bx],ax	; save the handle
		xor	ax,ax			; return zero for OK
open_err:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jdos_open	endp


;
; short far jdos_close (short handle)
;
; Close the file handle
;
_jdos_close	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	bx,word ptr [bp+6]	; file handle
		mov	ah,3eh			; close file
		int	21h
		jc	close_err		; if failed, return error code
		xor	ax,ax			; return zero for OK
close_err:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jdos_close	endp


;
; short far jdos_seek (short handle, long offset)
;
; Set file position
;
_jdos_seek	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	bx,word ptr [bp+6]	; file handle
		mov	dx,word ptr [bp+8]	; LS offset
		mov	cx,word ptr [bp+10]	; MS offset
		mov	ax,4200h		; absolute seek
		int	21h
		jc	seek_err		; if failed, return error code
		xor	ax,ax			; return zero for OK
seek_err:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jdos_seek	endp


;
; short far jdos_read (short handle, void far * buffer, unsigned short count)
;
; Read from file
;
_jdos_read	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	bx,word ptr [bp+6]	; file handle
		lds	dx,dword ptr [bp+8]	; buffer address
		mov	cx,word ptr [bp+12]	; number of bytes
		mov	ah,3fh			; read file
		int	21h
		jc	read_err		; if failed, return error code
		cmp	ax,word ptr [bp+12]	; make sure all bytes were read
		je	read_ok
		mov	ax,1			; else return 1 for not OK
		jmp	short read_err
read_ok:	xor	ax,ax			; return zero for OK
read_err:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jdos_read	endp


;
; short far jdos_write (short handle, void far * buffer, unsigned short count)
;
; Write to file
;
_jdos_write	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	bx,word ptr [bp+6]	; file handle
		lds	dx,dword ptr [bp+8]	; buffer address
		mov	cx,word ptr [bp+12]	; number of bytes
		mov	ah,40h			; write file
		int	21h
		jc	write_err		; if failed, return error code
		cmp	ax,word ptr [bp+12]	; make sure all bytes written
		je	write_ok
		mov	ax,1			; else return 1 for not OK
		jmp	short write_err
write_ok:	xor	ax,ax			; return zero for OK
write_err:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jdos_write	endp


;
; void far jxms_getdriver (XMSDRIVER far *)
;
; Get the address of the XMS driver, or NULL if not available
;
_jxms_getdriver	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov 	ax,4300h		; call multiplex interrupt with
		int	2fh			; a magic cookie, hex 4300
		cmp 	al,80h			; AL should contain hex 80
		je	xmsavail
		xor 	dx,dx			; no XMS driver available
		xor 	ax,ax			; return a nil pointer
		jmp	short xmsavail_done
xmsavail:	mov 	ax,4310h		; fetch driver address with
		int	2fh			; another magic cookie
		mov 	dx,es			; copy address to dx:ax
		mov 	ax,bx
xmsavail_done:	les 	bx,dword ptr [bp+6]	; get pointer to return value
		mov	word ptr es:[bx],ax
		mov	word ptr es:[bx+2],dx
		pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop	bp
		ret
_jxms_getdriver	endp


;
; void far jxms_calldriver (XMSDRIVER, XMScontext far *)
;
; The XMScontext structure contains values for the AX,DX,BX,SI,DS registers.
; These are loaded, the XMS call is performed, and the new values of the
; AX,DX,BX registers are written back to the context structure.
;
_jxms_calldriver 	proc	far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		les 	bx,dword ptr [bp+10]	; get XMScontext pointer
		mov 	ax,word ptr es:[bx]	; load registers
		mov 	dx,word ptr es:[bx+2]
		mov 	si,word ptr es:[bx+6]
		mov 	ds,word ptr es:[bx+8]
		mov 	bx,word ptr es:[bx+4]
		call	dword ptr [bp+6]	; call the driver
		mov	cx,bx			; save returned BX for a sec
		les 	bx,dword ptr [bp+10]	; get XMScontext pointer
		mov 	word ptr es:[bx],ax	; put back ax,dx,bx
		mov 	word ptr es:[bx+2],dx
		mov 	word ptr es:[bx+4],cx
		pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jxms_calldriver 	endp


;
; short far jems_available (void)
;
; Have we got an EMS driver? (this comes straight from the EMS 4.0 specs)
;
_jems_available	proc	far
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		mov	ax,3567h		; get interrupt vector 67h
		int	21h
		push	cs
		pop	ds
		mov	di,000ah		; check offs 10 in returned seg
		lea	si,ASCII_device_name	; against literal string
		mov	cx,8
		cld
		repe cmpsb
		jne	no_ems
		mov	ax,1			; match, it's there
		jmp	short avail_done
no_ems:		xor	ax,ax			; it's not there
avail_done:	pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		ret

ASCII_device_name	db	"EMMXXXX0"

_jems_available	endp


;
; void far jems_calldriver (EMScontext far *)
;
; The EMScontext structure contains values for the AX,DX,BX,SI,DS registers.
; These are loaded, the EMS trap is performed, and the new values of the
; AX,DX,BX registers are written back to the context structure.
;
_jems_calldriver	proc far
		push	bp			; linkage
		mov 	bp,sp
		push	si			; save all registers for safety
		push	di
		push	bx
		push	cx
		push	dx
		push	es
		push	ds
		les 	bx,dword ptr [bp+6]	; get EMScontext pointer
		mov 	ax,word ptr es:[bx]	; load registers
		mov 	dx,word ptr es:[bx+2]
		mov 	si,word ptr es:[bx+6]
		mov 	ds,word ptr es:[bx+8]
		mov 	bx,word ptr es:[bx+4]
		int	67h			; call the EMS driver
		mov	cx,bx			; save returned BX for a sec
		les 	bx,dword ptr [bp+6]	; get EMScontext pointer
		mov 	word ptr es:[bx],ax	; put back ax,dx,bx
		mov 	word ptr es:[bx+2],dx
		mov 	word ptr es:[bx+4],cx
		pop	ds			; restore registers and exit
		pop	es
		pop	dx
		pop	cx
		pop	bx
		pop	di
		pop	si
		pop 	bp
		ret
_jems_calldriver	endp

JMEMDOSA_TXT	ends

		end
