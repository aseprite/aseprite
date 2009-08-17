#ifndef FILE_GIF_FORMAT_H_INCLUDED
#define FILE_GIF_FORMAT_H_INCLUDED

#include <allegro/color.h>

typedef struct GIF_ANIMATION GIF_ANIMATION;
typedef struct GIF_FRAME GIF_FRAME;
typedef struct GIF_PALETTE GIF_PALETTE;

struct GIF_PALETTE
{
    int colors_count;
    RGB colors[256];
};

struct GIF_ANIMATION
{
    int width, height;
    int frames_count;
    int background_index;
    int loop; /* -1 = no, 0 = forever, 1..65535 = that many times */
    GIF_PALETTE palette;
    GIF_FRAME *frames;
};

struct GIF_FRAME
{
    int w, h;
    unsigned char *bitmap_8_bit;
    GIF_PALETTE palette;
    int xoff, yoff;
    int duration;               /* in 1/100th seconds */
    int disposal_method;        /* 0 = don't care, 1 = keep, 2 = background, 3 = previous */
    int transparent_index;
};

GIF_ANIMATION *gif_create_animation(int frames_count);
void gif_destroy_animation(GIF_ANIMATION *gif);
int gif_save_animation(const char *filename, GIF_ANIMATION *gif,
		       void (*progress) (void *, float),
		       void *dp);
GIF_ANIMATION *gif_load_animation(const char *filename,
				  void (*progress) (void *, float),
				  void *dp);

#endif

