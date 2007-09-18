/***************************************************************************/
/*                                                                         */
/*  t1parse.h                                                              */
/*                                                                         */
/*    Type 1 parser (specification).                                       */
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


#ifndef __T1PARSE_H__
#define __T1PARSE_H__


#include <ft2build.h>
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_INTERNAL_STREAM_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_ParserRec                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A T1_ParserRec is an object used to parse a Type 1 fonts very      */
  /*    quickly.                                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root         :: The root parser.                                   */
  /*                                                                       */
  /*    stream       :: The current input stream.                          */
  /*                                                                       */
  /*    base_dict    :: A pointer to the top-level dictionary.             */
  /*                                                                       */
  /*    base_len     :: The length in bytes of the top dictionary.         */
  /*                                                                       */
  /*    private_dict :: A pointer to the private dictionary.               */
  /*                                                                       */
  /*    private_len  :: The length in bytes of the private dictionary.     */
  /*                                                                       */
  /*    in_pfb       :: A boolean.  Indicates that we are handling a PFB   */
  /*                    file.                                              */
  /*                                                                       */
  /*    in_memory    :: A boolean.  Indicates a memory-based stream.       */
  /*                                                                       */
  /*    single_block :: A boolean.  Indicates that the private dictionary  */
  /*                    is stored in lieu of the base dictionary.          */
  /*                                                                       */
  typedef struct  T1_ParserRec_
  {
    T1_Parser  root;
    FT_Stream  stream;

    FT_Byte*   base_dict;
    FT_Int     base_len;

    FT_Byte*   private_dict;
    FT_Int     private_len;

    FT_Byte    in_pfb;
    FT_Byte    in_memory;
    FT_Byte    single_block;

  } T1_ParserRec;


#define T1_Add_Table( p, i, o, l )  (p)->funcs.add( (p), i, o, l )
#define T1_Done_Table( p )          \
          do                        \
          {                         \
            if ( (p)->funcs.done )  \
              (p)->funcs.done( p ); \
          } while ( 0 )
#define T1_Release_Table( p )          \
          do                           \
          {                            \
            if ( (p)->funcs.release )  \
              (p)->funcs.release( p ); \
          } while ( 0 )


#define T1_Skip_Spaces( p )  (p)->root.funcs.skip_spaces( &(p)->root )
#define T1_Skip_Alpha( p )   (p)->root.funcs.skip_alpha ( &(p)->root )

#define T1_ToInt( p )       (p)->root.funcs.to_int( &(p)->root )
#define T1_ToFixed( p, t )  (p)->root.funcs.to_fixed( &(p)->root, t )

#define T1_ToCoordArray( p, m, c )    \
          (p)->root.funcs.to_coord_array( &(p)->root, m, c )
#define T1_ToFixedArray( p, m, f, t ) \
          (p)->root.funcs.to_fixed_array( &(p)->root, m, f, t )
#define T1_ToToken( p, t )            \
          (p)->root.funcs.to_token( &(p)->root, t )
#define T1_ToTokenArray( p, t, m, c ) \
          (p)->root.funcs.to_token_array( &(p)->root, t, m, c )

#define T1_Load_Field( p, f, o, m, pf )       \
          (p)->root.funcs.load_field( &(p)->root, f, o, m, pf )
#define T1_Load_Field_Table( p, f, o, m, pf ) \
          (p)->root.funcs.load_field_table( &(p)->root, f, o, m, pf )


  FT_LOCAL FT_Error
  T1_New_Parser( T1_ParserRec*     parser,
                 FT_Stream         stream,
                 FT_Memory         memory,
                 PSAux_Interface*  psaux );

  FT_LOCAL FT_Error
  T1_Get_Private_Dict( T1_ParserRec*     parser,
                       PSAux_Interface*  psaux );

  FT_LOCAL void
  T1_Finalize_Parser( T1_ParserRec*  parser );


FT_END_HEADER

#endif /* __T1PARSE_H__ */


/* END */
