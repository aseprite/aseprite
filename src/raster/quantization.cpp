/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "effect/effect.h"
#include "effect/images_ref.h"

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/rgbmap.h"

using namespace gfx;

// Converts a RGB image to indexed with ordered dithering method.
static Image* ordered_dithering(const Image* src_image,
				int offsetx, int offsety,
				const RgbMap* rgbmap,
				const Palette* palette);

static int create_palette_from_bitmaps(Image **image, int nimage, RGB *pal, int *bmp_i, int fill_other);

Palette* quantization::create_palette_from_rgb(const Sprite* sprite)
{
  Palette* palette = new Palette(0, 256);
  Image* flat_image;
  Image** image_array;
  ImageRef* p;
  ImageRef* images;
  int c, nimage;

  images = images_ref_get_from_sprite(sprite, TARGET_ALL_LAYERS |
					      TARGET_ALL_FRAMES, false);
  if (images != NULL) {
    /* add a flat image with the current sprite's frame rendered */
    flat_image = image_new(sprite->getImgType(), sprite->getWidth(), sprite->getHeight());
    image_clear(flat_image, 0);
    sprite->render(flat_image, 0, 0);

    /* count images in the 'images' list */
    c = 0;
    for (p=images; p; p=p->next)
      c++;
    c++;			/* the 'flat_image' */

    /* create an array of images */
    nimage = c;
    image_array = (Image**)jmalloc(sizeof(Image*) * nimage);

    c = 0;
    for (p=images; p; p=p->next)
      image_array[c++] = p->image;
    image_array[c++] = flat_image; /* the 'flat_image' */
    
    /* generate the optimized palette */
    {
      PALETTE rgbpal;
      int *ibmp;

      for (c=0; c<256; c++)
	rgbpal[c].r = rgbpal[c].g = rgbpal[c].b = 255;

      ibmp = (int*)jmalloc(sizeof(int) * nimage);
      for (c=0; c<nimage; c++)
	ibmp[c] = 128;

      create_palette_from_bitmaps(image_array, nimage, rgbpal, ibmp, true);

      palette->fromAllegro(rgbpal);

      jfree(ibmp);
    }

    jfree(image_array);
    image_free(flat_image);
    images_ref_free(images);
  }

  return palette;
}

Image* quantization::convert_imgtype(const Image* image, int imgtype,
				     DitheringMethod ditheringMethod,
				     const RgbMap* rgbmap,
				     const Palette* palette)
{
  ase_uint32* rgb_address;
  ase_uint16* gray_address;
  ase_uint8* idx_address;
  ase_uint32 c;
  int i, r, g, b, size;
  Image *new_image;

  // no convertion
  if (image->imgtype == imgtype)
    return NULL;
  // RGB -> Indexed with ordered dithering
  else if (image->imgtype == IMAGE_RGB &&
	   imgtype == IMAGE_INDEXED &&
	   ditheringMethod == DITHERING_ORDERED) {
    return ordered_dithering(image, 0, 0, rgbmap, palette);
  }

  new_image = image_new(imgtype, image->w, image->h);
  if (!new_image)
    return NULL;

  size = image->w*image->h;

  switch (image->imgtype) {

    case IMAGE_RGB:
      rgb_address = (ase_uint32*)image->dat;

      switch (new_image->imgtype) {
	// RGB -> Grayscale
	case IMAGE_GRAYSCALE:
	  gray_address = (ase_uint16*)new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *rgb_address;

	    g = 255 * Hsv(Rgb(_rgba_getr(c),
	    		      _rgba_getg(c),
	    		      _rgba_getb(c))).valueInt() / 100;
	    *gray_address = _graya(g, _rgba_geta(c));

	    rgb_address++;
	    gray_address++;
	  }
	  break;
	// RGB -> Indexed
	case IMAGE_INDEXED:
	  idx_address = new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *rgb_address;
	    r = _rgba_getr(c);
	    g = _rgba_getg(c);
	    b = _rgba_getb(c);
	    if (_rgba_geta(c) == 0)
	      *idx_address = 0;
	    else
	      *idx_address = rgbmap->mapColor(r, g, b);
	    rgb_address++;
	    idx_address++;
	  }
	  break;
      }
      break;

    case IMAGE_GRAYSCALE:
      gray_address = (ase_uint16*)image->dat;

      switch (new_image->imgtype) {
	// Grayscale -> RGB
	case IMAGE_RGB:
	  rgb_address = (ase_uint32*)new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *gray_address;
	    g = _graya_getv(c);
	    *rgb_address = _rgba(g, g, g, _graya_geta(c));
	    gray_address++;
	    rgb_address++;
	  }
	  break;
	// Grayscale -> Indexed
	case IMAGE_INDEXED:
	  idx_address = new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *gray_address;
	    if (_graya_geta(c) == 0)
	      *idx_address = 0;
	    else
	      *idx_address = _graya_getv(c);
	    gray_address++;
	    idx_address++;
	  }
	  break;
      }
      break;

    case IMAGE_INDEXED:
      idx_address = image->dat;

      switch (new_image->imgtype) {
	// Indexed -> RGB
	case IMAGE_RGB:
	  rgb_address = (ase_uint32*)new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *idx_address;
	    if (c == 0)
	      *rgb_address = 0;
	    else
	      *rgb_address = _rgba(_rgba_getr(palette->getEntry(c)),
				   _rgba_getg(palette->getEntry(c)),
				   _rgba_getb(palette->getEntry(c)), 255);
	    idx_address++;
	    rgb_address++;
	  }
	  break;
	// Indexed -> Grayscale
	case IMAGE_GRAYSCALE:
	  gray_address = (ase_uint16*)new_image->dat;
	  for (i=0; i<size; i++) {
	    c = *idx_address;
	    if (c == 0)
	      *gray_address = 0;
	    else {
	      r = _rgba_getr(palette->getEntry(c));
	      g = _rgba_getg(palette->getEntry(c));
	      b = _rgba_getb(palette->getEntry(c));

	      g = 255 * Hsv(Rgb(r, g, b)).valueInt() / 100;
	      *gray_address = _graya(g, 255);
	    }
	    idx_address++;
	    gray_address++;
	  }
	  break;
      }
      break;
  }

  return new_image;
}

/* Based on Gary Oberbrunner: */
/*----------------------------------------------------------------------
 * Color image quantizer, from Paul Heckbert's paper in
 * Computer Graphics, vol.16 #3, July 1982 (Siggraph proceedings),
 * pp. 297-304.
 * By Gary Oberbrunner, copyright c. 1988.
 *----------------------------------------------------------------------
 */

/* Bayer-method ordered dither.	 The array line[] contains the
 * intensity values for the line being processed.  As you can see, the
 * ordered dither is much simpler than the error dispersion dither.
 * It is also many times faster, but it is not as accurate and
 * produces cross-hatch * patterns on the output.
 */

static int pattern[8][8] = {
  {  0, 32,  8, 40,  2, 34, 10, 42 }, /* 8x8 Bayer ordered dithering  */
  { 48, 16, 56, 24, 50, 18, 58, 26 }, /* pattern.  Each input pixel   */
  { 12, 44,  4, 36, 14, 46,  6, 38 }, /* is scaled to the 0..63 range */
  { 60, 28, 52, 20, 62, 30, 54, 22 }, /* before looking in this table */
  {  3, 35, 11, 43,  1, 33,  9, 41 }, /* to determine the action.     */
  { 51, 19, 59, 27, 49, 17, 57, 25 },
  { 15, 47,  7, 39, 13, 45,  5, 37 },
  { 63, 31, 55, 23, 61, 29, 53, 21 }
};

#define DIST(r1,g1,b1,r2,g2,b2) (3 * ((r1)-(r2)) * ((r1)-(r2)) +	\
				 4 * ((g1)-(g2)) * ((g1)-(g2)) +	\
				 2 * ((b1)-(b2)) * ((b1)-(b2)))

static Image* ordered_dithering(const Image* src_image,
				int offsetx, int offsety,
				const RgbMap* rgbmap,
				const Palette* palette)
{
  int oppr, oppg, oppb, oppnrcm;
  Image *dst_image;
  int dither_const;
  int nr, ng, nb;
  int r, g, b, a;
  int nearestcm;
  int c, x, y;

  dst_image = image_new(IMAGE_INDEXED, src_image->w, src_image->h);
  if (!dst_image)
    return NULL;

  for (y=0; y<src_image->h; y++) {
    for (x=0; x<src_image->w; x++) {
      c = image_getpixel_fast<RgbTraits>(src_image, x, y);

      r = _rgba_getr(c);
      g = _rgba_getg(c);
      b = _rgba_getb(c);
      a = _rgba_geta(c);

      if (a != 0) {
	nearestcm = rgbmap->mapColor(r, g, b);
	/* rgb values for nearest color */
	nr = _rgba_getr(palette->getEntry(nearestcm));
	ng = _rgba_getg(palette->getEntry(nearestcm));
	nb = _rgba_getb(palette->getEntry(nearestcm));
	/* Color as far from rgb as nrngnb but in the other direction */
	oppr = MID(0, 2*r - nr, 255);
	oppg = MID(0, 2*g - ng, 255);
	oppb = MID(0, 2*b - nb, 255);
	/* Nearest match for opposite color: */
	oppnrcm = rgbmap->mapColor(oppr, oppg, oppb);
	/* If they're not the same, dither between them. */
	/* Dither constant is measured by where the true
	   color lies between the two nearest approximations.
	   Since the most nearly opposite color is not necessarily
	   on the line from the nearest through the true color,
	   some triangulation error can be introduced.	In the worst
	   case the r-nr distance can actually be less than the nr-oppr
	   distance. */
	if (oppnrcm != nearestcm) {
	  oppr = _rgba_getr(palette->getEntry(oppnrcm));
	  oppg = _rgba_getg(palette->getEntry(oppnrcm));
	  oppb = _rgba_getb(palette->getEntry(oppnrcm));

	  dither_const = DIST(nr, ng, nb, oppr, oppg, oppb);
	  if (dither_const != 0) {
	    dither_const = 64 * DIST(r, g, b, nr, ng, nb) / dither_const;
	    dither_const = MIN(63, dither_const);

	    if (pattern[(x+offsetx) & 7][(y+offsety) & 7] < dither_const)
	      nearestcm = oppnrcm;
	  }
	}
      }
      else
	nearestcm = 0;

      image_putpixel_fast<IndexedTraits>(dst_image, x, y, nearestcm);
    }
  }

  return dst_image;
}

/* quantize.c
 * Copyright (C) 2000-2002 by Ben "entheh" Davis
 *
 * Nobody can use these routines without the explicit
 * permission of Ben Davis: entheh@users.sourceforge.net
 *
 * Adapted to ASE by David A. Capello.
 *
 * See "LEGAL.txt" for details.
/ */

/* HACK ALERT: We use 'filler' to represent whether a colour in the palette
 * is reserved. This is only tested when the .r element can no longer
 * indicate whether an entry is reserved, i.e. when converting bitmaps.
 * NOTE: When converting bitmaps, they will not use reserved colours.
 */

#define TREE_DEPTH 2
/* TREE_DEPTH should be	 a power of 2.	The lower it is,  the deeper the tree
 * will go.  This will not usually affect  the accuracy of colours generated,
 * but with images with	 very subtle changes of colour,	 the variation can be
 * lost with higher values of TREE_DEPTH.
 *
 * As these trees go deeper they use an extortionate amount of memory.	If it
 * runs out, you have no choice but to increase the value of TREE_DEPTH.
 */

/* create_palette_from_bitmaps:
 * generates an	 optimised palette  for the  list of
 * bitmaps specified. All bitmaps must be true-colour.	The number of bitmaps
 * must be passed in n_bmp, and bmp must point to an array of pointers to
 * BITMAP  structures.	bmp_i  should  point  to  an array  parallel  to bmp,
 * containing importance values for the bitmaps.  pal must point to a PALETTE
 * structure which will be filled with the optimised palette.
 *
 * pal will be scanned for predefined colours. Any entry where the .r element
 * is 255 will be considered a free entry. All others are taken as predefined
 * colours.  Predefined colours	 will not  be changed,	and the	 rest  of the
 * palette will be unaffected by them. There may be copies of these colours.
 *
 * If in the bitmaps this routine finds any occurrence of the colour to which
 * palette entry 0 is set,  it will be treated as  a transparency colour.  It
 * will not be considered when generating the palette.
 *
 * Under some  circumstances  there will  be palette  entries  left over.  If
 * fill_other != 0, they will be filled with black.  If fill_other == 0, they
 * will be left containing whatever values they contained before.
 *
 * This function  does not  convert the	 bitmaps to  256-colour	 format.  The
 * conversion must be done afterwards by the main program.
 */

typedef struct PALETTE_NODE
{
  unsigned int rl,gl,bl,rh,gh,bh;
  unsigned int n1,n2,Sr,Sg,Sb,E;
  struct PALETTE_NODE *parent;
  struct PALETTE_NODE *subnode[2][2][2];
} PALETTE_NODE;

static PALETTE_NODE *rgb_node[64][64][64];

static PALETTE_NODE *create_node(unsigned int rl,
				 unsigned int gl,
				 unsigned int bl,
				 unsigned int rh,
				 unsigned int gh,
				 unsigned int bh,PALETTE_NODE *parent)
{
  PALETTE_NODE *node;
  unsigned int rm,gm,bm;
  node=(PALETTE_NODE*)jmalloc(sizeof(PALETTE_NODE));
  if (!node)
    return NULL;
  node->rl=rl;
  node->gl=gl;
  node->bl=bl;
  node->rh=rh;
  node->gh=gh;
  node->bh=bh;
  node->E=node->Sb=node->Sg=node->Sr=node->n2=node->n1=0;
  node->parent=parent;
  if (rh-rl>TREE_DEPTH) {
    rm=(rl+rh)>>1;
    gm=(gl+gh)>>1;
    bm=(bl+bh)>>1;
    node->subnode[0][0][0]=create_node(rl,gl,bl,rm,gm,bm,node);
    node->subnode[0][0][1]=create_node(rm,gl,bl,rh,gm,bm,node);
    node->subnode[0][1][0]=create_node(rl,gm,bl,rm,gh,bm,node);
    node->subnode[0][1][1]=create_node(rm,gm,bl,rh,gh,bm,node);
    node->subnode[1][0][0]=create_node(rl,gl,bm,rm,gm,bh,node);
    node->subnode[1][0][1]=create_node(rm,gl,bm,rh,gm,bh,node);
    node->subnode[1][1][0]=create_node(rl,gm,bm,rm,gh,bh,node);
    node->subnode[1][1][1]=create_node(rm,gm,bm,rh,gh,bh,node);
  } else {
    for (bm=bl;bm<bh;bm++) {
      for (gm=gl;gm<gh;gm++) {
	for (rm=rl;rm<rh;rm++) {
	  rgb_node[bm][gm][rm]=node;
	}
      }
    }
    node->subnode[1][1][1]=
      node->subnode[1][1][0]=
      node->subnode[1][0][1]=
      node->subnode[1][0][0]=
      node->subnode[0][1][1]=
      node->subnode[0][1][0]=
      node->subnode[0][0][1]=
      node->subnode[0][0][0]=0;
  }
  return node;
}

static PALETTE_NODE *collapse_empty(PALETTE_NODE *node,unsigned int *n_colours)
{
  unsigned int b,g,r;
  for (b=0;b<2;b++) {
    for (g=0;g<2;g++) {
      for (r=0;r<2;r++) {
	if (node->subnode[b][g][r])
	  node->subnode[b][g][r]=collapse_empty(node->subnode[b][g][r],n_colours);
      }
    }
  }
  if (node->n1==0) {
    jfree(node);
    node=0;
  } else if (node->n2) (*n_colours)++;
  return node;
}

static PALETTE_NODE *collapse_nodes(PALETTE_NODE *node,unsigned int *n_colours,
				    unsigned int n_entries,unsigned int Ep)
{
  unsigned int b,g,r;
  if (node->E<=Ep) {
    for (b=0;b<2;b++) {
      for (g=0;g<2;g++) {
	for (r=0;r<2;r++) {
	  if (node->subnode[b][g][r]) {
	    node->subnode[b][g][r]=collapse_nodes(node->subnode[b][g][r],
						  n_colours,n_entries,0);
	    if (*n_colours<=n_entries) return node;
	  }
	}
      }
    }
    if (node->parent->n2) (*n_colours)--;
    node->parent->n2+=node->n2;
    node->parent->Sr+=node->Sr;
    node->parent->Sg+=node->Sg;
    node->parent->Sb+=node->Sb;
    jfree(node);
    node=0;
  } else {
    for (b=0;b<2;b++) {
      for (g=0;g<2;g++) {
	for (r=0;r<2;r++) {
	  if (node->subnode[b][g][r]) {
	    node->subnode[b][g][r]=collapse_nodes(node->subnode[b][g][r],
						  n_colours,n_entries,Ep);
	    if (*n_colours<=n_entries) return node;
	  }
	}
      }
    }
  }
  return node;
}

static unsigned int distance_squared(int r,int g,int b)
{
  return r*r+g*g+b*b;
}

static void minimum_Ep(PALETTE_NODE *node,unsigned int *Ep)
{
  unsigned int r,g,b;
  if (node->E<*Ep) *Ep=node->E;
  for (b=0;b<2;b++) {
    for (g=0;g<2;g++) {
      for (r=0;r<2;r++) {
	if (node->subnode[b][g][r]) minimum_Ep(node->subnode[b][g][r],Ep);
      }
    }
  }
}

static void fill_palette(PALETTE_NODE *node,unsigned int *c,RGB *pal,int depth)
{
  unsigned int r,g,b;
  if (node->n2) {
    for (;pal[*c].r!=255;(*c)++);
    pal[*c].r=node->Sr/node->n2;
    pal[*c].g=node->Sg/node->n2;
    pal[*c].b=node->Sb/node->n2;
    (*c)++;
  }
  for (b=0;b<2;b++) {
    for (g=0;g<2;g++) {
      for (r=0;r<2;r++) {
	if (node->subnode[b][g][r])
	  fill_palette(node->subnode[b][g][r],c,pal,depth+1);
      }
    }
  }
}

static void destroy_tree(PALETTE_NODE *tree)
{
  unsigned int r,g,b;
  for (b=0;b<2;b++) {
    for (g=0;g<2;g++) {
      for (r=0;r<2;r++) {
	if (tree->subnode[b][g][r]) destroy_tree(tree->subnode[b][g][r]);
      }
    }
  }
  jfree(tree);
}

static int create_palette_from_bitmaps(Image **image, int nimage, RGB *pal, int *bmp_i, int fill_other)
{
  int c_bmp,x,y,r,g,b;
  unsigned int n_colours=0;
  unsigned int n_entries=0;
  unsigned int c, Ep;
  PALETTE_NODE *tree,*node;

  /* only support RGB bitmaps */
  if ((nimage < 1) || (image[0]->imgtype != IMAGE_RGB))
    return 0;

  /*Create the tree structure*/
  tree=create_node(0,0,0,64,64,64,0);
  /*Scan the bitmaps*/
  /*  add_progress(nimage+1); */
  for (c_bmp=0;c_bmp<nimage;c_bmp++) {
    /*	  add_progress(image[c_bmp]->h); */
    for (y=0;y<image[c_bmp]->h;y++) {
      for (x=0;x<image[c_bmp]->w;x++) {
	c=image[c_bmp]->getpixel(x,y);
	r=_rgba_getr(c)>>2;
	g=_rgba_getg(c)>>2;
	b=_rgba_getb(c)>>2;
	node=rgb_node[b][g][r];
	node->n2+=bmp_i[c_bmp];
	node->Sr+=r*bmp_i[c_bmp];
	node->Sg+=g*bmp_i[c_bmp];
	node->Sb+=b*bmp_i[c_bmp];
	do {
	  node->n1+=bmp_i[c_bmp];
	  node->E+=distance_squared((r<<1)-node->rl-node->rh,
				    (g<<1)-node->gl-node->gh,
				    (b<<1)-node->bl-node->bh)*bmp_i[c_bmp];
	  node=node->parent;
	} while (node);
      }
      /*     do_progress(y); */
    }
    /*	  del_progress(); */
    /*	 do_progress(c_bmp); */
  }
  /*Collapse empty nodes in the tree, and count leaves*/
  tree=collapse_empty(tree,&n_colours);
  /*Count free palette entries*/
  for (c=0;c<256;c++) {
    if (pal[c].r==255) n_entries++;
  }
  /*Collapse nodes until there are few enough to fit in the palette*/
  if (n_colours > n_entries) {
    /*	 int n_colours1 = n_colours; */
    /*	 add_progress(n_colours1 - n_entries); */
    while (n_colours>n_entries) {
      Ep=0xFFFFFFFFul;
      minimum_Ep(tree,&Ep);
      tree=collapse_nodes(tree,&n_colours,n_entries,Ep);

      /*    if (n_colours > n_entries) */
      /*     do_progress(n_colours1 - n_colours); */
    }
    /*	 del_progress(); */
  }
  /*  del_progress(); */
  /* fill palette */
  c=0;
  fill_palette(tree,&c,pal,1);
  if (fill_other) {
    for (;c<256;c++) {
      if (pal[c].r==255)
	pal[c].b=pal[c].g=pal[c].r=0;
    }
  }
  /* free memory used by tree */
  destroy_tree(tree);
  return n_colours;
}
