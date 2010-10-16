/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Inline functions (MSVC style 386 asm).
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#if (!defined ALLEGRO_MSVC) || (!defined ALLEGRO_I386)
   #error bad include
#endif


#pragma warning (disable: 4035)


#ifdef ALLEGRO_IMPORT_GFX_ASM

/* _default_ds:
 *  Return a copy of the current %ds selector.
 */
INLINE _AL_DLL int _default_ds(void)
{
   _asm {
      mov eax, 0
      mov ax, ds
   }
}

END_OF_INLINE(_default_ds);



/* bmp_write_line:
 *  Bank switch function.
 */
INLINE _AL_DLL uintptr_t bmp_write_line(BITMAP *bmp, int lyne)
{ 
   _asm { 
      mov edx, bmp
      mov ecx, [edx]BITMAP.write_bank
      mov eax, lyne
      call ecx
   }
}

END_OF_INLINE(bmp_write_line);



/* bmp_read_line:
 *  Bank switch function.
 */
INLINE _AL_DLL uintptr_t bmp_read_line(BITMAP *bmp, int lyne)
{
   _asm {
      mov edx, bmp
      mov ecx, [edx]BITMAP.read_bank
      mov eax, lyne
      call ecx
   }
}

END_OF_INLINE(bmp_read_line);



/* bmp_unwrite_line:
 *  Terminate bank switch function.
 */
INLINE _AL_DLL void bmp_unwrite_line(BITMAP *bmp)
{
   _asm {
      mov edx, bmp
      mov ecx, [edx]BITMAP.vtable
      mov ecx, [ecx]GFX_VTABLE.unwrite_bank
      call ecx
   }
}

END_OF_INLINE(bmp_unwrite_line);


#endif /* ALLEGRO_IMPORT_GFX_ASM */


#ifdef ALLEGRO_IMPORT_MATH_ASM

/* _set_errno_erange:
 */
INLINE void _set_errno_erange(void)
{
   *allegro_errno = ERANGE;
}

END_OF_INLINE(_set_errno_erange);



/* fixadd:
 *  Fixed point (16.16) addition.
 */
INLINE _AL_DLL fixed fixadd(fixed x, fixed y)
{
   _asm {
      mov eax, x
      add eax, y
      jno Out1
      call _set_errno_erange
      mov eax, 0x7FFFFFFF
      cmp y, 0
      jg Out1
      neg eax
   Out1:
   }
}

END_OF_INLINE(fixadd);



/* fixsub:
 *  Fixed point (16.16) subtraction.
 */
INLINE _AL_DLL fixed fixsub(fixed x, fixed y)
{
   _asm {
      mov eax, x
      sub eax, y
      jno Out1
      call _set_errno_erange
      mov eax, 0x7FFFFFFF
      cmp y, 0
      jl Out1
      neg eax
   Out1:
   }
}

END_OF_INLINE(fixsub);



/* fixmul:
 *  Fixed point (16.16) multiplication.
 */
INLINE _AL_DLL fixed fixmul(fixed x, fixed y)
{
   _asm {
      mov eax, x
      imul y
      shrd eax, edx, 16
      sar edx, 15
      jz Out2
      cmp edx, -1
      jz Out2
      call _set_errno_erange
      mov eax, 0x7FFFFFFF
      cmp x, 0
      jge Out1
      neg eax
   Out1:
      cmp y, 0
      jge Out2
      neg eax
   Out2:
   }
}

END_OF_INLINE(fixmul);



/* fixdiv:
 *  Fixed point (16.16) division.
 */
INLINE _AL_DLL fixed fixdiv(fixed x, fixed y)
{
   _asm {
      mov ecx, y
      xor ebx, ebx
      mov eax, x
      or eax, eax
      jns Out1
      neg eax
      inc ebx
   Out1:
      or ecx, ecx
      jns Out2
      neg ecx
      inc ebx
   Out2:
      mov edx, eax
      shr edx, 0x10
      shl eax, 0x10
      cmp edx, ecx
      jae Out3
      div ecx
      or eax, eax
      jns Out4
   Out3:
      call _set_errno_erange
      mov eax, 0x7FFFFFFF
   Out4:
      test ebx, 1
      je Out5
      neg eax
   Out5:
   }
}

END_OF_INLINE(fixdiv);



/* fixfloor :
 * Fixed point version of floor().
 * Note that it returns an integer result (not a fixed one)
 */
INLINE _AL_DLL int fixfloor(fixed x)
{
   _asm {
      mov eax, x
      sar eax, 0x10
   }
}

END_OF_INLINE(fixfloor);



/* fixceil:
 *  Fixed point version of ceil().
 *  Note that it returns an integer result (not a fixed one)
 */
INLINE _AL_DLL int fixceil(fixed x)
{
   _asm {
      mov eax, x
      add eax, 0xFFFF
      jns Out1
      jo  Out2
   Out1:
      sar eax, 0x10
      jmp Out3
   Out2:
      call _set_errno_erange
      mov eax, 0x7FFF
   Out3:
   }
}

END_OF_INLINE(fixceil);

#endif /* ALLEGRO_IMPORT_MATH_ASM */


#pragma warning (default: 4035)

