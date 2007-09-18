/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_BUTTON_H
#define JINETE_BUTTON_H

#include "jinete/base.h"

JI_BEGIN_DECLS

struct BITMAP;

/******************/
/* generic button */

JWidget ji_generic_button_new(const char *text,
			      int behavior_type,
			      int draw_type);

void ji_generic_button_set_icon(JWidget button, struct BITMAP *icon);
void ji_generic_button_set_icon_align(JWidget button, int icon_align);

struct BITMAP *ji_generic_button_get_icon(JWidget button);
int ji_generic_button_get_icon_align(JWidget button);

/*****************/
/* normal button */

JWidget jbutton_new(const char *text);

void jbutton_set_bevel(JWidget button, int b0, int b1, int b2, int b3);
void jbutton_get_bevel(JWidget button, int *b4);

void jbutton_add_command(JWidget button, void (*command)(JWidget button));
void jbutton_add_command_data(JWidget button,
			      void (*command)(JWidget button, void *data),
			      void *data);

/****************/
/* check button */

JWidget jcheck_new(const char *text);

/****************/
/* radio button */

JWidget jradio_new(const char *text, int radio_group);

void jradio_set_group(JWidget radio, int radio_group);
int jradio_get_group(JWidget radio);
void jradio_deselect_group(JWidget radio);

JI_END_DECLS

#endif /* JINETE_BUTTON_H */
