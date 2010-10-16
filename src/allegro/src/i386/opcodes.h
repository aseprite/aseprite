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
 *      i386 opcodes for the bitmap stretcher and mode-X sprite compilers.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_I386_OPCODES_H
#define ALLEGRO_I386_OPCODES_H

/*
 * SELinux is the only target for which ALLEGRO_USE_MMAP_GEN_CODE_BUF has to be
 * defined.  So it's okay, for now, if code conditional to
 * ALLEGRO_USE_MMAP_GEN_CODE_BUF uses a Linux-specific feature.
 */
#ifdef __linux__ 
   #define ALLEGRO_USE_MMAP_GEN_CODE_BUF
   #define __USE_GNU     /* for mremap */
   #include <stdlib.h>   /* for mkstemp */
   #include <unistd.h>   /* for unlink */
   #include <sys/mman.h> /* for mmap */

   static void *_exec_map;
   static void *_rw_map;
   static int _map_size;
   static int _map_fd;

   #define GROW_GEN_CODE_BUF(size)                                           \
   do {                                                                      \
      size_t page_size = _unix_get_page_size();                              \
      if (!_map_size) {                                                      \
         /* Create backing file. FIXME: error-checking, but how? */          \
         char tempfile_name[] = "/tmp/allegroXXXXXX";                        \
         _map_fd = mkstemp(tempfile_name);                                   \
         unlink(tempfile_name);                                              \
         /* Grow backing file to multiple of page size */                    \
         _map_size = (size + (page_size-1)) & ~(page_size-1);                \
         ftruncate(_map_fd, _map_size);                                      \
         /* And create the 2 mappings */                                     \
         _exec_map = mmap(0, _map_size, PROT_EXEC | PROT_READ, MAP_SHARED,   \
            _map_fd, 0);                                                     \
         _rw_map = mmap(0, _map_size, PROT_READ | PROT_WRITE, MAP_SHARED,    \
            _map_fd, 0);                                                     \
      }                                                                      \
      else if (size > _map_size) {                                           \
         int old_size = _map_size;                                           \
         /* Grow backing file to multiple of page size */                    \
         _map_size = (size + (page_size-1)) & ~(page_size-1);                \
         ftruncate(_map_fd, _map_size);                                      \
         /* And remap the 2 mappings */                                      \
         _exec_map = mremap(_exec_map, old_size, _map_size, MREMAP_MAYMOVE); \
         _rw_map = mremap(_rw_map, old_size, _map_size, MREMAP_MAYMOVE);     \
      }                                                                      \
   } while (0)

   #define GEN_CODE_BUF _rw_map
#else
   #define GROW_GEN_CODE_BUF(size) _grow_scratch_mem(size)
   #define GEN_CODE_BUF _scratch_mem
#endif

#ifdef ALLEGRO_ASM_USE_FS
   #define FS_PREFIX()     COMPILER_BYTE(0x64)
   #define FS_SIZE         1
#else
   #define FS_PREFIX()
   #define FS_SIZE         0
#endif


#define COMPILER_BYTE(val) {                                                 \
   *(((unsigned char *)GEN_CODE_BUF)+compiler_pos) = val;                    \
   compiler_pos++;                                                           \
}


#define COMPILER_WORD(val) {                                                 \
   *((unsigned short *)(((char *)GEN_CODE_BUF)+compiler_pos)) = val;         \
   compiler_pos += 2;                                                        \
}


#define COMPILER_LONG(val) {                                                 \
   *((unsigned long *)(((char *)GEN_CODE_BUF)+compiler_pos)) = val;          \
   compiler_pos += 4;                                                        \
}


#define COMPILER_INC_ESI() {                                                 \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x46);          /* incl %esi */                             \
}


#define COMPILER_INC_EDI() {                                                 \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x47);          /* incl %edi */                             \
}


#define COMPILER_ADD_ESI(val) {                                              \
   GROW_GEN_CODE_BUF(compiler_pos+6);                                        \
   COMPILER_BYTE(0x81);          /* addl $val, %esi */                       \
   COMPILER_BYTE(0xC6);                                                      \
   COMPILER_LONG(val);                                                       \
}


#define COMPILER_ADD_ECX_EAX() {                                             \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0x01);          /* addl %ecx, %eax */                       \
   COMPILER_BYTE(0xC8);                                                      \
}


#define COMPILER_MOV_EAX(val) {                                              \
   GROW_GEN_CODE_BUF(compiler_pos+5);                                        \
   COMPILER_BYTE(0xB8);          /* movl $val, %eax */                       \
   COMPILER_LONG(val);                                                       \
}


#define COMPILER_MOV_ECX(val) {                                              \
   GROW_GEN_CODE_BUF(compiler_pos+5);                                        \
   COMPILER_BYTE(0xB9);          /* movl $val, %ecx */                       \
   COMPILER_LONG(val);                                                       \
}


#define COMPILER_MOV_EDX(val) {                                              \
   GROW_GEN_CODE_BUF(compiler_pos+5);                                        \
   COMPILER_BYTE(0xBA);          /* movl $val, %edx */                       \
   COMPILER_LONG(val);                                                       \
}


#define COMPILER_MOV_EDI_EAX() {                                             \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0x89);          /* movl %edi, %eax */                       \
   COMPILER_BYTE(0xF8);                                                      \
}


#define COMPILER_CALL_ESI() {                                                \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0xFF);          /* call *%esi */                            \
   COMPILER_BYTE(0xD6);                                                      \
}


#define COMPILER_OUTW() {                                                    \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0x66);          /* outw %ax, %dx */                         \
   COMPILER_BYTE(0xEF);                                                      \
}


#define COMPILER_PUSH_ESI() {                                                \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x56);          /* pushl %esi */                            \
}


#define COMPILER_PUSH_EDI() {                                                \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x57);          /* pushl %edi */                            \
}


#define COMPILER_POP_ESI() {                                                 \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x5E);          /* popl %esi */                             \
}


#define COMPILER_POP_EDI() {                                                 \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0x5F);          /* popl %edi */                             \
}


#define COMPILER_REP_MOVSB() {                                               \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0xF2);          /* rep */                                   \
   COMPILER_BYTE(0xA4);          /* movsb */                                 \
}


#define COMPILER_REP_MOVSW() {                                               \
   GROW_GEN_CODE_BUF(compiler_pos+3);                                        \
   COMPILER_BYTE(0xF3);          /* rep */                                   \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0xA5);          /* movsw */                                 \
}


#define COMPILER_REP_MOVSL() {                                               \
   GROW_GEN_CODE_BUF(compiler_pos+2);                                        \
   COMPILER_BYTE(0xF3);          /* rep */                                   \
   COMPILER_BYTE(0xA5);          /* movsl */                                 \
}


#define COMPILER_REP_MOVSL2() {                                              \
   GROW_GEN_CODE_BUF(compiler_pos+17);                                       \
   COMPILER_BYTE(0x8D);          /* leal (%ecx, %ecx, 2), %ecx */            \
   COMPILER_BYTE(0x0C);                                                      \
   COMPILER_BYTE(0x49);                                                      \
   COMPILER_BYTE(0x8B);          /* movl %ecx, %edx */                       \
   COMPILER_BYTE(0xD1);                                                      \
   COMPILER_BYTE(0x83);                                                      \
   COMPILER_BYTE(0xE2);          /* andl $3, %edx */                         \
   COMPILER_BYTE(0x03);                                                      \
   COMPILER_BYTE(0xC1);                                                      \
   COMPILER_BYTE(0xE9);          /* shrl $2, %ecx */                         \
   COMPILER_BYTE(0x02);                                                      \
   COMPILER_BYTE(0xF3);          /* rep */                                   \
   COMPILER_BYTE(0xA5);          /* movsl */                                 \
   COMPILER_BYTE(0x8B);          /* movl %edx, %ecx */                       \
   COMPILER_BYTE(0xCA);                                                      \
   COMPILER_BYTE(0xF3);          /* rep */                                   \
   COMPILER_BYTE(0xA4);          /* movsb */                                 \
}


#define COMPILER_LODSB() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+3);                                        \
   COMPILER_BYTE(0x8A);          /* movb (%esi), %al */                      \
   COMPILER_BYTE(0x06);                                                      \
   COMPILER_BYTE(0x46);          /* incl %esi */                             \
}


#define COMPILER_LODSW() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+6);                                        \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x8B);          /* movw (%esi), %ax */                      \
   COMPILER_BYTE(0x06);                                                      \
   COMPILER_BYTE(0x83);          /* addl $2, %esi */                         \
   COMPILER_BYTE(0xC6);                                                      \
   COMPILER_BYTE(0x02);                                                      \
}


#define COMPILER_LODSL() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+5);                                        \
   COMPILER_BYTE(0x8B);          /* movl (%esi), %eax */                     \
   COMPILER_BYTE(0x06);                                                      \
   COMPILER_BYTE(0x83);          /* addl $4, %esi */                         \
   COMPILER_BYTE(0xC6);                                                      \
   COMPILER_BYTE(0x04);                                                      \
}


#define COMPILER_LODSL2() {                                                  \
   GROW_GEN_CODE_BUF(compiler_pos+15);                                       \
   COMPILER_BYTE(0x8B);          /* movl (%esi), %eax */                     \
   COMPILER_BYTE(0x06);                                                      \
   COMPILER_BYTE(0x25);          /* andl $0xFFFFFF, %eax */                  \
   COMPILER_LONG(0xFFFFFF);                                                  \
   COMPILER_BYTE(0x8B);          /* movl %eax, %ebx */                       \
   COMPILER_BYTE(0xD8);                                                      \
   COMPILER_BYTE(0xC1);          /* shrl $16, %ebx */                        \
   COMPILER_BYTE(0xEB);                                                      \
   COMPILER_BYTE(0x10);                                                      \
   COMPILER_BYTE(0x83);          /* addl $3, %esi */                         \
   COMPILER_BYTE(0xC6);                                                      \
   COMPILER_BYTE(0x03);                                                      \
}


#define COMPILER_STOSB() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+4);                                        \
   COMPILER_BYTE(0x26);          /* movb %al, %es:(%edi) */                  \
   COMPILER_BYTE(0x88);                                                      \
   COMPILER_BYTE(0x07);                                                      \
   COMPILER_BYTE(0x47);          /* incl %edi */                             \
}


#define COMPILER_STOSW() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+7);                                        \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x89);          /* movw %ax, %es:(%edi) */                  \
   COMPILER_BYTE(0x07);                                                      \
   COMPILER_BYTE(0x83);          /* addl $2, %edi */                         \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x02);                                                      \
}


#define COMPILER_STOSL() {                                                   \
   GROW_GEN_CODE_BUF(compiler_pos+6);                                        \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x89);          /* movl %eax, %es:(%edi) */                 \
   COMPILER_BYTE(0x07);                                                      \
   COMPILER_BYTE(0x83);          /* addl $4, %edi */                         \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x04);                                                      \
}


#define COMPILER_STOSL2() {                                                  \
   GROW_GEN_CODE_BUF(compiler_pos+11);                                       \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x89);          /* movw %ax, %es:(%edi) */                  \
   COMPILER_BYTE(0x07);                                                      \
   COMPILER_BYTE(0x26);          /* movb %bl, %es:2(%edi) */                 \
   COMPILER_BYTE(0x88);                                                      \
   COMPILER_BYTE(0x5F);                                                      \
   COMPILER_BYTE(0x02);                                                      \
   COMPILER_BYTE(0x83);          /* addl $3, %edi */                         \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x03);                                                      \
}


#define COMPILER_MASKED_STOSB(mask_color) {                                  \
   GROW_GEN_CODE_BUF(compiler_pos+8);                                        \
   COMPILER_BYTE(0x08);          /* orb %al, %al */                          \
   COMPILER_BYTE(0xC0);                                                      \
   COMPILER_BYTE(0x74);          /* jz skip */                               \
   COMPILER_BYTE(0x03);                                                      \
   COMPILER_BYTE(0x26);          /* movb %al, %es:(%edi) */                  \
   COMPILER_BYTE(0x88);                                                      \
   COMPILER_BYTE(0x07);                                                      \
   COMPILER_BYTE(0x47);          /* skip: incl %edi */                       \
}


#define COMPILER_MASKED_STOSW(mask_color) {                                  \
   GROW_GEN_CODE_BUF(compiler_pos+13);                                       \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x3D);          /* cmpw mask_color, %ax */                  \
   COMPILER_WORD(mask_color);                                                \
   COMPILER_BYTE(0x74);          /* jz skip */                               \
   COMPILER_BYTE(0x04);          /* how much bytes to jump */                \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x89);          /* movw */                                  \
   COMPILER_BYTE(0x07);          /* %ax, %es:(%edi) */                       \
   COMPILER_BYTE(0x83);          /* skip: addl $2, %edi */                   \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x02);                                                      \
}


#define COMPILER_MASKED_STOSL(mask_color) {                                  \
   GROW_GEN_CODE_BUF(compiler_pos+13);                                       \
   COMPILER_BYTE(0x3D);          /* cmpl mask_color, %eax */                 \
   COMPILER_LONG(mask_color);                                                \
   COMPILER_BYTE(0x74);          /* jz skip */                               \
   COMPILER_BYTE(0x03);          /* how much bytes to jump */                \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x89);          /* movl */                                  \
   COMPILER_BYTE(0x07);          /* %eax, %es:(%edi) */                      \
   COMPILER_BYTE(0x83);          /* skip: addl $4, %edi */                   \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x04);                                                      \
}


#define COMPILER_MASKED_STOSL2(mask_color) {                                 \
   GROW_GEN_CODE_BUF(compiler_pos+18);                                       \
   COMPILER_BYTE(0x3D);          /* cmpl mask_color, %eax */                 \
   COMPILER_LONG(mask_color);                                                \
   COMPILER_BYTE(0x74);          /* jz skip */                               \
   COMPILER_BYTE(0x08);          /* how much bytes to jump */                \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   COMPILER_BYTE(0x26);          /* es segment prefix */                     \
   COMPILER_BYTE(0x89);          /* movw */                                  \
   COMPILER_BYTE(0x07);          /* %ax, %es:(%edi) */                       \
   COMPILER_BYTE(0x26);          /* movb %bl, %es:2(%edi) */                 \
   COMPILER_BYTE(0x88);                                                      \
   COMPILER_BYTE(0x5F);                                                      \
   COMPILER_BYTE(0x02);                                                      \
   COMPILER_BYTE(0x83);          /* skip: addl $3, %edi */                   \
   COMPILER_BYTE(0xC7);                                                      \
   COMPILER_BYTE(0x03);                                                      \
}


#define MOV_IMMED_SIZE(x, offset)                                            \
   (compiler_pos + x + ((offset < 128) ? 2 : 5))


#define MOV_IMMED(offset) {                                                  \
   if (offset < 128) {                                                       \
      COMPILER_BYTE(0x40);       /* byte address offset */                   \
      COMPILER_BYTE(offset);                                                 \
   }                                                                         \
   else {                                                                    \
      COMPILER_BYTE(0x80);       /* long address offset */                   \
      COMPILER_LONG(offset);                                                 \
   }                                                                         \
}


#define COMPILER_MOVB_IMMED(offset, val) {                                   \
   GROW_GEN_CODE_BUF(MOV_IMMED_SIZE(2+FS_SIZE, offset));                     \
   FS_PREFIX();                  /* fs: */                                   \
   COMPILER_BYTE(0xC6);          /* movb $val, offset(%eax) */               \
   MOV_IMMED(offset);                                                        \
   COMPILER_BYTE(val);                                                       \
}


#define COMPILER_MOVW_IMMED(offset, val) {                                   \
   GROW_GEN_CODE_BUF(MOV_IMMED_SIZE(4+FS_SIZE, offset));                     \
   COMPILER_BYTE(0x66);          /* word prefix */                           \
   FS_PREFIX();                  /* fs: */                                   \
   COMPILER_BYTE(0xC7);          /* movw $val, offset(%eax) */               \
   MOV_IMMED(offset);                                                        \
   COMPILER_WORD(val);                                                       \
}


#define COMPILER_MOVL_IMMED(offset, val) {                                   \
   GROW_GEN_CODE_BUF(MOV_IMMED_SIZE(5+FS_SIZE, offset));                     \
   FS_PREFIX();                  /* fs: */                                   \
   COMPILER_BYTE(0xC7);          /* movl $val, offset(%eax) */               \
   MOV_IMMED(offset);                                                        \
   COMPILER_LONG(val);                                                       \
}


#define COMPILER_RET() {                                                     \
   GROW_GEN_CODE_BUF(compiler_pos+1);                                        \
   COMPILER_BYTE(0xC3);          /* ret */                                   \
}


#endif          /* ifndef ALLEGRO_I386_OPCODES_H */

