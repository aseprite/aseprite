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
 *      Video memory manager routines for the PSP.
 *
 *      By diedel, heavily based upon Minix OS code.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"



#define PREFIX_I                "al-pvram INFO: "
#define PREFIX_W                "al-pvram WARNING: "
#define PREFIX_E                "al-pvram ERROR: "



/* Pointer to the first element of the holes list. */
static VRAM_HOLE *hole_head;


static void vmm_del_slot(VRAM_HOLE *prev_ptr, VRAM_HOLE *hp);
static void vmm_merge(VRAM_HOLE *hp);



/* vmm_init:
 *  Initializes the vram holes list.
 *  Initially this list contains an element with the available video memory.
 */
void vmm_init(uintptr_t base, unsigned int available_vram)
{
   VRAM_HOLE *new_h;

   new_h = _AL_MALLOC(sizeof(VRAM_HOLE));
   if (new_h) {
      new_h->h_base = base;
      new_h->h_len = available_vram;
      new_h->h_next = NULL;
      hole_head = new_h;
   }

   TRACE(PREFIX_I "Available VRAM: %u bytes  Base: %x\n", new_h->h_len, new_h->h_base);

}



/* vmm_alloc_mem:
 *  Assigns a free video memory block from the first hole with enough space.
 */
uintptr_t vmm_alloc_mem(unsigned int size)
{
   VRAM_HOLE *hp, *prev_ptr = NULL;
   uintptr_t old_base;

   hp = hole_head;
   while (hp != NULL) {
      if (hp->h_len >= size) {
         /* We have found a hole big enough. */
         old_base = hp->h_base;
         hp->h_base += size;
         hp->h_len -= size;

         if (hp->h_len == 0)
            /* The hole has no more space. Update the holes list. */
            vmm_del_slot(prev_ptr, hp);

         TRACE(PREFIX_I "%u bytes assigned starting at %x\n", size, old_base);
         return old_base;
      }

      prev_ptr = hp;
      hp = hp->h_next;
   }

   return VMM_NO_MEM;
}



/* vmm_free_mem:
 *  Returns a video memory block to the holes list.
 *  If it's adjacent to some of the existing holes we merge it.
 */
void vmm_free_mem(uintptr_t base, unsigned int size)
{
   VRAM_HOLE *hp, *new_ptr, *prev_ptr;

   new_ptr = _AL_MALLOC(sizeof(VRAM_HOLE));
   if (new_ptr) {
      new_ptr->h_base = base;
      new_ptr->h_len = size;
      TRACE(PREFIX_I "New hole: %u bytes at %x\n", new_ptr->h_len, new_ptr->h_base);

      hp = hole_head;
      if (hp == NULL || base <= hp->h_base) {
         /* The free block becomes the head of the holes list. */
         new_ptr->h_next = hp;
         hole_head = new_ptr;
         vmm_merge(new_ptr);
      }
      else {
         while ( hp != NULL && base > hp->h_base) {
            prev_ptr  = hp;
            hp = hp->h_next;
         }

         /* Insert it after prev_ptr. */
         new_ptr->h_next = prev_ptr->h_next;
         prev_ptr->h_next = new_ptr;

         /* We try to merge the sequence 'prev_ptr', 'new_ptr', 'hp' */
         vmm_merge(prev_ptr);
      }
   }
}



/* vmm_del_slot:
 *  Destroy an element of the holes list.
 */
static void vmm_del_slot(VRAM_HOLE *prev_ptr, VRAM_HOLE *hp)
{
   TRACE(PREFIX_I "Slot deleted: %u bytes at %x\n", hp->h_len, hp->h_base);
   if (hp == hole_head)
      hole_head = hp->h_next;
   else
      prev_ptr->h_next = hp->h_next;

   _AL_FREE(hp);
}



/* vmm_merge:
 *  Try to merge a freed video memory block with the adjacent holes.
 *  'hp' is the first of a three holes chain potentially mergeable.
 */
static void vmm_merge(VRAM_HOLE *hp)
{
   VRAM_HOLE *next_ptr;

   if ((next_ptr = hp->h_next) != NULL) {
      if (hp->h_base + hp->h_len == next_ptr->h_base) {
         /* The first absorbs the second. */
         TRACE(PREFIX_I "Hole 1 %u bytes at %x merged with hole 2 %u bytes at %x\n",
               hp->h_len, hp->h_base, next_ptr->h_len, next_ptr->h_base);
         hp->h_len += next_ptr->h_len;
         vmm_del_slot(hp, next_ptr);
      }
      else
         hp = next_ptr;

      /* Now try to merge with the third. */
      if ((next_ptr = hp->h_next) != NULL) {
         if (hp->h_base + hp->h_len == next_ptr->h_base) {
            /* Absorb the third. */
            TRACE(PREFIX_I "Hole 1/2 %u bytes at %x merged with hole 3 %u bytes at %x\n",
               hp->h_len, hp->h_base, next_ptr->h_len, next_ptr->h_base);
            hp->h_len += next_ptr->h_len;
            vmm_del_slot(hp, next_ptr);
         }
      }
   }
}
