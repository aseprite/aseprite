/***************************************************************************/
/*                                                                         */
/*  cidparse.h                                                             */
/*                                                                         */
/*    CID-keyed Type1 parser (specification).                              */
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


#ifndef __CIDPARSE_H__
#define __CIDPARSE_H__


#include <ft2build.h>
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    CID_Parser                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A CID_Parser is an object used to parse a Type 1 fonts very        */
  /*    quickly.                                                           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root           :: the root T1_Parser fields                        */
  /*                                                                       */
  /*    stream         :: The current input stream.                        */
  /*                                                                       */
  /*    postscript     :: A pointer to the data to be parsed.              */
  /*                                                                       */
  /*    postscript_len :: The length of the data to be parsed.             */
  /*                                                                       */
  /*    data_offset    :: The start position of the binary data (i.e., the */
  /*                      end of the data to be parsed.                    */
  /*                                                                       */
  /*    cid            :: A structure which holds the information about    */
  /*                      the current font.                                */
  /*                                                                       */
  /*    num_dict       :: The number of font dictionaries.                 */
  /*                                                                       */
  typedef struct  CID_Parser_
  {
    T1_Parser  root;
    FT_Stream  stream;

    FT_Byte*   postscript;
    FT_Int     postscript_len;

    FT_ULong   data_offset;

    CID_Info*  cid;
    FT_Int     num_dict;

  } CID_Parser;


  FT_LOCAL FT_Error
  CID_New_Parser( CID_Parser*       parser,
                  FT_Stream         stream,
                  FT_Memory         memory,
                  PSAux_Interface*  psaux );

  FT_LOCAL void
  CID_Done_Parser( CID_Parser*  parser );


  /*************************************************************************/
  /*                                                                       */
  /*                            PARSING ROUTINES                           */
  /*                                                                       */
  /*************************************************************************/

#define CID_Skip_Spaces( p )  (p)->root.funcs.skip_spaces( &(p)->root )
#define CID_Skip_Alpha( p )   (p)->root.funcs.skip_alpha ( &(p)->root )

#define CID_ToInt( p )       (p)->root.funcs.to_int( &(p)->root )
#define CID_ToFixed( p, t )  (p)->root.funcs.to_fixed( &(p)->root, t )

#define CID_ToCoordArray( p, m, c )    \
          (p)->root.funcs.to_coord_array( &(p)->root, m, c )
#define CID_ToFixedArray( p, m, f, t ) \
          (p)->root.funcs.to_fixed_array( &(p)->root, m, f, t )
#define CID_ToToken( p, t )            \
          (p)->root.funcs.to_token( &(p)->root, t )
#define CID_ToTokenArray( p, t, m, c ) \
          (p)->root.funcs.to_token_array( &(p)->root, t, m, c )

#define CID_Load_Field( p, f, o )       \
          (p)->root.funcs.load_field( &(p)->root, f, o, 0, 0 )
#define CID_Load_Field_Table( p, f, o ) \
          (p)->root.funcs.load_field_table( &(p)->root, f, o, 0, 0 )


FT_END_HEADER

#endif /* __CIDPARSE_H__ */


/* END */
