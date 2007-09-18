/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_IMAGE_H
#define JINETE_IMAGE_H

#include "jinete/base.h"

JI_BEGIN_DECLS

struct BITMAP;

JWidget ji_image_new(struct BITMAP *bmp, int align);

JI_END_DECLS

#endif /* JINETE_IMAGE_H */
