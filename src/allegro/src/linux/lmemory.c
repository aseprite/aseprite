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
 *      Memory mapping helpers for Linux Allegro.
 *
 *      By George Foot, heavily based upon Marek Habersack's code.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#if !defined(_POSIX_MAPPED_FILES) || !defined(ALLEGRO_HAVE_MMAP)
#error "Sorry, mapped files are required for Linux console Allegro to work!"
#endif

#include <unistd.h>
#include <sys/mman.h>


static int mem_fd = -1;      /* fd of /dev/mem */

/* __al_linux_init_memory:
 *  Prepares to be able to map memory; returns 0 on success.
 */
int __al_linux_init_memory (void)
{
	mem_fd = open ("/dev/mem", O_RDWR);
	if (mem_fd < 0) return 1;

	mprotect ((void *)&mem_fd, sizeof mem_fd, PROT_READ);
	return 0;
}

/* __al_linux_shutdown_memory:
 *  Turns off memory mapping, returning 0 on success.
 */
int __al_linux_shutdown_memory (void)
{
	if (mem_fd < 0) return 1;

	mprotect ((void *)&mem_fd, sizeof mem_fd, PROT_READ | PROT_WRITE);
	close (mem_fd);
	mem_fd = -1;
	return 0;
}


/* __al_linux_map_memory:
 *  Given a MAPPED_MEMORY struct with physical address, size and 
 *  permissions filled in, this function maps the memory and fills
 *  in the pointer in the struct.  Returns 0 on success; errno will
 *  be set on error.
 */
int __al_linux_map_memory (struct MAPPED_MEMORY *info)
{
	ASSERT(info);
	info->data = mmap (0, info->size, info->perms, MAP_SHARED, mem_fd, info->base);
	if (info->data == MAP_FAILED) {
		info->data = NULL;
		return 1;
	}
	return 0;
}

/* __al_linux_unmap_memory:
 *  Given a MAPPED_MEMORY struct, this function unmaps
 *  the block.  Returns 0 on success, errno set on error.
 */
int __al_linux_unmap_memory (struct MAPPED_MEMORY *info)
{
	ASSERT(info);
	if (info->data == NULL)
		return 0;
	if (!munmap (info->data, info->size)) {
		info->data = NULL;
		return 0;
	}
	return 1;
}

