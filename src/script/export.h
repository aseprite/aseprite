/*===================================================================*/
/* Base routines                                                     */
/*===================================================================*/

double MAX (double x, double y);
double MIN (double x, double y);
double MID (double x, double y, double z);

void include (const char *filename);
void dofile (const char *filename);
void print (const char *buf);

double rand (double min, double max); /* CODE */

/* string routines */

const char *_ (const char *msgid);

int strcmp (const char *s1, const char *s2);

/* math routines */

#define PI

double fabs (double x);
double ceil (double x);
double floor (double x);

double exp (double x);
double log (double x);
double log10 (double x);
double pow (double x, double y);
double sqrt (double x);
double hypot (double x, double y);

double cos (double x);
double sin (double x);
double tan (double x);
double acos (double x);
double asin (double x);
double atan (double x);
double atan2 (double y, double x);

double cosh (double x);
double sinh (double x);
double tanh (double x);
/* XXX these routines aren't in MinGW math library */
/* double acosh (double x); */
/* double asinh (double x); */
/* double atanh (double x); */

/* file routines */

bool file_exists (const char *filename);
char *get_filename (const char *filename);

/* configuration routines */

int get_config_int (const char *section, const char *name, int value);
void set_config_int (const char *section, const char *name, int value);

const char *get_config_string (const char *section, const char *name, const char *value);
void set_config_string (const char *section, const char *name, const char *value);

float get_config_float (const char *section, const char *name, float value);
void set_config_float (const char *section, const char *name, float value);

bool get_config_bool (const char *section, const char *name, bool value);
void set_config_bool (const char *section, const char *name, bool value);

void get_config_rect (const char *section, const char *name, JRect rect);
void set_config_rect (const char *section, const char *name, JRect rect);

/*===================================================================*/
/* Miscellaneous routines                                            */
/*===================================================================*/

/* console/console.c */

void do_progress (int progress);
void add_progress (int max);
void del_progress (void);

/* core/core.c */

bool is_interactive (void);

/* core/app.c */

void app_refresh_screen (void);

int app_get_current_image_type (void);

JWidget app_get_top_window (void);
JWidget app_get_menu_bar (void);
JWidget app_get_status_bar (void);
JWidget app_get_color_bar (void);
JWidget app_get_tool_bar (void);

void app_switch (JWidget widget);

/* intl/intl.c */

void intl_load_lang (void);
const char *intl_get_lang (void);
void intl_set_lang (const char *lang);

/* modules/rootmenu.c */

void show_filters_popup_menu (void);

/* modules/tools.c */

void select_tool (const char *tool_name);

/* Brush *get_brush (void); */
int get_brush_type (void);
int get_brush_size (void);
int get_brush_angle (void);
int get_brush_mode (void);
int get_glass_dirty (void);
int get_spray_width (void);
int get_air_speed (void);
bool get_filled_mode (void);
bool get_tiled_mode (void);
bool get_use_grid (void);
bool get_view_grid (void);
JRect get_grid (void);
bool get_onionskin (void);

void set_brush_type (int type);
void set_brush_size (int size);
void set_brush_angle (int angle);
void set_brush_mode (int mode);
void set_glass_dirty (int glass_dirty);
void set_spray_width (int spray_width);
void set_air_speed (int air_speed);
void set_filled_mode (bool status);
void set_tiled_mode (bool status);
void set_use_grid (bool status);
void set_view_grid (bool status);
void set_grid (JRect rect);
void set_onionskin (bool status);

/* modules/tools2.c */

void SetBrush (const char *string);
void SetDrawMode (const char *string);
void ToolTrace (const char *string);

void ResetConfig (void);
void RestoreConfig (void);

/* modules/color.c */

const char *get_fg_color (void);
const char *get_bg_color (void);
void set_fg_color (const char *string);
void set_bg_color (const char *string);

int get_color_for_image (int image_type, const char *color);
char *image_getpixel_color (Image *image, int x, int y);

/* modules/sprites.c */

/* JList get_sprite_list (void); */
Sprite *get_first_sprite (void);
Sprite *get_next_sprite (Sprite *sprite);

Sprite *get_clipboard_sprite (void);
void set_clipboard_sprite (Sprite *sprite);

void sprite_mount (Sprite *sprite);
void sprite_unmount (Sprite *sprite);
void set_current_sprite (Sprite *sprite);
void sprite_show (Sprite *sprite);

void sprite_generate_mask_boundaries (Sprite *sprite);

/* file/file.c */

Sprite *sprite_load (const char *filename);
int sprite_save (Sprite *sprite);

char *get_readable_extensions (void);
char *get_writeable_extensions (void);

/* modules/editors.c */

void set_current_editor (JWidget editor);

void update_editors_with_sprite(Sprite *sprite);
void editors_draw_sprite(Sprite *sprite, int x1, int y1, int x2, int y2);
void replace_sprite_in_editors(Sprite *old_sprite, Sprite *new_sprite);

void set_sprite_in_current_editor (Sprite *sprite);
void set_sprite_in_more_reliable_editor (Sprite *sprite);

void split_editor (JWidget editor, int align);
void close_editor (JWidget editor);
void make_unique_editor (JWidget editor);

/* core/recent.c */

void recent_file (const char *filename);
void unrecent_file (const char *filename);

/* modules/palette.c */

void use_current_sprite_rgb_map (void);
void use_sprite_rgb_map (Sprite *sprite);
void restore_rgb_map (void);

/* util/autocrop.c */

void autocrop_sprite (void);

/* util/canvasze.c */

void canvas_resize (void);

/* util/clipbrd.c */

void cut_to_clipboard (void);
void copy_to_clipboard (void);
void paste_from_clipboard (void);

/* util/crop.c */

void crop_sprite (void);
void crop_layer (void);
void crop_frame (void);

/* util/flip.c */

void flip_horizontal (void);
void flip_vertical (void);

/* util/frame.c */

void set_frame_to_handle (Layer *layer, Frame *frame);

void new_frame (void);
void move_frame (void);
void copy_frame (void);
void link_frame (void);

/* util/mapgen.c */

void mapgen (Image *image, int seed, float fractal_factor);

/* util/misc.c */

Image *GetImage (void);
Image *GetImage2 (Sprite *sprite, int *x, int *y, int *opacity); /* CODE */

void LoadPalette (const char *filename);

void ClearMask (void);
Layer *NewLayerFromMask (void);

/* util/msk_file.c */

Mask *load_msk_file (const char *filename);
int save_msk_file (Mask *mask, const char *filename);

/* util/quantize.c */

void sprite_quantize (Sprite *sprite);

/* util/session.c */

bool load_session (const char *filename);
bool save_session (const char *filename);

bool is_backup_session (void);

/* modules/gui.c */

void GUI_Refresh (Sprite *sprite);

void rebuild_root_menu (void);
void rebuild_sprite_list (void);
void rebuild_recent_list (void);

/* dialogs/quick.c */

void quick_move (void);
void quick_copy (void);
void quick_swap (void);

/* dialogs/playfli.c */

void play_fli_animation (const char *filename, bool loop, bool fullscreen);

/* util/recscr.c */

bool is_rec_screen (void);
void rec_screen_on (void);
void rec_screen_off (void);

/* util/scrshot.c */

void screen_shot (void);

/* util/setgfx.c */

int set_gfx (const char *card, int w, int h, int depth);

/* dialogs/... */

void GUI_About (void);
void GUI_ColorCurve (void);
void GUI_ConvolutionMatrix (void);
void GUI_DrawText (void);
void switch_between_film_and_sprite_editor (void);
void GUI_FrameLength (int frpos);
void GUI_InvertColor (void);
void GUI_MapGen (void);
void GUI_MaskColor (void);
void GUI_MaskRepository (void);
void GUI_MedianFilter (void);
void GUI_Options (void);
void show_palette_editor (void);
void GUI_ReplaceColor (void);
void GUI_ScreenSaver (void);
void GUI_SelectLanguage (bool force);
void GUI_Tips (bool forced);
void GUI_ToolsConfiguration (void);
void GUI_VectorMap (void);

/* dialogs/view.c */

void view_tiled (void);
void view_normal (void);
void view_fullscreen (void);

/* dialogs/drawtext.c */

Image *RenderText (const char *fontname, int size, int color, const char *text);

/*===================================================================*/
/* Graphics routines                                                 */
/*===================================================================*/

/* GfxObj ***********************************************************/

#define GFXOBJ_IMAGE
#define GFXOBJ_STOCK
#define GFXOBJ_FRAME
#define GFXOBJ_LAYER_IMAGE
#define GFXOBJ_LAYER_SET
#define GFXOBJ_LAYER_TEXT
#define GFXOBJ_SPRITE
#define GFXOBJ_MASK
#define GFXOBJ_PATH
#define GFXOBJ_UNDO

/* Blend ************************************************************/

#define BLEND_MODE_NORMAL
#define BLEND_MODE_DISSOLVE
#define BLEND_MODE_MULTIPLY
#define BLEND_MODE_SCREEN
#define BLEND_MODE_OVERLAY
#define BLEND_MODE_HARD_LIGHT
#define BLEND_MODE_DODGE
#define BLEND_MODE_BURN
#define BLEND_MODE_DARKEN
#define BLEND_MODE_LIGHTEN
#define BLEND_MODE_ADDITION
#define BLEND_MODE_SUBTRACT
#define BLEND_MODE_DIFFERENCE
#define BLEND_MODE_HUE
#define BLEND_MODE_SATURATION
#define BLEND_MODE_COLOR
#define BLEND_MODE_LUMINOSITY

/* Image ************************************************************/

#define _rgba_r_shift
#define _rgba_g_shift
#define _rgba_b_shift
#define _rgba_a_shift

int _rgba_getr (int c);
int _rgba_getg (int c);
int _rgba_getb (int c);
int _rgba_geta (int c);
int _rgba (int r, int g, int b, int a);

#define _graya_k_shift
#define _graya_a_shift

int _graya_getk (int c);
int _graya_geta (int c);
int _graya (int k, int a);

#define IMAGE_RGB
#define IMAGE_GRAYSCALE
#define IMAGE_INDEXED
#define IMAGE_BITMAP

Image *image_new (int imgtype, int w, int h);
Image *image_new_copy (Image *image);
void image_free (Image *image);

int image_getpixel (Image *image, int x, int y);
void image_putpixel (Image *image, int x, int y, int color);

void image_clear (Image *image, int color);

void image_copy (Image *dst, Image *src, int x, int y);
void image_merge (Image *dst, Image *src, int x, int y, int opacity, int blend_mode);
Image *image_crop (Image *image, int x, int y, int w, int h);

void image_hline (Image *image, int x1, int y, int x2, int color);
void image_vline (Image *image, int x, int y1, int y2, int color);
void image_rect (Image *image, int x1, int y1, int x2, int y2, int color);
void image_rectfill (Image *image, int x1, int y1, int x2, int y2, int color);
void image_line (Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipse (Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipsefill (Image *image, int x1, int y1, int x2, int y2, int color);
			
void image_convert (Image *dst, Image *src);
int image_count_diff (Image *i1, Image *i2);

/* rotate */

void image_scale (Image *dst, Image *src, int x, int y, int w, int h);
void image_rotate (Image *dst, Image *src, int x, int y, int w, int h, int cx, int cy, double angle);
void image_parallelogram (Image *bmp, Image *sprite, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

/* Frame ************************************************************/

Frame *frame_new (int frpos, int image);
Frame *frame_new_copy (Frame *frame);
void frame_free (Frame *frame);

Frame *frame_is_link (Frame *frame, Layer *layer);
 
void frame_set_frpos (Frame *frame, int frpos);
void frame_set_image (Frame *frame, int image);
void frame_set_position (Frame *frame, int x, int y);
void frame_set_opacity (Frame *frame, int opacity);

/* Layer ************************************************************/

Layer *layer_new (int imgtype);
Layer *layer_set_new (void);
/* Layer *layer_text_new (const char *text); */
Layer *layer_new_copy (Layer *layer);
Layer *layer_new_with_image (int imgtype, int x, int y, int w, int h, int frpos);
void layer_free (Layer *layer);

bool layer_is_image (Layer *layer);
bool layer_is_set (Layer *layer);

Layer *layer_get_prev (Layer *layer);
Layer *layer_get_next (Layer *layer);

void layer_set_name (Layer *layer, const char *name);
void layer_set_blend_mode (Layer *layer, int blend_mode);
/* void layer_set_parent (Layer *layer, GfxObj *gfxobj); */

/* for LAYER_IMAGE */
void layer_add_frame (Layer *layer, Frame *frame);
void layer_remove_frame (Layer *layer, Frame *frame);
Frame *layer_get_frame (Layer *layer, int frpos);

/* for LAYER_SET */
void layer_add_layer (Layer *set, Layer *layer);
void layer_remove_layer (Layer *set, Layer *layer);
void layer_move_layer (Layer *set, Layer *layer, Layer *after);

void layer_render (Layer *layer, Image *image, int x, int y, int frpos);

Layer *layer_flatten (Layer *layer, int imgtype, int x, int y, int w, int h, int frmin, int frmax);

/* Mask *************************************************************/

Mask *mask_new (void);
Mask *mask_new_copy (Mask *mask);
void mask_free (Mask *mask);

bool mask_is_empty (Mask *mask);
void mask_set_name (Mask *mask, const char *name);

void mask_move (Mask *mask, int x, int y);
void mask_none (Mask *mask);
void mask_invert (Mask *mask);
void mask_replace (Mask *mask, int x, int y, int w, int h);
void mask_union (Mask *mask, int x, int y, int w, int h);
void mask_subtract (Mask *mask, int x, int y, int w, int h);
void mask_intersect (Mask *mask, int x, int y, int w, int h);

void mask_merge (Mask *dst, Mask *src);
void mask_by_color (Mask *mask, Image *image, int color, int fuzziness);
void mask_crop (Mask *mask, Image *image);

/* Path *************************************************************/


#define PATH_JOIN_MITER
#define PATH_JOIN_ROUND
#define PATH_JOIN_BEVEL

#define PATH_CAP_BUTT
#define PATH_CAP_ROUND
#define PATH_CAP_SQUARE

Path *path_new (const char *name);
void path_free (Path *path);

void path_set_join (Path *path, int join);
void path_set_cap (Path *path, int cap);

void path_moveto (Path *path, double x, double y);
void path_lineto (Path *path, double x, double y);
void path_curveto (Path *path, double control_x1, double control_y1, double control_x2, double control_y2, double end_x, double end_y);
void path_close (Path *path);

void path_move (Path *path, double x, double y);

void path_stroke (Path *path, Image *image, int color, double brush_size);
void path_fill (Path *path, Image *image, int color);

/* Sprite ***********************************************************/

#define DITHERING_NONE
#define DITHERING_ORDERED

Sprite *sprite_new (int imgtype, int w, int h);
Sprite *sprite_new_copy (Sprite *sprite);
Sprite *sprite_new_flatten_copy (Sprite *sprite);
Sprite *sprite_new_with_layer (int imgtype, int w, int h);
void sprite_free (Sprite *sprite);

bool sprite_is_modified (Sprite *sprite);
void sprite_was_saved (Sprite *sprite);

void sprite_set_filename (Sprite *sprite, const char *filename);
void sprite_set_size (Sprite *sprite, int w, int h);
void sprite_set_frames (Sprite *sprite, int frames);
void sprite_set_frlen (Sprite *sprite, int msecs, int frpos);
int sprite_get_frlen (Sprite *sprite, int frpos);
void sprite_set_speed (Sprite *sprite, int speed);
void sprite_set_path (Sprite *sprite, Path *path);
void sprite_set_mask (Sprite *sprite, Mask *mask);
void sprite_set_layer (Sprite *sprite, Layer *layer);
void sprite_set_frpos (Sprite *sprite, int frpos);
void sprite_set_imgtype (Sprite *sprite, int imgtype, int dithering_method);

void sprite_add_path (Sprite *sprite, Path *path);
void sprite_remove_path (Sprite *sprite, Path *path);

void sprite_add_mask (Sprite *sprite, Mask *mask);
void sprite_remove_mask (Sprite *sprite, Mask *mask);
Mask *sprite_request_mask (Sprite *sprite, const char *name);

void sprite_render (Sprite *sprite, Image *image, int x, int y);

/* Stock ************************************************************/

Stock *stock_new (int imgtype);
Stock *stock_new_copy (Stock *stock);
void stock_free (Stock *stock);

int stock_add_image (Stock *stock, Image *image);
void stock_remove_image (Stock *stock, Image *image);
void stock_replace_image (Stock *stock, int index, Image *image);

Image *stock_get_image (Stock *stock, int index);

/* Undo *************************************************************/

Undo *undo_new (Sprite *sprite);
void undo_free (Undo *undo);

void undo_enable (Undo *undo);
void undo_disable (Undo *undo);

bool undo_is_enabled (Undo *undo);
bool undo_is_disabled (Undo *undo);

bool undo_can_undo (Undo *undo);
bool undo_can_redo (Undo *undo);

void undo_undo (Undo *undo);
void undo_redo (Undo *undo);

void undo_open (Undo *undo);
void undo_close (Undo *undo);
/* void undo_data (Undo *undo, GfxObj *gfxobj, void *data, int size); */
void undo_image (Undo *undo, Image *image, int x, int y, int w, int h);
void undo_flip (Undo *undo, Image *image, int x1, int y1, int x2, int y2, int horz);
/* void undo_dirty (Undo *undo, Dirty *dirty); */
void undo_add_image (Undo *undo, Stock *stock, Image *image);
void undo_remove_image (Undo *undo, Stock *stock, Image *image);
void undo_replace_image (Undo *undo, Stock *stock, int index);
void undo_add_frame (Undo *undo, Layer *layer, Frame *frame);
void undo_remove_frame (Undo *undo, Layer *layer, Frame *frame);
void undo_add_layer (Undo *undo, Layer *set, Layer *layer);
void undo_remove_layer (Undo *undo, Layer *layer);
void undo_move_layer (Undo *undo, Layer *layer);
void undo_set_layer (Undo *undo, Sprite *sprite);
void undo_set_mask (Undo *undo, Sprite *sprite);

/*===================================================================*/
/* Effect routines                                                   */
/*===================================================================*/

Effect *effect_new (Sprite *sprite, const char *name);
void effect_free (Effect *effect);

void effect_load_target (Effect *effect);
void effect_set_target (Effect *effect, bool r, bool g, bool b, bool k, bool a, bool index);
void effect_set_target_rgb (Effect *effect, bool r, bool g, bool b, bool a);
void effect_set_target_grayscale (Effect *effect, bool k, bool a);
void effect_set_target_indexed (Effect *effect, bool r, bool g, bool b, bool index);

void effect_begin (Effect *effect);
void effect_begin_for_preview (Effect *effect);
int effect_apply_step (Effect *effect);

void effect_apply (Effect *effect);
void effect_flush (Effect *effect);

void effect_apply_to_image (Effect *effect, Image *image, int x, int y);
void effect_apply_to_target (Effect *effect);

/* effect/colcurve.c */

#define CURVE_LINEAR
#define CURVE_SPLINE

CurvePoint *curve_point_new (int x, int y);
void curve_point_free (CurvePoint *point);

Curve *curve_new (int type);
void curve_free (Curve *curve);
void curve_add_point (Curve *curve, CurvePoint *point);
void curve_remove_point (Curve *curve, CurvePoint *point);

void set_color_curve (Curve *curve);

/* effect/convmatr.c */

ConvMatr *convmatr_new (int w, int h);
ConvMatr *convmatr_new_string (const char *format);
void convmatr_free (ConvMatr *convmatr);

void set_convmatr (ConvMatr *convmatr);
ConvMatr *get_convmatr (void);
ConvMatr *get_convmatr_by_name (const char *name);

/* void reload_matrices_stock (void); */
/* void clean_matrices_stock (void); */
/* JList get_convmatr_stock (void); */

/* effect/replcol.c */

void set_replace_colors (int from, int to, int fuzziness);

/* effect/median.c */

void set_median_size (int w, int h);

/*===================================================================*/
/* GUI routines                                                      */
/*===================================================================*/

void reload_default_font (void);

void load_window_pos (JWidget window, const char *section);
void save_window_pos (JWidget window, const char *section);

JWidget ji_load_widget (const char *filename, const char *name); /* CODE */

char *ji_file_select (const char *message, const char *init_path, const char *exts); /* CODE */
char *ji_color_select (int imgtype, const char *color); /* CODE */

/* Alert ************************************************************/

JWidget jalert_new (const char *format);
int jalert (const char *format);

/* Base *************************************************************/

#define JI_HORIZONTAL
#define JI_VERTICAL
#define JI_LEFT
#define JI_CENTER
#define JI_RIGHT
#define JI_TOP
#define JI_MIDDLE
#define JI_BOTTOM
#define JI_HOMOGENEOUS
#define JI_WORDWRAP

#define JI_HIDDEN
#define JI_SELECTED
#define JI_DISABLED
#define JI_HASFOCUS
#define JI_HASMOUSE
#define JI_HASCAPTURE
#define JI_FOCUSREST
#define JI_MAGNETIC
#define JI_EXPANSIVE
#define JI_DECORATIVE
#define JI_AUTODESTROY
#define JI_HARDCAPTURE
#define JI_INITIALIZED

#define JI_WIDGET
#define JI_BOX
#define JI_BUTTON
#define JI_CHECK
#define JI_COMBOBOX
#define JI_ENTRY
#define JI_IMAGE
#define JI_LABEL
#define JI_LISTBOX
#define JI_LISTITEM
#define JI_MANAGER
#define JI_MENU
#define JI_MENUBAR
#define JI_MENUBOX
#define JI_MENUITEM
#define JI_PANEL
#define JI_RADIO
#define JI_SEPARATOR
#define JI_SLIDER
#define JI_TEXTBOX
#define JI_VIEW
#define JI_VIEW_SCROLLBAR
#define JI_VIEW_VIEWPORT
#define JI_WINDOW
#define JI_USER_WIDGET

#define JM_OPEN
#define JM_CLOSE
#define JM_DESTROY
#define JM_DRAW
#define JM_IDLE
#define JM_SIGNAL
#define JM_REQSIZE
#define JM_SETPOS
#define JM_WINMOVE
#define JM_DRAWRGN
#define JM_DIRTYCHILDREN
#define JM_CHAR
#define JM_KEYPRESSED
#define JM_KEYRELEASED
#define JM_FOCUSENTER
#define JM_FOCUSLEAVE
#define JM_BUTTONPRESSED
#define JM_BUTTONRELEASED
#define JM_DOUBLECLICK
#define JM_MOUSEENTER
#define JM_MOUSELEAVE
#define JM_MOTION
#define JM_WHEEL

#define JI_SIGNAL_DIRTY
#define JI_SIGNAL_ENABLE
#define JI_SIGNAL_DISABLE
#define JI_SIGNAL_SELECT
#define JI_SIGNAL_DESELECT
#define JI_SIGNAL_SHOW
#define JI_SIGNAL_HIDE
#define JI_SIGNAL_ADD_CHILD
#define JI_SIGNAL_REMOVE_CHILD
#define JI_SIGNAL_NEW_PARENT
#define JI_SIGNAL_GET_TEXT
#define JI_SIGNAL_SET_TEXT
#define JI_SIGNAL_BUTTON_SELECT
#define JI_SIGNAL_CHECK_CHANGE
#define JI_SIGNAL_RADIO_CHANGE
#define JI_SIGNAL_ENTRY_CHANGE
#define JI_SIGNAL_LISTBOX_CHANGE
#define JI_SIGNAL_LISTBOX_SELECT
#define JI_SIGNAL_MANAGER_EXTERNAL_CLOSE
#define JI_SIGNAL_MANAGER_ADD_WINDOW
#define JI_SIGNAL_MANAGER_REMOVE_WINDOW
#define JI_SIGNAL_MANAGER_LOSTCHAR
#define JI_SIGNAL_MENUITEM_SELECT
#define JI_SIGNAL_SLIDER_CHANGE
#define JI_SIGNAL_WINDOW_CLOSE
#define JI_SIGNAL_WINDOW_RESIZE

/* Box **************************************************************/

JWidget jbox_new (int align);

/* Button ***********************************************************/

JWidget jbutton_new (const char *text);

/* void jbutton_set_icon (JWidget button, struct BITMAP *icon); */
/* void jbutton_set_icon_align (JWidget button, int icon_align); */

/* void jbutton_set_bevel (JWidget button, int b0, int b1, int b2, int b3); */
/* void jbutton_get_bevel (JWidget button, int *b4); */

/* Check ************************************************************/

JWidget jcheck_new (const char *text);

/* void jcheck_set_icon_align (JWidget check, int icon_align); */

/* Combobox *********************************************************/

JWidget jcombobox_new (void);

void jcombobox_editable (JWidget combobox, bool state);
void jcombobox_clickopen (JWidget combobox, bool state);
void jcombobox_casesensitive (JWidget combobox, bool state);

bool jcombobox_is_editable (JWidget combobox);
bool jcombobox_is_clickopen (JWidget combobox);
bool jcombobox_is_casesensitive (JWidget combobox);

void jcombobox_add_string (JWidget combobox, const char *string);
void jcombobox_del_string (JWidget combobox, const char *string);
void jcombobox_del_index (JWidget combobox, int index);

void jcombobox_select_index (JWidget combobox, int index);
void jcombobox_select_string (JWidget combobox, const char *string);
int jcombobox_get_selected_index (JWidget combobox);
const char *jcombobox_get_selected_string (JWidget combobox);

const char *jcombobox_get_string (JWidget combobox, int index);
int jcombobox_get_index (JWidget combobox, const char *string);
int jcombobox_get_count (JWidget combobox);

JWidget jcombobox_get_entry_widget (JWidget combobox);

/* Clipboard ********************************************************/

const char *jclipboard_get_text (void);
void jclipboard_set_text (const char *text);

/* Entry ************************************************************/

JWidget jentry_new (int maxsize, const char *format);

void jentry_readonly (JWidget entry, bool state);
void jentry_password (JWidget entry, bool state);
bool jentry_is_password (JWidget entry);
bool jentry_is_readonly (JWidget entry);

void jentry_show_cursor (JWidget entry);
void jentry_hide_cursor (JWidget entry);

void jentry_set_cursor_pos (JWidget entry, int pos);
void jentry_select_text (JWidget entry, int from, int to);
void jentry_deselect_text (JWidget entry);

/* Image ************************************************************/

/* JWidget ji_image_new (struct BITMAP *bmp, int align); */

/* Label ************************************************************/

JWidget jlabel_new (const char *text);

/* Listbox **********************************************************/

JWidget jlistbox_new (void);
JWidget jlistitem_new (const char *text);

JWidget jlistbox_get_selected_child (JWidget listbox);
int jlistbox_get_selected_index (JWidget listbox);
void jlistbox_select_child (JWidget listbox, JWidget listitem);
void jlistbox_select_index (JWidget listbox, int index);

void jlistbox_center_scroll (JWidget listbox);

/* Manager **********************************************************/

JWidget ji_get_default_manager (void);

JWidget jmanager_new (void);
void jmanager_free (JWidget manager);

void jmanager_run (JWidget manager);
bool jmanager_poll (JWidget manager, bool all_windows);

void jmanager_send_message (JMessage msg);
void jmanager_dispatch_messages (void);
void jmanager_dispatch_draw_messages (void);

JWidget jmanager_get_focus (void);
JWidget jmanager_get_mouse (void);
JWidget jmanager_get_capture (void);

void jmanager_set_focus (JWidget widget);
void jmanager_set_mouse (JWidget widget);
void jmanager_set_capture (JWidget widget);
void jmanager_attract_focus (JWidget widget);
void jmanager_free_focus (void);
void jmanager_free_mouse (void);
void jmanager_free_capture (void);
void jmanager_free_widget (JWidget widget);
void jmanager_remove_message (JMessage msg);
void jmanager_remove_messages_for (JWidget widget);
void jmanager_refresh_screen (void);

/* Menu *************************************************************/

JWidget jmenu_new (void);
JWidget jmenubar_new (void);
JWidget jmenubox_new (void);
JWidget jmenuitem_new (const char *text);

JWidget jmenubox_get_menu (JWidget menubox);
JWidget jmenubar_get_menu (JWidget menubar);
JWidget jmenuitem_get_submenu (JWidget menuitem);
/* JAccel jmenuitem_get_accel (JWidget menuitem); */

void jmenubox_set_menu (JWidget menubox, JWidget menu);
void jmenubar_set_menu (JWidget menubar, JWidget menu);
void jmenuitem_set_submenu (JWidget menuitem, JWidget menu);
/* void jmenuitem_set_accel (JWidget menuitem, JAccel accel); */

int jmenuitem_is_highlight (JWidget menuitem);

void jmenu_popup (JWidget menu, int x, int y);

/* Message **********************************************************/

JMessage jmessage_new (int type);
JMessage jmessage_new_copy (JMessage msg);
void jmessage_free (JMessage msg);

void jmessage_add_dest (JMessage msg, JWidget widget);

void jmessage_broadcast_to_children (JMessage msg, JWidget widget);
void jmessage_broadcast_to_parents (JMessage msg, JWidget widget);

void jmessage_set_sub_msg (JMessage msg, JMessage sub_msg);

/* Panel ************************************************************/

JWidget ji_panel_new (int align);

double ji_panel_get_pos (JWidget panel);
void ji_panel_set_pos (JWidget panel, double pos);

/* QuickMenu ********************************************************/

/* JWidget jmenubar_new_quickmenu (JQuickMenu quick_menu); */
/* JWidget jmenubox_new_quickmenu (JQuickMenu quick_menu); */

/* Radio ************************************************************/

JWidget jradio_new (const char *text, int radio_group);

/* void jradio_set_icon_align (JWidget radio, int icon_align); */

int jradio_get_group (JWidget radio);
void jradio_deselect_group (JWidget radio);

/* Rect *************************************************************/

int jrect_w (JRect rect);
int jrect_h (JRect rect);
bool jrect_point_in (JRect rect, int x, int y);

JRect jrect_new (int x1, int y1, int x2, int y2);
JRect jrect_new_copy (const JRect rect);
void jrect_free (JRect rect);

void jrect_copy (JRect dst, const JRect src);
void jrect_replace (JRect rect, int x1, int y1, int x2, int y2);

void jrect_union (JRect r1, const JRect r2);
bool jrect_intersect (JRect r1, const JRect r2);

void jrect_shrink (JRect rect, int border);
void jrect_stretch (JRect rect, int border);

void jrect_moveto (JRect rect, int x, int y);
void jrect_displace (JRect rect, int dx, int dy);

/* Region ***********************************************************/

JRegion jregion_new (JRect rect, int size);
void jregion_init (JRegion reg, JRect rect, int size);
void jregion_free (JRegion reg);
void jregion_uninit (JRegion reg);

bool jregion_copy (JRegion dst, JRegion src);
bool jregion_intersect (JRegion new, JRegion reg1, JRegion reg2);
bool jregion_union (JRegion new, JRegion reg1, JRegion reg2);
bool jregion_append (JRegion dstrgn, JRegion rgn);
/* bool jregion_validate (JRegion badreg, bool *overlap); */

/* JRegion jrects_to_region (int nrects, JRect *prect, int ctype); */

bool jregion_subtract (JRegion regD, JRegion regM, JRegion regS);
bool jregion_inverse (JRegion newReg, JRegion reg1, JRect invRect);

int jregion_rect_in (JRegion region, JRect rect);
void jregion_translate (JRegion reg, int x, int y);

void jregion_reset (JRegion reg, JRect box);
bool jregion_break (JRegion reg);
bool jregion_point_in (JRegion reg, int x, int y, JRect box);

bool jregion_equal (JRegion reg1, JRegion reg2);
bool jregion_notempty (JRegion reg);
void jregion_empty (JRegion reg);
JRect jregion_extents (JRegion reg);

/* Separator ********************************************************/

JWidget ji_separator_new (const char *text, int align);

/* Slider ***********************************************************/

JWidget jslider_new (int min, int max, int value);

void jslider_set_value (JWidget slider, int value);
int jslider_get_value (JWidget slider);

/* System ***********************************************************/

/* Textbox **********************************************************/

JWidget jtextbox_new (const char *text, int align);

/* Theme ************************************************************/

/* View *************************************************************/

JWidget jview_new (void);

bool jview_has_bars (JWidget view);

void jview_attach (JWidget view, JWidget viewable_widget);
void jview_maxsize (JWidget view);
void jview_without_bars (JWidget view);

void jview_set_size (JWidget view, int w, int h);
void jview_set_scroll (JWidget view, int x, int y);
/* void jview_get_scroll (JWidget view, int *x, int *y); */
/* void jview_get_max_size (JWidget view, int *w, int *h); */

void jview_update (JWidget view);

JWidget jview_get_viewport (JWidget view);
JRect jview_get_viewport_position (JWidget view);

JWidget jwidget_get_view (JWidget viewable_widget);

/* Widget ***********************************************************/

int ji_register_widget_type (void);

JWidget jwidget_new (int type);
void jwidget_free (JWidget widget);

int jwidget_get_type (JWidget widget);
const char *jwidget_get_name (JWidget widget);
const char *jwidget_get_text (JWidget widget);
int jwidget_get_align (JWidget widget);

void jwidget_set_name (JWidget widget, const char *name);
void jwidget_set_text (JWidget widget, const char *text);
void jwidget_set_align (JWidget widget, int align);

void jwidget_magnetic (JWidget widget, bool state);
void jwidget_expansive (JWidget widget, bool state);
void jwidget_decorative (JWidget widget, bool state);
void jwidget_autodestroy (JWidget widget, bool state);
void jwidget_focusrest (JWidget widget, bool state);

bool jwidget_is_magnetic (JWidget widget);
bool jwidget_is_expansive (JWidget widget);
bool jwidget_is_decorative (JWidget widget);
bool jwidget_is_autodestroy (JWidget widget);
bool jwidget_is_focusrest (JWidget widget);

void jwidget_dirty (JWidget widget);
void jwidget_show (JWidget widget);
void jwidget_hide (JWidget widget);
void jwidget_enable (JWidget widget);
void jwidget_disable (JWidget widget);
void jwidget_select (JWidget widget);
void jwidget_deselect (JWidget widget);

bool jwidget_is_visible (JWidget widget);
bool jwidget_is_hidden (JWidget widget);
bool jwidget_is_enabled (JWidget widget);
bool jwidget_is_disabled (JWidget widget);
bool jwidget_is_selected (JWidget widget);
bool jwidget_is_deselected (JWidget widget);

bool jwidget_has_focus (JWidget widget);
bool jwidget_has_mouse (JWidget widget);
bool jwidget_has_capture (JWidget widget);

void jwidget_add_child (JWidget widget, JWidget child);
void jwidget_remove_child (JWidget widget, JWidget child);
void jwidget_replace_child (JWidget widget, JWidget old_child, JWidget new_child);

JWidget jwidget_get_parent (JWidget widget);
JWidget jwidget_get_window (JWidget widget);
/* JList jwidget_get_parents (JWidget widget, bool ascendant); */
/* JList jwidget_get_children (JWidget widget); */
JWidget jwidget_pick (JWidget widget, int x, int y);
bool jwidget_has_child (JWidget widget, JWidget child);

JRect jwidget_get_rect (JWidget widget);
JRect jwidget_get_child_rect (JWidget widget);
JRegion jwidget_get_region (JWidget widget);
JRegion jwidget_get_drawable_region (JWidget widget, int flags);
int jwidget_get_bg_color (JWidget widget);
int jwidget_get_text_length (JWidget widget);
int jwidget_get_text_height (JWidget widget);

void jwidget_noborders (JWidget widget);
void jwidget_set_border (JWidget widget, int l, int t, int r, int b);
void jwidget_set_rect (JWidget widget, JRect rect);
void jwidget_set_static_size (JWidget widget, int w, int h);
void jwidget_set_bg_color (JWidget widget, int color);

void jwidget_flush_redraw (JWidget widget);
void jwidget_redraw_region (JWidget widget, JRegion region);

void jwidget_signal_on (JWidget widget);
void jwidget_signal_off (JWidget widget);

int jwidget_emit_signal (JWidget widget, int signal);

bool jwidget_send_message (JWidget widget, JMessage msg);
bool jwidget_send_message_after_type (JWidget widget, JMessage msg, int type);
void jwidget_close_window (JWidget widget);
void jwidget_capture_mouse (JWidget widget);
void jwidget_hard_capture_mouse (JWidget widget);
void jwidget_release_mouse (JWidget widget);

JWidget jwidget_find_name (JWidget widget, const char *name);
bool jwidget_check_underscored (JWidget widget, int scancode);

/* Window routines **************************************************/

JWidget jwindow_new (const char *text);
JWidget jwindow_new_desktop (void);

JWidget jwindow_get_killer (JWidget window);
JWidget jwindow_get_manager (JWidget window);

void jwindow_moveable (JWidget window, bool state);
void jwindow_sizeable (JWidget window, bool state);
void jwindow_ontop (JWidget window, bool state);

void jwindow_remap (JWidget window);
void jwindow_center (JWidget window);
void jwindow_position (JWidget window, int x, int y);
void jwindow_move (JWidget window, JRect rect);

void jwindow_open (JWidget window);
void jwindow_open_fg (JWidget window);
void jwindow_open_bg (JWidget window);
void jwindow_close (JWidget window, JWidget killer);

bool jwindow_is_toplevel (JWidget window);
bool jwindow_is_foreground (JWidget window);
bool jwindow_is_desktop (JWidget window);
bool jwindow_is_ontop (JWidget window);

/*===================================================================*/
/* End                                                               */
/*===================================================================*/
