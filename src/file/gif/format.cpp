#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <allegro.h>

#include "file/gif/format.h"
#include "file/gif/lzw.h"
#include "file/file.h"

GIF_ANIMATION* gif_create_animation(int frames_count)
{
  GIF_ANIMATION* gif = reinterpret_cast<GIF_ANIMATION*>(calloc(1, sizeof *gif));
  /* Create frames. */
  gif->frames_count = frames_count;
  gif->frames = reinterpret_cast<GIF_FRAME*>(calloc(gif->frames_count, sizeof *gif->frames));
  return gif;
}

/* Destroy a complete gif, including all frames. */
void
gif_destroy_animation (GIF_ANIMATION *gif)
{
    int i;

    for (i = 0; i < gif->frames_count; i++)
    {
	GIF_FRAME *frame = gif->frames + i;

	if (frame->bitmap_8_bit)
	    free (frame->bitmap_8_bit);
    }
    free (gif->frames);
    free (gif);
}

static void
write_palette(FILE* file, GIF_PALETTE* palette, int bits)
{
    int i;
    for (i = 0; i < (1 << bits); i++)
    {
	fputc (palette->colors[i].r, file);
	fputc (palette->colors[i].g, file);
	fputc (palette->colors[i].b, file);
    }
}

static void
read_palette (FILE * file, GIF_PALETTE *palette)
{
    int i;

    for (i = 0; i < palette->colors_count; i++)
    {
	palette->colors[i].r = fgetc (file);
	palette->colors[i].g = fgetc (file);
	palette->colors[i].b = fgetc (file);
    }
}



static int lzw_read_pixel (int pos, unsigned char *data)
{
    unsigned char *bitmap = data;
    ASSERT(pos >= 0);
    return bitmap[pos];
}



static void lzw_write_pixel (int pos, int c, unsigned char *data)
{
    unsigned char *bitmap = data;
    ASSERT(pos >= 0);
    ASSERT(c >= 0 && c <= 255);
    bitmap[pos] = c;
}



/*
 * Compresses all the frames in the animation and writes to a gif file.
 * Nothing else like extensions or comments is written.
 *
 * Returns 0 on success.
 *
 * Note: All bitmaps must have a color depth of 8.
 */
int
gif_save_animation (const char *filename, GIF_ANIMATION *gif,
		    void (*progress) (void *, float),
		    void *dp)
{
    int frame;
    int i, j;
    FILE *file;

    file = fopen(filename, "wb");
    if (!file)
	return -1;

    fwrite ("GIF89a", 1, 6, file);
    fputw (gif->width, file);
    fputw (gif->height, file);
    /* 7 global palette
     * 456 color richness
     * 3 sorted
     * 012 palette bits
     */
    for (i = 1, j = 0; i < gif->palette.colors_count; i *= 2, j++);
    fputc ((j ? 128 : 0) + 64 + 32 + 16 + (j ? j - 1 : 0), file);
    fputc (gif->background_index, file);
    fputc (0, file);        /* No aspect ratio. */

    if (j)
	write_palette (file, &gif->palette, j);

    if (gif->loop != -1)
    /* Loop count extension. */
    {
	fputc (0x21, file); /* Extension Introducer. */
	fputc (0xff, file); /* Application Extension. */
	fputc (11, file);    /* Size. */
	fwrite ("NETSCAPE2.0", 1, 11, file);
	fputc (3, file); /* Size. */
	fputc (1, file);
	fputw (gif->loop, file);
	fputc (0, file);
    }

    progress(dp, 0.0f);
    for (frame = 0; frame < gif->frames_count; frame++)
    {
	int w = gif->frames[frame].w;
	int h = gif->frames[frame].h;

	fputc (0x21, file); /* Extension Introducer. */
	fputc (0xf9, file); /* Graphic Control Extension. */
	fputc (4, file);    /* Size. */
	/* Disposal method, and enable transparency. */
	i = gif->frames[frame].disposal_method << 2;
	if (gif->frames[frame].transparent_index != -1)
	    i |= 1;
	fputc (i, file);
	fputw (gif->frames[frame].duration, file); /* In 1/100th seconds. */
	if (gif->frames[frame].transparent_index != -1)
	    fputc (gif->frames[frame].transparent_index, file);       /* Transparent color index. */
	else
	     fputc (0, file);
	fputc (0x00, file); /* Terminator. */

	fputc (0x2c, file); /* Image Descriptor. */
	fputw (gif->frames[frame].xoff, file);
	fputw (gif->frames[frame].yoff, file);
	fputw (w, file);
	fputw (h, file);

	 /* 7: local palette
	 * 6: interlaced
	 * 5: sorted
	 * 012: palette bits
	 */

	for (i = 1, j = 0; i < gif->frames[frame].palette.colors_count; i *= 2, j++);
	fputc ((j ? 128 : 0) + (j ? j - 1 : 0), file);

	if (j)
	    write_palette (file, &gif->frames[frame].palette, j);

	LZW_encode (file, lzw_read_pixel, w * h,
	    gif->frames[frame].bitmap_8_bit);

	fputc (0x00, file); /* Terminator. */

	progress(dp, (float)frame / (float)gif->frames_count);
    }
    progress(dp, 1.0f);

    fputc (0x3b, file);     /* Trailer. */

    fclose (file);
    return 0;
}

static void
deinterlace(unsigned char* bmp, int w, int h)
{
  unsigned char* temp = (unsigned char*)malloc(w * h);
  int y, i = 0;
  for (y = 0; y < h; y += 8) {
    memcpy(temp, bmp + i++ * w, w);
  }
  for (y = 4; y < h; y += 8) {
    memcpy(temp, bmp + i++ * w, w);
  }
  for (y = 2; y < h; y += 4) {
    memcpy(temp, bmp + i++ * w, w);
  }
  for (y = 1; y < h; y += 2) {
    memcpy(temp, bmp + i++ * w, w);
  }
  memcpy(bmp, temp, w * h);
  free(temp);
}

static GIF_ANIMATION*
load_object(FILE* file, long size, void(*progress)(void*, float), void* dp)
{
    int version;
    unsigned char* bmp = NULL;
    int i, j;
    GIF_ANIMATION* gif = reinterpret_cast<GIF_ANIMATION*>(calloc(1, sizeof *gif));
    GIF_FRAME frame;
    int have_global_palette = 0;

    gif->frames_count = 0;

    /* is it really a GIF? */
    if (fgetc (file) != 'G')
	goto error;
    if (fgetc (file) != 'I')
	goto error;
    if (fgetc (file) != 'F')
	goto error;
    if (fgetc (file) != '8')
	goto error;
    /* '7' or '9', for 87a or 89a. */
    version = fgetc (file);
    if (version != '7' && version != '9')
	goto error;
    if (fgetc (file) != 'a')
	goto error;

    gif->width = fgetw (file);
    gif->height = fgetw (file);
    i = fgetc (file);
    /* Global color table? */
    if (i & 128)
	gif->palette.colors_count = 1 << ((i & 7) + 1);
    else
	gif->palette.colors_count = 0;
    /* Background color is only valid with a global palette. */
    gif->background_index = fgetc (file);

    /* Skip aspect ratio. */
    fseek (file, 1, SEEK_CUR);

    if (gif->palette.colors_count)
    {
	read_palette (file, &gif->palette);
	have_global_palette = 1;
    }

    progress(dp, 0.0f);
    do
    {
	i = fgetc (file);
	progress(dp, (float)i / (float)size);

	switch (i)
	{
	    case 0x2c:         /* Image Descriptor */
	    {
		int w, h;
		int interlaced = 0;

		frame.xoff = fgetw (file);
		frame.yoff = fgetw (file);
		w = fgetw (file);
		h = fgetw (file);
		if (w < 1 || h < 1)
		    goto error;
		bmp = reinterpret_cast<unsigned char*>(calloc(w, h));
		if (!bmp)
		    goto error;
		frame.w = w;
		frame.h = h;
		i = fgetc (file);

		/* Local palette. */
		if (i & 128)
		{
		    frame.palette.colors_count = 1 << ((i & 7) + 1);
		    read_palette (file, &frame.palette);
		}
		else
		{
		    frame.palette.colors_count = 0;
		}

		if (i & 64)
		    interlaced = 1;

		if (ferror (file) ||
		    LZW_decode (file, lzw_write_pixel, bmp))
		    goto error;

		if (interlaced)
		    deinterlace (bmp, w, h);

		frame.bitmap_8_bit = bmp;
		bmp = NULL;

		gif->frames_count++;
		gif->frames = reinterpret_cast<GIF_FRAME*>
		  (realloc(gif->frames,
			   gif->frames_count * sizeof *gif->frames));
		gif->frames[gif->frames_count - 1] = frame;
		break;
	    }
	    case 0x21: /* Extension Introducer. */
		j = fgetc (file); /* Extension Type. */
		i = fgetc (file); /* Size. */
		if (j == 0xf9) /* Graphic Control Extension. */
		{
		    /* size must be 4 */
		    if (i != 4)
			goto error;
		    i = fgetc (file);
		    frame.disposal_method = (i >> 2) & 7;
		    frame.duration = fgetw (file);
		    if (i & 1)  /* Transparency? */
		    {
			frame.transparent_index = fgetc (file);
		    }
		    else
		    {
			fseek (file, 1, SEEK_CUR);
			frame.transparent_index = -1;
		    }
		    i = fgetc (file); /* Size. */
		}
		/* Application Extension. */
		else if (j == 0xff)
		{
		    if (i == 11)
		    {
			char name[12];
			fread (name, 1, 11, file);
			i = fgetc (file); /* Size. */
			name[11] = '\0';
			if (!strcmp (name, "NETSCAPE2.0"))
			{
			    if (i == 3)
			    {
				j = fgetc (file);
				gif->loop = fgetw (file);
				if (j != 1)
				    gif->loop = 0;
				i = fgetc (file); /* Size. */
			    }
			}
		    }
		}

		/* Possibly more blocks until terminator block (0). */
		while (i)
		{
		    fseek (file, i, SEEK_CUR);
		    i = fgetc (file);
		}
		break;
	    case 0x3b:
		/* GIF Trailer. */
		fclose (file);
		progress(dp, 1.0f);
		return gif;
	}
    }
    while (true);
  error:
    if (file)
	fclose (file);
    if (gif)
	gif_destroy_animation (gif);
    if (bmp)
	free (bmp);
    return NULL;
}

/*
 * Allocates and reads a GIF_ANIMATION structure, filling in all the
 * frames found in the file. On error, nothing is allocated, and NULL is
 * returned. No extensions or comments are read in. If the gif contains
 * a transparency index, and it is no 0, it is swapped with 0 - so index
 * 0 will be the transparent color. There is no way to know when a file
 * contained no transparency originally. Frame duration is specified in
 * 1/100th seconds.
 *
 * All bitmaps will have a color depth of 8.
 */
GIF_ANIMATION*
gif_load_animation(const char* filename, void (*progress)(void*, float), void* dp)
{
  FILE* file;
  GIF_ANIMATION* gif = NULL;
#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,		\
					   ALLEGRO_SUB_VERSION,		\
					   ALLEGRO_WIP_VERSION))
  int size = file_size(filename);
#else
  int size = file_size_ex(filename);
#endif

  file = fopen(filename, "rb");
  if (file)
    gif = load_object(file, size, progress, dp);
  return gif;
}

/* static int */
/* get_rgbcolor (RGB *rgb) */
/* { */
/*     return makecol (rgb->r, rgb->g, rgb->b); */
/* } */
