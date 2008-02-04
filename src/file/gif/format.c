#include <allegro.h>

#include "format.h"
#include "lzw.h"

#include <stdlib.h>
#include <string.h>

GIF_ANIMATION *
gif_create_animation(int frames_count)
{
    GIF_ANIMATION *gif = calloc (1, sizeof *gif);
    /* Create frames. */
    gif->frames_count = frames_count;
    gif->frames = calloc (gif->frames_count, sizeof *gif->frames);
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
write_palette (PACKFILE *file, GIF_PALETTE *palette, int bits)
{
    int i;
    for (i = 0; i < (1 << bits); i++)
    {
        pack_putc (palette->colors[i].r, file);
        pack_putc (palette->colors[i].g, file);
        pack_putc (palette->colors[i].b, file);
    }
}

static void
read_palette (PACKFILE * file, GIF_PALETTE *palette)
{
    int i;

    for (i = 0; i < palette->colors_count; i++)
    {
        palette->colors[i].r = pack_getc (file);
        palette->colors[i].g = pack_getc (file);
        palette->colors[i].b = pack_getc (file);
    }
}



static int lzw_read_pixel (int pos, unsigned char *data)
{
    unsigned char *bitmap = data;
    return bitmap[pos];
}



static void lzw_write_pixel (int pos, int c, unsigned char *data)
{
    unsigned char *bitmap = data;
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
    PACKFILE *file;

    file = pack_fopen (filename, "w");
    if (!file)
        return -1;

    pack_fwrite ("GIF89a", 6, file);
    pack_iputw (gif->width, file);
    pack_iputw (gif->height, file);
    /* 7 global palette
     * 456 color richness
     * 3 sorted
     * 012 palette bits
     */
    for (i = 1, j = 0; i < gif->palette.colors_count; i *= 2, j++);
    pack_putc ((j ? 128 : 0) + 64 + 32 + 16 + (j ? j - 1 : 0), file);
    pack_putc (gif->background_index, file);
    pack_putc (0, file);        /* No aspect ratio. */

    if (j)
        write_palette (file, &gif->palette, j);

    if (gif->loop != -1)
    /* Loop count extension. */
    {
        pack_putc (0x21, file); /* Extension Introducer. */
        pack_putc (0xff, file); /* Application Extension. */
        pack_putc (11, file);    /* Size. */
        pack_fwrite ("NETSCAPE2.0", 11, file);
        pack_putc (3, file); /* Size. */
        pack_putc (1, file);
        pack_iputw (gif->loop, file);
        pack_putc (0, file);
    }

    progress(dp, 0.0f);
    for (frame = 0; frame < gif->frames_count; frame++)
    {
        int w = gif->frames[frame].w;
        int h = gif->frames[frame].h;

        pack_putc (0x21, file); /* Extension Introducer. */
        pack_putc (0xf9, file); /* Graphic Control Extension. */
        pack_putc (4, file);    /* Size. */
        /* Disposal method, and enable transparency. */
        i = gif->frames[frame].disposal_method << 2;
        if (gif->frames[frame].transparent_index != -1)
            i |= 1;
        pack_putc (i, file);
        pack_iputw (gif->frames[frame].duration, file); /* In 1/100th seconds. */
        if (gif->frames[frame].transparent_index != -1)
            pack_putc (gif->frames[frame].transparent_index, file);       /* Transparent color index. */
        else
             pack_putc (0, file);
        pack_putc (0x00, file); /* Terminator. */

        pack_putc (0x2c, file); /* Image Descriptor. */
        pack_iputw (gif->frames[frame].xoff, file);
        pack_iputw (gif->frames[frame].yoff, file);
        pack_iputw (w, file);
        pack_iputw (h, file);

         /* 7: local palette
         * 6: interlaced
         * 5: sorted
         * 012: palette bits
         */

        for (i = 1, j = 0; i < gif->frames[frame].palette.colors_count; i *= 2, j++);
        pack_putc ((j ? 128 : 0) + (j ? j - 1 : 0), file);

        if (j)
            write_palette (file, &gif->frames[frame].palette, j);

        LZW_encode (file, lzw_read_pixel, w * h,
            gif->frames[frame].bitmap_8_bit);

        pack_putc (0x00, file); /* Terminator. */

	progress(dp, (float)frame / (float)gif->frames_count);
    }
    progress(dp, 1.0f);

    pack_putc (0x3b, file);     /* Trailer. */

    pack_fclose (file);
    return 0;
}

static void
deinterlace (unsigned char *bmp, int w, int h)
{
    unsigned char *temp = malloc(w * h);
    int y, i = 0;
    for (y = 0; y < h; y += 8)
    {
        memcpy (temp, bmp + i++ * w, w);
    }
    for (y = 4; y < h; y += 8)
    {
        memcpy (temp, bmp + i++ * w, w);
    }
    for (y = 2; y < h; y += 4)
    {
        memcpy (temp, bmp + i++ * w, w);
    }
    for (y = 1; y < h; y += 2)
    {
        memcpy (temp, bmp + i++ * w, w); 
    }
    memcpy (bmp, temp, w * h);
    free(temp);
}

static GIF_ANIMATION *
load_object (PACKFILE * file, long size, void (*progress) (void *, float), void *dp)
{
    int version;
    unsigned char *bmp = NULL;
    int i, j;
    GIF_ANIMATION *gif = calloc (1, sizeof *gif);
    GIF_FRAME frame;
    int have_global_palette = 0;

    gif->frames_count = 0;

    /* is it really a GIF? */
    if (pack_getc (file) != 'G')
        goto error;
    if (pack_getc (file) != 'I')
        goto error;
    if (pack_getc (file) != 'F')
        goto error;
    if (pack_getc (file) != '8')
        goto error;
    /* '7' or '9', for 87a or 89a. */
    version = pack_getc (file);
    if (version != '7' && version != '9')
        goto error;
    if (pack_getc (file) != 'a')
        goto error;

    gif->width = pack_igetw (file);
    gif->height = pack_igetw (file);
    i = pack_getc (file);
    /* Global color table? */
    if (i & 128)
        gif->palette.colors_count = 1 << ((i & 7) + 1);
    else
        gif->palette.colors_count = 0;
    /* Background color is only valid with a global palette. */
    gif->background_index = pack_getc (file);

    /* Skip aspect ratio. */
    pack_fseek (file, 1);

    if (gif->palette.colors_count)
    {
        read_palette (file, &gif->palette);
        have_global_palette = 1;
    }

    progress(dp, 0.0f);
    do
    {
        i = pack_getc (file);
	progress(dp, (float)i / (float)size);

        switch (i)
        {
            case 0x2c:         /* Image Descriptor */
            {
                int w, h;
                int interlaced = 0;

                frame.xoff = pack_igetw (file);
                frame.yoff = pack_igetw (file);
                w = pack_igetw (file);
                h = pack_igetw (file);
                bmp = calloc (w, h);
                if (!bmp)
                    goto error;
                frame.w = w;
                frame.h = h;
                i = pack_getc (file);

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

                if (LZW_decode (file, lzw_write_pixel, bmp))
                    goto error;

                if (interlaced)
                    deinterlace (bmp, w, h);

                frame.bitmap_8_bit = bmp;
                bmp = NULL;

                gif->frames_count++;
                gif->frames =
                    realloc (gif->frames,
                             gif->frames_count * sizeof *gif->frames);
                gif->frames[gif->frames_count - 1] = frame;
                break;
            }
            case 0x21: /* Extension Introducer. */
                j = pack_getc (file); /* Extension Type. */
                i = pack_getc (file); /* Size. */
                if (j == 0xf9) /* Graphic Control Extension. */
                {
                    /* size must be 4 */
                    if (i != 4)
                        goto error;
                    i = pack_getc (file);
                    frame.disposal_method = (i >> 2) & 7;
                    frame.duration = pack_igetw (file);
                    if (i & 1)  /* Transparency? */
                    {
                        frame.transparent_index = pack_getc (file);
                    }
                    else
                    {
                        pack_fseek (file, 1); 
                        frame.transparent_index = -1;
                    }
                    i = pack_getc (file); /* Size. */
                }
                /* Application Extension. */
                else if (j == 0xff)
                {
                    if (i == 11)
                    {
                        char name[12];
                        pack_fread (name, 11, file);
                        i = pack_getc (file); /* Size. */
                        name[11] = '\0';
                        if (!strcmp (name, "NETSCAPE2.0"))
                        {
                            if (i == 3)
                            {
                                j = pack_getc (file);
                                gif->loop = pack_igetw (file);
                                if (j != 1)
                                    gif->loop = 0;
                                i = pack_getc (file); /* Size. */
                            }
                        }
                    }
                }

                /* Possibly more blocks until terminator block (0). */
                while (i)
                {
                    pack_fseek (file, i);
                    i = pack_getc (file);
                }
                break;
            case 0x3b:
                /* GIF Trailer. */
                pack_fclose (file);
		progress(dp, 1.0f);
                return gif;
        }
    }
    while (TRUE);
  error:
    if (file)
        pack_fclose (file);
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
GIF_ANIMATION *
gif_load_animation (const char *filename, void (*progress) (void *, float), void *dp)
{
    PACKFILE *file;
    GIF_ANIMATION *gif = NULL;
#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,		\
					   ALLEGRO_SUB_VERSION,		\
					   ALLEGRO_WIP_VERSION))
    int size = file_size(filename);
#else
    int size = file_size_ex(filename);
#endif

    file = pack_fopen (filename, "r");
    if (file)
      gif = load_object (file, size, progress, dp);
    return gif;
}

/* static int */
/* get_rgbcolor (RGB *rgb) */
/* { */
/*     return makecol (rgb->r, rgb->g, rgb->b); */
/* } */

