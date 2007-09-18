/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window, box1, box2, button_ok, button_cancel;
  JWidget label1, label2, combobox1, combobox2;
  char buf[256];
  int c;

  /* Allegro stuff */
  allegro_init ();
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  /* Jinete initialization */
  manager = jmanager_new ();
  ji_set_standard_theme ();

  /* create main window */
  window = jwindow_new ("Combo-boxes");
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  label1 = jlabel_new ("Non-editable");
  label2 = jlabel_new ("Editable");
  combobox1 = jcombobox_new ();
  combobox2 = jcombobox_new ();
  button_ok = jbutton_new ("&OK");
  button_cancel = jbutton_new ("&Cancel");

  jwidget_expansive (label1, TRUE);
  jwidget_expansive (label2, TRUE);

  for (c=0; c<16; c++) {
    usprintf(buf, "Option %d", c);
    jcombobox_add_string(combobox1, buf);
  }

  jcombobox_add_string (combobox2, "First");
  jcombobox_add_string (combobox2, "Second");
  jcombobox_add_string (combobox2, "Third");
  jcombobox_add_string (combobox2, "Fourth");
  jcombobox_add_string (combobox2, "Fifth");
  jcombobox_add_string (combobox2, "Sixth");
  jcombobox_editable (combobox2, TRUE);

  jwidget_add_child (window, box1);
  jwidget_add_childs (box1, label1, combobox1, label2, combobox2, box2, 0);
  jwidget_add_childs (box2, button_ok, button_cancel, 0);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
