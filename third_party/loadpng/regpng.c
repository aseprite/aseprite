/* loadpng, Allegro wrapper routines for libpng
 * by Peter Wang (tjaden@users.sf.net).
 *
 * This file is hereby placed in the public domain.
 */


#include <allegro.h>
#include "loadpng.h"



/* register_png_file_type:
 */
void register_png_file_type(void)
{
    register_bitmap_file_type("png", load_png, save_png);
}



/* register_png_datafile_object:
 */

static void *load_datafile_png(PACKFILE *f, long size)
{
    BITMAP *bmp;
    void *buffer;

    buffer = malloc(size);
    if (!buffer)
	return NULL;

    if (pack_fread(buffer, size, f) != size) {
	free(buffer);
	return NULL;
    }

    bmp = load_memory_png(buffer, size, NULL);

    free(buffer);

    return bmp;
}

static void destroy_datafile_png(void *data)
{
    if (data) {
	destroy_bitmap((BITMAP *)data);
    }
}

void register_png_datafile_object(int id)
{
    register_datafile_object(id, load_datafile_png, destroy_datafile_png);
}



/* loadpng_init:
 *  This is supposed to resemble jpgalleg_init in JPGalleg 2.0.
 */
int loadpng_init(void)
{
    register_png_datafile_object(DAT_ID('P','N','G',' '));
    register_png_file_type();
    return 0;
}
