/***************************************************************************/
/*                                                                         */
/*  ttpload.c                                                              */
/*                                                                         */
/*    TrueType glyph data/program tables loader (body).                    */
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


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H

#include "ttpload.h"

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttpload


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Locations                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the locations table.                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_Locations( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_Short   LongOffsets;
    FT_ULong   table_len;


    FT_TRACE2(( "Locations " ));
    LongOffsets = face->header.Index_To_Loc_Format;

    error = face->goto_table( face, TTAG_loca, stream, &table_len );
    if ( error )
    {
      error = TT_Err_Locations_Missing;
      goto Exit;
    }

    if ( LongOffsets != 0 )
    {
      face->num_locations = (FT_UShort)( table_len >> 2 );

      FT_TRACE2(( "(32bit offsets): %12d ", face->num_locations ));

      if ( ALLOC_ARRAY( face->glyph_locations,
                        face->num_locations,
                        FT_Long ) )
        goto Exit;

      if ( ACCESS_Frame( face->num_locations * 4L ) )
        goto Exit;

      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = GET_Long();
      }

      FORGET_Frame();
    }
    else
    {
      face->num_locations = (FT_UShort)( table_len >> 1 );

      FT_TRACE2(( "(16bit offsets): %12d ", face->num_locations ));

      if ( ALLOC_ARRAY( face->glyph_locations,
                        face->num_locations,
                        FT_Long ) )
        goto Exit;

      if ( ACCESS_Frame( face->num_locations * 2L ) )
        goto Exit;
      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = (FT_Long)( (FT_ULong)GET_UShort() * 2 );
      }
      FORGET_Frame();
    }

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_CVT                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the control value table into a face object.                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_CVT( TT_Face    face,
               FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_ULong   table_len;


    FT_TRACE2(( "CVT " ));

    error = face->goto_table( face, TTAG_cvt, stream, &table_len );
    if ( error )
    {
      FT_TRACE2(( "is missing!\n" ));

      face->cvt_size = 0;
      face->cvt      = NULL;
      error          = TT_Err_Ok;

      goto Exit;
    }

    face->cvt_size = table_len / 2;

    if ( ALLOC_ARRAY( face->cvt,
                      face->cvt_size,
                      FT_Short ) )
      goto Exit;

    if ( ACCESS_Frame( face->cvt_size * 2L ) )
      goto Exit;

    {
      FT_Short*  cur   = face->cvt;
      FT_Short*  limit = cur + face->cvt_size;


      for ( ; cur <  limit; cur++ )
        *cur = GET_Short();
    }

    FORGET_Frame();
    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Progams                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the font program and the cvt program.                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_Programs( TT_Face    face,
                    FT_Stream  stream )
  {
    FT_Error   error;
    FT_ULong   table_len;


    FT_TRACE2(( "Font program " ));

    /* The font program is optional */
    error = face->goto_table( face, TTAG_fpgm, stream, &table_len );
    if ( error )
    {
      face->font_program      = NULL;
      face->font_program_size = 0;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->font_program_size = table_len;
      if ( EXTRACT_Frame( table_len, face->font_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->font_program_size ));
    }

    FT_TRACE2(( "Prep program " ));

    error = face->goto_table( face, TTAG_prep, stream, &table_len );
    if ( error )
    {
      face->cvt_program      = NULL;
      face->cvt_program_size = 0;
      error                  = TT_Err_Ok;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->cvt_program_size = table_len;
      if ( EXTRACT_Frame( table_len, face->cvt_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->cvt_program_size ));
    }

  Exit:
    return error;
  }


/* END */
