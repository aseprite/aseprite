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
 *      PowerPC CPU detection routines, by Peter Hull.
 *
 *      See readme.txt for copyright information.
 */
#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef SCAN_DEPEND
   #include <mach-o/arch.h>
#endif

void check_cpu() {
  const NXArchInfo* info=NXGetLocalArchInfo();
  cpu_family=info->cputype;
  cpu_model=info->cpusubtype;
  cpu_capabilities=CPU_ID|CPU_FPU; /* confident that this info is correct */
  do_uconvert(info->name, U_ASCII, cpu_vendor, U_CURRENT,
	      _AL_CPU_VENDOR_SIZE);
}
