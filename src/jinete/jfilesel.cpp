/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <errno.h>
#include <string.h>
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX || defined ALLEGRO_DJGPP || defined ALLEGRO_MINGW32
#  include <sys/stat.h>
#endif
#if defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX || defined ALLEGRO_MINGW32
#  include <sys/unistd.h>
#endif

#include "jinete/jinete.h"

#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
#  define HAVE_DRIVES
#endif

#undef FA_ALL
#define FA_TO_SHOW   FA_RDONLY | FA_DIREC | FA_ARCH | FA_SYSTEM
#define FA_ALL       FA_RDONLY | FA_DIREC | FA_ARCH | FA_HIDDEN | FA_SYSTEM

/* extension type */
enum {
  EQUAL_TO_TEXT,
  START_WITH_TEXT,
  END_WITH_TEXT,
};

typedef struct Extension
{
  int type;
  int size;
  char *text;
} Extension;

static JWidget listbox1, listbox2, entry_path, entry_name;
static JList paths, files;

static char current_path[1024];
static JList extensions;	/* list of supported Extensions */

static bool filesel_msg_proc(JWidget entry, JMessage msg);
static bool enter_to_path_in_entry();

static void fill_listbox_callback(const char* filename, int attrib, int param);
static void fill_listbox_with_files(char* path, int size);

static void generate_extensions(const char *exts);
static bool check_extension(const char *filename_ext);
static void free_extensions();

static void fixup_filename(char *buf, char *from, const char *filename);
static int my_ustrfilecmp(const char *s1, const char *s2);
static int my_ustrnicmp(AL_CONST char *s1, AL_CONST char *s2, int n);

static int filesel_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

char *ji_file_select(const char *message,
		     const char *init_path,
		     const char *exts)
{
  return ji_file_select_ex(message, init_path, exts, NULL);
}

/* customizable widgets: 
   widget_extension can have childs with these widget's names:
      "path"
      "name"
      "left"
      "top"
      "right"
      "bottom"
 */
char *ji_file_select_ex(const char *message,
			const char *init_path,
			const char *exts,
			JWidget widget_extension)
{
  JWidget box1, box2, box3, box4, panel, view1, view2;
  JWidget button_select, button_cancel, tmp;
  int size = 1024;
  char buf[1024];
  JLink link;
  char *s;
  char *selected_filename;
  bool entry_path_created;
  bool entry_name_created;

  generate_extensions(exts);

  ustrcpy(current_path, init_path);
  ustrcpy(get_filename(current_path), empty_string);

  ustrcpy(buf, init_path);

  Frame* window = new Frame(false, message);
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  box4 = jbox_new(JI_VERTICAL | JI_EXPANSIVE);
  panel = jpanel_new(JI_HORIZONTAL);
  view1 = jview_new();
  view2 = jview_new();
  listbox1 = jlistbox_new();
  listbox2 = jlistbox_new();
  button_select = jbutton_new(ji_translate_string("&OK"));
  button_cancel = jbutton_new(ji_translate_string("&Cancel"));

  /* create input widget (entry_path) */
  entry_path = NULL;
  entry_name = NULL;
  entry_path_created = false;
  entry_name_created = false;

  if (widget_extension) {
    entry_path = jwidget_find_name(widget_extension, "path");
    entry_name = jwidget_find_name(widget_extension, "name");
  }

  if (!entry_path) {
    entry_path = jentry_new(size-1, NULL);
    entry_path_created = true;
  }

  if (!entry_name) {
    entry_name = jentry_new(size-1, NULL);
    entry_name_created = true;
  }

  /* add hook */
  jwidget_add_hook(button_select, filesel_type(), filesel_msg_proc, NULL);
  jwidget_add_hook(entry_path, filesel_type(), filesel_msg_proc, NULL);
  jwidget_add_hook(entry_name, filesel_type(), filesel_msg_proc, NULL);
  jwidget_add_hook(listbox1, filesel_type(), filesel_msg_proc, listbox2);
  jwidget_add_hook(listbox2, filesel_type(), filesel_msg_proc, listbox1);

  /* attach the list-box to the view */
  jview_attach(view1, listbox1);
  jview_attach(view2, listbox2);

  jwidget_expansive(view1, true);
  jwidget_expansive(view2, true);
  jwidget_expansive(panel, true);
  jwidget_expansive(box2, true);

  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "top")))
    jwidget_add_child(box1, tmp);
  if (entry_path_created)
    jwidget_add_child(box1, entry_path);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "left")))
    jwidget_add_child(box2, tmp);
  jwidget_add_child(panel, view1);
  jwidget_add_child(panel, view2);
  if (!entry_name_created)
    jwidget_add_child(box2, panel);
  else {
    JWidget subbox1 = jbox_new(JI_VERTICAL);
    JWidget subbox2 = jbox_new(JI_HORIZONTAL);
    jwidget_expansive(entry_name, true);
    jwidget_add_child(box2, subbox1);
    jwidget_add_child(subbox1, panel);
    jwidget_add_child(subbox1, subbox2);
    jwidget_add_child(subbox2, new Label(ji_translate_string("File name:")));
    jwidget_add_child(subbox2, entry_name);
  }
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "right")))
    jwidget_add_child(box2, tmp);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box3, box4);
  jwidget_add_child(box3, button_select);
  jwidget_add_child(box3, button_cancel);
  jwidget_add_child(box1, box3);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "bottom")))
    jwidget_add_child(box1, tmp);
  jwidget_add_child(window, box1);

  /* magnetic */
  jwidget_magnetic(button_select, true);

  /* setup window size */
  jwidget_set_min_size(panel, JI_SCREEN_W*9/10, JI_SCREEN_H*3/5);
  window->remap_window();
  window->center_window();
/*   jwidget_set_static_size(panel, JI_SCREEN_W*9/10, JI_SCREEN_H*3/5); */
/*   jwidget_set_static_size(panel, 0, 0); */

  /* fill the listbox with the files in the current directory */
  fill_listbox_with_files(buf, size);

  /* put the current directory in the entry */
  jlistbox_select_child(listbox1, NULL);

  s = get_filename(buf);
  if (s > buf) *(s-1) = 0;
  entry_path->setText(buf);
  entry_name->setText(s);

  /* select the filename */
  JI_LIST_FOR_EACH(listbox2->children, link) {
    if (ustrcmp(s, (reinterpret_cast<JWidget>(link->data))->getText()) == 0) {
      jlistbox_select_child(listbox2, reinterpret_cast<JWidget>(link->data));
      break;
    }
  }
  jlistbox_center_scroll(listbox1);
  jlistbox_center_scroll(listbox2);

  /* open and run */
  window->open_window_fg();

  /* button "select"? */
  if ((window->get_killer() == button_select) ||
      (window->get_killer() == listbox2) ||
      (window->get_killer() == entry_path) ||
      (window->get_killer() == entry_name)) {
    /* copy the new name */
    selected_filename = (char*)jmalloc(1024);

    ustrcpy(selected_filename, entry_path->getText());
    put_backslash(selected_filename);
    ustrcat(selected_filename, entry_name->getText());

    fix_filename_slashes(selected_filename);
  }
  else {
    /* nothing selected */
    selected_filename = NULL;
  }

  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "path")))
    jwidget_remove_child(tmp->parent, tmp);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "name")))
    jwidget_remove_child(tmp->parent, tmp);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "left")))
    jwidget_remove_child(tmp->parent, tmp);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "top")))
    jwidget_remove_child(tmp->parent, tmp);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "right")))
    jwidget_remove_child(tmp->parent, tmp);
  if (widget_extension && (tmp = jwidget_find_name(widget_extension, "bottom")))
    jwidget_remove_child(tmp->parent, tmp);

  jwidget_free(window);
  free_extensions();

  return selected_filename;
}

char *ji_file_select_get_current_path()
{
  return current_path;
}

void ji_file_select_refresh_listbox()
{
  fill_listbox_with_files(current_path, sizeof(current_path));
}

void ji_file_select_enter_to_path(const char *path)
{
  entry_path->setText(path);
  entry_name->setText("");
  enter_to_path_in_entry();
}

bool ji_dir_exists(const char *filename)
{
  struct al_ffblk info;
  char buf[1024];
  int ret;

  ustrcpy(buf, filename);
  put_backslash(buf);
  ustrcat(buf, "*.*");

  ret = al_findfirst(buf, &info, FA_ALL);
  al_findclose(&info);

  return (ret == 0);
}

static bool filesel_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      switch (msg->signal.num) {

	case JI_SIGNAL_BUTTON_SELECT:
	  if (!enter_to_path_in_entry())
	    jwidget_close_window(widget);
	  return true;

	case JI_SIGNAL_LISTBOX_SELECT: {
	  char filename[1024];
	  int attr;
	  enter_to_path_in_entry();
	  jmanager_set_focus(widget);

	  /* close the window when we double-click in a normal file */
	  ustrcpy(filename, entry_path->getText());
	  put_backslash(filename);
	  ustrcat(filename, entry_name->getText());

	  if (file_exists(filename, FA_ALL, &attr)) {
	    if (!(attr & FA_DIREC))
	      jwidget_close_window(widget);
	  }
	  break;
	}

	case JI_SIGNAL_LISTBOX_CHANGE: {
	  JWidget the_other_listbox = reinterpret_cast<JWidget>
	    (jwidget_get_data(widget, filesel_type()));

	  JWidget item = jlistbox_get_selected_child(widget);
	  const char *filename = item ? item->getText():
					empty_string;

	  jwidget_signal_off(the_other_listbox);
	  jlistbox_select_child(the_other_listbox, 0);
	  jwidget_signal_on(the_other_listbox);

	  entry_path->setText(current_path);
	  entry_name->setText(filename);
	  jentry_select_text(entry_name, 0, -1);
	  break;
	}

	case JI_SIGNAL_ENTRY_CHANGE: {
	  if (widget == entry_path) {
	    entry_name->setText("");
	  }
	  break;
	}
      }
      break;

    case JM_KEYPRESSED:
      if ((widget->hasFocus()) &&
	  ((msg->key.scancode == KEY_ENTER) ||
	   (msg->key.scancode == KEY_ENTER_PAD))) {
	if (enter_to_path_in_entry()) {
	  jmanager_set_focus(widget);
	  return true;
	}
	else {
	  jwidget_close_window(widget);
	  return true;
	}
      }
      break;
  }

  return false;
}

static bool enter_to_path_in_entry()
{
  char buf[1024], path[1024], from[1024];
  const char *filename;
  JLink link;

  jmanager_set_focus(entry_path);

  fix_filename_path(path, entry_path->getText(), sizeof(path));
  put_backslash(path);
  ustrcat(path, entry_name->getText());

  fixup_filename(buf, from, path);
  ustrcpy(path, buf);

#ifdef HAVE_DRIVES
  if (ugetat(buf, -1) == DEVICE_SEPARATOR) {
    uinsert(path, ustrlen(path), OTHER_PATH_SEPARATOR);
    ustrcat(buf, "./");
  }
  else if ((ugetat(buf, -1) == OTHER_PATH_SEPARATOR) &&
	   (ugetat(buf, -2) == DEVICE_SEPARATOR)) {
    uremove(buf, -1);
    ustrcat(buf, "./");
  }
#endif

  if (ji_dir_exists(buf)) {
    ustrcpy(buf, path);
    put_backslash(buf);

    ustrcpy(current_path, buf);

    /* make the new list of files */
    fill_listbox_with_files(buf, sizeof(buf));

    /* change the edit text */
    entry_path->setText(buf);
    jentry_select_text(entry_path, 0, -1);

    /* select the filename */
    if (!ugetat(from, 0))
      ustrcpy(from, "..");

    JI_LIST_FOR_EACH(listbox1->children, link) {
      filename = (reinterpret_cast<JWidget>(link->data))->getText();

#ifdef HAVE_DRIVES
      if (ustricmp(filename, from) == 0)
#else
      if (ustrcmp(filename, from) == 0)
#endif
	{
	  JWidget listitem = jlistbox_get_selected_child(listbox1);
	  if (listitem)
	    listitem->setSelected(false);
	  jlistbox_select_child(listbox1, reinterpret_cast<JWidget>(link->data));
	  jlistbox_center_scroll(listbox1);
	  break;
	}
    }
    return true;
  }
  else
    return false;
}

/* callback function for `for_each_file' routine */
static void fill_listbox_callback(const char* filename, int attrib, int param)
{
  const char *other_filename;
  JList *list;
  JLink link;
  char buf[1024];
  bool found;

  errno = 0;

  /* all directories */
  if (attrib & FA_DIREC) {
    list = &paths;
    found = true;
  }
  /* only files with an acceptable extension */
  else {
    list = &files;
    found = check_extension(get_extension(filename));
  }

  if (found) {
    ustrzcpy(buf, sizeof(buf), get_filename(filename));
    fix_filename_case(buf);

    if (attrib & FA_DIREC) {
      if (ustrcmp(buf, ".") == 0)
        return;
    }

    JI_LIST_FOR_EACH(*list, link) {
      other_filename = (reinterpret_cast<JWidget>(link->data))->getText();
      if (my_ustrfilecmp(buf, other_filename) < 0)
	break;
    }

    jlist_insert_before(*list, link, jlistitem_new(buf));
  }
}

static void fill_listbox_with_files(char* path, int size)
{
  char buf[1024], tmp[32];
  int ch, attr;
  JList list;
  JLink link;

  fix_filename_case(path);
  fix_filename_slashes(path);

  /* add the path */
  if (get_filename(path) == path) {
#ifdef HAVE_DRIVES
    int drive = _al_getdrive();
#else
    int drive = 0;
#endif
    _al_getdcwd(drive, path, size - ucwidth(OTHER_PATH_SEPARATOR));
  }

  ch = ugetat(path, -1);
  if ((ch != '/') && (ch != OTHER_PATH_SEPARATOR)) {
    if (file_exists(path, FA_ALL, &attr)) {
      if (attr & FA_DIREC)
        put_backslash(path);
    }
  }

  replace_filename(buf, path, uconvert_ascii("*.*", tmp), sizeof(buf));

  /* clean the list boxes */

  list = jwidget_get_children(listbox1);
  JI_LIST_FOR_EACH(list, link) {
    jmanager_free_widget(reinterpret_cast<JWidget>(link->data));
    jwidget_remove_child(listbox1, reinterpret_cast<JWidget>(link->data));
    jwidget_free(reinterpret_cast<JWidget>(link->data));
  }
  jlist_free(list);

  list = jwidget_get_children(listbox2);
  JI_LIST_FOR_EACH(list, link) {
    jmanager_free_widget(reinterpret_cast<JWidget>(link->data));
    jwidget_remove_child(listbox2,
			 reinterpret_cast<JWidget>(link->data));
    jwidget_free(reinterpret_cast<JWidget>(link->data));
  }
  jlist_free(list);

  /* fill the list boxes again */

  paths = jlist_new();
  files = jlist_new();

  for_each_file(buf, FA_TO_SHOW, fill_listbox_callback, 0);

  JI_LIST_FOR_EACH(paths, link) jwidget_add_child(listbox1, reinterpret_cast<JWidget>(link->data));
  JI_LIST_FOR_EACH(files, link) jwidget_add_child(listbox2, reinterpret_cast<JWidget>(link->data));

  jlist_free(paths);
  jlist_free(files);

  /* update viewports */

  jview_update(jwidget_get_view(listbox1));
  jview_update(jwidget_get_view(listbox2));

  /* select the first item */

  jlistbox_select_index(listbox1, 0);
  jview_set_scroll(jwidget_get_view(listbox2), 0, 0);
}

static void generate_extensions(const char *exts)
{
  char *tok, buf[1024];
  Extension *ext;

  extensions = jlist_new();

  ustrzcpy(buf, sizeof(buf), exts);

  for (tok = ustrtok(buf, ",;"); tok;
       tok = ustrtok(NULL, ",;")) {
    if (!*tok)
      continue;

    ext = jnew(Extension, 1);

    ext->text = jstrdup(tok);
    ext->size = ustrlen(ext->text);

    if (tok[0] == '*') {
      if (ext->size == 1) {
	jfree(ext);
	free_extensions();	/* all extensions */
	extensions = jlist_new(); /* empty */
	break;
      }
      else {
	ext->type = END_WITH_TEXT;
	uremove(ext->text, 0);
      }
    }
    else if (tok[ext->size-1] == '*') {
      ext->type = START_WITH_TEXT;
      uremove(ext->text, -1);
    }
    else
      ext->type = EQUAL_TO_TEXT;

    ext->size = ustrlen(ext->text);
    jlist_append(extensions, ext);
  }
}

static bool check_extension(const char *filename_ext)
{
  Extension *ext;
  JLink link;
  int len;

  ASSERT(extensions != NULL);

  if (jlist_empty(extensions))
    return true;		/* all extensions */

  len = ustrlen(filename_ext);

  JI_LIST_FOR_EACH(extensions, link) {
    ext = reinterpret_cast<Extension*>(link->data);

    switch (ext->type) {

      case EQUAL_TO_TEXT:
	if (ustricmp(ext->text, filename_ext) == 0)
	  return true;
	break;

      case START_WITH_TEXT:
	if (my_ustrnicmp(ext->text, filename_ext, ext->size) == 0)
	  return true;
	break;

      case END_WITH_TEXT:
	if (len >= ext->size &&
	    my_ustrnicmp(ext->text, filename_ext+len-ext->size, ext->size) == 0)
	  return true;
	break;
    }
  }

  return false;
}

static void free_extensions()
{
  Extension* ext;
  JLink link;

  JI_LIST_FOR_EACH(extensions, link) {
    ext = reinterpret_cast<Extension*>(link->data);

    if (ext->text)
      jfree(ext->text);

    jfree(ext);
  }

  jlist_free(extensions);
  extensions = NULL;
}

static void fixup_filename(char *buf, char *from, const char *filename)
{
  int c, ch;

  /* no `from' */
  ustrcpy(from, empty_string);

  /* copy the original string */
  ustrcpy(buf, filename);

  /* fixup some characters */
  fix_filename_case(buf);
  fix_filename_slashes(buf);

  /* remove all double-slashes "//" */
  for (c=0, ch=-1; ch; ) {
    ch = ugetat(buf, c);
    if ((ch == '/') || (ch == OTHER_PATH_SEPARATOR)) {
      ch = ugetat(buf, ++c);
      while ((ch == '/') || (ch == OTHER_PATH_SEPARATOR)) {
        uremove(buf, c);
        ch = ugetat(buf, c);
      }
    }
    else
      c++;
  }

  /* remove all "/." */
  for (c=0, ch=-1; ; ) {
    ch = ugetat(buf, c);

    if ((ch == '/') || (ch == OTHER_PATH_SEPARATOR)) {
      ch = ugetat(buf, c+2);
      if ((ugetat(buf, c+1) == '.') &&
          ((ch == 0) || (ch == '/') || (ch == OTHER_PATH_SEPARATOR))) {
        uremove(buf, c); /* `/' */
        uremove(buf, c); /* `.' */
        c--;
      }
    }

    if (!ch)
      break;
    else
      c++;
  }

  /* fixup all "dir/.." */
  for (c=0, ch=-1; ; ) {
    ch = ugetat(buf, c);

    if ((ch == '/') || (ch == OTHER_PATH_SEPARATOR)) {
      ch = ugetat(buf, c+3);
      if ((ugetat(buf, c+1) == '.') &&
	  (ugetat(buf, c+2) == '.') &&
          ((ch == 0) || (ch == '/') || (ch == OTHER_PATH_SEPARATOR))) {
        uremove(buf, c); /* `/' */
        uremove(buf, c); /* `.' */
        uremove(buf, c); /* `.' */

        /* remove the parent directory */

        for (ch=-1; ((c >= 0) && (ch != '/') && (ch != OTHER_PATH_SEPARATOR)); )
          ch = ugetat(buf, --c);

        c++;

        ustrcpy(from, empty_string);

        ch = ugetat(buf, c);
        for (; ((ch) && (ch != '/') && (ch != OTHER_PATH_SEPARATOR)); ) {
          uinsert(from, ustrlen(from), ch);

          uremove(buf, c);
          ch = ugetat(buf, c);
        }

        c--;

        if (c >= 0) {
          uremove(buf, c);
          ch = ugetat(buf, c--);
        }
      }
    }

    if (!ch)
      break;
    else
      c++;
  }
}

/* my_ustrfilecmp:
 *  ustricmp for filenames: makes sure that eg "foo.bar" comes before
 *  "foo-1.bar", and also that "foo9.bar" comes before "foo10.bar".
 */
static int my_ustrfilecmp(const char *s1, const char *s2)
{
   int c1, c2;
   int x1, x2;
   char *t1, *t2;

   for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if ((c1 >= '0') && (c1 <= '9') && (c2 >= '0') && (c2 <= '9')) {
         x1 = ustrtol(s1 - ucwidth(c1), &t1, 10);
         x2 = ustrtol(s2 - ucwidth(c2), &t2, 10);
         if (x1 != x2)
            return x1 - x2;
         else if (t1 - s1 != t2 - s2)
            return (t2 - s2) - (t1 - s1);
         s1 = t1;
         s2 = t2;
      }
      else if (c1 != c2) {
         if (!c1)
            return -1;
         else if (!c2)
            return 1;
         else if (c1 == '.')
            return -1;
         else if (c2 == '.')
            return 1;
         return c1 - c2;
      }

      if (!c1)
         return 0;
   }
}

/* ustrnicmp:
 *  Unicode-aware version of the DJGPP strnicmp() function.
 */
static int my_ustrnicmp(AL_CONST char *s1, AL_CONST char *s2, int n)
{
   int c1, c2;
/*    ASSERT(s1); */
/*    ASSERT(s2); */

   if (n <= 0)
      return 0;

   for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if (c1 != c2)
         return c1 - c2;

      if ((!c1) || (--n <= 0))
         return 0;
   }
}
