/* loadpng, Allegro wrapper routines for libpng
 * by Peter Wang (tjaden@users.sf.net).
 *
 * This file is hereby placed in the public domain.
 */


#include <png.h>
#include <allegro.h>
#include <allegro/internal/aintern.h>
#include "loadpng.h"

/* We need internals _color_load_depth and _fixup_loaded_bitmap.  The
 * first can be replaced by the new get_color_depth() function which
 * is in Allegro 4.1 branch.  But it's not worth it to break 4.0
 * compatibility.
 */



double _png_screen_gamma = -1.0;
int _png_compression_level = Z_BEST_COMPRESSION;



/* get_gamma:
 *  Get screen gamma value one of three ways.
 */
static double get_gamma(void)
{
    if (_png_screen_gamma == -1.0) {
	/* Use the environment variable if available.
	 * 2.2 is a good guess for PC monitors.
	 * 1.1 is good for my laptop.
	 */
	AL_CONST char *gamma_str = getenv("SCREEN_GAMMA");
	return (gamma_str) ? atof(gamma_str) : 2.2;
    }

    return _png_screen_gamma;
}



/* read_data:
 *  Custom read function to use Allegro packfile routines,
 *  rather than C streams (so we can read from datafiles!)
 */
static void read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    PACKFILE *f = (PACKFILE *)png_get_io_ptr(png_ptr);
    if ((png_uint_32)pack_fread(data, length, f) != length)
	png_error(png_ptr, "read error (loadpng calling pack_fread)");
}



/* check_if_png:
 *  Check if input file is really PNG format.
 */
#define PNG_BYTES_TO_CHECK 4

static int check_if_png(PACKFILE *fp)
{
    unsigned char buf[PNG_BYTES_TO_CHECK];

    ASSERT(fp);

    if (pack_fread(buf, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK)
	return 0;

    return (png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK) == 0);
}



/* really_load_png:
 *  Worker routine, used by load_png and load_memory_png.
 */
static BITMAP *really_load_png(png_structp png_ptr, png_infop info_ptr, RGB *pal)
{
    BITMAP *bmp;
    PALETTE tmppal;
    png_uint_32 width, height, rowbytes;
    int bit_depth, color_type, interlace_type;
    double image_gamma, screen_gamma;
    int intent;
    int bpp, dest_bpp;
    int tRNS_to_alpha = FALSE;
    int number_passes, pass;

    ASSERT(png_ptr && info_ptr && rgb);

    /* The call to png_read_info() gives us all of the information from the
     * PNG file before the first IDAT (image data chunk).
     */
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		 &interlace_type, NULL, NULL);

    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images).
     */
    png_set_packing(png_ptr);

    /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
    if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
	png_set_expand(png_ptr);

    /* Adds a full alpha channel if there is transparency information
     * in a tRNS chunk. */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	png_set_tRNS_to_alpha(png_ptr);
	tRNS_to_alpha = TRUE;
    }

    /* Convert 16-bits per colour component to 8-bits per colour component. */
    if (bit_depth == 16)
	png_set_strip_16(png_ptr);

    /* Convert grayscale to RGB triplets */
    if ((color_type == PNG_COLOR_TYPE_GRAY) ||
	(color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
	png_set_gray_to_rgb(png_ptr);

    /* Optionally, tell libpng to handle the gamma correction for us. */
    if (_png_screen_gamma != 0.0) {
	screen_gamma = get_gamma();

	if (png_get_sRGB(png_ptr, info_ptr, &intent))
	    png_set_gamma(png_ptr, screen_gamma, 0.45455);
	else {
	    if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
		png_set_gamma(png_ptr, screen_gamma, image_gamma);
	    else
		png_set_gamma(png_ptr, screen_gamma, 0.45455);
	}
    }

    /* Turn on interlace handling. */
    number_passes = png_set_interlace_handling(png_ptr);
    
    /* Call to gamma correct and add the background to the palette
     * and update info structure.
     */
    png_read_update_info(png_ptr, info_ptr);
    
    /* Even if the user doesn't supply space for a palette, we want
     * one for the load process.
     */
    if (!pal)
	pal = tmppal;
    
    /* Palettes. */
    if (color_type & PNG_COLOR_MASK_PALETTE) {
	int num_palette, i;
	png_colorp palette;

	if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
	    /* We don't actually dither, we just copy the palette. */
	    for (i = 0; ((i < num_palette) && (i < 256)); i++) {
		pal[i].r = palette[i].red >> 2;		/* 256 -> 64 */
		pal[i].g = palette[i].green >> 2;
		pal[i].b = palette[i].blue >> 2;
	    }

	    for (; i < 256; i++)
		pal[i].r = pal[i].g = pal[i].b = 0;
	}
    }
    else {
	generate_332_palette(pal);
    }

    rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    /* Allocate the memory to hold the image using the fields of info_ptr. */
    bpp = rowbytes * 8 / width;

    /* Allegro cannot handle less than 8 bpp. */
    if (bpp < 8)
	bpp = 8;

    dest_bpp = _color_load_depth(bpp, (bpp == 32));
    bmp = create_bitmap_ex(bpp, width, height);

    /* Maybe flip RGB to BGR. */
    if ((bpp == 24) || (bpp == 32)) {
	int c = makecol_depth(bpp, 0, 0, 255);
	unsigned char *pc = (unsigned char *)&c;
	if (pc[0] == 255)
	    png_set_bgr(png_ptr);
#ifdef ALLEGRO_BIG_ENDIAN	    
	png_set_swap_alpha(png_ptr);
#endif	    
    }

    /* Read the image, one line at a line (easier to debug!) */
    for (pass = 0; pass < number_passes; pass++) {
	png_uint_32 y;
	for (y = 0; y < height; y++)
	    png_read_row(png_ptr, bmp->line[y], NULL);
    }

    /* Let Allegro convert the image into the desired colour depth. */
    if (dest_bpp != bpp)
	bmp = _fixup_loaded_bitmap(bmp, pal, dest_bpp);

    /* Read rest of file, and get additional chunks in info_ptr. */
    png_read_end(png_ptr, info_ptr);

    return bmp;
}



/* load_png:
 *  Load a PNG file from disk, doing colour coversion if required.
 */
BITMAP *load_png(AL_CONST char *filename, RGB *pal)
{
    PACKFILE *fp;
    BITMAP *bmp;

    ASSERT(filename);

    fp = pack_fopen(filename, "r");
    if (!fp)
	return NULL;

    bmp = load_png_pf(fp, pal);

    pack_fclose(fp);

    return bmp;
}



/* load_png_pf:
 *  Load a PNG file from disk, doing colour coversion if required.
 */
BITMAP *load_png_pf(PACKFILE *fp, RGB *pal)
{
    BITMAP *bmp;
    png_structp png_ptr;
    png_infop info_ptr;

    ASSERT(fp);

    if (!check_if_png(fp)) {
	return NULL;
    }

    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also supply the
     * the compiler header file version, so that we know if the application
     * was compiled with a compatible version of the library.
     */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				     (void *)NULL, NULL, NULL);
    if (!png_ptr) {
	return NULL;
    }

    /* Allocate/initialize the memory for image information. */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
	return NULL;
    }

    /* Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
    if (setjmp(png_ptr->jmpbuf)) {
	/* Free all of the memory associated with the png_ptr and info_ptr */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	/* If we get here, we had a problem reading the file */
	return NULL;
    }

    /* Use Allegro packfile routines. */
    png_set_read_fn(png_ptr, fp, (png_rw_ptr)read_data);

    /* We have already read some of the signature. */
    png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

    /* Really load the image now. */
    bmp = really_load_png(png_ptr, info_ptr, pal);

    /* Clean up after the read, and free any memory allocated. */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    return bmp;
}



/* read_data_memory:
 *  Custom reader function to read a PNG file from a memory buffer.
 */

typedef struct {
    AL_CONST unsigned char *buffer;
    png_uint_32 bufsize;
    png_uint_32 current_pos;
} MEMORY_READER_STATE;

static void read_data_memory(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    MEMORY_READER_STATE *f = (MEMORY_READER_STATE *)png_get_io_ptr(png_ptr);

    if (length > (f->bufsize - f->current_pos))
	png_error(png_ptr, "read error in read_data_memory (loadpng)");

    memcpy(data, f->buffer + f->current_pos, length);
    f->current_pos += length;
}



/* check_if_png_memory:
 *  Check if input buffer is really PNG format.
 */
static int check_if_png_memory(AL_CONST void *buffer)
{
    unsigned char *buf = (unsigned char *)buffer;
    return (png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK) == 0);
}



/* load_memory_png:
 *  Load a PNG file from memory, doing colour coversion if required.
 */
BITMAP *load_memory_png(AL_CONST void *buffer, int bufsize, RGB *pal)
{
    MEMORY_READER_STATE memory_reader_state;
    BITMAP *bmp;
    png_structp png_ptr;
    png_infop info_ptr;

    if (!buffer || (bufsize <= 0))
    	return NULL;

    if (!check_if_png_memory(buffer))
	return NULL;

    /* Create and initialize the png_struct with the desired error handler
     * functions.  If you want to use the default stderr and longjump method,
     * you can supply NULL for the last three parameters.  We also supply the
     * the compiler header file version, so that we know if the application
     * was compiled with a compatible version of the library.
     */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				     (void *)NULL, NULL, NULL);
    if (!png_ptr)
	return NULL;

    /* Allocate/initialize the memory for image information. */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
	return NULL;
    }

    /* Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
    if (setjmp(png_ptr->jmpbuf)) {
	/* Free all of the memory associated with the png_ptr and info_ptr */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	/* If we get here, we had a problem reading the file */
	return NULL;
    }

    /* Set up the reader state. */
    memory_reader_state.buffer = (unsigned char *)buffer;
    memory_reader_state.bufsize = bufsize;
    memory_reader_state.current_pos = PNG_BYTES_TO_CHECK;

    /* Tell libpng to use our custom reader. */
    png_set_read_fn(png_ptr, &memory_reader_state, (png_rw_ptr)read_data_memory);

    /* We have already read some of the signature. */
    png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

    /* Really load the image now. */
    bmp = really_load_png(png_ptr, info_ptr, pal);

    /* Clean up after the read, and free any memory allocated. */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    
    return bmp;
}
