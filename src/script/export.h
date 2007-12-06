/*===================================================================*/
/* Base routines                                                     */
/*===================================================================*/

double MAX(double x, double y);
double MIN(double x, double y);
double MID(double x, double y, double z);

void include(const char *filename);
void dofile(const char *filename);
void print(const char *buf);

double rand(double min, double max); /* CODE */

/* string routines */

const char *_(const char *msgid);

int strcmp(const char *s1, const char *s2);

/* math routines */

#define PI

double fabs(double x);
double ceil(double x);
double floor(double x);

double exp(double x);
double log(double x);
double log10(double x);
double pow(double x, double y);
double sqrt(double x);
double hypot(double x, double y);

double cos(double x);
double sin(double x);
double tan(double x);
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);

double cosh(double x);
double sinh(double x);
double tanh(double x);
/* TODO these routines aren't in MinGW math library */
/* double acosh (double x); */
/* double asinh (double x); */
/* double atanh (double x); */

/* file routines */

bool file_exists(const char *filename);
char *get_filename(const char *filename);

/*===================================================================*/
/* Miscellaneous routines                                            */
/*===================================================================*/

/* console/console.c */

void do_progress(int progress);
void add_progress(int max);
void del_progress(void);

/* core/core.c */

bool is_interactive(void);

/* core/app.c */

void app_refresh_screen(void);

int app_get_current_image_type(void);

JWidget app_get_top_window(void);
JWidget app_get_menu_bar(void);
JWidget app_get_status_bar(void);
JWidget app_get_color_bar(void);
JWidget app_get_tool_bar(void);

/* intl/intl.c */

void intl_load_lang(void);
const char *intl_get_lang(void);
void intl_set_lang(const char *lang);

/* modules/rootmenu.c */

void show_fx_popup_menu(void);

/* Brush *get_brush (void); */
int get_brush_type(void);
int get_brush_size(void);
int get_brush_angle(void);
int get_brush_mode(void);
int get_glass_dirty(void);
int get_spray_width(void);
int get_air_speed(void);
bool get_filled_mode(void);
bool get_tiled_mode(void);
bool get_use_grid(void);
bool get_view_grid(void);
JRect get_grid(void);
bool get_onionskin(void);

void set_brush_type(int type);
void set_brush_size(int size);
void set_brush_angle(int angle);
void set_brush_mode(int mode);
void set_glass_dirty(int glass_dirty);
void set_spray_width(int spray_width);
void set_air_speed(int air_speed);
void set_filled_mode(bool status);
void set_tiled_mode(bool status);
void set_use_grid(bool status);
void set_view_grid(bool status);
void set_grid(JRect rect);
void set_onionskin(bool status);

/* modules/tools2.c */

void SetBrush(const char *string);
void SetDrawMode(const char *string);
void ToolTrace(const char *string);

/* modules/color.c */

const char *get_fg_color(void);
const char *get_bg_color(void);
void set_fg_color(const char *string);
void set_bg_color(const char *string);

int get_color_for_image(int image_type, const char *color);
char *image_getpixel_color(Image *image, int x, int y);

/* modules/sprites.c */

/* JList get_sprite_list (void); */
Sprite *get_first_sprite(void);
Sprite *get_next_sprite(Sprite *sprite);

Sprite *get_clipboard_sprite(void);
void set_clipboard_sprite(Sprite *sprite);

void sprite_mount(Sprite *sprite);
void sprite_unmount(Sprite *sprite);
void set_current_sprite(Sprite *sprite);
void sprite_show(Sprite *sprite);

void sprite_generate_mask_boundaries(Sprite *sprite);

/* file/file.c */

Sprite *sprite_load(const char *filename);
int sprite_save(Sprite *sprite);

char *get_readable_extensions(void);
char *get_writable_extensions(void);

/* modules/editors.c */

void set_current_editor(JWidget editor);

void update_editors_with_sprite(Sprite *sprite);
void editors_draw_sprite(Sprite *sprite, int x1, int y1, int x2, int y2);
void replace_sprite_in_editors(Sprite *old_sprite, Sprite *new_sprite);

void set_sprite_in_current_editor(Sprite *sprite);
void set_sprite_in_more_reliable_editor(Sprite *sprite);

void split_editor(JWidget editor, int align);
void close_editor(JWidget editor);
void make_unique_editor(JWidget editor);

/* core/recent.c */

void recent_file(const char *filename);
void unrecent_file(const char *filename);

/* modules/palette.c */

void use_current_sprite_rgb_map(void);
void use_sprite_rgb_map(Sprite *sprite);
void restore_rgb_map(void);

/* util/autocrop.c */

void autocrop_sprite(void);

/* util/canvasze.c */

void canvas_resize(void);

/* util/clipbrd.c */

void cut_to_clipboard(void);
void copy_to_clipboard(void);
void paste_from_clipboard(void);

/* util/crop.c */

void crop_sprite(void);
void crop_layer(void);
void crop_cel(void);

/* util/celmove.c */

void set_cel_to_handle(Layer *layer, Cel *cel);

void move_cel(void);
void copy_cel(void);
void link_cel(void);

/* util/mapgen.c */

void mapgen(Image *image, int seed, float fractal_factor);

/* util/misc.c */

Image *GetImage(void);
Image *GetImage2(Sprite *sprite, int *x, int *y, int *opacity); /* CODE */

void LoadPalette(const char *filename);

void ClearMask(void);
Layer *NewLayerFromMask(Sprite *src_sprite, Sprite *dst_sprite);

/* util/msk_file.c */

Mask *load_msk_file(const char *filename);
int save_msk_file(Mask *mask, const char *filename);

/* util/quantize.c */

void sprite_quantize(Sprite *sprite);

/* modules/gui.c */

void update_screen_for_sprite(Sprite *sprite);

void rebuild_root_menu(void);
void rebuild_sprite_list(void);
void rebuild_recent_list(void);

/* dialogs/quick.c */

void quick_move(void);
void quick_copy(void);
void quick_swap(void);

/* dialogs/playfli.c */

void play_fli_animation(const char *filename, bool loop, bool fullscreen);

/* dialogs/... */

void dialogs_draw_text(void);
void switch_between_film_and_sprite_editor(void);
void dialogs_mapgen(void);
void dialogs_mask_color(void);
void dialogs_options(void);
void dialogs_palette_editor(void);
void dialogs_screen_saver(void);
void dialogs_select_language(bool force);
void dialogs_tips(bool forced);
void dialogs_vector_map(void);

/* dialogs/drawtext.c */

Image *RenderText(const char *fontname, int size, int color, const char *text);

/*===================================================================*/
/* Graphics routines                                                 */
/*===================================================================*/

/* GfxObj ***********************************************************/

#define GFXOBJ_IMAGE
#define GFXOBJ_STOCK
#define GFXOBJ_CEL
#define GFXOBJ_LAYER_IMAGE
#define GFXOBJ_LAYER_SET
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

int _rgba_getr(int c);
int _rgba_getg(int c);
int _rgba_getb(int c);
int _rgba_geta(int c);
int _rgba(int r, int g, int b, int a);

#define _graya_k_shift
#define _graya_a_shift

int _graya_getk(int c);
int _graya_geta(int c);
int _graya(int k, int a);

#define IMAGE_RGB
#define IMAGE_GRAYSCALE
#define IMAGE_INDEXED
#define IMAGE_BITMAP

Image *image_new(int imgtype, int w, int h);
Image *image_new_copy(Image *image);
void image_free(Image *image);

int image_getpixel(Image *image, int x, int y);
void image_putpixel(Image *image, int x, int y, int color);

void image_clear(Image *image, int color);

void image_copy(Image *dst, Image *src, int x, int y);
void image_merge(Image *dst, Image *src, int x, int y, int opacity, int blend_mode);
Image *image_crop(Image *image, int x, int y, int w, int h);

void image_hline(Image *image, int x1, int y, int x2, int color);
void image_vline(Image *image, int x, int y1, int y2, int color);
void image_rect(Image *image, int x1, int y1, int x2, int y2, int color);
void image_rectfill(Image *image, int x1, int y1, int x2, int y2, int color);
void image_line(Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipse(Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipsefill(Image *image, int x1, int y1, int x2, int y2, int color);
			
void image_convert(Image *dst, Image *src);
int image_count_diff(Image *i1, Image *i2);

/* rotate */

void image_scale(Image *dst, Image *src, int x, int y, int w, int h);
void image_rotate(Image *dst, Image *src, int x, int y, int w, int h, int cx, int cy, double angle);
void image_parallelogram(Image *bmp, Image *sprite, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

/* Cel ************************************************************/

Cel *cel_new(int frame, int image);
Cel *cel_new_copy(Cel *cel);
void cel_free(Cel *cel);

Cel *cel_is_link(Cel *cel, Layer *layer);
 
void cel_set_frame(Cel *cel, int frame);
void cel_set_image(Cel *cel, int image);
void cel_set_position(Cel *cel, int x, int y);
void cel_set_opacity(Cel *cel, int opacity);

/* Layer ************************************************************/

Layer *layer_new(Sprite *sprite);
Layer *layer_set_new(Sprite *sprite);
/* Layer *layer_text_new(const char *text); */
Layer *layer_new_copy(Layer *layer);
void layer_free(Layer *layer);

bool layer_is_image(Layer *layer);
bool layer_is_set(Layer *layer);

Layer *layer_get_prev(Layer *layer);
Layer *layer_get_next(Layer *layer);

void layer_set_name(Layer *layer, const char *name);
void layer_set_blend_mode(Layer *layer, int blend_mode);
/* void layer_set_parent(Layer *layer, GfxObj *gfxobj); */

/* for LAYER_IMAGE */
void layer_add_cel(Layer *layer, Cel *cel);
void layer_remove_cel(Layer *layer, Cel *cel);
Cel *layer_get_cel(Layer *layer, int frame);

/* for LAYER_SET */
void layer_add_layer(Layer *set, Layer *layer);
void layer_remove_layer(Layer *set, Layer *layer);
void layer_move_layer(Layer *set, Layer *layer, Layer *after);

void layer_render(Layer *layer, Image *image, int x, int y, int frame);

Layer *layer_flatten(Layer *layer, int x, int y, int w, int h, int frmin, int frmax);

/* Mask *************************************************************/

Mask *mask_new(void);
Mask *mask_new_copy(Mask *mask);
void mask_free(Mask *mask);

bool mask_is_empty(Mask *mask);
void mask_set_name(Mask *mask, const char *name);

void mask_move(Mask *mask, int x, int y);
void mask_none(Mask *mask);
void mask_invert(Mask *mask);
void mask_replace(Mask *mask, int x, int y, int w, int h);
void mask_union(Mask *mask, int x, int y, int w, int h);
void mask_subtract(Mask *mask, int x, int y, int w, int h);
void mask_intersect(Mask *mask, int x, int y, int w, int h);

void mask_merge(Mask *dst, Mask *src);
void mask_by_color(Mask *mask, Image *image, int color, int fuzziness);
void mask_crop(Mask *mask, Image *image);

/* Path *************************************************************/

#define PATH_JOIN_MITER
#define PATH_JOIN_ROUND
#define PATH_JOIN_BEVEL

#define PATH_CAP_BUTT
#define PATH_CAP_ROUND
#define PATH_CAP_SQUARE

Path *path_new(const char *name);
void path_free(Path *path);

void path_set_join(Path *path, int join);
void path_set_cap(Path *path, int cap);

void path_moveto(Path *path, double x, double y);
void path_lineto(Path *path, double x, double y);
void path_curveto(Path *path, double control_x1, double control_y1, double control_x2, double control_y2, double end_x, double end_y);
void path_close(Path *path);

void path_move(Path *path, double x, double y);

void path_stroke(Path *path, Image *image, int color, double brush_size);
void path_fill(Path *path, Image *image, int color);

/* Sprite ***********************************************************/

#define DITHERING_NONE
#define DITHERING_ORDERED

Sprite *sprite_new(int imgtype, int w, int h);
Sprite *sprite_new_copy(Sprite *sprite);
Sprite *sprite_new_flatten_copy(Sprite *sprite);
Sprite *sprite_new_with_layer(int imgtype, int w, int h);
void sprite_free(Sprite *sprite);

bool sprite_is_modified(Sprite *sprite);
bool sprite_is_associated_to_file(Sprite *sprite);
void sprite_mark_as_saved(Sprite *sprite);

void sprite_set_filename(Sprite *sprite, const char *filename);
void sprite_set_size(Sprite *sprite, int w, int h);
void sprite_set_frames(Sprite *sprite, int frames);
void sprite_set_frlen(Sprite *sprite, int msecs, int frame);
int sprite_get_frlen(Sprite *sprite, int frame);
void sprite_set_speed(Sprite *sprite, int speed);
void sprite_set_path(Sprite *sprite, Path *path);
void sprite_set_mask(Sprite *sprite, Mask *mask);
void sprite_set_layer(Sprite *sprite, Layer *layer);
void sprite_set_frame(Sprite *sprite, int frame);
void sprite_set_imgtype(Sprite *sprite, int imgtype, int dithering_method);

void sprite_add_path(Sprite *sprite, Path *path);
void sprite_remove_path(Sprite *sprite, Path *path);

void sprite_add_mask(Sprite *sprite, Mask *mask);
void sprite_remove_mask(Sprite *sprite, Mask *mask);
Mask *sprite_request_mask(Sprite *sprite, const char *name);

void sprite_render(Sprite *sprite, Image *image, int x, int y);

/* Stock ************************************************************/

Stock *stock_new(int imgtype);
Stock *stock_new_copy(Stock *stock);
void stock_free(Stock *stock);

int stock_add_image(Stock *stock, Image *image);
void stock_remove_image(Stock *stock, Image *image);
void stock_replace_image(Stock *stock, int index, Image *image);

Image *stock_get_image(Stock *stock, int index);

/* Undo *************************************************************/

Undo *undo_new(Sprite *sprite);
void undo_free(Undo *undo);

void undo_enable(Undo *undo);
void undo_disable(Undo *undo);

bool undo_is_enabled(Undo *undo);
bool undo_is_disabled(Undo *undo);

bool undo_can_undo(Undo *undo);
bool undo_can_redo(Undo *undo);

void undo_undo(Undo *undo);
void undo_redo(Undo *undo);

void undo_open(Undo *undo);
void undo_close(Undo *undo);
/* void undo_data(Undo *undo, GfxObj *gfxobj, void *data, int size); */
void undo_image(Undo *undo, Image *image, int x, int y, int w, int h);
void undo_flip(Undo *undo, Image *image, int x1, int y1, int x2, int y2, int horz);
/* void undo_dirty(Undo *undo, Dirty *dirty); */
void undo_add_image(Undo *undo, Stock *stock, Image *image);
void undo_remove_image(Undo *undo, Stock *stock, Image *image);
void undo_replace_image(Undo *undo, Stock *stock, int index);
void undo_add_cel(Undo *undo, Layer *layer, Cel *cel);
void undo_remove_cel(Undo *undo, Layer *layer, Cel *cel);
void undo_add_layer(Undo *undo, Layer *set, Layer *layer);
void undo_remove_layer(Undo *undo, Layer *layer);
void undo_move_layer(Undo *undo, Layer *layer);
void undo_set_layer(Undo *undo, Sprite *sprite);
void undo_set_mask(Undo *undo, Sprite *sprite);

/*===================================================================*/
/* Effect routines                                                   */
/*===================================================================*/

Effect *effect_new(Sprite *sprite, const char *name);
void effect_free(Effect *effect);

void effect_load_target(Effect *effect);
void effect_set_target(Effect *effect, bool r, bool g, bool b, bool k, bool a, bool index);
void effect_set_target_rgb(Effect *effect, bool r, bool g, bool b, bool a);
void effect_set_target_grayscale(Effect *effect, bool k, bool a);
void effect_set_target_indexed(Effect *effect, bool r, bool g, bool b, bool index);

void effect_begin(Effect *effect);
void effect_begin_for_preview(Effect *effect);
int effect_apply_step(Effect *effect);

void effect_apply(Effect *effect);
void effect_flush(Effect *effect);

void effect_apply_to_image(Effect *effect, Image *image, int x, int y);
void effect_apply_to_target(Effect *effect);

/* effect/colcurve.c */

#define CURVE_LINEAR
#define CURVE_SPLINE

CurvePoint *curve_point_new(int x, int y);
void curve_point_free(CurvePoint *point);

Curve *curve_new(int type);
void curve_free(Curve *curve);
void curve_add_point(Curve *curve, CurvePoint *point);
void curve_remove_point(Curve *curve, CurvePoint *point);

void set_color_curve(Curve *curve);

/* effect/convmatr.c */

ConvMatr *convmatr_new(int w, int h);
ConvMatr *convmatr_new_string(const char *format);
void convmatr_free(ConvMatr *convmatr);

void set_convmatr(ConvMatr *convmatr);
ConvMatr *get_convmatr(void);
ConvMatr *get_convmatr_by_name(const char *name);

/* void reload_matrices_stock(void); */
/* void clean_matrices_stock(void); */
/* JList get_convmatr_stock(void); */

/* effect/replcol.c */

void set_replace_colors(int from, int to, int fuzziness);

/* effect/median.c */

void set_median_size(int w, int h);

/*===================================================================*/
/* End                                                               */
/*===================================================================*/
