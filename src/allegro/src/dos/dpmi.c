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
 *      Helpers for calling DPMI functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* _create_physical_mapping:
 *  Maps a physical address range into linear memory, and allocates a 
 *  selector to access it.
 */
int _create_physical_mapping(unsigned long *linear, int *segment, unsigned long physaddr, int size)
{
   if (_create_linear_mapping(linear, physaddr, size) != 0)
      return -1;

   if (_create_selector(segment, *linear, size) != 0) {
      _remove_linear_mapping(linear);
      return -1;
   }

   return 0;
}



/* _remove_physical_mapping:
 *  Frees the DPMI resources being used to map a physical address range.
 */
void _remove_physical_mapping(unsigned long *linear, int *segment)
{
   _remove_linear_mapping(linear);
   _remove_selector(segment);
}



/* _create_linear_mapping:
 *  Maps a physical address range into linear memory.
 */
int _create_linear_mapping(unsigned long *linear, unsigned long physaddr, int size)
{
   __dpmi_meminfo meminfo;

   if (physaddr >= 0x100000) {
      /* map into linear memory */
      meminfo.address = physaddr;
      meminfo.size = size;
      if (__dpmi_physical_address_mapping(&meminfo) != 0)
	 return -1;

      *linear = meminfo.address;

      /* lock the linear memory range */
      __dpmi_lock_linear_region(&meminfo);
   }
   else
      /* exploit 1 -> 1 physical to linear mapping in low megabyte */
      *linear = physaddr;

   return 0;
}



/* _remove_linear_mapping:
 *  Frees the DPMI resources being used to map a linear address range.
 */
void _remove_linear_mapping(unsigned long *linear)
{
   __dpmi_meminfo meminfo;

   if (*linear) {
      if (*linear >= 0x100000) {
	 meminfo.address = *linear;
	 __dpmi_free_physical_address_mapping(&meminfo);
      }

      *linear = 0;
   }
}



/* _create_selector:
 *  Allocates a selector to access a region of linear memory.
 */
int _create_selector(int *segment, unsigned long linear, int size)
{
   /* allocate an ldt descriptor */
   *segment = __dpmi_allocate_ldt_descriptors(1);
   if (*segment < 0) {
      *segment = 0;
      return -1;
   }

   /* set the descriptor base and limit */
   __dpmi_set_segment_base_address(*segment, linear);
   __dpmi_set_segment_limit(*segment, MAX(size-1, 0xFFFF));

   return 0;
}



/* _remove_selector:
 *  Frees a DPMI segment selector.
 */
void _remove_selector(int *segment)
{
   if (*segment) {
      __dpmi_free_ldt_descriptor(*segment);
      *segment = 0;
   }
}



/* _unlock_dpmi_data:
 *  Unlocks a memory region (this is needed when repeatedly allocating and
 *  freeing large samples, to prevent all of memory ending up in a locked
 *  state).
 */
void _unlock_dpmi_data(void *addr, int size)
{
   unsigned long baseaddr;
   __dpmi_meminfo mem;

   __dpmi_get_segment_base_address(_default_ds(), &baseaddr);

   mem.address = baseaddr + (unsigned long)addr;
   mem.size = size;

   __dpmi_unlock_linear_region(&mem);
}

