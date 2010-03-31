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

#include <allegro/color.h>

#include "console.h"
#include "effect/effect.h"
#include "effect/images_ref.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "util/quantize.h"

static int quantize_bitmaps(Image **image, int nimage, RGB *pal, int *bmp_i, int fill_other);

void sprite_quantize(Sprite *sprite)
{
  Palette* palette = new Palette(0, 256); // TODO sprite_quantize_ex should change the number of colors of this palette

  sprite_quantize_ex(sprite, palette);

  // Just one palette
  sprite->resetPalettes();
  sprite->setPalette(palette, false);

  delete palette;
}

void sprite_quantize_ex(Sprite *sprite, Palette *palette)
{
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

      quantize_bitmaps(image_array, nimage, rgbpal, ibmp, true);

      palette->fromAllegro(rgbpal);

      jfree(ibmp);
    }

    jfree(image_array);
    image_free(flat_image);
    images_ref_free(images);
  }
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
/* TREE_DEPTH should be  a power of 2.  The lower it is,  the deeper the tree
 * will go.  This will not usually affect  the accuracy of colours generated,
 * but with images with  very subtle changes of colour,  the variation can be
 * lost with higher values of TREE_DEPTH.
 *
 * As these trees go deeper they use an extortionate amount of memory.  If it
 * runs out, you have no choice but to increase the value of TREE_DEPTH.
 */

/* quantize_bitmaps:
 * generates an  optimised palette  for the  list of
 * bitmaps specified. All bitmaps must be true-colour.  The number of bitmaps
 * must be passed in n_bmp, and bmp must point to an array of pointers to
 * BITMAP  structures.  bmp_i  should  point  to  an array  parallel  to bmp,
 * containing importance values for the bitmaps.  pal must point to a PALETTE
 * structure which will be filled with the optimised palette.
 *
 * pal will be scanned for predefined colours. Any entry where the .r element
 * is 255 will be considered a free entry. All others are taken as predefined
 * colours.  Predefined colours  will not  be changed,  and the  rest  of the
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
 * This function  does not  convert the  bitmaps to  256-colour  format.  The
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

static int quantize_bitmaps(Image **image, int nimage, RGB *pal, int *bmp_i, int fill_other)
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
    /*    add_progress(image[c_bmp]->h); */
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
    /*    del_progress(); */
    /*   do_progress(c_bmp); */
  }
  /*Collapse empty nodes in the tree, and count leaves*/
  tree=collapse_empty(tree,&n_colours);
  /*Count free palette entries*/
  for (c=0;c<256;c++) {
    if (pal[c].r==255) n_entries++;
  }
  /*Collapse nodes until there are few enough to fit in the palette*/
  if (n_colours > n_entries) {
    /*   int n_colours1 = n_colours; */
    /*   add_progress(n_colours1 - n_entries); */
    while (n_colours>n_entries) {
      Ep=0xFFFFFFFFul;
      minimum_Ep(tree,&Ep);
      tree=collapse_nodes(tree,&n_colours,n_entries,Ep);

      /*    if (n_colours > n_entries) */
      /*     do_progress(n_colours1 - n_colours); */
    }
    /*   del_progress(); */
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
