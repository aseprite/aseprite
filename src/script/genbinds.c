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

static int bind_NewSprite(lua_State *L)
{
  Sprite *return_value;
  int imgtype;
  int w;
  int h;
  GetArg(1, imgtype, int, number);
  GetArg(2, w, int, number);
  GetArg(3, h, int, number);
  return_value = NewSprite(imgtype, w, h);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_LoadSprite(lua_State *L)
{
  Sprite *return_value;
  const char *filename;
  GetArg(1, filename, const char *, string);
  return_value = LoadSprite(filename);
  push_userdata(L, Type_Sprite, return_value);
  return 1;
}

static int bind_SaveSprite(lua_State *L)
{
  const char *filename;
  GetArg(1, filename, const char *, string);
  SaveSprite(filename);
  return 0;
}

static int bind_SetSprite(lua_State *L)
{
  Sprite *sprite;
  GetUD(1, sprite, Sprite);
  SetSprite(sprite);
  return 0;
}

static int bind_NewLayer(lua_State *L)
{
  Layer *return_value;
  return_value = NewLayer();
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_NewLayerSet(lua_State *L)
{
  Layer *return_value;
  return_value = NewLayerSet();
  push_userdata(L, Type_Layer, return_value);
  return 1;
}

static int bind_RemoveLayer(lua_State *L)
{
  RemoveLayer();
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
  { "NewSprite", bind_NewSprite },
  { "LoadSprite", bind_LoadSprite },
  { "SaveSprite", bind_SaveSprite },
  { "SetSprite", bind_SetSprite },
  { "NewLayer", bind_NewLayer },
  { "NewLayerSet", bind_NewLayerSet },
  { "RemoveLayer", bind_RemoveLayer },
  { NULL, NULL }
};

struct _bindings_constants bindings_constants[] = {
  { "PI", PI },
  { "IMAGE_RGB", IMAGE_RGB },
  { "IMAGE_GRAYSCALE", IMAGE_GRAYSCALE },
  { "IMAGE_INDEXED", IMAGE_INDEXED },
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


