/***************************************************************************/
/*                                                                         */
/*  psaux.h                                                                */
/*                                                                         */
/*    Auxiliary functions and data structures related to PostScript fonts  */
/*    (specification).                                                     */
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


#ifndef __PSAUX_H__
#define __PSAUX_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                             T1_TABLE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  typedef struct PS_Table_  PS_Table;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    PS_Table_Funcs                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A set of function pointers to manage PS_Table objects.             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    table_init    :: Used to initialize a table.                       */
  /*                                                                       */
  /*    table_done    :: Finalizes resp. destroy a given table.            */
  /*                                                                       */
  /*    table_add     :: Adds a new object to a table.                     */
  /*                                                                       */
  /*    table_release :: Releases table data, then finalizes it.           */
  /*                                                                       */
  typedef struct  PS_Table_Funcs_
  {
    FT_Error
    (*init)( PS_Table*  table,
             FT_Int     count,
             FT_Memory  memory );

    void
    (*done)( PS_Table*  table );

    FT_Error
    (*add)( PS_Table*  table,
            FT_Int     index,
            void*      object,
            FT_Int     length );

    void
    (*release)( PS_Table*  table );

  } PS_Table_Funcs;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    PS_Table                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A PS_Table is a simple object used to store an array of objects in */
  /*    a single memory block.                                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    block     :: The address in memory of the growheap's block.  This  */
  /*                 can change between two object adds, due to            */
  /*                 reallocation.                                         */
  /*                                                                       */
  /*    cursor    :: The current top of the grow heap within its block.    */
  /*                                                                       */
  /*    capacity  :: The current size of the heap block.  Increments by    */
  /*                 1kByte chunks.                                        */
  /*                                                                       */
  /*    max_elems :: The maximum number of elements in table.              */
  /*                                                                       */
  /*    num_elems :: The current number of elements in table.              */
  /*                                                                       */
  /*    elements  :: A table of element addresses within the block.        */
  /*                                                                       */
  /*    lengths   :: A table of element sizes within the block.            */
  /*                                                                       */
  /*    memory    :: The object used for memory operations                 */
  /*                 (alloc/realloc).                                      */
  /*                                                                       */
  /*    funcs     :: A table of method pointers for this object.           */
  /*                                                                       */
  struct  PS_Table_
  {
    FT_Byte*        block;          /* current memory block           */
    FT_Offset       cursor;         /* current cursor in memory block */
    FT_Offset       capacity;       /* current size of memory block   */
    FT_Long         init;

    FT_Int          max_elems;
    FT_Int          num_elems;
    FT_Byte**       elements;       /* addresses of table elements */
    FT_Int*         lengths;        /* lengths of table elements   */

    FT_Memory       memory;
    PS_Table_Funcs  funcs;

  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       T1 FIELDS & TOKENS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct T1_Parser_  T1_Parser;

  /* simple enumeration type used to identify token types */
  typedef enum  T1_Token_Type_
  {
    t1_token_none = 0,
    t1_token_any,
    t1_token_string,
    t1_token_array,

    /* do not remove */
    t1_token_max

  } T1_Token_Type;


  /* a simple structure used to identify tokens */
  typedef struct  T1_Token_
  {
    FT_Byte*       start;   /* first character of token in input stream */
    FT_Byte*       limit;   /* first character after the token          */
    T1_Token_Type  type;    /* type of token                            */

  } T1_Token;


  /* enumeration type used to identify object fields */
  typedef enum  T1_Field_Type_
  {
    t1_field_none = 0,
    t1_field_bool,
    t1_field_integer,
    t1_field_fixed,
    t1_field_string,
    t1_field_integer_array,
    t1_field_fixed_array,
    t1_field_callback,

    /* do not remove */
    t1_field_max

  } T1_Field_Type;

  typedef enum  T1_Field_Location_
  {
    t1_field_cid_info,
    t1_field_font_dict,
    t1_field_font_info,
    t1_field_private,

    /* do not remove */
    t1_field_location_max

  } T1_Field_Location;


  typedef void
  (*T1_Field_Parser)( FT_Face     face,
                      FT_Pointer  parser );


  /* structure type used to model object fields */
  typedef struct  T1_Field_
  {
    const char*        ident;        /* field identifier               */
    T1_Field_Location  location;
    T1_Field_Type      type;         /* type of field                  */
    T1_Field_Parser    reader;
    FT_UInt            offset;       /* offset of field in object      */
    FT_Byte            size;         /* size of field in bytes         */
    FT_UInt            array_max;    /* maximal number of elements for */
                                     /* array                          */
    FT_UInt            count_offset; /* offset of element count for    */
                                     /* arrays                         */
  } T1_Field;


#define T1_NEW_SIMPLE_FIELD( _ident, _type, _fname ) \
          {                                          \
            _ident, T1CODE, _type,                   \
            0,                                       \
            FT_FIELD_OFFSET( _fname ),               \
            FT_FIELD_SIZE( _fname ),                 \
            0, 0                                     \
          },

#define T1_NEW_CALLBACK_FIELD( _ident, _reader ) \
          {                                      \
            _ident, T1CODE, t1_field_callback,   \
            (T1_Field_Parser)_reader,            \
            0, 0,                                \
            0, 0                                 \
          },

#define T1_NEW_TABLE_FIELD( _ident, _type, _fname, _max ) \
          {                                               \
            _ident, T1CODE, _type,                        \
            0,                                            \
            FT_FIELD_OFFSET( _fname ),                    \
            FT_FIELD_SIZE_DELTA( _fname ),                \
            _max,                                         \
            FT_FIELD_OFFSET( num_ ## _fname )             \
          },

#define T1_NEW_TABLE_FIELD2( _ident, _type, _fname, _max ) \
          {                                                \
            _ident, T1CODE, _type,                         \
            0,                                             \
            FT_FIELD_OFFSET( _fname ),                     \
            FT_FIELD_SIZE_DELTA( _fname ),                 \
            _max, 0                                        \
          },


#define T1_FIELD_BOOL( _ident, _fname )                           \
          T1_NEW_SIMPLE_FIELD( _ident, t1_field_bool, _fname )

#define T1_FIELD_NUM( _ident, _fname )                            \
          T1_NEW_SIMPLE_FIELD( _ident, t1_field_integer, _fname )

#define T1_FIELD_FIXED( _ident, _fname )                          \
          T1_NEW_SIMPLE_FIELD( _ident, t1_field_fixed, _fname )

#define T1_FIELD_STRING( _ident, _fname )                         \
          T1_NEW_SIMPLE_FIELD( _ident, t1_field_string, _fname )

#define T1_FIELD_NUM_TABLE( _ident, _fname, _fmax )               \
          T1_NEW_TABLE_FIELD( _ident, t1_field_integer_array,     \
                               _fname, _fmax )

#define T1_FIELD_FIXED_TABLE( _ident, _fname, _fmax )             \
          T1_NEW_TABLE_FIELD( _ident, t1_field_fixed_array,       \
                               _fname, _fmax )

#define T1_FIELD_NUM_TABLE2( _ident, _fname, _fmax )              \
          T1_NEW_TABLE_FIELD2( _ident, t1_field_integer_array,    \
                                _fname, _fmax )

#define T1_FIELD_FIXED_TABLE2( _ident, _fname, _fmax )            \
          T1_NEW_TABLE_FIELD2( _ident, t1_field_fixed_array,      \
                                _fname, _fmax )

#define T1_FIELD_CALLBACK( _ident, _name )                        \
          T1_NEW_CALLBACK_FIELD( _ident, _name )




  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                            T1 PARSER                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  T1_Parser_Funcs_
  {
    void
    (*init)( T1_Parser*  parser,
             FT_Byte*    base,
             FT_Byte*    limit,
             FT_Memory   memory );

    void
    (*done)( T1_Parser*  parser );

    void
    (*skip_spaces)( T1_Parser*  parser );
    void
    (*skip_alpha)( T1_Parser*  parser );

    FT_Long
    (*to_int)( T1_Parser*  parser );
    FT_Fixed
    (*to_fixed)( T1_Parser*  parser,
                 FT_Int      power_ten );
    FT_Int
    (*to_coord_array)( T1_Parser*  parser,
                       FT_Int      max_coords,
                       FT_Short*   coords );
    FT_Int
    (*to_fixed_array)( T1_Parser*  parser,
                       FT_Int      max_values,
                       FT_Fixed*   values,
                       FT_Int      power_ten );

    void
    (*to_token)( T1_Parser*  parser,
                 T1_Token*   token );
    void
    (*to_token_array)( T1_Parser*  parser,
                       T1_Token*   tokens,
                       FT_UInt     max_tokens,
                       FT_Int*     pnum_tokens );

    FT_Error
    (*load_field)( T1_Parser*       parser,
                   const T1_Field*  field,
                   void**           objects,
                   FT_UInt          max_objects,
                   FT_ULong*        pflags );

    FT_Error
    (*load_field_table)( T1_Parser*       parser,
                         const T1_Field*  field,
                         void**           objects,
                         FT_UInt          max_objects,
                         FT_ULong*        pflags );

  } T1_Parser_Funcs;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Parser                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A T1_Parser is an object used to parse a Type 1 font very quickly. */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    cursor :: The current position in the text.                        */
  /*                                                                       */
  /*    base   :: Start of the processed text.                             */
  /*                                                                       */
  /*    limit  :: End of the processed text.                               */
  /*                                                                       */
  /*    error  :: The last error returned.                                 */
  /*                                                                       */
  /*    memory :: The object used for memory operations (alloc/realloc).   */
  /*                                                                       */
  /*    funcs  :: A table of functions for the parser.                     */
  /*                                                                       */
  struct T1_Parser_
  {
    FT_Byte*         cursor;
    FT_Byte*         base;
    FT_Byte*         limit;
    FT_Error         error;
    FT_Memory        memory;

    T1_Parser_Funcs  funcs;
  };



  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                         T1 BUILDER                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  typedef struct T1_Builder_  T1_Builder;


  typedef FT_Error
  (*T1_Builder_Check_Points_Func)( T1_Builder*  builder,
                                  FT_Int       count );

  typedef void
  (*T1_Builder_Add_Point_Func)( T1_Builder*  builder,
                                FT_Pos       x,
                                FT_Pos       y,
                                FT_Byte      flag );

  typedef FT_Error
  (*T1_Builder_Add_Point1_Func)( T1_Builder*  builder,
                                 FT_Pos       x,
                                 FT_Pos       y );

  typedef FT_Error
  (*T1_Builder_Add_Contour_Func)( T1_Builder*  builder );

  typedef FT_Error
  (*T1_Builder_Start_Point_Func)( T1_Builder*  builder,
                                  FT_Pos       x,
                                  FT_Pos       y );

  typedef void
  (*T1_Builder_Close_Contour_Func)( T1_Builder*  builder );


  typedef struct  T1_Builder_Funcs_
  {
    void
    (*init)( T1_Builder*   builder,
             FT_Face       face,
             FT_Size       size,
             FT_GlyphSlot  slot,
             FT_Bool       hinting );

    void
    (*done)( T1_Builder*   builder );

    T1_Builder_Check_Points_Func   check_points;
    T1_Builder_Add_Point_Func      add_point;
    T1_Builder_Add_Point1_Func     add_point1;
    T1_Builder_Add_Contour_Func    add_contour;
    T1_Builder_Start_Point_Func    start_point;
    T1_Builder_Close_Contour_Func  close_contour;

  } T1_Builder_Funcs;



  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    T1_Builder                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*     A structure used during glyph loading to store its outline.       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    face         :: The current face object.                           */
  /*                                                                       */
  /*    glyph        :: The current glyph slot.                            */
  /*                                                                       */
  /*    loader       :: XXX                                                */
  /*                                                                       */
  /*    base         :: The base glyph outline.                            */
  /*                                                                       */
  /*    current      :: The current glyph outline.                         */
  /*                                                                       */
  /*    max_points   :: maximum points in builder outline                  */
  /*                                                                       */
  /*    max_contours :: Maximal number of contours in builder outline.     */
  /*                                                                       */
  /*    last         :: The last point position.                           */
  /*                                                                       */
  /*    scale_x      :: The horizontal scale (FUnits to sub-pixels).       */
  /*                                                                       */
  /*    scale_y      :: The vertical scale (FUnits to sub-pixels).         */
  /*                                                                       */
  /*    pos_x        :: The horizontal translation (if composite glyph).   */
  /*                                                                       */
  /*    pos_y        :: The vertical translation (if composite glyph).     */
  /*                                                                       */
  /*    left_bearing :: The left side bearing point.                       */
  /*                                                                       */
  /*    advance      :: The horizontal advance vector.                     */
  /*                                                                       */
  /*    bbox         :: Unused.                                            */
  /*                                                                       */
  /*    path_begun   :: A flag which indicates that a new path has begun.  */
  /*                                                                       */
  /*    load_points  :: If this flag is not set, no points are loaded.     */
  /*                                                                       */
  /*    no_recurse   :: Set but not used.                                  */
  /*                                                                       */
  /*    error        :: An error code that is only used to report memory   */
  /*                    allocation problems.                               */
  /*                                                                       */
  /*    metrics_only :: A boolean indicating that we only want to compute  */
  /*                    the metrics of a given glyph, not load all of its  */
  /*                    points.                                            */
  /*                                                                       */
  /*    funcs        :: An array of function pointers for the builder.     */
  /*                                                                       */
  struct  T1_Builder_
  {
    FT_Memory         memory;
    FT_Face           face;
    FT_GlyphSlot      glyph;
    FT_GlyphLoader*   loader;
    FT_Outline*       base;
    FT_Outline*       current;

    FT_Vector         last;

    FT_Fixed          scale_x;
    FT_Fixed          scale_y;

    FT_Pos            pos_x;
    FT_Pos            pos_y;

    FT_Vector         left_bearing;
    FT_Vector         advance;

    FT_BBox           bbox;          /* bounding box */
    FT_Bool           path_begun;
    FT_Bool           load_points;
    FT_Bool           no_recurse;
    FT_Bool           shift;

    FT_Error          error;         /* only used for memory errors */
    FT_Bool           metrics_only;

    void*             hints_funcs;    /* hinter-specific */
    void*             hints_globals;  /* hinter-specific */

    T1_Builder_Funcs  funcs;      
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                         T1 DECODER                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#if 0

  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_SUBRS_CALLS details the maximum number of nested sub-routine   */
  /* calls during glyph loading.                                           */
  /*                                                                       */
#define T1_MAX_SUBRS_CALLS  8


  /*************************************************************************/
  /*                                                                       */
  /* T1_MAX_CHARSTRING_OPERANDS is the charstring stack's capacity.  A     */
  /* minimum of 16 is required.                                            */
  /*                                                                       */
#define T1_MAX_CHARSTRINGS_OPERANDS  32

#endif /* 0 */


  typedef struct  T1_Decoder_Zone_
  {
    FT_Byte*  cursor;
    FT_Byte*  base;
    FT_Byte*  limit;

  } T1_Decoder_Zone;


  typedef struct T1_Decoder_        T1_Decoder;
  typedef struct T1_Decoder_Funcs_  T1_Decoder_Funcs;


  typedef FT_Error
  (*T1_Decoder_Callback)( T1_Decoder*  decoder,
                          FT_UInt      glyph_index );


  struct  T1_Decoder_Funcs_
  {
    FT_Error
    (*init) ( T1_Decoder*          decoder,
              FT_Face              face,
              FT_Size              size,
              FT_GlyphSlot         slot,
              FT_Byte**            glyph_names,
              T1_Blend*            blend,
              FT_Bool              hinting,
              T1_Decoder_Callback  callback );

    void
    (*done) ( T1_Decoder*  decoder );

    FT_Error
    (*parse_charstrings)( T1_Decoder*  decoder,
                          FT_Byte*     base,
                          FT_UInt      len );
  };


  struct  T1_Decoder_
  {
    T1_Builder           builder;

    FT_Long              stack[T1_MAX_CHARSTRINGS_OPERANDS];
    FT_Long*             top;

    T1_Decoder_Zone      zones[T1_MAX_SUBRS_CALLS + 1];
    T1_Decoder_Zone*     zone;

    PSNames_Interface*   psnames;      /* for seac */
    FT_UInt              num_glyphs;
    FT_Byte**            glyph_names;

    FT_Int               lenIV;        /* internal for sub routine calls */
    FT_UInt              num_subrs;
    FT_Byte**            subrs;
    FT_Int*              subrs_len;    /* array of subrs length (optional) */

    FT_Matrix            font_matrix;
    FT_Vector            font_offset;

    FT_Int               flex_state;
    FT_Int               num_flex_vectors;
    FT_Vector            flex_vectors[7];

    T1_Blend*            blend;       /* for multiple master support */

    T1_Decoder_Callback  parse_callback;
    T1_Decoder_Funcs     funcs;
  };


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                        PSAux Module Interface                 *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  PSAux_Interface_
  {
    const PS_Table_Funcs*    ps_table_funcs;
    const T1_Parser_Funcs*   t1_parser_funcs;
    const T1_Builder_Funcs*  t1_builder_funcs;
    const T1_Decoder_Funcs*  t1_decoder_funcs;

    void
    (*t1_decrypt)( FT_Byte*   buffer,
                   FT_Offset  length,
                   FT_UShort  seed );

  } PSAux_Interface;


FT_END_HEADER

#endif /* __PSAUX_H__ */


/* END */
