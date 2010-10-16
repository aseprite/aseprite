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
 *      Configuration routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Hook functions added by Martijn Versteegh.
 *
 *      Annie Testes lifted several hardcoded length limitations.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct CONFIG_ENTRY
{
   char *name;                      /* variable name (NULL if comment) */
   char *data;                      /* variable value */
   struct CONFIG_ENTRY *next;       /* linked list */
} CONFIG_ENTRY;


typedef struct CONFIG
{
   CONFIG_ENTRY *head;              /* linked list of config entries */
   char *filename;                  /* where were we loaded from? */
   int dirty;                       /* has our data changed? */
} CONFIG;


typedef struct CONFIG_HOOK
{
   char *section;                   /* hooked config section info */
   int (*intgetter)(AL_CONST char *name, int def);
   AL_CONST char *(*stringgetter)(AL_CONST char *name, AL_CONST char *def);
   void (*stringsetter)(AL_CONST char *name, AL_CONST char *value);
   struct CONFIG_HOOK *next; 
} CONFIG_HOOK;


#define MAX_CONFIGS     4

static CONFIG *config[MAX_CONFIGS] = { NULL, NULL, NULL, NULL };
static CONFIG *config_override = NULL;
static CONFIG *config_language = NULL;
static CONFIG *system_config = NULL;

static CONFIG_HOOK *config_hook = NULL;

static int config_installed = FALSE;

static char **config_argv = NULL;
static char *argv_buf = NULL;
static int argv_buf_size = 0;



/* flush_config:
 *  Writes out a config structure to disk if the contents
 *  have changed.
 */
static void flush_config(CONFIG *cfg)
{
   CONFIG_ENTRY *pos;
   PACKFILE *f;
   char cr[16];

   usetc(cr+usetc(cr, '\n'), 0);

   if (cfg && cfg->filename && cfg->dirty) {
      /* write changed data to disk */
      f = pack_fopen(cfg->filename, F_WRITE);

      if (f) {
	 pos = cfg->head;

	 while (pos) {
	    if (pos->name) {
	       pack_fputs(pos->name, f);

	       if (ugetc(pos->name) != '[') {
		  pack_putc(' ', f);
		  pack_putc('=', f);
		  pack_putc(' ', f);
	       }
	    }

	    if (pos->data)
	       pack_fputs(pos->data, f);

	    pack_fputs(cr, f);

	    pos = pos->next;
         }

	 pack_fclose(f);
	 cfg->dirty = FALSE;
      }
   }
}



/* flush_config_file:
 *  Writes out the config file to disk if the contents
 *  have changed.
 */
void flush_config_file(void)
{
   flush_config(config[0]);
}



/* destroy_config:
 *  Destroys a config structure, writing it out to disk if the contents
 *  have changed.
 */
static void destroy_config(CONFIG *cfg)
{
   CONFIG_ENTRY *pos, *prev;

   if (cfg) {
      flush_config(cfg);

      if (cfg->filename)
	 _AL_FREE(cfg->filename);

      /* destroy the variable list */
      pos = cfg->head;

      while (pos) {
	 prev = pos;
	 pos = pos->next;

	 if (prev->name)
	    _AL_FREE(prev->name);

	 if (prev->data)
	    _AL_FREE(prev->data);

	 _AL_FREE(prev);
      }

      _AL_FREE(cfg);
   }
}



/* config_cleanup:
 *  Called at shutdown time to free memory being used by the config routines,
 *  and write any changed data out to disk.
 */
static void config_cleanup(void)
{
   CONFIG_HOOK *hook, *nexthook;
   int i;

   for (i=0; i<MAX_CONFIGS; i++) {
      if (config[i]) {
	 destroy_config(config[i]);
	 config[i] = NULL;
      }
   }

   if (config_override) {
      destroy_config(config_override);
      config_override = NULL;
   }

   if (config_language) {
      destroy_config(config_language);
      config_language = NULL;
   }

   if (system_config) {
      destroy_config(system_config);
      system_config = NULL;
   }

   if (config_hook) {
      hook = config_hook;

      while (hook) {
	 if (hook->section)
	    _AL_FREE(hook->section);

	 nexthook = hook->next; 
	 _AL_FREE(hook);
	 hook = nexthook;
      }

      config_hook = NULL;
   }

   _AL_FREE(config_argv);
   config_argv = NULL;
   
   _AL_FREE(argv_buf);
   argv_buf = NULL;
   
   argv_buf_size = 0;

   _remove_exit_func(config_cleanup);
   config_installed = FALSE;
}



/* init_config:
 *  Sets up the configuration routines ready for use, also loading the
 *  default config file if the loaddata flag is set and no other config
 *  file is in memory.
 */
static void init_config(int loaddata)
{
   char filename[1024], tmp[128], *cfg_name;

   if (!config_installed) {
      _add_exit_func(config_cleanup, "config_cleanup");
      config_installed = TRUE;
   }

   if ((loaddata) && (!config[0])) {
      cfg_name = uconvert_ascii("allegro.cfg", tmp);

      if (find_allegro_resource(filename, cfg_name, NULL, NULL, NULL, NULL, NULL, sizeof(filename)) != 0) {
	 get_executable_name(filename, sizeof(filename));
	 usetc(get_filename(filename), 0);
	 ustrzcat(filename, sizeof(filename), cfg_name);
      }

      set_config_file(filename);
   }

   if (!system_config) {
      system_config = _AL_MALLOC(sizeof(CONFIG));
      if (system_config) {
	 system_config->head = NULL;
	 system_config->filename = NULL;
	 system_config->dirty = FALSE;
      }
   }
}



/* get_line:
 *  Helper for splitting files up into individual lines. Returns the length
 *  in bytes of the sequence of characters delimited by the first EOL marker
 *  in the array DATA of length LENGTH, and allocates NAME and VAL to record
 *  the name and the value of the config entry respectively; otherwise set
 *  NAME to NULL and returns a copy of the line through VAL if the line was
 *  blank or a comment. Returns -1 and set allegro_errno on failure.
 */
static int get_line(AL_CONST char *data, int length, char **name, char **val)
{
   char *buf;
   int buf_size=256;
   int inpos, outpos, i, j;
   int c, c2, w0;

   inpos = 0;
   outpos = 0;
   w0 = ucwidth(0);

   buf = _AL_MALLOC_ATOMIC(buf_size);
   if (!buf) {
     *allegro_errno = ENOMEM;
     return -1;
   }

   /* search for an EOL marker */
   while (inpos<length) {
      c = ugetc(data+inpos);
      if ((c == '\r') || (c == '\n')) {
	 inpos += uwidth(data+inpos);
	 if (inpos < length) {
	    c2 = ugetc(data+inpos);
	    if (((c == '\r') && (c2 == '\n')) || ((c == '\n') && (c2 == '\r')))
	       inpos += uwidth(data+inpos);
	 }
	 break;
      }

      /* increase the buffer size if needed */
      if (outpos>=(int)buf_size-w0) {
	 buf_size *= 2;
	 buf = _al_sane_realloc(buf, buf_size);
	 if (!buf) {
	    *allegro_errno = ENOMEM;
	    return -1;
	 }
      }

      outpos += usetc(buf+outpos, c);
      inpos += uwidth(data+inpos);
   }

   usetc(buf+outpos, 0);

   /* skip leading spaces */
   i = 0;
   c = ugetc(buf);

   while ((c) && (uisspace(c))) {
      i += uwidth(buf+i);
      c = ugetc(buf+i);
   }

   /* read name string */
   j = 0;

   /* compute name length */
   while ((c) && (!uisspace(c)) && (c != '=') && (c != '#')) {
      j += ucwidth(c);
      i += uwidth(buf+i);
      c = ugetc(buf+i);
   }

   if (j) {
      /* got a variable */
      *name = _AL_MALLOC_ATOMIC(j+w0);
      if (!(*name)) {
	 *allegro_errno = ENOMEM;
	 _AL_FREE(buf);
	 return -1;
      }

      ustrzcpy(*name, j+w0, buf+i-j);

      while ((c) && ((uisspace(c)) || (c == '='))) {
	 i += uwidth(buf+i);
	 c = ugetc(buf+i);
      }

      *val = _al_ustrdup(buf+i);
      if (!(*val)) {
	 _AL_FREE(name);
	 _AL_FREE(buf);
	 return -1;
      }

      /* strip trailing spaces */
      i = ustrlen(*val) - 1;
      while ((i >= 0) && (uisspace(ugetat(*val, i))))
	 usetat(*val, i--, 0);
   }
   else {
      /* blank line or comment */
      *name = NULL;
      *val = _al_ustrdup(buf);
      if (!(*val)) {
	 _AL_FREE(buf);
	 return -1;
      }
   }

   _AL_FREE(buf);

   return inpos;
}



/* set_config:
 *  Does the work of setting up a config structure.
 */
static void set_config(CONFIG **config, AL_CONST char *data, int length, AL_CONST char *filename)
{
   CONFIG_ENTRY **prev, *p;
   char *name, *val;
   int ret, pos;

   init_config(FALSE);

   if (*config) {
      destroy_config(*config);
      *config = NULL;
   }

   *config = _AL_MALLOC(sizeof(CONFIG));
   if (!(*config)) {
      *allegro_errno = ENOMEM;
      return;
   }

   (*config)->head = NULL;
   (*config)->dirty = FALSE;

   if (filename) {
      (*config)->filename = _al_ustrdup(filename);
      if (!(*config)->filename) {
	 _AL_FREE(*config);
	 *config = NULL;
	 return;
      }
   }
   else
      (*config)->filename = NULL;

   prev = &(*config)->head;
   pos = 0;

   while (pos < length) {
      ret = get_line(data+pos, length-pos, &name, &val);
      if (ret<0) {
	 _AL_FREE(*config);
	 *config = NULL;
	 return;
      }

      pos += ret;

      p = _AL_MALLOC(sizeof(CONFIG_ENTRY));
      if (!p) {
	 *allegro_errno = ENOMEM;
	 _AL_FREE(*config);
	 *config = NULL;
	 return;
      }

      p->name = name;
      p->data = val;

      p->next = NULL;
      *prev = p;
      prev = &p->next;
   }
}



/* load_config_file:
 *  Does the work of loading a config file.
 */
static void load_config_file(CONFIG **config, AL_CONST char *filename, AL_CONST char *savefile)
{
   char *tmp, *tmp2;
   uint64_t length;

   if (*config) {
      destroy_config(*config);
      *config = NULL;
   }

   /* Special case when allegro_init has not been called yet. */
   if (!system_driver) {
      set_config(config, NULL, 0, savefile);
      return;
   }

   length = file_size_ex(filename);

   if (length > 0) {
      PACKFILE *f = pack_fopen(filename, F_READ);

      if (f) {
	 tmp = _AL_MALLOC_ATOMIC(length+1);

	 if (tmp) {
	    pack_fread(tmp, length, f);
	    tmp[length] = 0;

	    if (need_uconvert(tmp, U_UTF8, U_CURRENT)) {
	       length = uconvert_size(tmp, U_UTF8, U_CURRENT);
	       tmp2 = _AL_MALLOC_ATOMIC(length);

	       if (tmp2)
		  do_uconvert(tmp, U_UTF8, tmp2, U_CURRENT, length);

	       length -= ucwidth(0);
	    }
	    else
	       tmp2 = tmp;

	    if (tmp2) {
	       set_config(config, tmp2, length, savefile);

	       if (tmp2 != tmp)
		  _AL_FREE(tmp2);
	    }

	    _AL_FREE(tmp);
	 }
	 else
	    set_config(config, NULL, 0, savefile);

	 pack_fclose(f);
      }
      else
	 set_config(config, NULL, 0, savefile);
   }
   else
      set_config(config, NULL, 0, savefile);
}



/* set_config_file:
 *  Sets the file to be used for all future configuration operations.
 */
void set_config_file(AL_CONST char *filename)
{
   ASSERT(filename);
   load_config_file(&config[0], filename, filename);
}



/* set_config_data:
 *  Sets the block of data to be used for all future configuration 
 *  operations.
 */
void set_config_data(AL_CONST char *data, int length)
{
   ASSERT(data);
   set_config(&config[0], data, length, NULL);
}



/* override_config_file:
 *  Sets the file that will override all future configuration operations.
 */
void override_config_file(AL_CONST char *filename)
{
   /* load other configuration file to override settings */
   if (filename)
      load_config_file(&config_override, filename, filename);
   /* destroy the current one */
   else if (config_override) {
      destroy_config(config_override);
      config_override = NULL;
   }
}



/* override_config_data:
 *  Sets the block of data that will override all future configuration 
 *  operations.
 */
void override_config_data(AL_CONST char *data, int length)
{
   ASSERT(data);
   set_config(&config_override, data, length, NULL);
}



/* push_config_state:
 *  Pushes the current config state onto the stack.
 */
void push_config_state(void)
{
   int i;

   if (config[MAX_CONFIGS-1])
      destroy_config(config[MAX_CONFIGS-1]);

   for (i=MAX_CONFIGS-1; i>0; i--)
      config[i] = config[i-1];

   config[0] = NULL;
}



/* pop_config_state:
 *  Pops the current config state off the stack.
 */
void pop_config_state(void)
{
   int i;

   if (config[0])
      destroy_config(config[0]);

   for (i=0; i<MAX_CONFIGS-1; i++)
      config[i] = config[i+1];

   config[MAX_CONFIGS-1] = NULL;
}



/* prettify_section_name:
 *  Helper for ensuring that a section name is enclosed by [ ] braces.
 */
static void prettify_section_name(AL_CONST char *in, char *out, int out_size)
{
   int p;

   if ((in) && (ustrlen(in))) {
      if (ugetc(in) != '[') {
	 p = usetc(out, '[');
	 usetc(out+p, 0);
      }
      else
	 usetc(out, 0);

      ustrzcat(out, out_size - ucwidth(']'), in);

      out += uoffset(out, -1);

      if (ugetc(out) != ']') {
	 out += uwidth(out);
	 out += usetc(out, ']');
	 usetc(out, 0);
      }
   }
   else
      usetc(out, 0);
}



/* hook_config_section:
 *  Hooks a config section to a set of getter/setter functions. This will 
 *  override the normal table of values, and give the provider of the hooks 
 *  complete control over that section.
 */
void hook_config_section(AL_CONST char *section, int (*intgetter)(AL_CONST char *, int), AL_CONST char *(*stringgetter)(AL_CONST char *, AL_CONST char *), void (*stringsetter)(AL_CONST char *, AL_CONST char *))
{
   CONFIG_HOOK *hook, **prev;
   char section_name[256];

   init_config(FALSE);

   prettify_section_name(section, section_name, sizeof(section_name));

   hook = config_hook;
   prev = &config_hook;

   while (hook) {
      if (ustricmp(section_name, hook->section) == 0) {
	 if ((intgetter) || (stringgetter) || (stringsetter)) {
	    /* modify existing hook */
	    hook->intgetter = intgetter;
	    hook->stringgetter = stringgetter;
	    hook->stringsetter = stringsetter;
	 }
	 else {
	    /* remove a hook */
	    *prev = hook->next;
	    _AL_FREE(hook->section);
	    _AL_FREE(hook);
	 }

	 return;
      }

      prev = &hook->next;
      hook = hook->next;
   }

   /* add a new hook */
   hook = _AL_MALLOC(sizeof(CONFIG_HOOK));
   if (!hook)
      return;

   hook->section = _al_ustrdup(section_name);
   if (!(hook->section)) {
      _AL_FREE(hook);
      return;
   }

   hook->intgetter = intgetter;
   hook->stringgetter = stringgetter;
   hook->stringsetter = stringsetter;

   hook->next = config_hook;
   config_hook = hook;
}



/* is_config_hooked:
 *  Checks whether a specific section is hooked in any way.
 */
int config_is_hooked(AL_CONST char *section)
{
   CONFIG_HOOK *hook = config_hook;
   char section_name[256];

   prettify_section_name(section, section_name, sizeof(section_name));

   while (hook) {
      if (ustricmp(section_name, hook->section) == 0)
	 return TRUE;

      hook = hook->next;
   }

   return FALSE;
}



/* find_config_string:
 *  Helper for finding an entry in the configuration file.
 */
static CONFIG_ENTRY *find_config_string(CONFIG *config, AL_CONST char *section, AL_CONST char *name, CONFIG_ENTRY **prev)
{
   CONFIG_ENTRY *p;
   int in_section;

   if (config) {
      p = config->head;

      if (prev)
	 *prev = NULL;

      if (section && ugetc(section))
         in_section = FALSE;
      else
         in_section = TRUE;

      while (p) {
	 if (p->name) {
	    if ((section) && (ugetc(p->name) == '[') && (ugetat(p->name, -1) == ']')) {
	       /* change section */
	       in_section = (ustricmp(section, p->name) == 0);
	    }
	    if ((in_section) || (ugetc(name) == '[')) {
	       /* is this the one? */
	       if (ustricmp(p->name, name) == 0)
		  return p;
	    }
	 }

	 if (prev)
	    *prev = p;

	 p = p->next;
      }
   }

   return NULL;
}



/* get_config_string:
 *  Reads a string from the configuration file.
 */
AL_CONST char *get_config_string(AL_CONST char *section, AL_CONST char *name, AL_CONST char *def)
{
   char section_name[256];
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p;

   init_config(TRUE);

   prettify_section_name(section, section_name, sizeof(section_name));

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (ustricmp(section_name, hook->section) == 0) {
	 if (hook->stringgetter)
	    return hook->stringgetter(name, def);
	 else
	    return def;
      }
      hook = hook->next;
   }

   /* find the string */
   p = find_config_string(config_override, section_name, name, NULL);

   if (!p) {
      if ((ugetc(name) == '#') || ((ugetc(section_name) == '[') && (ugetat(section_name, 1) == '#')))
	 p = find_config_string(system_config, section_name, name, NULL);
      else
	 p = find_config_string(config[0], section_name, name, NULL);
   }

   if (p && p->data && (ustrlen(p->data) != 0))
      return p->data;
   else
      return def;
}



/* get_config_int:
 *  Reads an integer from the configuration file.
 */
int get_config_int(AL_CONST char *section, AL_CONST char *name, int def)
{
   CONFIG_HOOK *hook;
   char section_name[256];
   AL_CONST char *s;

   prettify_section_name(section, section_name, sizeof(section_name));

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (ustricmp(section_name, hook->section) == 0) {
	 if (hook->intgetter) {
	    return hook->intgetter(name, def);
	 }
	 else if (hook->stringgetter) {
	    s = hook->stringgetter(name, NULL);
	    if ((s) && (ugetc(s)))
	       return ustrtol(s, NULL, 0);
	    else
	       return def;
	 }
	 else
	    return def;
      }
      hook = hook->next;
   }

   /* read normal data */
   s = get_config_string(section_name, name, NULL);

   if ((s) && (ugetc(s)))
      return ustrtol(s, NULL, 0);

   return def;
}



/* get_config_hex:
 *  Reads a hexadecimal integer from the configuration file.
 */
int get_config_hex(AL_CONST char *section, AL_CONST char *name, int def)
{
   AL_CONST char *s = get_config_string(section, name, NULL);
   char tmp[64];
   int i;

   if ((s) && (ugetc(s))) {
      i = ustrtol(s, NULL, 16);
      if ((i == 0x7FFFFFFF) && (ustricmp(s, uconvert_ascii("7FFFFFFF", tmp)) != 0))
	 i = -1;
      return i;
   }

   return def;
}



/* get_config_float:
 *  Reads a float from the configuration file.
 */
float get_config_float(AL_CONST char *section, AL_CONST char *name, float def)
{
   AL_CONST char* s = get_config_string(section, name, NULL);

   if ((s) && (ugetc(s)))
      return uatof(s);

   return def;
}



/* get_config_id:
 *  Reads a driver ID number from the configuration file.
 */
int get_config_id(AL_CONST char *section, AL_CONST char *name, int def)
{
   AL_CONST char *s = get_config_string(section, name, NULL);
   char tmp[4];
   char* endp;
   int val, i;

   if ((s) && (ugetc(s))) {
      val = ustrtol(s, &endp, 0);
      if (!ugetc(endp))
	 return val;

      tmp[0] = tmp[1] = tmp[2] = tmp[3] = ' ';

      for (i=0; i<4; i++) {
	 if (ugetat(s, i))
	    tmp[i] = utoupper(ugetat(s ,i));
	 else
	    break;
      }

      return AL_ID(tmp[0], tmp[1], tmp[2], tmp[3]);
   }

   return def;
}



/* get_config_argv:
 *  Reads an argc/argv style token list from the configuration file.
 */
char **get_config_argv(AL_CONST char *section, AL_CONST char *name, int *argc)
{
   int pos, ac, q, c;
   int s_size;
   int i;

   AL_CONST char *s = get_config_string(section, name, NULL);

   if (!s) {
      *argc = 0;
      return NULL;
   }

   /* clean up the old argv that was allocated the last time this function was
    * called.
    */
   _AL_FREE(config_argv);
   config_argv = NULL;

   /* increase the buffer size if needed */
   s_size = ustrsizez(s);
   if (s_size>argv_buf_size) {
      argv_buf_size = s_size;
      argv_buf = _al_sane_realloc(argv_buf, argv_buf_size);
      if (!argv_buf) {
         *allegro_errno = ENOMEM;
         *argc = 0;
         return NULL;
      }
   }

   ustrzcpy(argv_buf, argv_buf_size, s);
   pos = 0;
   ac = 0;

   /* tokenize the buffer and count the number of words; every space character
    * as well as single and double quotes are replaced with zeros; comments * that start with # are also cut off with a 0
    */
   c = ugetc(argv_buf);
   while ((c) && (c != '#')) {
      /* replace all spaces up to the next word with 0 */
      while ((c) && (uisspace(c))) {
	 usetat(argv_buf+pos, 0, 0);
	 pos += ucwidth(0);
	 c = ugetc(argv_buf+pos);
      }

      /* quit if we reached the end of the buffer or a comment */
      if ((c) && (c != '#')) {
         /* found another word! */
         ac++;

         /* cut away quotes by replacing them with 0 */
         if ((c == '\'') || (c == '"')) {
            q = c;
            usetat(argv_buf+pos, 0, 0);
            pos += ucwidth(0);
            c = ugetc(argv_buf+pos);
         }
         else {
            q = 0;
         }

         /* search for the end of the word */
         while ((c) && ((q) ? (c != q) : (!uisspace(c)))) {
            pos += ucwidth(c);
            c = ugetc(argv_buf+pos);
         }
      }
   }

   /* now that we know how many words there are in the buffer, allocate enough
    * space for a list of pointers to them, or return 0 if there are no words
    */
   if (ac > 0) {
      config_argv = _AL_MALLOC(ac*sizeof *config_argv);
   }
   else {
      *argc = 0;
      return NULL;
   }

   /* go through the tokenized buffer and assign pointers in argv to point to
    * the beginning of each individual word
    */
   for (i=0,pos=0,c=ugetc(argv_buf); i<ac; i++) {
      /* find next word */
      while (!c) {
         pos += ucwidth(c);
         c = ugetc(argv_buf+pos);
      }

      /* assign pointer */
      config_argv[i] = argv_buf+pos;

      /* find end of the word */
      while (c) {
         pos += ucwidth(c);
         c = ugetc(argv_buf+pos);
      }
   }

   *argc = ac;
   return config_argv;
}



/* insert_variable:
 *  Helper for inserting a new variable into a configuration file.
 */
static CONFIG_ENTRY *insert_variable(CONFIG *the_config, CONFIG_ENTRY *p, AL_CONST char *name, AL_CONST char *data)
{
   CONFIG_ENTRY *n = _AL_MALLOC(sizeof(CONFIG_ENTRY));

   if (!n)
      return NULL;

   if (name)
      n->name = _al_ustrdup(name);
   else
      n->name = NULL;

   if (data)
      n->data = _al_ustrdup(data);
   else
      n->data = NULL;

   if (p) {
      n->next = p->next;
      p->next = n; 
   }
   else {
      n->next = NULL;
      the_config->head = n;
   }

   return n;
}



/* set_config_string:
 *  Writes a string to the configuration file.
 */
void set_config_string(AL_CONST char *section, AL_CONST char *name, AL_CONST char *val)
{
   CONFIG *the_config;
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p, *prev;
   char section_name[256];

   init_config(TRUE);

   prettify_section_name(section, section_name, sizeof(section_name));

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (ustricmp(section_name, hook->section) == 0) {
	 if (hook->stringsetter)
	    hook->stringsetter(name, val);
	 return;
      }
      hook = hook->next;
   }

   /* decide which config file to use */
   if ((ugetc(name) == '#') || ((ugetc(section_name) == '[') && (ugetat(section_name, 1) == '#')))
      the_config = system_config;
   else if (config_override)
      the_config = config_override;
   else
      the_config = config[0];

   if (the_config) {
      p = find_config_string(the_config, section_name, name, &prev);

      if (p) {
	 if ((val) && (ugetc(val))) {
	    /* modify existing variable */
	    if (p->data)
	       _AL_FREE(p->data);

	    p->data = _al_ustrdup(val);
	 }
	 else {
	    /* delete variable */
	    if (p->name)
	       _AL_FREE(p->name);

	    if (p->data)
	       _AL_FREE(p->data);

	    if (prev)
	       prev->next = p->next;
	    else
	       the_config->head = p->next;

	    _AL_FREE(p);
	 }
      }
      else {
	 if ((val) && (ugetc(val))) {
	    /* add a new variable */
	    if (ugetc(section_name)) {
	       p = find_config_string(the_config, NULL, section_name, &prev);

	       if (!p) {
		  /* create a new section */
		  p = the_config->head;
		  while ((p) && (p->next))
		     p = p->next;

		  if ((p) && (p->data) && (ugetc(p->data)))
		     p = insert_variable(the_config, p, NULL, NULL);

		  p = insert_variable(the_config, p, section_name, NULL);
	       }

	       /* append to the end of the section */
	       while ((p) && (p->next) && 
		      (((p->next->name) && (ugetc(p->next->name))) || 
		       ((p->next->data) && (ugetc(p->next->data)))))
		  p = p->next;

	       p = insert_variable(the_config, p, name, val);
	    }
	    else {
	       /* global variable */
	       p = the_config->head;
	       insert_variable(the_config, NULL, name, val);
	       the_config->head->next = p;
	    }
	 } 
      }

      the_config->dirty = TRUE;
   }
}



/* set_config_int:
 *  Writes an integer to the configuration file.
 */
void set_config_int(AL_CONST char *section, AL_CONST char *name, int val)
{
   char buf[32], tmp[32];
   uszprintf(buf, sizeof(buf), uconvert_ascii("%d", tmp), val);
   set_config_string(section, name, buf);
}



/* set_config_hex:
 *  Writes a hexadecimal integer to the configuration file.
 */
void set_config_hex(AL_CONST char *section, AL_CONST char *name, int val)
{
   char buf[32], tmp[32];

   if (val >= 0) {
      uszprintf(buf, sizeof(buf), uconvert_ascii("%X", tmp), val);
      set_config_string(section, name, buf);
   }
   else
      set_config_string(section, name, uconvert_ascii("-1", buf));
}



/* set_config_float:
 *  Writes a float to the configuration file.
 */
void set_config_float(AL_CONST char *section, AL_CONST char *name, float val)
{
   char buf[32], tmp[32];
   uszprintf(buf, sizeof(buf), uconvert_ascii("%f", tmp), val);
   set_config_string(section, name, buf);
}



/* set_config_id:
 *  Writes a driver ID to the configuration file.
 */
void set_config_id(AL_CONST char *section, AL_CONST char *name, int val)
{
   char buf[32], tmp[32];
   int v[4];
   int pos = 0;
   int i;

   if (val < 256) {
      uszprintf(buf, sizeof(buf), uconvert_ascii("%d", tmp), val);
   }
   else {
      v[0] = (val>>24)&0xFF;
      v[1] = (val>>16)&0xFF;
      v[2] = (val>>8)&0xFF;
      v[3] = val&0xFF;

      for (i=0; (i<4) && (v[i]) && (v[i] != ' '); i++)
	 pos += usetc(buf+pos, v[i]);

      usetc(buf+pos, 0);
   }

   set_config_string(section, name, buf);
}



/* _reload_config:
 *  Internal helper to reload the configuration from allegro_init, in case
 *  set_config_file was called before allegro_init.
 */
void _reload_config(void)
{
   if (config[0]) {
      char *name = _al_ustrdup(config[0]->filename);
      set_config_file(name);
      _AL_FREE(name);
   }
}



/* reload_config_texts:
 *  Reads in a block of translated system text, looking for either a
 *  user-specified file, a ??text.cfg file, or a language.dat#??TEXT_CFG 
 *  datafile object. If new_language is not NULL, the language config
 *  variable will be set to new_language before reloading the
 *  configuration files.
 */
void reload_config_texts(AL_CONST char *new_language)
{
   char filename[1024], tmp1[128], tmp2[128];
   AL_CONST char *name, *ext, *datafile;
   char *namecpy;

   if (config_language) {
      destroy_config(config_language);
      config_language = NULL;
   }

   if (new_language)
      set_config_string("system", "language", new_language);

   name = get_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("language", tmp2), NULL);

   if ((name) && (ugetc(name))) {
      namecpy = _al_ustrdup(name);
      ustrlwr (namecpy);
      if ((ustrlen(namecpy)<4) || (ustricmp(namecpy+uoffset(namecpy, -4), uconvert_ascii("text", tmp1)) != 0))
	 ext = uconvert_ascii("text.cfg", tmp1);
      else
	 ext = uconvert_ascii(".cfg", tmp1);

      datafile = uconvert_ascii("language.dat", tmp2);

      if (find_allegro_resource(filename, namecpy, ext, datafile, NULL, NULL, NULL, sizeof(filename)) == 0) {
	 _AL_FREE(namecpy);
	 load_config_file(&config_language, filename, NULL);
	 return;
      }

      _AL_FREE(namecpy);
   }

   config_language = _AL_MALLOC(sizeof(CONFIG));
   if (config_language ) {
      config_language ->head = NULL;
      config_language ->filename = NULL;
      config_language ->dirty = FALSE;
   }
}



/* get_config_text:
 *  Looks up a translated version of the specified English string,
 *  returning a suitable message in the current language if one is
 *  available, or a copy of the parameter if no translation can be found.
 */
AL_CONST char *get_config_text(AL_CONST char *msg)
{
   char tmp1[256];
   AL_CONST char *section;
   AL_CONST char *umsg;
   AL_CONST char *s;
   AL_CONST char *ret = NULL;
   char *name;
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p;
   int c, pos, size;
   ASSERT(msg);

   /* Hack: the inline definition of install_allegro() from 4.2.0 calls
    * get_config_text() even before Allegro has been initialised, leading
    * to a crash in get_executable_name().  To retain binary compatibility
    * we check for this case.
    */
   if (_allegro_count == 0) {
      return msg;
   }

   init_config(TRUE);

   section = uconvert_ascii("[language]", tmp1);

   /* allocate memory and convert message to current encoding format */
   if (need_uconvert(msg, U_ASCII, U_CURRENT)) {
      size = uconvert_size(msg, U_ASCII, U_CURRENT);
      umsg = _AL_MALLOC_ATOMIC(size);
      if (!umsg) {
	 *allegro_errno = ENOMEM;
	 return empty_string;
      }

      name = _AL_MALLOC_ATOMIC(size);
      if (!name) {
	 _AL_FREE((char *)umsg);  /* remove constness */
	 *allegro_errno = ENOMEM;
	 return empty_string;
      }

      do_uconvert(msg, U_ASCII, (char*)umsg, U_CURRENT, size);
   }
   else {
      umsg = msg;
      name = _AL_MALLOC_ATOMIC(ustrsizez(msg));
      if (!name) {
	 *allegro_errno = ENOMEM;
	 return empty_string;
      }
   }

   s = umsg;
   pos = 0;

   while ((c = ugetxc(&s)) != 0) {
      if ((uisspace(c)) || (c == '=') || (c == '#'))
	 pos += usetc(name+pos, '_');
      else
	 pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (ustricmp(section, hook->section) == 0) {
	 if (hook->stringgetter) {
	    ret = hook->stringgetter(name, umsg);
	    break;
	 }
      }

      hook = hook->next;
   }

   if (!ret) {
      /* find the string */
      p = find_config_string(config_override, section, name, NULL);

      if (!p) {
	 p = find_config_string(config[0], section, name, NULL);

	 if (!p)
	    p = find_config_string(config_language, section, name, NULL);
      }

      if (p) {
	 ret = (p->data ? p->data : empty_string);
      }
      else {
	 /* no translation, so store off this value in the file */
	 p = config_language->head;
	 insert_variable(config_language, NULL, name, umsg);
	 config_language->head->next = p;
	 ret = config_language->head->data;
      }
   }

   /* free memory */
   if (umsg!=msg)
      _AL_FREE((char*) umsg);  /* remove constness */

   _AL_FREE(name);

   return ret;
}



/* add_unique_name
 *  Helper to add a name to a list of names.
 */
static int add_unique_name(AL_CONST char ***names, int n, char const *name)
{
   int i;
   /* FIXME: use better search algorithm */
   for (i = 0; i < n; i++)
      if (!ustrcmp((*names)[i], name))
         return n;

   *names = _al_sane_realloc((void *)*names, (n + 1) * sizeof **names);
   (*names)[n] = name;
   return n + 1;
}



/* attach_config_entries
 *  Helper function to attach key or section names to a list of strings.
 */
static int attach_config_entries(CONFIG *conf, AL_CONST char *section,
   int n, AL_CONST char ***names, int list_sections)
{
   CONFIG_ENTRY *p;
   char section_name[256];
   int in_section;

   prettify_section_name(section, section_name, sizeof(section_name));

   if (conf) {
      p = conf->head;

      /* If section is NULL, only initial, section-less entries are used. */
      if (ugetc(section_name))
         in_section = FALSE;
      else
         in_section = TRUE;

      while (p) {
         if (p->name) {
            /* a section start is just a list entry enclosed in [] */
            if (ugetc(p->name) == '[' && ugetat(p->name, -1) == ']') {
               if (list_sections) {
                  n = add_unique_name(names, n, p->name);
               }
               in_section = (ustricmp(section_name, p->name) == 0);
            }
            else if (in_section && !list_sections) {
               n = add_unique_name(names, n, p->name);
            }
         }
         p = p->next;
      }
   }
   return n;
}



/* list_config_entires:
 *  Returns the names of all config entries in a section. The names parameter is
 *  a pointer to a strings array that will contain the config keys. If it points to
 *  a NULL pointer, it will be allocated, or else re-allocated accordingly. The
 *  return value tells how many valid string pointers it contains after the
 *  function returns.
 */
int list_config_entries(AL_CONST char *section, AL_CONST char ***names)
{
   int n = 0;
   n = attach_config_entries(config_override, section, n, names, 0);
   n = attach_config_entries(config[0], section, n, names, 0);
   return n;
}



/* list_config_sections:
 *  Returns the names of all current config sections, enclodes in []. The names
 *  parameter and return value is like in list_config_entires above.
 */
int list_config_sections(AL_CONST char ***names)
{
   int n = 0;
   n = attach_config_entries(config_override, NULL, n, names, 1);
   n = attach_config_entries(config[0], NULL, n, names, 1);
   return n;
}



/* free_config_entries:
 *  Frees the entries list returned by list_config_entires or
 *  list_config_sections again.
 */
void free_config_entries(AL_CONST char ***names)
{
    _AL_FREE(*names);
    *names = NULL;
}
