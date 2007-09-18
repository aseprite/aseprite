#! /usr/bin/env python
# Lua bindings generator
# by David A. Capello

import re

routines = [ ]
constants = [ ]
import_routines = [ ]

comment1_regex = re.compile (r"^\/\*")
comment2_regex = re.compile (r"^\/\/")
nullline_regex = re.compile (r"^[ \t]*$")
function_regex = re.compile (r"^ *(?P<type>(const )?[A-Za-z0-9_]+ \*?)?(?P<name>[A-Za-z0-9_]+) \((?P<args>.*)\)(?P<extra>.*)")
void_regex = re.compile (r"^void")
constant_regex = re.compile (r"^\#define (?P<name>[A-Za-z0-9_]+)")
arg_regex = re.compile (r" ?(const )?(?P<type>[A-Za-z_]+ ?\*?\&?)(?P<name>[A-Za-z0-9_]*)")
# userdata_regex = re.compile (r"(?P<type>[A-Za-z_]+) \*$")
# userdata_regex = re.compile (r"(?P<type>[A-Z][a-z_]*)(\ \*)?$")
userdata_regex = re.compile (r"(?P<type>[A-Za-z_]*) ?\*?")
codetag_regex = re.compile (r"CODE")

# ----------------------------------------------------------------------
def convert_to_lua_type (type):
  if re.compile (r"^bool").search (type): return "boolean"
  if re.compile (r"^int").search (type): return "number"
  if re.compile (r"^float").search (type): return "number"
  if re.compile (r"^double").search (type): return "number"
  if re.compile (r"char\ \*").search (type): return "string"
  return "userdata"

# ----------------------------------------------------------------------
def convert_to_c_type (type):
  if re.compile (r"^bool").search (type): return "bool "
  if re.compile (r"^int").search (type): return "int "
  if re.compile (r"^float").search (type): return "float "
  if re.compile (r"^double").search (type): return "double "
  if re.compile (r"char\ \*").search (type): return "const char *"
  return type

# ----------------------------------------------------------------------
def declare_arguments (args, tabs, offset):
  if args:
    for c in range (len (args)):
      arg = arg_regex.match (args[c])
      cast = convert_to_c_type (arg.group ("type"))
      if not void_regex.search (cast):
        lua_type = convert_to_lua_type (arg.group ("type"))
        if lua_type != "userdata":
          file_out.write (tabs + convert_to_c_type (arg.group ("type")) +
                          arg.group ("name") + ";\n")
        else:
          userdata = userdata_regex.match (arg.group ("type"))
          if userdata and userdata.group ("type"):
#           file_out.write (tabs + "UserData *" + arg.group ("name") + ";\n")
#           file_out.write (tabs + userdata.group ("type") + " *" + arg.group ("name") + ";\n")
            file_out.write (tabs + cast + arg.group ("name") + ";\n")

# ----------------------------------------------------------------------
def check_arguments (args, tabs, offset):
  if args:
    for c in range (len (args)):
      arg = arg_regex.match (args[c])
      cast = convert_to_c_type (arg.group ("type"))
      if not void_regex.search (cast):
        lua_type = convert_to_lua_type (arg.group ("type"))
        if lua_type != "userdata":
          file_out.write (tabs + "GetArg ("  +
                          str (c+offset)     + ", " +
                          arg.group ("name") + ", " +
                          cast.strip ()      + ", " +
                          lua_type           + ");\n")
        else:
          userdata = userdata_regex.match (arg.group ("type"))
          if userdata and userdata.group ("type"):
#           file_out.write (tabs + "GetUD ("   +
#                           str (c+offset)     + ", " +
#                           arg.group ("name") + ");\n")
            file_out.write (tabs + "GetUD ("       +
                            str (c+offset)         + ", " +
                            arg.group ("name")     + ", " +
                            userdata.group ("type") + ");\n")

# ----------------------------------------------------------------------
def call_arguments (args, offset):
  if args:
    for c in range (len (args)):
      arg = arg_regex.match (args[c])
      cast = convert_to_c_type (arg.group ("type"))
      if not void_regex.search (cast):
        lua_type = convert_to_lua_type (arg.group ("type"))
        file_out.write (arg.group ("name"))
        if c < len (args)-1: file_out.write (", ")
#       if lua_type != "userdata":
#         file_out.write (arg.group ("name"))
#       else:
#         userdata = userdata_regex.match (arg.group ("type"))
#         if userdata and userdata.group ("type"):
#           file_out.write ("GetUD ("              +
#                           userdata.group ("type") + ", " +
#                           arg.group ("name")     + ")")

# ----------------------------------------------------------------------
def export_function (type, name, args, extra):
  args_sep = args.split (',')
  print "Export function " + name
  if codetag_regex.search (extra):
    routines.append (name)
    return
  file_out.write ("static int bind_" + name + " (lua_State *L)\n")
  file_out.write ("{\n")
  # declare return value
  if not void_regex.search (type):
    file_out.write ("  " + convert_to_c_type (type) + "return_value;\n")
  # declare arguments
  declare_arguments (args_sep, "  ", 1)
  # check arguments
  check_arguments (args_sep, "  ", 1)
  # call the routine
  file_out.write ("  ")
  if not void_regex.search (type):
    file_out.write ("return_value = ")
  file_out.write (name + " (")
  call_arguments (args_sep, 1)
  file_out.write (");\n")
  # return value
  if not void_regex.search (type):
    lua_type = convert_to_lua_type (type)
    if lua_type == "userdata":
      userdata = userdata_regex.match (type)
      if userdata and userdata.group ("type"):
        file_out.write ("  push_userdata (L, Type_" +
                        userdata.group ("type") + ", return_value);\n")
    else:
      file_out.write ("  lua_push" + lua_type + " (L, return_value);\n")
    file_out.write ("  return 1;\n")
  else:
    file_out.write ("  return 0;\n")
  # done
  file_out.write ("}\n\n")
  # count this routine
  routines.append (name)

# ----------------------------------------------------------------------
def export_constant (name):
  print "Export constant " + name
  constants.append (name)

# ----------------------------------------------------------------------
def import_function (type, name, args):
  args_sep = args.split (',')
  print "Import function " + name
  file_out.write (type + name + " (" + args + ")\n")
  file_out.write ("{\n")
  file_out.write ("  lua_State *L = get_lua_state ();\n")
  # declare return value
  if not void_regex.search (type):
    file_out.write ("  " + type + "return_value;\n")
  # push function
  file_out.write ("  lua_pushstring (L, \"" + name + "\");\n");
  file_out.write ("  lua_gettable (L, LUA_GLOBALSINDEX);\n");
  # how many arguments
  if not void_regex.search (args):
    nargs = len (args_sep)
  else:
    nargs = 0
  # push arguments
  for c in range (nargs):
    arg = arg_regex.match (args_sep[c])
    cast = convert_to_c_type (arg.group ("type"))
    lua_type = convert_to_lua_type (arg.group ("type"))
    if lua_type != "userdata":
      file_out.write ("  lua_push" + lua_type + " (L, " +
                      arg.group ("name") + ");\n")
    else:
      userdata = userdata_regex.match (arg.group ("type"))
      if userdata and userdata.group ("type"):
        file_out.write ("  push_userdata (L, " +
                        "Type_" + userdata.group ("type") + ", " +
                        arg.group ("name") + ");\n")
  # call the script routine
  if not void_regex.search (type):
    ret = 1
  else:
    ret = 0
  file_out.write ("  do_script_raw (L, " + str (nargs) + ", " + str (ret) + ");\n");
  # return value
  if not void_regex.search (type):
    lua_type = convert_to_lua_type (type)
    if lua_type != "userdata":
      file_out.write ("  return_value = lua_to" + lua_type + " (L, -1);\n")
    else:
      userdata = userdata_regex.match (type)
      if userdata and userdata.group ("type"):
        file_out.write ("  return_value = to_userdata (L, Type_" +
                        userdata.group ("type") + ", -1);\n");
    file_out.write ("  lua_pop (L, 1);\n");
    file_out.write ("  return return_value;\n");
  # done
  file_out.write ("}\n\n")

# ----------------------------------------------------------------------

file_in = open ("export.h", 'r')
file_out = open ("genbinds.c", 'w')

file_out.write ("/* Generated with bindings.gen */\n")
file_out.write ("\n")
file_out.write ("/*======================================================================*/\n")
file_out.write ("/* C -> Lua                                                             */\n")
file_out.write ("/*======================================================================*/\n")
file_out.write ("\n")
file_out.write ("#define GetArg(idx, var, cast, type)               \\\n")
file_out.write ("  if (lua_isnil (L, idx) || lua_is##type (L, idx)) \\\n")
file_out.write ("    var = (cast)lua_to##type (L, idx);             \\\n")
file_out.write ("  else                                             \\\n")
file_out.write ("    return 0;\n")
file_out.write ("\n")
file_out.write ("#define GetUD(idx, var, type)                      \\\n")
file_out.write ("  var = to_userdata (L, Type_##type, idx);\n")
file_out.write ("\n")

while 1:
  line = file_in.readline ()
  if not line: break
  line = line.strip ()
  # kill comments and white lines
  if comment1_regex.search (line): continue
  elif comment2_regex.search (line): continue
  elif nullline_regex.search (line): continue
  # functions: type name (args...)
  elif function_regex.search (line):
    func = function_regex.match (line)
    export_function (func.group ("type"),
                     func.group ("name"),
                     func.group ("args"),
                     func.group ("extra"))
  # constants: type name (args...)
  elif constant_regex.search (line):
    c = constant_regex.match (line)
    export_constant (c.group ("name"))

if len (routines) > 0:
  file_out.write ("const luaL_reg bindings_routines[] = {\n")
  for c in range (len (routines)):
    file_out.write ("  { \"" + routines[c] + "\", bind_" + routines[c] + " }")
    file_out.write (",\n")
  file_out.write ("  { NULL, NULL }\n")
  file_out.write ("};\n\n")

if len (constants) > 0:
  file_out.write ("struct _bindings_constants bindings_constants[] = {\n")
  for c in range (len (constants)):
    file_out.write ("  { \"" + constants[c] + "\", " + constants[c] + " }")
    file_out.write (",\n")
  file_out.write ("  { NULL, 0 }\n")
  file_out.write ("};\n\n")

file_out.write ("void register_bindings (lua_State *L)\n")
file_out.write ("{\n")
file_out.write ("  int c;\n")

if len (routines) > 0:
  file_out.write ("\n")
  file_out.write ("  for (c=0; bindings_routines[c].name; c++)\n")
  file_out.write ("    lua_register (L,\n")
  file_out.write ("                  bindings_routines[c].name,\n")
  file_out.write ("                  bindings_routines[c].func);\n")

if len (constants) > 0:
  file_out.write ("\n")
  file_out.write ("  for (c=0; bindings_constants[c].name; c++) {\n")
  file_out.write ("    lua_pushnumber (L, bindings_constants[c].value);\n")
  file_out.write ("    lua_setglobal (L, bindings_constants[c].name);\n")
  file_out.write ("  }\n")
  
file_out.write ("}\n\n")

file_in.close ()
file_in = open ("import.h", 'r')

file_out.write ("/*======================================================================*/\n")
file_out.write ("/* Lua -> C                                                             */\n")
file_out.write ("/*======================================================================*/\n")
file_out.write ("\n")

while 1:
  line = file_in.readline ()
  if not line: break
  line = line.strip ()
  # kill comments and white lines
  if comment1_regex.search (line): continue
  elif comment2_regex.search (line): continue
  elif nullline_regex.search (line): continue
  # functions: type name (args...)
  elif function_regex.search (line):
    func = function_regex.match (line)
    import_function (func.group ("type"),
                     func.group ("name"),
                     func.group ("args"))

file_out.write ("\n")

file_in.close ()
file_out.close ()

# header file

file_out = open ("genbinds.h", 'w')
file_out.write ("/* Generated with bindings.gen */\n")
file_out.write ("\n")
file_out.write ("#ifndef SCRIPT_GENBINDS_H\n")
file_out.write ("#define SCRIPT_GENBINDS_H\n")
file_out.write ("\n")
file_out.write ("#include \"script/script.h\"\n")
if len (routines) > 0:
  file_out.write ("\n")
  file_out.write ("extern const luaL_reg bindings_routines[];\n")
if len (constants) > 0:
  file_out.write ("\n");
  file_out.write ("struct _bindings_constants {\n")
  file_out.write ("  const char *name;\n")
  file_out.write ("  double value;\n")
  file_out.write ("};\n")
  file_out.write ("\n")
  file_out.write ("extern struct _bindings_constants bindings_constants[];\n")
  file_out.write ("\n")
  file_out.write ("void register_bindings (lua_State *L);\n")
file_out.write ("\n#endif /* SCRIPT_GENBINDS_H */\n")
file_out.close ()
