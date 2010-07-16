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

#include <allegro/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/cfg.h"
#include "intl/intl.h"
#include "intl/msgids.h"
#include "modules/gui.h"
#include "resource_finder.h"

IntlModule::IntlModule()
{
  PRINTF("Internationalization module: starting\n");

  // load the language file
  intl_load_lang();

  PRINTF("Internationalization module: started\n");
}

IntlModule::~IntlModule()
{
  PRINTF("Internationalization module: shutting down\n");

  msgids_clear();

  PRINTF("Internationalization module: shut down\n");
}

void intl_load_lang()
{
  const char *lang = intl_get_lang();
  char buf[512];

  sprintf(buf, "po/%s.po", lang);
  ResourceFinder rf;
  rf.findInDataDir(buf);

  while (const char* path = rf.next()) {
    if (exists(path)) {
      if (msgids_load(path) < 0) {
	// TODO error loading language file... doesn't matter
      }
      break;
    }
  }
}

const char *intl_get_lang()
{
  return get_config_string("Options", "Language", "en");
}

void intl_set_lang(const char *lang)
{
  set_config_string("Options", "Language", lang);

  /* clear msgids and load them again */
  msgids_clear();
  intl_load_lang();
}

/* int init_intl() */
/* { */
/* #ifdef ENABLE_NLS */
/*   char buf[512], locale_path[512]; */
/*   DIRS *dirs, *dir; */

/*   strcpy(buf, "locale"); */
/*   dirs = filename_in_datadir(buf); */

/*   /\* search the configuration file from first to last path *\/ */
/*   for (dir=dirs; dir; dir=dir->next) { */
/*     if ((dir->path) && file_exists (dir->path, FA_DIREC, NULL)) { */
/*       strcpy(buf, dir->path); */
/*       break; */
/*     } */
/*   } */

/*   dirs_free(dirs); */

/*   fix_filename_path(locale_path, buf, sizeof(locale_path)); */

/* #ifndef __MINGW32__ */
/*   setlocale(LC_MESSAGES, ""); */
/*   bindtextdomain(PACKAGE, locale_path); */
/*   textdomain(PACKAGE); */
/*   setenv("OUTPUT_CHARSET", "ISO-8859-1", true); */
/* #endif */

/*   /\* Set current language.  *\/ */
/* /\*   const char *langname = get_config_string("Options", "Language", ""); *\/ */

/* /\*   setenv("LANGUAGE", "en", 1); *\/ */
/* /\*   { *\/ */
/* /\*     extern int _nl_msg_cat_cntr; *\/ */
/* /\*     ++_nl_msg_cat_cntr; *\/ */
/* /\*   } *\/ */
/* #endif */
/*   return 0; */
/* } */

/* const char *get_language_code() */
/* { */
/* #ifdef ENABLE_NLS */
/*   static char buf[8]; */
/*   const char *language; */

/*   language = getenv("LANGUAGE"); */
/*   if (language != NULL && language[0] == '\0') */
/*     language = NULL; */

/*   if (!language) { */
/*     language = getenv("LC_ALL"); */
/*     if (language != NULL && language[0] == '\0') */
/*       language = NULL; */
/*   } */

/*   if (!language) { */
/*     language = getenv("LANG"); */
/*     if (language != NULL && language[0] == '\0') */
/*       language = NULL; */
/*   } */

/*   if (language && language[0] && language[1]) */
/*     sprintf(buf, "%c%c", language[0], language[1]); */
/*   else */
/*     strcpy(buf, "en"); */

/*   return buf; */
/* #else */
/*   return "en"; */
/* #endif */
/* } */

/* #ifdef ENABLE_NLS */
/* #ifdef __MINGW32__ */
/* char *gettext(const char *__msgid) */
/* { */
/*   return gettext__ (__msgid); */
/* } */
/* #endif */
/* #endif */
