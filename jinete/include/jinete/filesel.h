/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_FILESEL_H
#define JINETE_FILESEL_H

#include "jinete/base.h"

JI_BEGIN_DECLS

char *ji_file_select(const char *message,
		     const char *init_path,
		     const char *exts);

char *ji_file_select_ex(const char *message,
			const char *init_path,
			const char *exts,
			JWidget widget_extension);

char *ji_file_select_get_current_path(void);
void ji_file_select_refresh_listbox(void);
void ji_file_select_enter_to_path(const char *path);

bool ji_dir_exists(const char *filename);

JI_END_DECLS

#endif /* JINETE_FILESEL_H */
