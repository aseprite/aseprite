/* Generated with bindings.gen */

/*======================================================================*/
/* C -> Lua                                                             */
/*======================================================================*/

#define GetArg(idx, var, cast, type)             \
  if (lua_isnil(L, idx) || lua_is##type(L, idx)) \
    var = (cast)lua_to##type(L, idx);            \
  else                                           \
    return 0;

#define GetUD(idx, var, type)                    \
  var = to_userdata(L, Type_##type, idx);

static int bind_MAX(lua_State *L)
{
  double return_value;
  double x;
  double y;
  GetArg(1, x, double, number);
  GetArg(2, y, double, number);
  return_value = MAX(x, y);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_MIN(lua_State *L)
{
  double return_value;
  double x;
  double y;
  GetArg(1, x, double, number);
  GetArg(2, y, double, number);
  return_value = MIN(x, y);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_MID(lua_State *L)
{
  double return_value;
  double x;
  double y;
  double z;
  GetArg(1, x, double, number);
  GetArg(2, y, double, number);
  GetArg(3, z, double, number);
  return_value = MID(x, y, z);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_include(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  include(filename);
  return 0;
}

static int bind_dofile(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  dofile(filename);
  return 0;
}

static int bind_print(lua_State *L)
{
  const char *buf;
  GetArg(1, buf, const char *, string);
  print(buf);
  return 0;
}

static int bind__(lua_State *L)
{
  const char *return_value;
  const char *msgid;
  GetArg(1, msgid, const char *, string);
  return_value = _(msgid);
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_strcmp(lua_State *L)
{
  int return_value;
  const char *s1;
  const char *s2;
  GetArg(1, s1, const char *, string);
  GetArg(2, s2, const char *, string);
  return_value = strcmp(s1, s2);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_fabs(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = fabs(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_ceil(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = ceil(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_floor(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = floor(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_exp(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = exp(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_log(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = log(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_log10(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = log10(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_pow(lua_State *L)
{
  double return_value;
  double x;
  double y;
  GetArg(1, x, double, number);
  GetArg(2, y, double, number);
  return_value = pow(x, y);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_sqrt(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = sqrt(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_hypot(lua_State *L)
{
  double return_value;
  double x;
  double y;
  GetArg(1, x, double, number);
  GetArg(2, y, double, number);
  return_value = hypot(x, y);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_cos(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = cos(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_sin(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = sin(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_tan(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = tan(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_acos(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = acos(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_asin(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = asin(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_atan(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = atan(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_atan2(lua_State *L)
{
  double return_value;
  double y;
  double x;
  GetArg(1, y, double, number);
  GetArg(2, x, double, number);
  return_value = atan2(y, x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_cosh(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = cosh(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_sinh(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = sinh(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_tanh(lua_State *L)
{
  double return_value;
  double x;
  GetArg(1, x, double, number);
  return_value = tanh(x);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_file_exists(lua_State *L)
{
  bool return_value;
  const char *filename;
  GetArg(1, filename, const char *, string);
  return_value = file_exists(filename);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_get_filename(lua_State *L)
{
  const char *return_value;
  const char *filename;
  GetArg(1, filename, const char *, string);
  return_value = get_filename(filename);
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_do_progress(lua_State *L)
{
  int progress;
  GetArg(1, progress, int, number);
  do_progress(progress);
  return 0;
}

static int bind_add_progress(lua_State *L)
{
  int max;
  GetArg(1, max, int, number);
  add_progress(max);
  return 0;
}

static int bind_del_progress(lua_State *L)
{
  del_progress();
  return 0;
}

static int bind_is_interactive(lua_State *L)
{
  bool return_value;
  return_value = is_interactive();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_app_refresh_screen(lua_State *L)
{
  app_refresh_screen();
  return 0;
}

static int bind_app_get_current_image_type(lua_State *L)
{
  int return_value;
  return_value = app_get_current_image_type();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_app_get_top_window(lua_State *L)
{
  JWidget return_value;
  return_value = app_get_top_window();
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

static int bind_app_get_menu_bar(lua_State *L)
{
  JWidget return_value;
  return_value = app_get_menu_bar();
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

static int bind_app_get_status_bar(lua_State *L)
{
  JWidget return_value;
  return_value = app_get_status_bar();
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

static int bind_app_get_color_bar(lua_State *L)
{
  JWidget return_value;
  return_value = app_get_color_bar();
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

static int bind_app_get_tool_bar(lua_State *L)
{
  JWidget return_value;
  return_value = app_get_tool_bar();
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

static int bind_intl_load_lang(lua_State *L)
{
  intl_load_lang();
  return 0;
}

static int bind_intl_get_lang(lua_State *L)
{
  const char *return_value;
  return_value = intl_get_lang();
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_intl_set_lang(lua_State *L)
{
  const char *lang;
  GetArg(1, lang, const char *, string);
  intl_set_lang(lang);
  return 0;
}

static int bind_show_fx_popup_menu(lua_State *L)
{
  show_fx_popup_menu();
  return 0;
}

static int bind_get_brush_type(lua_State *L)
{
  int return_value;
  return_value = get_brush_type();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_brush_size(lua_State *L)
{
  int return_value;
  return_value = get_brush_size();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_brush_angle(lua_State *L)
{
  int return_value;
  return_value = get_brush_angle();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_brush_mode(lua_State *L)
{
  int return_value;
  return_value = get_brush_mode();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_glass_dirty(lua_State *L)
{
  int return_value;
  return_value = get_glass_dirty();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_spray_width(lua_State *L)
{
  int return_value;
  return_value = get_spray_width();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_air_speed(lua_State *L)
{
  int return_value;
  return_value = get_air_speed();
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_filled_mode(lua_State *L)
{
  bool return_value;
  return_value = get_filled_mode();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_get_tiled_mode(lua_State *L)
{
  bool return_value;
  return_value = get_tiled_mode();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_get_use_grid(lua_State *L)
{
  bool return_value;
  return_value = get_use_grid();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_get_view_grid(lua_State *L)
{
  bool return_value;
  return_value = get_view_grid();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_get_grid(lua_State *L)
{
  JRect return_value;
  return_value = get_grid();
  push_userdata(L, Type_JRect, return_value);
  return 1;
}

static int bind_get_onionskin(lua_State *L)
{
  bool return_value;
  return_value = get_onionskin();
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_set_brush_type(lua_State *L)
{
  int type;
  GetArg(1, type, int, number);
  set_brush_type(type);
  return 0;
}

static int bind_set_brush_size(lua_State *L)
{
  int size;
  GetArg(1, size, int, number);
  set_brush_size(size);
  return 0;
}

static int bind_set_brush_angle(lua_State *L)
{
  int angle;
  GetArg(1, angle, int, number);
  set_brush_angle(angle);
  return 0;
}

static int bind_set_brush_mode(lua_State *L)
{
  int mode;
  GetArg(1, mode, int, number);
  set_brush_mode(mode);
  return 0;
}

static int bind_set_glass_dirty(lua_State *L)
{
  int glass_dirty;
  GetArg(1, glass_dirty, int, number);
  set_glass_dirty(glass_dirty);
  return 0;
}

static int bind_set_spray_width(lua_State *L)
{
  int spray_width;
  GetArg(1, spray_width, int, number);
  set_spray_width(spray_width);
  return 0;
}

static int bind_set_air_speed(lua_State *L)
{
  int air_speed;
  GetArg(1, air_speed, int, number);
  set_air_speed(air_speed);
  return 0;
}

static int bind_set_filled_mode(lua_State *L)
{
  bool status;
  GetArg(1, status, bool, boolean);
  set_filled_mode(status);
  return 0;
}

static int bind_set_tiled_mode(lua_State *L)
{
  bool status;
  GetArg(1, status, bool, boolean);
  set_tiled_mode(status);
  return 0;
}

static int bind_set_use_grid(lua_State *L)
{
  bool status;
  GetArg(1, status, bool, boolean);
  set_use_grid(status);
  return 0;
}

static int bind_set_view_grid(lua_State *L)
{
  bool status;
  GetArg(1, status, bool, boolean);
  set_view_grid(status);
  return 0;
}

static int bind_set_grid(lua_State *L)
{
  JRect rect;
  GetUD(1, rect, JRect);
  set_grid(rect);
  return 0;
}

static int bind_set_onionskin(lua_State *L)
{
  bool status;
  GetArg(1, status, bool, boolean);
  set_onionskin(status);
  return 0;
}

static int bind_SetBrush(lua_State *L)
{
  const char *string;
  GetArg(1, string, const char *, string);
  SetBrush(string);
  return 0;
}

static int bind_SetDrawMode(lua_State *L)
{
  const char *string;
  GetArg(1, string, const char *, string);
  SetDrawMode(string);
  return 0;
}

static int bind_ToolTrace(lua_State *L)
{
  const char *string;
  GetArg(1, string, const char *, string);
  ToolTrace(string);
  return 0;
}

static int bind_get_fg_color(lua_State *L)
{
  const char *return_value;
  return_value = get_fg_color();
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_get_bg_color(lua_State *L)
{
  const char *return_value;
  return_value = get_bg_color();
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_set_fg_color(lua_State *L)
{
  const char *string;
  GetArg(1, string, const char *, string);
  set_fg_color(string);
  return 0;
}

static int bind_set_bg_color(lua_State *L)
{
  const char *string;
  GetArg(1, string, const char *, string);
  set_bg_color(string);
  return 0;
}

static int bind_get_color_for_image(lua_State *L)
{
  int return_value;
  int image_type;
  const char *color;
  GetArg(1, image_type, int, number);
  GetArg(2, color, const char *, string);
  return_value = get_color_for_image(image_type, color);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_image_getpixel_color(lua_State *L)
{
  const char *return_value;
  Image *image;
  int x;
  int y;
  GetUD(1, image, Image);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  return_value = image_getpixel_color(image, x, y);
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_get_first_sprite(lua_State *L)
{
  Sprite *return_value;
  return_value = get_first_sprite();
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_get_next_sprite(lua_State *L)
{
  Sprite *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = get_next_sprite(sprite);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_get_clipboard_sprite(lua_State *L)
{
  Sprite *return_value;
  return_value = get_clipboard_sprite();
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_set_clipboard_sprite(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  set_clipboard_sprite(sprite);
  return 0;
}

static int bind_sprite_mount(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_mount(sprite);
  return 0;
}

static int bind_sprite_unmount(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_unmount(sprite);
  return 0;
}

static int bind_set_current_sprite(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  set_current_sprite(sprite);
  return 0;
}

static int bind_sprite_show(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_show(sprite);
  return 0;
}

static int bind_sprite_generate_mask_boundaries(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_generate_mask_boundaries(sprite);
  return 0;
}

static int bind_sprite_load(lua_State *L)
{
  Sprite *return_value;
  const char *filename;
  GetArg(1, filename, const char *, string);
  return_value = sprite_load(filename);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_sprite_save(lua_State *L)
{
  int return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = sprite_save(sprite);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_get_readable_extensions(lua_State *L)
{
  const char *return_value;
  return_value = get_readable_extensions();
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_get_writable_extensions(lua_State *L)
{
  const char *return_value;
  return_value = get_writable_extensions();
  lua_pushstring(L, return_value);
  return 1;
}

static int bind_set_current_editor(lua_State *L)
{
  JWidget editor;
  GetUD(1, editor, JWidget);
  set_current_editor(editor);
  return 0;
}

static int bind_update_editors_with_sprite(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  update_editors_with_sprite(sprite);
  return 0;
}

static int bind_editors_draw_sprite(lua_State *L)
{
  Sprite *sprite;
  int x1;
  int y1;
  int x2;
  int y2;
  GetUD(1, sprite, Sprite);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  editors_draw_sprite(sprite, x1, y1, x2, y2);
  return 0;
}

static int bind_replace_sprite_in_editors(lua_State *L)
{
  Sprite *old_sprite;
  Sprite *new_sprite;
  GetUD(1, old_sprite, Sprite);
  GetUD(2, new_sprite, Sprite);
  replace_sprite_in_editors(old_sprite, new_sprite);
  return 0;
}

static int bind_set_sprite_in_current_editor(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  set_sprite_in_current_editor(sprite);
  return 0;
}

static int bind_set_sprite_in_more_reliable_editor(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  set_sprite_in_more_reliable_editor(sprite);
  return 0;
}

static int bind_split_editor(lua_State *L)
{
  JWidget editor;
  int align;
  GetUD(1, editor, JWidget);
  GetArg(2, align, int, number);
  split_editor(editor, align);
  return 0;
}

static int bind_close_editor(lua_State *L)
{
  JWidget editor;
  GetUD(1, editor, JWidget);
  close_editor(editor);
  return 0;
}

static int bind_make_unique_editor(lua_State *L)
{
  JWidget editor;
  GetUD(1, editor, JWidget);
  make_unique_editor(editor);
  return 0;
}

static int bind_recent_file(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  recent_file(filename);
  return 0;
}

static int bind_unrecent_file(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  unrecent_file(filename);
  return 0;
}

static int bind_use_current_sprite_rgb_map(lua_State *L)
{
  use_current_sprite_rgb_map();
  return 0;
}

static int bind_use_sprite_rgb_map(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  use_sprite_rgb_map(sprite);
  return 0;
}

static int bind_restore_rgb_map(lua_State *L)
{
  restore_rgb_map();
  return 0;
}

static int bind_autocrop_sprite(lua_State *L)
{
  autocrop_sprite();
  return 0;
}

static int bind_canvas_resize(lua_State *L)
{
  canvas_resize();
  return 0;
}

static int bind_cut_to_clipboard(lua_State *L)
{
  cut_to_clipboard();
  return 0;
}

static int bind_copy_to_clipboard(lua_State *L)
{
  copy_to_clipboard();
  return 0;
}

static int bind_paste_from_clipboard(lua_State *L)
{
  paste_from_clipboard();
  return 0;
}

static int bind_crop_sprite(lua_State *L)
{
  crop_sprite();
  return 0;
}

static int bind_crop_layer(lua_State *L)
{
  crop_layer();
  return 0;
}

static int bind_crop_cel(lua_State *L)
{
  crop_cel();
  return 0;
}

static int bind_set_cel_to_handle(lua_State *L)
{
  Layer *layer;
  Cel *cel;
  GetUD(1, layer, Layer);
  GetUD(2, cel, Cel);
  set_cel_to_handle(layer, cel);
  return 0;
}

static int bind_move_cel(lua_State *L)
{
  move_cel();
  return 0;
}

static int bind_copy_cel(lua_State *L)
{
  copy_cel();
  return 0;
}

static int bind_link_cel(lua_State *L)
{
  link_cel();
  return 0;
}

static int bind_mapgen(lua_State *L)
{
  Image *image;
  int seed;
  float fractal_factor;
  GetUD(1, image, Image);
  GetArg(2, seed, int, number);
  GetArg(3, fractal_factor, float, number);
  mapgen(image, seed, fractal_factor);
  return 0;
}

static int bind_GetImage(lua_State *L)
{
  Image *return_value;
  return_value = GetImage();
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind_LoadPalette(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  LoadPalette(filename);
  return 0;
}

static int bind_ClearMask(lua_State *L)
{
  ClearMask();
  return 0;
}

static int bind_load_msk_file(lua_State *L)
{
  Mask *return_value;
  const char *filename;
  GetArg(1, filename, const char *, string);
  return_value = load_msk_file(filename);
  push_userdata(L, Type_Mask, return_value);
  return 1;
}

static int bind_save_msk_file(lua_State *L)
{
  int return_value;
  Mask *mask;
  const char *filename;
  GetUD(1, mask, Mask);
  GetArg(2, filename, const char *, string);
  return_value = save_msk_file(mask, filename);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_sprite_quantize(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_quantize(sprite);
  return 0;
}

static int bind_update_screen_for_sprite(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  update_screen_for_sprite(sprite);
  return 0;
}

static int bind_rebuild_root_menu(lua_State *L)
{
  rebuild_root_menu();
  return 0;
}

static int bind_rebuild_sprite_list(lua_State *L)
{
  rebuild_sprite_list();
  return 0;
}

static int bind_rebuild_recent_list(lua_State *L)
{
  rebuild_recent_list();
  return 0;
}

static int bind_quick_move(lua_State *L)
{
  quick_move();
  return 0;
}

static int bind_quick_copy(lua_State *L)
{
  quick_copy();
  return 0;
}

static int bind_quick_swap(lua_State *L)
{
  quick_swap();
  return 0;
}

static int bind_play_fli_animation(lua_State *L)
{
  const char *filename;
  bool loop;
  bool fullscreen;
  GetArg(1, filename, const char *, string);
  GetArg(2, loop, bool, boolean);
  GetArg(3, fullscreen, bool, boolean);
  play_fli_animation(filename, loop, fullscreen);
  return 0;
}

static int bind_dialogs_draw_text(lua_State *L)
{
  dialogs_draw_text();
  return 0;
}

static int bind_switch_between_film_and_sprite_editor(lua_State *L)
{
  switch_between_film_and_sprite_editor();
  return 0;
}

static int bind_dialogs_mapgen(lua_State *L)
{
  dialogs_mapgen();
  return 0;
}

static int bind_dialogs_mask_color(lua_State *L)
{
  dialogs_mask_color();
  return 0;
}

static int bind_dialogs_options(lua_State *L)
{
  dialogs_options();
  return 0;
}

static int bind_dialogs_palette_editor(lua_State *L)
{
  dialogs_palette_editor();
  return 0;
}

static int bind_dialogs_screen_saver(lua_State *L)
{
  dialogs_screen_saver();
  return 0;
}

static int bind_dialogs_select_language(lua_State *L)
{
  bool force;
  GetArg(1, force, bool, boolean);
  dialogs_select_language(force);
  return 0;
}

static int bind_dialogs_tips(lua_State *L)
{
  bool forced;
  GetArg(1, forced, bool, boolean);
  dialogs_tips(forced);
  return 0;
}

static int bind_dialogs_vector_map(lua_State *L)
{
  dialogs_vector_map();
  return 0;
}

static int bind_RenderText(lua_State *L)
{
  Image *return_value;
  const char *fontname;
  int size;
  int color;
  const char *text;
  GetArg(1, fontname, const char *, string);
  GetArg(2, size, int, number);
  GetArg(3, color, int, number);
  GetArg(4, text, const char *, string);
  return_value = RenderText(fontname, size, color, text);
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind__rgba_getr(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _rgba_getr(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__rgba_getg(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _rgba_getg(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__rgba_getb(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _rgba_getb(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__rgba_geta(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _rgba_geta(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__rgba(lua_State *L)
{
  int return_value;
  int r;
  int g;
  int b;
  int a;
  GetArg(1, r, int, number);
  GetArg(2, g, int, number);
  GetArg(3, b, int, number);
  GetArg(4, a, int, number);
  return_value = _rgba(r, g, b, a);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__graya_getk(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _graya_getk(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__graya_geta(lua_State *L)
{
  int return_value;
  int c;
  GetArg(1, c, int, number);
  return_value = _graya_geta(c);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind__graya(lua_State *L)
{
  int return_value;
  int k;
  int a;
  GetArg(1, k, int, number);
  GetArg(2, a, int, number);
  return_value = _graya(k, a);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_image_new(lua_State *L)
{
  Image *return_value;
  int imgtype;
  int w;
  int h;
  GetArg(1, imgtype, int, number);
  GetArg(2, w, int, number);
  GetArg(3, h, int, number);
  return_value = image_new(imgtype, w, h);
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind_image_new_copy(lua_State *L)
{
  Image *return_value;
  Image *image;
  GetUD(1, image, Image);
  return_value = image_new_copy(image);
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind_image_free(lua_State *L)
{
  Image *image;
  GetUD(1, image, Image);
  image_free(image);
  return 0;
}

static int bind_image_getpixel(lua_State *L)
{
  int return_value;
  Image *image;
  int x;
  int y;
  GetUD(1, image, Image);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  return_value = image_getpixel(image, x, y);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_image_putpixel(lua_State *L)
{
  Image *image;
  int x;
  int y;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, color, int, number);
  image_putpixel(image, x, y, color);
  return 0;
}

static int bind_image_clear(lua_State *L)
{
  Image *image;
  int color;
  GetUD(1, image, Image);
  GetArg(2, color, int, number);
  image_clear(image, color);
  return 0;
}

static int bind_image_copy(lua_State *L)
{
  Image *dst;
  Image *src;
  int x;
  int y;
  GetUD(1, dst, Image);
  GetUD(2, src, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  image_copy(dst, src, x, y);
  return 0;
}

static int bind_image_merge(lua_State *L)
{
  Image *dst;
  Image *src;
  int x;
  int y;
  int opacity;
  int blend_mode;
  GetUD(1, dst, Image);
  GetUD(2, src, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  GetArg(5, opacity, int, number);
  GetArg(6, blend_mode, int, number);
  image_merge(dst, src, x, y, opacity, blend_mode);
  return 0;
}

static int bind_image_crop(lua_State *L)
{
  Image *return_value;
  Image *image;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, image, Image);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  return_value = image_crop(image, x, y, w, h);
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind_image_hline(lua_State *L)
{
  Image *image;
  int x1;
  int y;
  int x2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, color, int, number);
  image_hline(image, x1, y, x2, color);
  return 0;
}

static int bind_image_vline(lua_State *L)
{
  Image *image;
  int x;
  int y1;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, y2, int, number);
  GetArg(5, color, int, number);
  image_vline(image, x, y1, y2, color);
  return 0;
}

static int bind_image_rect(lua_State *L)
{
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  GetArg(6, color, int, number);
  image_rect(image, x1, y1, x2, y2, color);
  return 0;
}

static int bind_image_rectfill(lua_State *L)
{
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  GetArg(6, color, int, number);
  image_rectfill(image, x1, y1, x2, y2, color);
  return 0;
}

static int bind_image_line(lua_State *L)
{
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  GetArg(6, color, int, number);
  image_line(image, x1, y1, x2, y2, color);
  return 0;
}

static int bind_image_ellipse(lua_State *L)
{
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  GetArg(6, color, int, number);
  image_ellipse(image, x1, y1, x2, y2, color);
  return 0;
}

static int bind_image_ellipsefill(lua_State *L)
{
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int color;
  GetUD(1, image, Image);
  GetArg(2, x1, int, number);
  GetArg(3, y1, int, number);
  GetArg(4, x2, int, number);
  GetArg(5, y2, int, number);
  GetArg(6, color, int, number);
  image_ellipsefill(image, x1, y1, x2, y2, color);
  return 0;
}

static int bind_image_convert(lua_State *L)
{
  Image *dst;
  Image *src;
  GetUD(1, dst, Image);
  GetUD(2, src, Image);
  image_convert(dst, src);
  return 0;
}

static int bind_image_count_diff(lua_State *L)
{
  int return_value;
  Image *i1;
  Image *i2;
  GetUD(1, i1, Image);
  GetUD(2, i2, Image);
  return_value = image_count_diff(i1, i2);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_image_scale(lua_State *L)
{
  Image *dst;
  Image *src;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, dst, Image);
  GetUD(2, src, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  GetArg(5, w, int, number);
  GetArg(6, h, int, number);
  image_scale(dst, src, x, y, w, h);
  return 0;
}

static int bind_image_rotate(lua_State *L)
{
  Image *dst;
  Image *src;
  int x;
  int y;
  int w;
  int h;
  int cx;
  int cy;
  double angle;
  GetUD(1, dst, Image);
  GetUD(2, src, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  GetArg(5, w, int, number);
  GetArg(6, h, int, number);
  GetArg(7, cx, int, number);
  GetArg(8, cy, int, number);
  GetArg(9, angle, double, number);
  image_rotate(dst, src, x, y, w, h, cx, cy, angle);
  return 0;
}

static int bind_image_parallelogram(lua_State *L)
{
  Image *bmp;
  Image *sprite;
  int x1;
  int y1;
  int x2;
  int y2;
  int x3;
  int y3;
  int x4;
  int y4;
  GetUD(1, bmp, Image);
  GetUD(2, sprite, Image);
  GetArg(3, x1, int, number);
  GetArg(4, y1, int, number);
  GetArg(5, x2, int, number);
  GetArg(6, y2, int, number);
  GetArg(7, x3, int, number);
  GetArg(8, y3, int, number);
  GetArg(9, x4, int, number);
  GetArg(10, y4, int, number);
  image_parallelogram(bmp, sprite, x1, y1, x2, y2, x3, y3, x4, y4);
  return 0;
}

static int bind_cel_new(lua_State *L)
{
  Cel *return_value;
  int frame;
  int image;
  GetArg(1, frame, int, number);
  GetArg(2, image, int, number);
  return_value = cel_new(frame, image);
  push_userdata(L, Type_Cel, return_value);
  return 1;
}

static int bind_cel_new_copy(lua_State *L)
{
  Cel *return_value;
  Cel *cel;
  GetUD(1, cel, Cel);
  return_value = cel_new_copy(cel);
  push_userdata(L, Type_Cel, return_value);
  return 1;
}

static int bind_cel_free(lua_State *L)
{
  Cel *cel;
  GetUD(1, cel, Cel);
  cel_free(cel);
  return 0;
}

static int bind_cel_is_link(lua_State *L)
{
  Cel *return_value;
  Cel *cel;
  Layer *layer;
  GetUD(1, cel, Cel);
  GetUD(2, layer, Layer);
  return_value = cel_is_link(cel, layer);
  push_userdata(L, Type_Cel, return_value);
  return 1;
}

static int bind_cel_set_frame(lua_State *L)
{
  Cel *cel;
  int frame;
  GetUD(1, cel, Cel);
  GetArg(2, frame, int, number);
  cel_set_frame(cel, frame);
  return 0;
}

static int bind_cel_set_image(lua_State *L)
{
  Cel *cel;
  int image;
  GetUD(1, cel, Cel);
  GetArg(2, image, int, number);
  cel_set_image(cel, image);
  return 0;
}

static int bind_cel_set_position(lua_State *L)
{
  Cel *cel;
  int x;
  int y;
  GetUD(1, cel, Cel);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  cel_set_position(cel, x, y);
  return 0;
}

static int bind_cel_set_opacity(lua_State *L)
{
  Cel *cel;
  int opacity;
  GetUD(1, cel, Cel);
  GetArg(2, opacity, int, number);
  cel_set_opacity(cel, opacity);
  return 0;
}

static int bind_layer_new(lua_State *L)
{
  Layer *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = layer_new(sprite);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_layer_set_new(lua_State *L)
{
  Layer *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = layer_set_new(sprite);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_layer_new_copy(lua_State *L)
{
  Layer *return_value;
  Layer *layer;
  GetUD(1, layer, Layer);
  return_value = layer_new_copy(layer);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_layer_free(lua_State *L)
{
  Layer *layer;
  GetUD(1, layer, Layer);
  layer_free(layer);
  return 0;
}

static int bind_layer_is_image(lua_State *L)
{
  bool return_value;
  Layer *layer;
  GetUD(1, layer, Layer);
  return_value = layer_is_image(layer);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_layer_is_set(lua_State *L)
{
  bool return_value;
  Layer *layer;
  GetUD(1, layer, Layer);
  return_value = layer_is_set(layer);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_layer_get_prev(lua_State *L)
{
  Layer *return_value;
  Layer *layer;
  GetUD(1, layer, Layer);
  return_value = layer_get_prev(layer);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_layer_get_next(lua_State *L)
{
  Layer *return_value;
  Layer *layer;
  GetUD(1, layer, Layer);
  return_value = layer_get_next(layer);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_layer_set_name(lua_State *L)
{
  Layer *layer;
  const char *name;
  GetUD(1, layer, Layer);
  GetArg(2, name, const char *, string);
  layer_set_name(layer, name);
  return 0;
}

static int bind_layer_set_blend_mode(lua_State *L)
{
  Layer *layer;
  int blend_mode;
  GetUD(1, layer, Layer);
  GetArg(2, blend_mode, int, number);
  layer_set_blend_mode(layer, blend_mode);
  return 0;
}

static int bind_layer_add_cel(lua_State *L)
{
  Layer *layer;
  Cel *cel;
  GetUD(1, layer, Layer);
  GetUD(2, cel, Cel);
  layer_add_cel(layer, cel);
  return 0;
}

static int bind_layer_remove_cel(lua_State *L)
{
  Layer *layer;
  Cel *cel;
  GetUD(1, layer, Layer);
  GetUD(2, cel, Cel);
  layer_remove_cel(layer, cel);
  return 0;
}

static int bind_layer_get_cel(lua_State *L)
{
  Cel *return_value;
  Layer *layer;
  int frame;
  GetUD(1, layer, Layer);
  GetArg(2, frame, int, number);
  return_value = layer_get_cel(layer, frame);
  push_userdata(L, Type_Cel, return_value);
  return 1;
}

static int bind_layer_add_layer(lua_State *L)
{
  Layer *set;
  Layer *layer;
  GetUD(1, set, Layer);
  GetUD(2, layer, Layer);
  layer_add_layer(set, layer);
  return 0;
}

static int bind_layer_remove_layer(lua_State *L)
{
  Layer *set;
  Layer *layer;
  GetUD(1, set, Layer);
  GetUD(2, layer, Layer);
  layer_remove_layer(set, layer);
  return 0;
}

static int bind_layer_move_layer(lua_State *L)
{
  Layer *set;
  Layer *layer;
  Layer *after;
  GetUD(1, set, Layer);
  GetUD(2, layer, Layer);
  GetUD(3, after, Layer);
  layer_move_layer(set, layer, after);
  return 0;
}

static int bind_layer_render(lua_State *L)
{
  Layer *layer;
  Image *image;
  int x;
  int y;
  int frame;
  GetUD(1, layer, Layer);
  GetUD(2, image, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  GetArg(5, frame, int, number);
  layer_render(layer, image, x, y, frame);
  return 0;
}

static int bind_layer_flatten(lua_State *L)
{
  Layer *return_value;
  Layer *layer;
  int x;
  int y;
  int w;
  int h;
  int frmin;
  int frmax;
  GetUD(1, layer, Layer);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  GetArg(6, frmin, int, number);
  GetArg(7, frmax, int, number);
  return_value = layer_flatten(layer, x, y, w, h, frmin, frmax);
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_mask_new(lua_State *L)
{
  Mask *return_value;
  return_value = mask_new();
  push_userdata(L, Type_Mask, return_value);
  return 1;
}

static int bind_mask_new_copy(lua_State *L)
{
  Mask *return_value;
  Mask *mask;
  GetUD(1, mask, Mask);
  return_value = mask_new_copy(mask);
  push_userdata(L, Type_Mask, return_value);
  return 1;
}

static int bind_mask_free(lua_State *L)
{
  Mask *mask;
  GetUD(1, mask, Mask);
  mask_free(mask);
  return 0;
}

static int bind_mask_is_empty(lua_State *L)
{
  bool return_value;
  Mask *mask;
  GetUD(1, mask, Mask);
  return_value = mask_is_empty(mask);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_mask_set_name(lua_State *L)
{
  Mask *mask;
  const char *name;
  GetUD(1, mask, Mask);
  GetArg(2, name, const char *, string);
  mask_set_name(mask, name);
  return 0;
}

static int bind_mask_move(lua_State *L)
{
  Mask *mask;
  int x;
  int y;
  GetUD(1, mask, Mask);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  mask_move(mask, x, y);
  return 0;
}

static int bind_mask_none(lua_State *L)
{
  Mask *mask;
  GetUD(1, mask, Mask);
  mask_none(mask);
  return 0;
}

static int bind_mask_invert(lua_State *L)
{
  Mask *mask;
  GetUD(1, mask, Mask);
  mask_invert(mask);
  return 0;
}

static int bind_mask_replace(lua_State *L)
{
  Mask *mask;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, mask, Mask);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  mask_replace(mask, x, y, w, h);
  return 0;
}

static int bind_mask_union(lua_State *L)
{
  Mask *mask;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, mask, Mask);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  mask_union(mask, x, y, w, h);
  return 0;
}

static int bind_mask_subtract(lua_State *L)
{
  Mask *mask;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, mask, Mask);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  mask_subtract(mask, x, y, w, h);
  return 0;
}

static int bind_mask_intersect(lua_State *L)
{
  Mask *mask;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, mask, Mask);
  GetArg(2, x, int, number);
  GetArg(3, y, int, number);
  GetArg(4, w, int, number);
  GetArg(5, h, int, number);
  mask_intersect(mask, x, y, w, h);
  return 0;
}

static int bind_mask_merge(lua_State *L)
{
  Mask *dst;
  Mask *src;
  GetUD(1, dst, Mask);
  GetUD(2, src, Mask);
  mask_merge(dst, src);
  return 0;
}

static int bind_mask_by_color(lua_State *L)
{
  Mask *mask;
  Image *image;
  int color;
  int fuzziness;
  GetUD(1, mask, Mask);
  GetUD(2, image, Image);
  GetArg(3, color, int, number);
  GetArg(4, fuzziness, int, number);
  mask_by_color(mask, image, color, fuzziness);
  return 0;
}

static int bind_mask_crop(lua_State *L)
{
  Mask *mask;
  Image *image;
  GetUD(1, mask, Mask);
  GetUD(2, image, Image);
  mask_crop(mask, image);
  return 0;
}

static int bind_path_new(lua_State *L)
{
  Path *return_value;
  const char *name;
  GetArg(1, name, const char *, string);
  return_value = path_new(name);
  push_userdata(L, Type_Path, return_value);
  return 1;
}

static int bind_path_free(lua_State *L)
{
  Path *path;
  GetUD(1, path, Path);
  path_free(path);
  return 0;
}

static int bind_path_set_join(lua_State *L)
{
  Path *path;
  int join;
  GetUD(1, path, Path);
  GetArg(2, join, int, number);
  path_set_join(path, join);
  return 0;
}

static int bind_path_set_cap(lua_State *L)
{
  Path *path;
  int cap;
  GetUD(1, path, Path);
  GetArg(2, cap, int, number);
  path_set_cap(path, cap);
  return 0;
}

static int bind_path_moveto(lua_State *L)
{
  Path *path;
  double x;
  double y;
  GetUD(1, path, Path);
  GetArg(2, x, double, number);
  GetArg(3, y, double, number);
  path_moveto(path, x, y);
  return 0;
}

static int bind_path_lineto(lua_State *L)
{
  Path *path;
  double x;
  double y;
  GetUD(1, path, Path);
  GetArg(2, x, double, number);
  GetArg(3, y, double, number);
  path_lineto(path, x, y);
  return 0;
}

static int bind_path_curveto(lua_State *L)
{
  Path *path;
  double control_x1;
  double control_y1;
  double control_x2;
  double control_y2;
  double end_x;
  double end_y;
  GetUD(1, path, Path);
  GetArg(2, control_x1, double, number);
  GetArg(3, control_y1, double, number);
  GetArg(4, control_x2, double, number);
  GetArg(5, control_y2, double, number);
  GetArg(6, end_x, double, number);
  GetArg(7, end_y, double, number);
  path_curveto(path, control_x1, control_y1, control_x2, control_y2, end_x, end_y);
  return 0;
}

static int bind_path_close(lua_State *L)
{
  Path *path;
  GetUD(1, path, Path);
  path_close(path);
  return 0;
}

static int bind_path_move(lua_State *L)
{
  Path *path;
  double x;
  double y;
  GetUD(1, path, Path);
  GetArg(2, x, double, number);
  GetArg(3, y, double, number);
  path_move(path, x, y);
  return 0;
}

static int bind_path_stroke(lua_State *L)
{
  Path *path;
  Image *image;
  int color;
  double brush_size;
  GetUD(1, path, Path);
  GetUD(2, image, Image);
  GetArg(3, color, int, number);
  GetArg(4, brush_size, double, number);
  path_stroke(path, image, color, brush_size);
  return 0;
}

static int bind_path_fill(lua_State *L)
{
  Path *path;
  Image *image;
  int color;
  GetUD(1, path, Path);
  GetUD(2, image, Image);
  GetArg(3, color, int, number);
  path_fill(path, image, color);
  return 0;
}

static int bind_sprite_new(lua_State *L)
{
  Sprite *return_value;
  int imgtype;
  int w;
  int h;
  GetArg(1, imgtype, int, number);
  GetArg(2, w, int, number);
  GetArg(3, h, int, number);
  return_value = sprite_new(imgtype, w, h);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_sprite_new_copy(lua_State *L)
{
  Sprite *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = sprite_new_copy(sprite);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_sprite_new_flatten_copy(lua_State *L)
{
  Sprite *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = sprite_new_flatten_copy(sprite);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_sprite_new_with_layer(lua_State *L)
{
  Sprite *return_value;
  int imgtype;
  int w;
  int h;
  GetArg(1, imgtype, int, number);
  GetArg(2, w, int, number);
  GetArg(3, h, int, number);
  return_value = sprite_new_with_layer(imgtype, w, h);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_sprite_free(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_free(sprite);
  return 0;
}

static int bind_sprite_is_modified(lua_State *L)
{
  bool return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = sprite_is_modified(sprite);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_sprite_is_associated_to_file(lua_State *L)
{
  bool return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = sprite_is_associated_to_file(sprite);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_sprite_mark_as_saved(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  sprite_mark_as_saved(sprite);
  return 0;
}

static int bind_sprite_set_filename(lua_State *L)
{
  Sprite *sprite;
  const char *filename;
  GetUD(1, sprite, Sprite);
  GetArg(2, filename, const char *, string);
  sprite_set_filename(sprite, filename);
  return 0;
}

static int bind_sprite_set_size(lua_State *L)
{
  Sprite *sprite;
  int w;
  int h;
  GetUD(1, sprite, Sprite);
  GetArg(2, w, int, number);
  GetArg(3, h, int, number);
  sprite_set_size(sprite, w, h);
  return 0;
}

static int bind_sprite_set_frames(lua_State *L)
{
  Sprite *sprite;
  int frames;
  GetUD(1, sprite, Sprite);
  GetArg(2, frames, int, number);
  sprite_set_frames(sprite, frames);
  return 0;
}

static int bind_sprite_set_frlen(lua_State *L)
{
  Sprite *sprite;
  int msecs;
  int frame;
  GetUD(1, sprite, Sprite);
  GetArg(2, msecs, int, number);
  GetArg(3, frame, int, number);
  sprite_set_frlen(sprite, msecs, frame);
  return 0;
}

static int bind_sprite_get_frlen(lua_State *L)
{
  int return_value;
  Sprite *sprite;
  int frame;
  GetUD(1, sprite, Sprite);
  GetArg(2, frame, int, number);
  return_value = sprite_get_frlen(sprite, frame);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_sprite_set_speed(lua_State *L)
{
  Sprite *sprite;
  int speed;
  GetUD(1, sprite, Sprite);
  GetArg(2, speed, int, number);
  sprite_set_speed(sprite, speed);
  return 0;
}

static int bind_sprite_set_path(lua_State *L)
{
  Sprite *sprite;
  Path *path;
  GetUD(1, sprite, Sprite);
  GetUD(2, path, Path);
  sprite_set_path(sprite, path);
  return 0;
}

static int bind_sprite_set_mask(lua_State *L)
{
  Sprite *sprite;
  Mask *mask;
  GetUD(1, sprite, Sprite);
  GetUD(2, mask, Mask);
  sprite_set_mask(sprite, mask);
  return 0;
}

static int bind_sprite_set_layer(lua_State *L)
{
  Sprite *sprite;
  Layer *layer;
  GetUD(1, sprite, Sprite);
  GetUD(2, layer, Layer);
  sprite_set_layer(sprite, layer);
  return 0;
}

static int bind_sprite_set_frame(lua_State *L)
{
  Sprite *sprite;
  int frame;
  GetUD(1, sprite, Sprite);
  GetArg(2, frame, int, number);
  sprite_set_frame(sprite, frame);
  return 0;
}

static int bind_sprite_set_imgtype(lua_State *L)
{
  Sprite *sprite;
  int imgtype;
  int dithering_method;
  GetUD(1, sprite, Sprite);
  GetArg(2, imgtype, int, number);
  GetArg(3, dithering_method, int, number);
  sprite_set_imgtype(sprite, imgtype, dithering_method);
  return 0;
}

static int bind_sprite_add_path(lua_State *L)
{
  Sprite *sprite;
  Path *path;
  GetUD(1, sprite, Sprite);
  GetUD(2, path, Path);
  sprite_add_path(sprite, path);
  return 0;
}

static int bind_sprite_remove_path(lua_State *L)
{
  Sprite *sprite;
  Path *path;
  GetUD(1, sprite, Sprite);
  GetUD(2, path, Path);
  sprite_remove_path(sprite, path);
  return 0;
}

static int bind_sprite_add_mask(lua_State *L)
{
  Sprite *sprite;
  Mask *mask;
  GetUD(1, sprite, Sprite);
  GetUD(2, mask, Mask);
  sprite_add_mask(sprite, mask);
  return 0;
}

static int bind_sprite_remove_mask(lua_State *L)
{
  Sprite *sprite;
  Mask *mask;
  GetUD(1, sprite, Sprite);
  GetUD(2, mask, Mask);
  sprite_remove_mask(sprite, mask);
  return 0;
}

static int bind_sprite_request_mask(lua_State *L)
{
  Mask *return_value;
  Sprite *sprite;
  const char *name;
  GetUD(1, sprite, Sprite);
  GetArg(2, name, const char *, string);
  return_value = sprite_request_mask(sprite, name);
  push_userdata(L, Type_Mask, return_value);
  return 1;
}

static int bind_sprite_render(lua_State *L)
{
  Sprite *sprite;
  Image *image;
  int x;
  int y;
  GetUD(1, sprite, Sprite);
  GetUD(2, image, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  sprite_render(sprite, image, x, y);
  return 0;
}

static int bind_stock_new(lua_State *L)
{
  Stock *return_value;
  int imgtype;
  GetArg(1, imgtype, int, number);
  return_value = stock_new(imgtype);
  push_userdata(L, Type_Stock, return_value);
  return 1;
}

static int bind_stock_new_copy(lua_State *L)
{
  Stock *return_value;
  Stock *stock;
  GetUD(1, stock, Stock);
  return_value = stock_new_copy(stock);
  push_userdata(L, Type_Stock, return_value);
  return 1;
}

static int bind_stock_free(lua_State *L)
{
  Stock *stock;
  GetUD(1, stock, Stock);
  stock_free(stock);
  return 0;
}

static int bind_stock_add_image(lua_State *L)
{
  int return_value;
  Stock *stock;
  Image *image;
  GetUD(1, stock, Stock);
  GetUD(2, image, Image);
  return_value = stock_add_image(stock, image);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_stock_remove_image(lua_State *L)
{
  Stock *stock;
  Image *image;
  GetUD(1, stock, Stock);
  GetUD(2, image, Image);
  stock_remove_image(stock, image);
  return 0;
}

static int bind_stock_replace_image(lua_State *L)
{
  Stock *stock;
  int index;
  Image *image;
  GetUD(1, stock, Stock);
  GetArg(2, index, int, number);
  GetUD(3, image, Image);
  stock_replace_image(stock, index, image);
  return 0;
}

static int bind_stock_get_image(lua_State *L)
{
  Image *return_value;
  Stock *stock;
  int index;
  GetUD(1, stock, Stock);
  GetArg(2, index, int, number);
  return_value = stock_get_image(stock, index);
  push_userdata(L, Type_Image, return_value);
  return 1;
}

static int bind_undo_new(lua_State *L)
{
  Undo *return_value;
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  return_value = undo_new(sprite);
  push_userdata(L, Type_Undo, return_value);
  return 1;
}

static int bind_undo_free(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_free(undo);
  return 0;
}

static int bind_undo_enable(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_enable(undo);
  return 0;
}

static int bind_undo_disable(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_disable(undo);
  return 0;
}

static int bind_undo_is_enabled(lua_State *L)
{
  bool return_value;
  Undo *undo;
  GetUD(1, undo, Undo);
  return_value = undo_is_enabled(undo);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_undo_is_disabled(lua_State *L)
{
  bool return_value;
  Undo *undo;
  GetUD(1, undo, Undo);
  return_value = undo_is_disabled(undo);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_undo_can_undo(lua_State *L)
{
  bool return_value;
  Undo *undo;
  GetUD(1, undo, Undo);
  return_value = undo_can_undo(undo);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_undo_can_redo(lua_State *L)
{
  bool return_value;
  Undo *undo;
  GetUD(1, undo, Undo);
  return_value = undo_can_redo(undo);
  lua_pushboolean(L, return_value);
  return 1;
}

static int bind_undo_undo(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_undo(undo);
  return 0;
}

static int bind_undo_redo(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_redo(undo);
  return 0;
}

static int bind_undo_open(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_open(undo);
  return 0;
}

static int bind_undo_close(lua_State *L)
{
  Undo *undo;
  GetUD(1, undo, Undo);
  undo_close(undo);
  return 0;
}

static int bind_undo_image(lua_State *L)
{
  Undo *undo;
  Image *image;
  int x;
  int y;
  int w;
  int h;
  GetUD(1, undo, Undo);
  GetUD(2, image, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  GetArg(5, w, int, number);
  GetArg(6, h, int, number);
  undo_image(undo, image, x, y, w, h);
  return 0;
}

static int bind_undo_flip(lua_State *L)
{
  Undo *undo;
  Image *image;
  int x1;
  int y1;
  int x2;
  int y2;
  int horz;
  GetUD(1, undo, Undo);
  GetUD(2, image, Image);
  GetArg(3, x1, int, number);
  GetArg(4, y1, int, number);
  GetArg(5, x2, int, number);
  GetArg(6, y2, int, number);
  GetArg(7, horz, int, number);
  undo_flip(undo, image, x1, y1, x2, y2, horz);
  return 0;
}

static int bind_undo_add_image(lua_State *L)
{
  Undo *undo;
  Stock *stock;
  Image *image;
  GetUD(1, undo, Undo);
  GetUD(2, stock, Stock);
  GetUD(3, image, Image);
  undo_add_image(undo, stock, image);
  return 0;
}

static int bind_undo_remove_image(lua_State *L)
{
  Undo *undo;
  Stock *stock;
  Image *image;
  GetUD(1, undo, Undo);
  GetUD(2, stock, Stock);
  GetUD(3, image, Image);
  undo_remove_image(undo, stock, image);
  return 0;
}

static int bind_undo_replace_image(lua_State *L)
{
  Undo *undo;
  Stock *stock;
  int index;
  GetUD(1, undo, Undo);
  GetUD(2, stock, Stock);
  GetArg(3, index, int, number);
  undo_replace_image(undo, stock, index);
  return 0;
}

static int bind_undo_add_cel(lua_State *L)
{
  Undo *undo;
  Layer *layer;
  Cel *cel;
  GetUD(1, undo, Undo);
  GetUD(2, layer, Layer);
  GetUD(3, cel, Cel);
  undo_add_cel(undo, layer, cel);
  return 0;
}

static int bind_undo_remove_cel(lua_State *L)
{
  Undo *undo;
  Layer *layer;
  Cel *cel;
  GetUD(1, undo, Undo);
  GetUD(2, layer, Layer);
  GetUD(3, cel, Cel);
  undo_remove_cel(undo, layer, cel);
  return 0;
}

static int bind_undo_add_layer(lua_State *L)
{
  Undo *undo;
  Layer *set;
  Layer *layer;
  GetUD(1, undo, Undo);
  GetUD(2, set, Layer);
  GetUD(3, layer, Layer);
  undo_add_layer(undo, set, layer);
  return 0;
}

static int bind_undo_remove_layer(lua_State *L)
{
  Undo *undo;
  Layer *layer;
  GetUD(1, undo, Undo);
  GetUD(2, layer, Layer);
  undo_remove_layer(undo, layer);
  return 0;
}

static int bind_undo_move_layer(lua_State *L)
{
  Undo *undo;
  Layer *layer;
  GetUD(1, undo, Undo);
  GetUD(2, layer, Layer);
  undo_move_layer(undo, layer);
  return 0;
}

static int bind_undo_set_layer(lua_State *L)
{
  Undo *undo;
  Sprite *sprite;
  GetUD(1, undo, Undo);
  GetUD(2, sprite, Sprite);
  undo_set_layer(undo, sprite);
  return 0;
}

static int bind_undo_set_mask(lua_State *L)
{
  Undo *undo;
  Sprite *sprite;
  GetUD(1, undo, Undo);
  GetUD(2, sprite, Sprite);
  undo_set_mask(undo, sprite);
  return 0;
}

static int bind_effect_new(lua_State *L)
{
  Effect *return_value;
  Sprite *sprite;
  const char *name;
  GetUD(1, sprite, Sprite);
  GetArg(2, name, const char *, string);
  return_value = effect_new(sprite, name);
  push_userdata(L, Type_Effect, return_value);
  return 1;
}

static int bind_effect_free(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_free(effect);
  return 0;
}

static int bind_effect_load_target(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_load_target(effect);
  return 0;
}

static int bind_effect_set_target(lua_State *L)
{
  Effect *effect;
  bool r;
  bool g;
  bool b;
  bool k;
  bool a;
  bool index;
  GetUD(1, effect, Effect);
  GetArg(2, r, bool, boolean);
  GetArg(3, g, bool, boolean);
  GetArg(4, b, bool, boolean);
  GetArg(5, k, bool, boolean);
  GetArg(6, a, bool, boolean);
  GetArg(7, index, bool, boolean);
  effect_set_target(effect, r, g, b, k, a, index);
  return 0;
}

static int bind_effect_set_target_rgb(lua_State *L)
{
  Effect *effect;
  bool r;
  bool g;
  bool b;
  bool a;
  GetUD(1, effect, Effect);
  GetArg(2, r, bool, boolean);
  GetArg(3, g, bool, boolean);
  GetArg(4, b, bool, boolean);
  GetArg(5, a, bool, boolean);
  effect_set_target_rgb(effect, r, g, b, a);
  return 0;
}

static int bind_effect_set_target_grayscale(lua_State *L)
{
  Effect *effect;
  bool k;
  bool a;
  GetUD(1, effect, Effect);
  GetArg(2, k, bool, boolean);
  GetArg(3, a, bool, boolean);
  effect_set_target_grayscale(effect, k, a);
  return 0;
}

static int bind_effect_set_target_indexed(lua_State *L)
{
  Effect *effect;
  bool r;
  bool g;
  bool b;
  bool index;
  GetUD(1, effect, Effect);
  GetArg(2, r, bool, boolean);
  GetArg(3, g, bool, boolean);
  GetArg(4, b, bool, boolean);
  GetArg(5, index, bool, boolean);
  effect_set_target_indexed(effect, r, g, b, index);
  return 0;
}

static int bind_effect_begin(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_begin(effect);
  return 0;
}

static int bind_effect_begin_for_preview(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_begin_for_preview(effect);
  return 0;
}

static int bind_effect_apply_step(lua_State *L)
{
  int return_value;
  Effect *effect;
  GetUD(1, effect, Effect);
  return_value = effect_apply_step(effect);
  lua_pushnumber(L, return_value);
  return 1;
}

static int bind_effect_apply(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_apply(effect);
  return 0;
}

static int bind_effect_flush(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_flush(effect);
  return 0;
}

static int bind_effect_apply_to_image(lua_State *L)
{
  Effect *effect;
  Image *image;
  int x;
  int y;
  GetUD(1, effect, Effect);
  GetUD(2, image, Image);
  GetArg(3, x, int, number);
  GetArg(4, y, int, number);
  effect_apply_to_image(effect, image, x, y);
  return 0;
}

static int bind_effect_apply_to_target(lua_State *L)
{
  Effect *effect;
  GetUD(1, effect, Effect);
  effect_apply_to_target(effect);
  return 0;
}

static int bind_curve_point_new(lua_State *L)
{
  CurvePoint *return_value;
  int x;
  int y;
  GetArg(1, x, int, number);
  GetArg(2, y, int, number);
  return_value = curve_point_new(x, y);
  push_userdata(L, Type_CurvePoint, return_value);
  return 1;
}

static int bind_curve_point_free(lua_State *L)
{
  CurvePoint *point;
  GetUD(1, point, CurvePoint);
  curve_point_free(point);
  return 0;
}

static int bind_curve_new(lua_State *L)
{
  Curve *return_value;
  int type;
  GetArg(1, type, int, number);
  return_value = curve_new(type);
  push_userdata(L, Type_Curve, return_value);
  return 1;
}

static int bind_curve_free(lua_State *L)
{
  Curve *curve;
  GetUD(1, curve, Curve);
  curve_free(curve);
  return 0;
}

static int bind_curve_add_point(lua_State *L)
{
  Curve *curve;
  CurvePoint *point;
  GetUD(1, curve, Curve);
  GetUD(2, point, CurvePoint);
  curve_add_point(curve, point);
  return 0;
}

static int bind_curve_remove_point(lua_State *L)
{
  Curve *curve;
  CurvePoint *point;
  GetUD(1, curve, Curve);
  GetUD(2, point, CurvePoint);
  curve_remove_point(curve, point);
  return 0;
}

static int bind_set_color_curve(lua_State *L)
{
  Curve *curve;
  GetUD(1, curve, Curve);
  set_color_curve(curve);
  return 0;
}

static int bind_convmatr_new(lua_State *L)
{
  ConvMatr *return_value;
  int w;
  int h;
  GetArg(1, w, int, number);
  GetArg(2, h, int, number);
  return_value = convmatr_new(w, h);
  push_userdata(L, Type_ConvMatr, return_value);
  return 1;
}

static int bind_convmatr_new_string(lua_State *L)
{
  ConvMatr *return_value;
  const char *format;
  GetArg(1, format, const char *, string);
  return_value = convmatr_new_string(format);
  push_userdata(L, Type_ConvMatr, return_value);
  return 1;
}

static int bind_convmatr_free(lua_State *L)
{
  ConvMatr *convmatr;
  GetUD(1, convmatr, ConvMatr);
  convmatr_free(convmatr);
  return 0;
}

static int bind_set_convmatr(lua_State *L)
{
  ConvMatr *convmatr;
  GetUD(1, convmatr, ConvMatr);
  set_convmatr(convmatr);
  return 0;
}

static int bind_get_convmatr(lua_State *L)
{
  ConvMatr *return_value;
  return_value = get_convmatr();
  push_userdata(L, Type_ConvMatr, return_value);
  return 1;
}

static int bind_get_convmatr_by_name(lua_State *L)
{
  ConvMatr *return_value;
  const char *name;
  GetArg(1, name, const char *, string);
  return_value = get_convmatr_by_name(name);
  push_userdata(L, Type_ConvMatr, return_value);
  return 1;
}

static int bind_set_replace_colors(lua_State *L)
{
  int from;
  int to;
  int fuzziness;
  GetArg(1, from, int, number);
  GetArg(2, to, int, number);
  GetArg(3, fuzziness, int, number);
  set_replace_colors(from, to, fuzziness);
  return 0;
}

static int bind_set_median_size(lua_State *L)
{
  int w;
  int h;
  GetArg(1, w, int, number);
  GetArg(2, h, int, number);
  set_median_size(w, h);
  return 0;
}

const luaL_reg bindings_routines[] = {
  { "MAX", bind_MAX },
  { "MIN", bind_MIN },
  { "MID", bind_MID },
  { "include", bind_include },
  { "dofile", bind_dofile },
  { "print", bind_print },
  { "rand", bind_rand },
  { "_", bind__ },
  { "strcmp", bind_strcmp },
  { "fabs", bind_fabs },
  { "ceil", bind_ceil },
  { "floor", bind_floor },
  { "exp", bind_exp },
  { "log", bind_log },
  { "log10", bind_log10 },
  { "pow", bind_pow },
  { "sqrt", bind_sqrt },
  { "hypot", bind_hypot },
  { "cos", bind_cos },
  { "sin", bind_sin },
  { "tan", bind_tan },
  { "acos", bind_acos },
  { "asin", bind_asin },
  { "atan", bind_atan },
  { "atan2", bind_atan2 },
  { "cosh", bind_cosh },
  { "sinh", bind_sinh },
  { "tanh", bind_tanh },
  { "file_exists", bind_file_exists },
  { "get_filename", bind_get_filename },
  { "do_progress", bind_do_progress },
  { "add_progress", bind_add_progress },
  { "del_progress", bind_del_progress },
  { "is_interactive", bind_is_interactive },
  { "app_refresh_screen", bind_app_refresh_screen },
  { "app_get_current_image_type", bind_app_get_current_image_type },
  { "app_get_top_window", bind_app_get_top_window },
  { "app_get_menu_bar", bind_app_get_menu_bar },
  { "app_get_status_bar", bind_app_get_status_bar },
  { "app_get_color_bar", bind_app_get_color_bar },
  { "app_get_tool_bar", bind_app_get_tool_bar },
  { "intl_load_lang", bind_intl_load_lang },
  { "intl_get_lang", bind_intl_get_lang },
  { "intl_set_lang", bind_intl_set_lang },
  { "show_fx_popup_menu", bind_show_fx_popup_menu },
  { "get_brush_type", bind_get_brush_type },
  { "get_brush_size", bind_get_brush_size },
  { "get_brush_angle", bind_get_brush_angle },
  { "get_brush_mode", bind_get_brush_mode },
  { "get_glass_dirty", bind_get_glass_dirty },
  { "get_spray_width", bind_get_spray_width },
  { "get_air_speed", bind_get_air_speed },
  { "get_filled_mode", bind_get_filled_mode },
  { "get_tiled_mode", bind_get_tiled_mode },
  { "get_use_grid", bind_get_use_grid },
  { "get_view_grid", bind_get_view_grid },
  { "get_grid", bind_get_grid },
  { "get_onionskin", bind_get_onionskin },
  { "set_brush_type", bind_set_brush_type },
  { "set_brush_size", bind_set_brush_size },
  { "set_brush_angle", bind_set_brush_angle },
  { "set_brush_mode", bind_set_brush_mode },
  { "set_glass_dirty", bind_set_glass_dirty },
  { "set_spray_width", bind_set_spray_width },
  { "set_air_speed", bind_set_air_speed },
  { "set_filled_mode", bind_set_filled_mode },
  { "set_tiled_mode", bind_set_tiled_mode },
  { "set_use_grid", bind_set_use_grid },
  { "set_view_grid", bind_set_view_grid },
  { "set_grid", bind_set_grid },
  { "set_onionskin", bind_set_onionskin },
  { "SetBrush", bind_SetBrush },
  { "SetDrawMode", bind_SetDrawMode },
  { "ToolTrace", bind_ToolTrace },
  { "get_fg_color", bind_get_fg_color },
  { "get_bg_color", bind_get_bg_color },
  { "set_fg_color", bind_set_fg_color },
  { "set_bg_color", bind_set_bg_color },
  { "get_color_for_image", bind_get_color_for_image },
  { "image_getpixel_color", bind_image_getpixel_color },
  { "get_first_sprite", bind_get_first_sprite },
  { "get_next_sprite", bind_get_next_sprite },
  { "get_clipboard_sprite", bind_get_clipboard_sprite },
  { "set_clipboard_sprite", bind_set_clipboard_sprite },
  { "sprite_mount", bind_sprite_mount },
  { "sprite_unmount", bind_sprite_unmount },
  { "set_current_sprite", bind_set_current_sprite },
  { "sprite_show", bind_sprite_show },
  { "sprite_generate_mask_boundaries", bind_sprite_generate_mask_boundaries },
  { "sprite_load", bind_sprite_load },
  { "sprite_save", bind_sprite_save },
  { "get_readable_extensions", bind_get_readable_extensions },
  { "get_writable_extensions", bind_get_writable_extensions },
  { "set_current_editor", bind_set_current_editor },
  { "update_editors_with_sprite", bind_update_editors_with_sprite },
  { "editors_draw_sprite", bind_editors_draw_sprite },
  { "replace_sprite_in_editors", bind_replace_sprite_in_editors },
  { "set_sprite_in_current_editor", bind_set_sprite_in_current_editor },
  { "set_sprite_in_more_reliable_editor", bind_set_sprite_in_more_reliable_editor },
  { "split_editor", bind_split_editor },
  { "close_editor", bind_close_editor },
  { "make_unique_editor", bind_make_unique_editor },
  { "recent_file", bind_recent_file },
  { "unrecent_file", bind_unrecent_file },
  { "use_current_sprite_rgb_map", bind_use_current_sprite_rgb_map },
  { "use_sprite_rgb_map", bind_use_sprite_rgb_map },
  { "restore_rgb_map", bind_restore_rgb_map },
  { "autocrop_sprite", bind_autocrop_sprite },
  { "canvas_resize", bind_canvas_resize },
  { "cut_to_clipboard", bind_cut_to_clipboard },
  { "copy_to_clipboard", bind_copy_to_clipboard },
  { "paste_from_clipboard", bind_paste_from_clipboard },
  { "crop_sprite", bind_crop_sprite },
  { "crop_layer", bind_crop_layer },
  { "crop_cel", bind_crop_cel },
  { "set_cel_to_handle", bind_set_cel_to_handle },
  { "move_cel", bind_move_cel },
  { "copy_cel", bind_copy_cel },
  { "link_cel", bind_link_cel },
  { "mapgen", bind_mapgen },
  { "GetImage", bind_GetImage },
  { "GetImage2", bind_GetImage2 },
  { "LoadPalette", bind_LoadPalette },
  { "ClearMask", bind_ClearMask },
  { "load_msk_file", bind_load_msk_file },
  { "save_msk_file", bind_save_msk_file },
  { "sprite_quantize", bind_sprite_quantize },
  { "update_screen_for_sprite", bind_update_screen_for_sprite },
  { "rebuild_root_menu", bind_rebuild_root_menu },
  { "rebuild_sprite_list", bind_rebuild_sprite_list },
  { "rebuild_recent_list", bind_rebuild_recent_list },
  { "quick_move", bind_quick_move },
  { "quick_copy", bind_quick_copy },
  { "quick_swap", bind_quick_swap },
  { "play_fli_animation", bind_play_fli_animation },
  { "dialogs_draw_text", bind_dialogs_draw_text },
  { "switch_between_film_and_sprite_editor", bind_switch_between_film_and_sprite_editor },
  { "dialogs_mapgen", bind_dialogs_mapgen },
  { "dialogs_mask_color", bind_dialogs_mask_color },
  { "dialogs_options", bind_dialogs_options },
  { "dialogs_palette_editor", bind_dialogs_palette_editor },
  { "dialogs_screen_saver", bind_dialogs_screen_saver },
  { "dialogs_select_language", bind_dialogs_select_language },
  { "dialogs_tips", bind_dialogs_tips },
  { "dialogs_vector_map", bind_dialogs_vector_map },
  { "RenderText", bind_RenderText },
  { "_rgba_getr", bind__rgba_getr },
  { "_rgba_getg", bind__rgba_getg },
  { "_rgba_getb", bind__rgba_getb },
  { "_rgba_geta", bind__rgba_geta },
  { "_rgba", bind__rgba },
  { "_graya_getk", bind__graya_getk },
  { "_graya_geta", bind__graya_geta },
  { "_graya", bind__graya },
  { "image_new", bind_image_new },
  { "image_new_copy", bind_image_new_copy },
  { "image_free", bind_image_free },
  { "image_getpixel", bind_image_getpixel },
  { "image_putpixel", bind_image_putpixel },
  { "image_clear", bind_image_clear },
  { "image_copy", bind_image_copy },
  { "image_merge", bind_image_merge },
  { "image_crop", bind_image_crop },
  { "image_hline", bind_image_hline },
  { "image_vline", bind_image_vline },
  { "image_rect", bind_image_rect },
  { "image_rectfill", bind_image_rectfill },
  { "image_line", bind_image_line },
  { "image_ellipse", bind_image_ellipse },
  { "image_ellipsefill", bind_image_ellipsefill },
  { "image_convert", bind_image_convert },
  { "image_count_diff", bind_image_count_diff },
  { "image_scale", bind_image_scale },
  { "image_rotate", bind_image_rotate },
  { "image_parallelogram", bind_image_parallelogram },
  { "cel_new", bind_cel_new },
  { "cel_new_copy", bind_cel_new_copy },
  { "cel_free", bind_cel_free },
  { "cel_is_link", bind_cel_is_link },
  { "cel_set_frame", bind_cel_set_frame },
  { "cel_set_image", bind_cel_set_image },
  { "cel_set_position", bind_cel_set_position },
  { "cel_set_opacity", bind_cel_set_opacity },
  { "layer_new", bind_layer_new },
  { "layer_set_new", bind_layer_set_new },
  { "layer_new_copy", bind_layer_new_copy },
  { "layer_free", bind_layer_free },
  { "layer_is_image", bind_layer_is_image },
  { "layer_is_set", bind_layer_is_set },
  { "layer_get_prev", bind_layer_get_prev },
  { "layer_get_next", bind_layer_get_next },
  { "layer_set_name", bind_layer_set_name },
  { "layer_set_blend_mode", bind_layer_set_blend_mode },
  { "layer_add_cel", bind_layer_add_cel },
  { "layer_remove_cel", bind_layer_remove_cel },
  { "layer_get_cel", bind_layer_get_cel },
  { "layer_add_layer", bind_layer_add_layer },
  { "layer_remove_layer", bind_layer_remove_layer },
  { "layer_move_layer", bind_layer_move_layer },
  { "layer_render", bind_layer_render },
  { "layer_flatten", bind_layer_flatten },
  { "mask_new", bind_mask_new },
  { "mask_new_copy", bind_mask_new_copy },
  { "mask_free", bind_mask_free },
  { "mask_is_empty", bind_mask_is_empty },
  { "mask_set_name", bind_mask_set_name },
  { "mask_move", bind_mask_move },
  { "mask_none", bind_mask_none },
  { "mask_invert", bind_mask_invert },
  { "mask_replace", bind_mask_replace },
  { "mask_union", bind_mask_union },
  { "mask_subtract", bind_mask_subtract },
  { "mask_intersect", bind_mask_intersect },
  { "mask_merge", bind_mask_merge },
  { "mask_by_color", bind_mask_by_color },
  { "mask_crop", bind_mask_crop },
  { "path_new", bind_path_new },
  { "path_free", bind_path_free },
  { "path_set_join", bind_path_set_join },
  { "path_set_cap", bind_path_set_cap },
  { "path_moveto", bind_path_moveto },
  { "path_lineto", bind_path_lineto },
  { "path_curveto", bind_path_curveto },
  { "path_close", bind_path_close },
  { "path_move", bind_path_move },
  { "path_stroke", bind_path_stroke },
  { "path_fill", bind_path_fill },
  { "sprite_new", bind_sprite_new },
  { "sprite_new_copy", bind_sprite_new_copy },
  { "sprite_new_flatten_copy", bind_sprite_new_flatten_copy },
  { "sprite_new_with_layer", bind_sprite_new_with_layer },
  { "sprite_free", bind_sprite_free },
  { "sprite_is_modified", bind_sprite_is_modified },
  { "sprite_is_associated_to_file", bind_sprite_is_associated_to_file },
  { "sprite_mark_as_saved", bind_sprite_mark_as_saved },
  { "sprite_set_filename", bind_sprite_set_filename },
  { "sprite_set_size", bind_sprite_set_size },
  { "sprite_set_frames", bind_sprite_set_frames },
  { "sprite_set_frlen", bind_sprite_set_frlen },
  { "sprite_get_frlen", bind_sprite_get_frlen },
  { "sprite_set_speed", bind_sprite_set_speed },
  { "sprite_set_path", bind_sprite_set_path },
  { "sprite_set_mask", bind_sprite_set_mask },
  { "sprite_set_layer", bind_sprite_set_layer },
  { "sprite_set_frame", bind_sprite_set_frame },
  { "sprite_set_imgtype", bind_sprite_set_imgtype },
  { "sprite_add_path", bind_sprite_add_path },
  { "sprite_remove_path", bind_sprite_remove_path },
  { "sprite_add_mask", bind_sprite_add_mask },
  { "sprite_remove_mask", bind_sprite_remove_mask },
  { "sprite_request_mask", bind_sprite_request_mask },
  { "sprite_render", bind_sprite_render },
  { "stock_new", bind_stock_new },
  { "stock_new_copy", bind_stock_new_copy },
  { "stock_free", bind_stock_free },
  { "stock_add_image", bind_stock_add_image },
  { "stock_remove_image", bind_stock_remove_image },
  { "stock_replace_image", bind_stock_replace_image },
  { "stock_get_image", bind_stock_get_image },
  { "undo_new", bind_undo_new },
  { "undo_free", bind_undo_free },
  { "undo_enable", bind_undo_enable },
  { "undo_disable", bind_undo_disable },
  { "undo_is_enabled", bind_undo_is_enabled },
  { "undo_is_disabled", bind_undo_is_disabled },
  { "undo_can_undo", bind_undo_can_undo },
  { "undo_can_redo", bind_undo_can_redo },
  { "undo_undo", bind_undo_undo },
  { "undo_redo", bind_undo_redo },
  { "undo_open", bind_undo_open },
  { "undo_close", bind_undo_close },
  { "undo_image", bind_undo_image },
  { "undo_flip", bind_undo_flip },
  { "undo_add_image", bind_undo_add_image },
  { "undo_remove_image", bind_undo_remove_image },
  { "undo_replace_image", bind_undo_replace_image },
  { "undo_add_cel", bind_undo_add_cel },
  { "undo_remove_cel", bind_undo_remove_cel },
  { "undo_add_layer", bind_undo_add_layer },
  { "undo_remove_layer", bind_undo_remove_layer },
  { "undo_move_layer", bind_undo_move_layer },
  { "undo_set_layer", bind_undo_set_layer },
  { "undo_set_mask", bind_undo_set_mask },
  { "effect_new", bind_effect_new },
  { "effect_free", bind_effect_free },
  { "effect_load_target", bind_effect_load_target },
  { "effect_set_target", bind_effect_set_target },
  { "effect_set_target_rgb", bind_effect_set_target_rgb },
  { "effect_set_target_grayscale", bind_effect_set_target_grayscale },
  { "effect_set_target_indexed", bind_effect_set_target_indexed },
  { "effect_begin", bind_effect_begin },
  { "effect_begin_for_preview", bind_effect_begin_for_preview },
  { "effect_apply_step", bind_effect_apply_step },
  { "effect_apply", bind_effect_apply },
  { "effect_flush", bind_effect_flush },
  { "effect_apply_to_image", bind_effect_apply_to_image },
  { "effect_apply_to_target", bind_effect_apply_to_target },
  { "curve_point_new", bind_curve_point_new },
  { "curve_point_free", bind_curve_point_free },
  { "curve_new", bind_curve_new },
  { "curve_free", bind_curve_free },
  { "curve_add_point", bind_curve_add_point },
  { "curve_remove_point", bind_curve_remove_point },
  { "set_color_curve", bind_set_color_curve },
  { "convmatr_new", bind_convmatr_new },
  { "convmatr_new_string", bind_convmatr_new_string },
  { "convmatr_free", bind_convmatr_free },
  { "set_convmatr", bind_set_convmatr },
  { "get_convmatr", bind_get_convmatr },
  { "get_convmatr_by_name", bind_get_convmatr_by_name },
  { "set_replace_colors", bind_set_replace_colors },
  { "set_median_size", bind_set_median_size },
  { NULL, NULL }
};

struct _bindings_constants bindings_constants[] = {
  { "PI", PI },
  { "GFXOBJ_IMAGE", GFXOBJ_IMAGE },
  { "GFXOBJ_STOCK", GFXOBJ_STOCK },
  { "GFXOBJ_CEL", GFXOBJ_CEL },
  { "GFXOBJ_LAYER_IMAGE", GFXOBJ_LAYER_IMAGE },
  { "GFXOBJ_LAYER_SET", GFXOBJ_LAYER_SET },
  { "GFXOBJ_SPRITE", GFXOBJ_SPRITE },
  { "GFXOBJ_MASK", GFXOBJ_MASK },
  { "GFXOBJ_PATH", GFXOBJ_PATH },
  { "GFXOBJ_UNDO", GFXOBJ_UNDO },
  { "BLEND_MODE_NORMAL", BLEND_MODE_NORMAL },
  { "BLEND_MODE_DISSOLVE", BLEND_MODE_DISSOLVE },
  { "BLEND_MODE_MULTIPLY", BLEND_MODE_MULTIPLY },
  { "BLEND_MODE_SCREEN", BLEND_MODE_SCREEN },
  { "BLEND_MODE_OVERLAY", BLEND_MODE_OVERLAY },
  { "BLEND_MODE_HARD_LIGHT", BLEND_MODE_HARD_LIGHT },
  { "BLEND_MODE_DODGE", BLEND_MODE_DODGE },
  { "BLEND_MODE_BURN", BLEND_MODE_BURN },
  { "BLEND_MODE_DARKEN", BLEND_MODE_DARKEN },
  { "BLEND_MODE_LIGHTEN", BLEND_MODE_LIGHTEN },
  { "BLEND_MODE_ADDITION", BLEND_MODE_ADDITION },
  { "BLEND_MODE_SUBTRACT", BLEND_MODE_SUBTRACT },
  { "BLEND_MODE_DIFFERENCE", BLEND_MODE_DIFFERENCE },
  { "BLEND_MODE_HUE", BLEND_MODE_HUE },
  { "BLEND_MODE_SATURATION", BLEND_MODE_SATURATION },
  { "BLEND_MODE_COLOR", BLEND_MODE_COLOR },
  { "BLEND_MODE_LUMINOSITY", BLEND_MODE_LUMINOSITY },
  { "_rgba_r_shift", _rgba_r_shift },
  { "_rgba_g_shift", _rgba_g_shift },
  { "_rgba_b_shift", _rgba_b_shift },
  { "_rgba_a_shift", _rgba_a_shift },
  { "_graya_k_shift", _graya_k_shift },
  { "_graya_a_shift", _graya_a_shift },
  { "IMAGE_RGB", IMAGE_RGB },
  { "IMAGE_GRAYSCALE", IMAGE_GRAYSCALE },
  { "IMAGE_INDEXED", IMAGE_INDEXED },
  { "IMAGE_BITMAP", IMAGE_BITMAP },
  { "PATH_JOIN_MITER", PATH_JOIN_MITER },
  { "PATH_JOIN_ROUND", PATH_JOIN_ROUND },
  { "PATH_JOIN_BEVEL", PATH_JOIN_BEVEL },
  { "PATH_CAP_BUTT", PATH_CAP_BUTT },
  { "PATH_CAP_ROUND", PATH_CAP_ROUND },
  { "PATH_CAP_SQUARE", PATH_CAP_SQUARE },
  { "DITHERING_NONE", DITHERING_NONE },
  { "DITHERING_ORDERED", DITHERING_ORDERED },
  { "CURVE_LINEAR", CURVE_LINEAR },
  { "CURVE_SPLINE", CURVE_SPLINE },
  { NULL, 0 }
};

void register_bindings(lua_State *L)
{
  int c;

  for (c=0; bindings_routines[c].name; c++)
    lua_register(L,
                 bindings_routines[c].name,
                 bindings_routines[c].func);

  for (c=0; bindings_constants[c].name; c++) {
    lua_pushnumber(L, bindings_constants[c].value);
    lua_setglobal(L, bindings_constants[c].name);
  }
}

/*======================================================================*/
/* Lua -> C                                                             */
/*======================================================================*/

void MaskAll(void)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "MaskAll");
  lua_gettable(L, LUA_GLOBALSINDEX);
  do_script_raw(L, 0, 0);
}

void DeselectMask(void)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "DeselectMask");
  lua_gettable(L, LUA_GLOBALSINDEX);
  do_script_raw(L, 0, 0);
}

void ReselectMask(void)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ReselectMask");
  lua_gettable(L, LUA_GLOBALSINDEX);
  do_script_raw(L, 0, 0);
}

void InvertMask(void)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "InvertMask");
  lua_gettable(L, LUA_GLOBALSINDEX);
  do_script_raw(L, 0, 0);
}

void StretchMaskBottom(void)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "StretchMaskBottom");
  lua_gettable(L, LUA_GLOBALSINDEX);
  do_script_raw(L, 0, 0);
}

void ConvolutionMatrix(const char *name, bool r, bool g, bool b, bool k, bool a, bool index)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrix");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  lua_pushboolean(L, r);
  lua_pushboolean(L, g);
  lua_pushboolean(L, b);
  lua_pushboolean(L, k);
  lua_pushboolean(L, a);
  lua_pushboolean(L, index);
  do_script_raw(L, 7, 0);
}

void ConvolutionMatrixRGB(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixRGB");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void ConvolutionMatrixRGBA(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixRGBA");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void ConvolutionMatrixGray(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixGray");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void ConvolutionMatrixGrayA(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixGrayA");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void ConvolutionMatrixIndex(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixIndex");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void ConvolutionMatrixAlpha(const char *name)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "ConvolutionMatrixAlpha");
  lua_gettable(L, LUA_GLOBALSINDEX);
  lua_pushstring(L, name);
  do_script_raw(L, 1, 0);
}

void _ColorCurve(Curve *curve, bool r, bool g, bool b, bool k, bool a, bool index)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurve");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  lua_pushboolean(L, r);
  lua_pushboolean(L, g);
  lua_pushboolean(L, b);
  lua_pushboolean(L, k);
  lua_pushboolean(L, a);
  lua_pushboolean(L, index);
  do_script_raw(L, 7, 0);
}

void _ColorCurveRGB(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveRGB");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}

void _ColorCurveRGBA(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveRGBA");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}

void _ColorCurveGray(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveGray");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}

void _ColorCurveGrayA(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveGrayA");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}

void _ColorCurveIndex(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveIndex");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}

void _ColorCurveAlpha(Curve *curve)
{
  lua_State *L = get_lua_state();
  lua_pushstring(L, "_ColorCurveAlpha");
  lua_gettable(L, LUA_GLOBALSINDEX);
  push_userdata(L, Type_Curve, curve);
  do_script_raw(L, 1, 0);
}


