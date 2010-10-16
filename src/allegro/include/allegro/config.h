/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Configuration file access routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_CONFIG_H
#define ALLEGRO_CONFIG_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(void, set_config_file, (AL_CONST char *filename));
AL_FUNC(void, set_config_data, (AL_CONST char *data, int length));
AL_FUNC(void, override_config_file, (AL_CONST char *filename));
AL_FUNC(void, override_config_data, (AL_CONST char *data, int length));
AL_FUNC(void, flush_config_file, (void));
AL_FUNC(void, reload_config_texts, (AL_CONST char *new_language));

AL_FUNC(void, push_config_state, (void));
AL_FUNC(void, pop_config_state, (void));

AL_FUNC(void, hook_config_section, (AL_CONST char *section, AL_METHOD(int, intgetter, (AL_CONST char *, int)), AL_METHOD(AL_CONST char *, stringgetter, (AL_CONST char *, AL_CONST char *)), AL_METHOD(void, stringsetter, (AL_CONST char *, AL_CONST char *))));
AL_FUNC(int, config_is_hooked, (AL_CONST char *section));

AL_FUNC(AL_CONST char *, get_config_string, (AL_CONST char *section, AL_CONST char *name, AL_CONST char *def));
AL_FUNC(int, get_config_int, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(int, get_config_hex, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(float, get_config_float, (AL_CONST char *section, AL_CONST char *name, float def));
AL_FUNC(int, get_config_id, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(char **, get_config_argv, (AL_CONST char *section, AL_CONST char *name, int *argc));
AL_FUNC(AL_CONST char *, get_config_text, (AL_CONST char *msg));

AL_FUNC(void, set_config_string, (AL_CONST char *section, AL_CONST char *name, AL_CONST char *val));
AL_FUNC(void, set_config_int, (AL_CONST char *section, AL_CONST char *name, int val));
AL_FUNC(void, set_config_hex, (AL_CONST char *section, AL_CONST char *name, int val));
AL_FUNC(void, set_config_float, (AL_CONST char *section, AL_CONST char *name, float val));
AL_FUNC(void, set_config_id, (AL_CONST char *section, AL_CONST char *name, int val));

AL_FUNC(int, list_config_entries, (AL_CONST char *section, AL_CONST char ***names));
AL_FUNC(int, list_config_sections, (AL_CONST char ***names));
AL_FUNC(void, free_config_entries, (AL_CONST char ***names));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_CONFIG_H */


