/***************************************************************************/
/*                                                                         */
/*  fterrors.h                                                             */
/*                                                                         */
/*    FreeType error codes (specification).                                */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This special header file is used to define the FT2 enumeration        */
  /* constants.  It can also be used to generate error message strings     */
  /* with a small macro trick explained below.                             */
  /*                                                                       */
  /* I - Error Formats                                                     */
  /* -----------------                                                     */
  /*                                                                       */
  /*   Since release 2.1, the error constants have changed.  The lower     */
  /*   byte of the error value gives the "generic" error code, while the   */
  /*   higher byte indicates in which module the error occured.            */
  /*                                                                       */
  /*   You can use the macro FT_ERROR_BASE(x) macro to extract the generic */
  /*   error code from an FT_Error value.                                  */
  /*                                                                       */
  /*   The configuration macro FT_CONFIG_OPTION_USE_MODULE_ERRORS can be   */
  /*   undefined in ftoption.h in order to make the higher byte always     */
  /*   zero, in case you need to be compatible with previous versions of   */
  /*   FreeType 2.                                                         */
  /*                                                                       */
  /*                                                                       */
  /* II - Error Message strings                                            */
  /* --------------------------                                            */
  /*                                                                       */
  /*   The error definitions below are made through special macros that    */
  /*   allow client applications to build a table of error message strings */
  /*   if they need it.  The strings are not included in a normal build of */
  /*   FreeType 2 to save space (most client applications do not use       */
  /*   them).                                                              */
  /*                                                                       */
  /*   To do so, you have to define the following macros before including  */
  /*   this file:                                                          */
  /*                                                                       */
  /*   FT_ERROR_START_LIST ::                                              */
  /*     This macro is called before anything else to define the start of  */
  /*     the error list.  It is followed by several FT_ERROR_DEF calls     */
  /*     (see below).                                                      */
  /*                                                                       */
  /*   FT_ERROR_DEF( e, v, s ) ::                                          */
  /*     This macro is called to define one single error.                  */
  /*     `e' is the error code identifier (e.g. FT_Err_Invalid_Argument).  */
  /*     `v' is the error numerical value.                                 */
  /*     `s' is the corresponding error string.                            */
  /*                                                                       */
  /*   FT_ERROR_END_LIST ::                                                */
  /*     This macro ends the list.                                         */
  /*                                                                       */
  /*   Additionally, you have to undefine __FTERRORS_H__ before #including */
  /*   this file.                                                          */
  /*                                                                       */
  /*   Here is a simple example:                                           */
  /*                                                                       */
  /*     {                                                                 */
  /*       #undef __FTERRORS_H__                                           */
  /*       #define FT_ERRORDEF( e, v, s )   { e, s },                      */
  /*       #define FT_ERROR_START_LIST      {                              */
  /*       #define FT_ERROR_END_LIST        { 0, 0 } };                    */
  /*                                                                       */
  /*       const struct                                                    */
  /*       {                                                               */
  /*         int          err_code;                                        */
  /*         const char*  err_msg                                          */
  /*       } ft_errors[] =                                                 */
  /*                                                                       */
  /*       #include FT_ERRORS_H                                            */
  /*     }                                                                 */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTERRORS_H__
#define __FTERRORS_H__


  /* include module base error codes */
#include FT_MODULE_ERRORS_H


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****                       SETUP MACROS                      *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/


#undef  FT_NEED_EXTERN_C
#define FT_ERR_XCAT( x, y )  x ## y
#define FT_ERR_CAT( x, y )   FT_ERR_XCAT( x, y )


  /* FT_ERR_PREFIX is used as a prefix for error identifiers. */
  /* By default, we use `FT_Err_'.                            */
  /*                                                          */
#ifndef FT_ERR_PREFIX
#define FT_ERR_PREFIX  FT_Err_
#endif


  /* FT_ERR_BASE is used as the base for module-specific errors. */
  /*                                                             */
#ifdef FT_CONFIG_OPTION_USE_MODULE_ERRORS

#ifndef FT_ERR_BASE
#define FT_ERR_BASE  FT_Mod_Err_Base
#endif

#else

#undef FT_ERR_BASE
#define FT_ERR_BASE  0

#endif /* FT_CONFIG_OPTION_USE_MODULE_ERRORS */


  /* If FT_ERRORDEF is not defined, we need to define a simple */
  /* enumeration type.                                         */
  /*                                                           */
#ifndef FT_ERRORDEF

#define FT_ERRORDEF( e, v, s )  e = v,
#define FT_ERROR_START_LIST     enum {
#define FT_ERROR_END_LIST       FT_ERR_CAT( FT_ERR_PREFIX, Max ) };

#ifdef __cplusplus
#define FT_NEED_EXTERN_C
  extern "C" {
#endif

#endif /* !FT_ERRORDEF */


  /* this macro is used to define an error */
#define FT_ERRORDEF_( e, v, s )   \
          FT_ERRORDEF( FT_ERR_CAT( FT_ERR_PREFIX, e ), v + FT_ERR_BASE, s )

  /* this is only used for FT_Err_Ok, which must be 0! */
#define FT_NOERRORDEF_( e, v, s ) \
          FT_ERRORDEF( FT_ERR_CAT( FT_ERR_PREFIX, e ), v, s )


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****                LIST OF ERROR CODES/MESSAGES             *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/


#ifdef FT_ERROR_START_LIST
  FT_ERROR_START_LIST
#endif


  /* generic errors */

  FT_NOERRORDEF_( Ok,                                        0x00, \
                  "no error" )

  FT_ERRORDEF_( Cannot_Open_Resource,                        0x01, \
               "cannot open resource" )
  FT_ERRORDEF_( Unknown_File_Format,                         0x02, \
               "unknown file format" )
  FT_ERRORDEF_( Invalid_File_Format,                         0x03, \
               "broken file" )
  FT_ERRORDEF_( Invalid_Version,                             0x04, \
               "invalid FreeType version" )
  FT_ERRORDEF_( Lower_Module_Version,                        0x05, \
               "module version is too low" )
  FT_ERRORDEF_( Invalid_Argument,                            0x06, \
               "invalid argument" )
  FT_ERRORDEF_( Unimplemented_Feature,                       0x07, \
               "unimplemented feature" )

  /* glyph/character errors */

  FT_ERRORDEF_( Invalid_Glyph_Index,                         0x10, \
               "invalid glyph index" )
  FT_ERRORDEF_( Invalid_Character_Code,                      0x11, \
               "invalid character code" )
  FT_ERRORDEF_( Invalid_Glyph_Format,                        0x12, \
               "unsupported glyph image format" )
  FT_ERRORDEF_( Cannot_Render_Glyph,                         0x13, \
               "cannot render this glyph format" )
  FT_ERRORDEF_( Invalid_Outline,                             0x14, \
               "invalid outline" )
  FT_ERRORDEF_( Invalid_Composite,                           0x15, \
               "invalid composite glyph" )
  FT_ERRORDEF_( Too_Many_Hints,                              0x16, \
               "too many hints" )
  FT_ERRORDEF_( Invalid_Pixel_Size,                          0x17, \
               "invalid pixel size" )

  /* handle errors */

  FT_ERRORDEF_( Invalid_Handle,                              0x20, \
               "invalid object handle" )
  FT_ERRORDEF_( Invalid_Library_Handle,                      0x21, \
               "invalid library handle" )
  FT_ERRORDEF_( Invalid_Driver_Handle,                       0x22, \
               "invalid module handle" )
  FT_ERRORDEF_( Invalid_Face_Handle,                         0x23, \
               "invalid face handle" )
  FT_ERRORDEF_( Invalid_Size_Handle,                         0x24, \
               "invalid size handle" )
  FT_ERRORDEF_( Invalid_Slot_Handle,                         0x25, \
               "invalid glyph slot handle" )
  FT_ERRORDEF_( Invalid_CharMap_Handle,                      0x26, \
               "invalid charmap handle" )
  FT_ERRORDEF_( Invalid_Cache_Handle,                        0x27, \
               "invalid cache manager handle" )
  FT_ERRORDEF_( Invalid_Stream_Handle,                       0x28, \
               "invalid stream handle" )

  /* driver errors */

  FT_ERRORDEF_( Too_Many_Drivers,                            0x30, \
               "too many modules" )
  FT_ERRORDEF_( Too_Many_Extensions,                         0x31, \
               "too many extensions" )

  /* memory errors */

  FT_ERRORDEF_( Out_Of_Memory,                               0x40, \
               "out of memory" )
  FT_ERRORDEF_( Unlisted_Object,                             0x41, \
               "unlisted object" )

  /* stream errors */

  FT_ERRORDEF_( Cannot_Open_Stream,                          0x51, \
               "cannot open stream" )
  FT_ERRORDEF_( Invalid_Stream_Seek,                         0x52, \
               "invalid stream seek" )
  FT_ERRORDEF_( Invalid_Stream_Skip,                         0x53, \
               "invalid stream skip" )
  FT_ERRORDEF_( Invalid_Stream_Read,                         0x54, \
               "invalid stream read" )
  FT_ERRORDEF_( Invalid_Stream_Operation,                    0x55, \
               "invalid stream operation" )
  FT_ERRORDEF_( Invalid_Frame_Operation,                     0x56, \
               "invalid frame operation" )
  FT_ERRORDEF_( Nested_Frame_Access,                         0x57, \
               "nested frame access" )
  FT_ERRORDEF_( Invalid_Frame_Read,                          0x58, \
               "invalid frame read" )

  /* raster errors */

  FT_ERRORDEF_( Raster_Uninitialized,                        0x60, \
               "raster uninitialized" )
  FT_ERRORDEF_( Raster_Corrupted,                            0x61, \
               "raster corrupted" )
  FT_ERRORDEF_( Raster_Overflow,                             0x62, \
               "raster overflow" )
  FT_ERRORDEF_( Raster_Negative_Height,                      0x63, \
               "negative height while rastering" )

  /* cache errors */

  FT_ERRORDEF_( Too_Many_Caches,                             0x70, \
               "too many registered caches" )

  /* TrueType and SFNT errors */

  FT_ERRORDEF_( Invalid_Opcode,                              0x80, \
               "invalid opcode" )
  FT_ERRORDEF_( Too_Few_Arguments,                           0x81, \
               "too few arguments" )
  FT_ERRORDEF_( Stack_Overflow,                              0x82, \
               "stack overflow" )
  FT_ERRORDEF_( Code_Overflow,                               0x83, \
               "code overflow" )
  FT_ERRORDEF_( Bad_Argument,                                0x84, \
               "bad argument" )
  FT_ERRORDEF_( Divide_By_Zero,                              0x85, \
               "division by zero" )
  FT_ERRORDEF_( Invalid_Reference,                           0x86, \
               "invalid reference" )
  FT_ERRORDEF_( Debug_OpCode,                                0x87, \
               "found debug opcode" )
  FT_ERRORDEF_( ENDF_In_Exec_Stream,                         0x88, \
               "found ENDF opcode in execution stream" )
  FT_ERRORDEF_( Nested_DEFS,                                 0x89, \
               "nested DEFS" )
  FT_ERRORDEF_( Invalid_CodeRange,                           0x8A, \
               "invalid code range" )
  FT_ERRORDEF_( Execution_Too_Long,                          0x8B, \
               "execution context too long" )
  FT_ERRORDEF_( Too_Many_Function_Defs,                      0x8C, \
               "too many function definitions" )
  FT_ERRORDEF_( Too_Many_Instruction_Defs,                   0x8D, \
               "too many instruction definitions" )
  FT_ERRORDEF_( Table_Missing,                               0x8E, \
               "SFNT font table missing" )
  FT_ERRORDEF_( Horiz_Header_Missing,                        0x8F, \
               "horizontal header (hhea) table missing" )
  FT_ERRORDEF_( Locations_Missing,                           0x90, \
               "locations (loca) table missing" )
  FT_ERRORDEF_( Name_Table_Missing,                          0x91, \
               "name table missing" )
  FT_ERRORDEF_( CMap_Table_Missing,                          0x92, \
               "character map (cmap) table missing" )
  FT_ERRORDEF_( Hmtx_Table_Missing,                          0x93, \
               "horizontal metrics (hmtx) table missing" )
  FT_ERRORDEF_( Post_Table_Missing,                          0x94, \
               "PostScript (post) table missing" )
  FT_ERRORDEF_( Invalid_Horiz_Metrics,                       0x95, \
               "invalid horizontal metrics" )
  FT_ERRORDEF_( Invalid_CharMap_Format,                      0x96, \
               "invalid character map (cmap) format" )
  FT_ERRORDEF_( Invalid_PPem,                                0x97, \
               "invalid ppem value" )
  FT_ERRORDEF_( Invalid_Vert_Metrics,                        0x98, \
               "invalid vertical metrics" )
  FT_ERRORDEF_( Could_Not_Find_Context,                      0x99, \
               "could not find context" )
  FT_ERRORDEF_( Invalid_Post_Table_Format,                   0x9A, \
               "invalid PostScript (post) table format" )
  FT_ERRORDEF_( Invalid_Post_Table,                          0x9B, \
               "invalid PostScript (post) table" )

  /* CFF, CID, and Type 1 errors */

  FT_ERRORDEF_( Syntax_Error,                                0xA0, \
               "opcode syntax error" )
  FT_ERRORDEF_( Stack_Underflow,                             0xA1, \
               "argument stack underflow" )


#ifdef FT_ERROR_END_LIST
  FT_ERROR_END_LIST
#endif


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****                      SIMPLE CLEANUP                     *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/

#ifdef FT_NEED_EXTERN_C
  }
#endif

#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

#undef FT_ERRORDEF
#undef FT_ERRORDEF_
#undef FT_NOERRORDEF_

#undef FT_NEED_EXTERN_C
#undef FT_ERR_PREFIX
#undef FT_ERR_BASE
#undef FT_ERR_CONCAT

#endif /* __FTERRORS_H__ */

/* END */
