/***************************************************************************/
/*                                                                         */
/*  t1load.c                                                               */
/*                                                                         */
/*    Type 1 font loader (body).                                           */
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
  /* This is the new and improved Type 1 data loader for FreeType 2.  The  */
  /* old loader has several problems: it is slow, complex, difficult to    */
  /* maintain, and contains incredible hacks to make it accept some        */
  /* ill-formed Type 1 fonts without hiccup-ing.  Moreover, about 5% of    */
  /* the Type 1 fonts on my machine still aren't loaded correctly by it.   */
  /*                                                                       */
  /* This version is much simpler, much faster and also easier to read and */
  /* maintain by a great order of magnitude.  The idea behind it is to     */
  /* _not_ try to read the Type 1 token stream with a state machine (i.e.  */
  /* a Postscript-like interpreter) but rather to perform simple pattern   */
  /* matching.                                                             */
  /*                                                                       */
  /* Indeed, nearly all data definitions follow a simple pattern like      */
  /*                                                                       */
  /*  ... /Field <data> ...                                                */
  /*                                                                       */
  /* where <data> can be a number, a boolean, a string, or an array of     */
  /* numbers.  There are a few exceptions, namely the encoding, font name, */
  /* charstrings, and subrs; they are handled with a special pattern       */
  /* matching routine.                                                     */
  /*                                                                       */
  /* All other common cases are handled very simply.  The matching rules   */
  /* are defined in the file `t1tokens.h' through the use of several       */
  /* macros calls PARSE_XXX.                                               */
  /*                                                                       */
  /* This file is included twice here; the first time to generate parsing  */
  /* callback functions, the second to generate a table of keywords (with  */
  /* pointers to the associated callback).                                 */
  /*                                                                       */
  /* The function `parse_dict' simply scans *linearly* a given dictionary  */
  /* (either the top-level or private one) and calls the appropriate       */
  /* callback when it encounters an immediate keyword.                     */
  /*                                                                       */
  /* This is by far the fastest way one can find to parse and read all     */
  /* data.                                                                 */
  /*                                                                       */
  /* This led to tremendous code size reduction.  Note that later, the     */
  /* glyph loader will also be _greatly_ simplified, and the automatic     */
  /* hinter will replace the clumsy `t1hinter'.                            */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_CONFIG_CONFIG_H
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_TYPE1_TYPES_H

#include "t1load.h"

#include "t1errors.h"

#include <string.h>     /* for strncmp(), strcmp() */
#include <ctype.h>      /* for isalnum()           */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1load


#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    MULTIPLE MASTERS SUPPORT                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  t1_allocate_blend( T1_Face  face,
                     FT_UInt  num_designs,
                     FT_UInt  num_axis )
  {
    T1_Blend*  blend;
    FT_Memory  memory = face->root.memory;
    FT_Error   error  = 0;


    blend = face->blend;
    if ( !blend )
    {
      if ( ALLOC( blend, sizeof ( *blend ) ) )
        goto Exit;

      face->blend = blend;
    }

    /* allocate design data if needed */
    if ( num_designs > 0 )
    {
      if ( blend->num_designs == 0 )
      {
        FT_UInt  nn;


        /* allocate the blend `private' and `font_info' dictionaries */
        if ( ALLOC_ARRAY( blend->font_infos[1], num_designs, T1_FontInfo )  ||
             ALLOC_ARRAY( blend->privates[1], num_designs, T1_Private )     ||
             ALLOC_ARRAY( blend->weight_vector, num_designs * 2, FT_Fixed ) )
          goto Exit;

        blend->default_weight_vector = blend->weight_vector + num_designs;

        blend->font_infos[0] = &face->type1.font_info;
        blend->privates  [0] = &face->type1.private_dict;

        for ( nn = 2; nn <= num_designs; nn++ )
        {
          blend->privates[nn]   = blend->privates  [nn - 1] + 1;
          blend->font_infos[nn] = blend->font_infos[nn - 1] + 1;
        }

        blend->num_designs   = num_designs;
      }
      else if ( blend->num_designs != num_designs )
        goto Fail;
    }

    /* allocate axis data if needed */
    if ( num_axis > 0 )
    {
      if ( blend->num_axis != 0 && blend->num_axis != num_axis )
        goto Fail;

      blend->num_axis = num_axis;
    }

    /* allocate the blend design pos table if needed */
    num_designs = blend->num_designs;
    num_axis    = blend->num_axis;
    if ( num_designs && num_axis && blend->design_pos[0] == 0 )
    {
      FT_UInt  n;


      if ( ALLOC_ARRAY( blend->design_pos[0],
                        num_designs * num_axis, FT_Fixed ) )
        goto Exit;

      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = blend->design_pos[0] + num_axis * n;
    }

  Exit:
    return error;

  Fail:
    error = -1;
    goto Exit;
  }


  FT_LOCAL_DEF FT_Error
  T1_Get_Multi_Master( T1_Face           face,
                       FT_Multi_Master*  master )
  {
    T1_Blend*  blend = face->blend;
    FT_UInt    n;
    FT_Error   error;


    error = T1_Err_Invalid_Argument;

    if ( blend )
    {
      master->num_axis    = blend->num_axis;
      master->num_designs = blend->num_designs;

      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_MM_Axis*    axis = master->axis + n;
        T1_DesignMap*  map = blend->design_map + n;


        axis->name    = blend->axis_names[n];
        axis->minimum = map->design_points[0];
        axis->maximum = map->design_points[map->num_points - 1];
      }
      error = 0;
    }
    return error;
  }


  FT_LOCAL_DEF FT_Error
  T1_Set_MM_Blend( T1_Face    face,
                   FT_UInt    num_coords,
                   FT_Fixed*  coords )
  {
    T1_Blend*  blend = face->blend;
    FT_Error   error;
    FT_UInt    n, m;


    error = T1_Err_Invalid_Argument;

    if ( blend && blend->num_axis == num_coords )
    {
      /* recompute the weight vector from the blend coordinates */
      error = T1_Err_Ok;

      for ( n = 0; n < blend->num_designs; n++ )
      {
        FT_Fixed  result = 0x10000L;  /* 1.0 fixed */


        for ( m = 0; m < blend->num_axis; m++ )
        {
          FT_Fixed  factor;


          /* get current blend axis position */
          factor = coords[m];
          if ( factor < 0 )        factor = 0;
          if ( factor > 0x10000L ) factor = 0x10000L;

          if ( ( n & ( 1 << m ) ) == 0 )
            factor = 0x10000L - factor;

          result = FT_MulFix( result, factor );
        }
        blend->weight_vector[n] = result;
      }

      error = T1_Err_Ok;
    }
    return error;
  }


  FT_LOCAL_DEF FT_Error
  T1_Set_MM_Design( T1_Face   face,
                    FT_UInt   num_coords,
                    FT_Long*  coords )
  {
    T1_Blend*  blend = face->blend;
    FT_Error   error;
    FT_UInt    n, p;


    error = T1_Err_Invalid_Argument;
    if ( blend && blend->num_axis == num_coords )
    {
      /* compute the blend coordinates through the blend design map */
      FT_Fixed  final_blends[T1_MAX_MM_DESIGNS];


      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_Long        design  = coords[n];
        FT_Fixed       the_blend;
        T1_DesignMap*  map     = blend->design_map + n;
        FT_Fixed*      designs = map->design_points;
        FT_Fixed*      blends  = map->blend_points;
        FT_Int         before  = -1, after = -1;

        for ( p = 0; p < (FT_UInt)map->num_points; p++ )
        {
          FT_Fixed  p_design = designs[p];


          /* exact match ? */
          if ( design == p_design )
          {
            the_blend = blends[p];
            goto Found;
          }

          if ( design < p_design )
          {
            after = p;
            break;
          }

          before = p;
        }

        /* now, interpolate if needed */
        if ( before < 0 )
          the_blend = blends[0];

        else if ( after < 0 )
          the_blend = blends[map->num_points - 1];

        else
          the_blend = FT_MulDiv( design         - designs[before],
                                 blends [after] - blends [before],
                                 designs[after] - designs[before] );

      Found:
        final_blends[n] = the_blend;
      }

      error = T1_Set_MM_Blend( face, num_coords, final_blends );
    }

    return error;
  }


  FT_LOCAL_DEF void
  T1_Done_Blend( T1_Face  face )
  {
    FT_Memory  memory = face->root.memory;
    T1_Blend*  blend  = face->blend;


    if ( blend )
    {
      FT_UInt  num_designs = blend->num_designs;
      FT_UInt  num_axis    = blend->num_axis;
      FT_UInt  n;


      /* release design pos table */
      FREE( blend->design_pos[0] );
      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = 0;

      /* release blend `private' and `font info' dictionaries */
      FREE( blend->privates[1] );
      FREE( blend->font_infos[1] );

      for ( n = 0; n < num_designs; n++ )
      {
        blend->privates  [n] = 0;
        blend->font_infos[n] = 0;
      }

      /* release weight vectors */
      FREE( blend->weight_vector );
      blend->default_weight_vector = 0;

      /* release axis names */
      for ( n = 0; n < num_axis; n++ )
        FREE( blend->axis_names[n] );

      /* release design map */
      for ( n = 0; n < num_axis; n++ )
      {
        T1_DesignMap*  dmap = blend->design_map + n;


        FREE( dmap->design_points );
        dmap->num_points = 0;
      }

      FREE( face->blend );
    }
  }


  static void
  parse_blend_axis_types( T1_Face     face,
                          T1_Loader*  loader )
  {
    T1_Token   axis_tokens[ T1_MAX_MM_AXIS ];
    FT_Int     n, num_axis;
    FT_Error   error = 0;
    T1_Blend*  blend;
    FT_Memory  memory;


    /* take an array of objects */
    T1_ToTokenArray( &loader->parser, axis_tokens,
                     T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis <= 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_axis_types: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    /* allocate blend if necessary */
    error = t1_allocate_blend( face, 0, (FT_UInt)num_axis );
    if ( error )
      goto Exit;

    blend  = face->blend;
    memory = face->root.memory;

    /* each token is an immediate containing the name of the axis */
    for ( n = 0; n < num_axis; n++ )
    {
      T1_Token*  token = axis_tokens + n;
      FT_Byte*   name;
      FT_Int     len;

      /* skip first slash, if any */
      if (token->start[0] == '/')
        token->start++;

      len = (FT_Int)( token->limit - token->start );
      if ( len <= 0 )
      {
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      if ( ALLOC( blend->axis_names[n], len + 1 ) )
        goto Exit;

      name = (FT_Byte*)blend->axis_names[n];
      MEM_Copy( name, token->start, len );
      name[len] = 0;
    }

  Exit:
    loader->parser.root.error = error;
  }


  static void
  parse_blend_design_positions( T1_Face     face,
                                T1_Loader*  loader )
  {
    T1_Token       design_tokens[ T1_MAX_MM_DESIGNS ];
    FT_Int         num_designs;
    FT_Int         num_axis;
    T1_ParserRec*  parser = &loader->parser;

    FT_Error       error = 0;
    T1_Blend*      blend;


    /* get the array of design tokens - compute number of designs */
    T1_ToTokenArray( parser, design_tokens, T1_MAX_MM_DESIGNS, &num_designs );
    if ( num_designs <= 0 || num_designs > T1_MAX_MM_DESIGNS )
    {
      FT_ERROR(( "parse_blend_design_positions:" ));
      FT_ERROR(( " incorrect number of designs: %d\n",
                 num_designs ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    {
      FT_Byte*  old_cursor = parser->root.cursor;
      FT_Byte*  old_limit  = parser->root.limit;
      FT_UInt   n;


      blend    = face->blend;
      num_axis = 0;  /* make compiler happy */

      for ( n = 0; n < (FT_UInt)num_designs; n++ )
      {
        T1_Token   axis_tokens[ T1_MAX_MM_DESIGNS ];
        T1_Token*  token;
        FT_Int     axis, n_axis;


        /* read axis/coordinates tokens */
        token = design_tokens + n;
        parser->root.cursor = token->start - 1;
        parser->root.limit  = token->limit + 1;
        T1_ToTokenArray( parser, axis_tokens, T1_MAX_MM_AXIS, &n_axis );

        if ( n == 0 )
        {
          num_axis = n_axis;
          error = t1_allocate_blend( face, num_designs, num_axis );
          if ( error )
            goto Exit;
          blend = face->blend;
        }
        else if ( n_axis != num_axis )
        {
          FT_ERROR(( "parse_blend_design_positions: incorrect table\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }

        /* now, read each axis token into the design position */
        for ( axis = 0; axis < n_axis; axis++ )
        {
          T1_Token*  token2 = axis_tokens + axis;


          parser->root.cursor = token2->start;
          parser->root.limit  = token2->limit;
          blend->design_pos[n][axis] = T1_ToFixed( parser, 0 );
        }
      }

      loader->parser.root.cursor = old_cursor;
      loader->parser.root.limit  = old_limit;
    }

  Exit:
    loader->parser.root.error = error;
  }


  static void
  parse_blend_design_map( T1_Face     face,
                          T1_Loader*  loader )
  {
    FT_Error       error  = 0;
    T1_ParserRec*  parser = &loader->parser;
    T1_Blend*      blend;
    T1_Token       axis_tokens[ T1_MAX_MM_AXIS ];
    FT_Int         n, num_axis;
    FT_Byte*       old_cursor;
    FT_Byte*       old_limit;
    FT_Memory      memory = face->root.memory;


    T1_ToTokenArray( parser, axis_tokens, T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis <= 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_design_map: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }
    old_cursor = parser->root.cursor;
    old_limit  = parser->root.limit;

    error = t1_allocate_blend( face, 0, num_axis );
    if ( error )
      goto Exit;
    blend = face->blend;

    /* now, read each axis design map */
    for ( n = 0; n < num_axis; n++ )
    {
      T1_DesignMap* map = blend->design_map + n;
      T1_Token*     token;
      FT_Int        p, num_points;


      token = axis_tokens + n;
      parser->root.cursor = token->start;
      parser->root.limit  = token->limit;

      /* count the number of map points */
      {
        FT_Byte*  ptr   = token->start;
        FT_Byte*  limit = token->limit;


        num_points = 0;
        for ( ; ptr < limit; ptr++ )
          if ( ptr[0] == '[' )
            num_points++;
      }
      if ( num_points <= 0 || num_points > T1_MAX_MM_MAP_POINTS )
      {
        FT_ERROR(( "parse_blend_design_map: incorrect table\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      /* allocate design map data */
      if ( ALLOC_ARRAY( map->design_points, num_points * 2, FT_Fixed ) )
        goto Exit;
      map->blend_points = map->design_points + num_points;
      map->num_points   = (FT_Byte)num_points;

      for ( p = 0; p < num_points; p++ )
      {
        map->design_points[p] = T1_ToInt( parser );
        map->blend_points [p] = T1_ToFixed( parser, 0 );
      }
    }

    parser->root.cursor = old_cursor;
    parser->root.limit  = old_limit;

  Exit:
    parser->root.error = error;
  }


  static void
  parse_weight_vector( T1_Face     face,
                       T1_Loader*  loader )
  {
    FT_Error       error  = 0;
    T1_ParserRec*  parser = &loader->parser;
    T1_Blend*      blend  = face->blend;
    T1_Token       master;
    FT_UInt        n;
    FT_Byte*       old_cursor;
    FT_Byte*       old_limit;


    if ( !blend || blend->num_designs == 0 )
    {
      FT_ERROR(( "parse_weight_vector: too early!\n" ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    T1_ToToken( parser, &master );
    if ( master.type != t1_token_array )
    {
      FT_ERROR(( "parse_weight_vector: incorrect format!\n" ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    old_cursor = parser->root.cursor;
    old_limit  = parser->root.limit;

    parser->root.cursor = master.start;
    parser->root.limit  = master.limit;

    for ( n = 0; n < blend->num_designs; n++ )
    {
      blend->default_weight_vector[n] =
      blend->weight_vector[n]         = T1_ToFixed( parser, 0 );
    }

    parser->root.cursor = old_cursor;
    parser->root.limit  = old_limit;

  Exit:
    parser->root.error = error;
  }


  /* the keyword `/shareddict' appears in some multiple master fonts   */
  /* with a lot of Postscript garbage behind it (that's completely out */
  /* of spec!); we detect it and terminate the parsing                 */
  /*                                                                   */
  static void
  parse_shared_dict( T1_Face     face,
                     T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;

    FT_UNUSED( face );


    parser->root.cursor = parser->root.limit;
    parser->root.error  = 0;
  }

#endif /* T1_CONFIG_OPTION_NO_MM_SUPPORT */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      TYPE 1 SYMBOL PARSING                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* First of all, define the token field static variables.  This is a set */
  /* of T1_Field variables used later.                                     */
  /*                                                                       */
  /*************************************************************************/


  static FT_Error
  t1_load_keyword( T1_Face     face,
                   T1_Loader*  loader,
                   T1_Field*   field )
  {
    FT_Error   error;
    void*      dummy_object;
    void**     objects;
    FT_UInt    max_objects;
    T1_Blend*  blend = face->blend;


    /* if the keyword has a dedicated callback, call it */
    if ( field->type == t1_field_callback )
    {
      field->reader( (FT_Face)face, loader );
      error = loader->parser.root.error;
      goto Exit;
    }

    /* now, the keyword is either a simple field, or a table of fields; */
    /* we are now going to take care of it                              */
    switch ( field->location )
    {
    case t1_field_font_info:
      dummy_object = &face->type1.font_info;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->font_infos;
        max_objects = blend->num_designs;
      }
      break;

    case t1_field_private:
      dummy_object = &face->type1.private_dict;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->privates;
        max_objects = blend->num_designs;
      }
      break;

    default:
      dummy_object = &face->type1;
      objects      = &dummy_object;
      max_objects  = 0;
    }

    if ( field->type == t1_field_integer_array ||
         field->type == t1_field_fixed_array   )
      error = T1_Load_Field_Table( &loader->parser, field,
                                   objects, max_objects, 0 );
    else
      error = T1_Load_Field( &loader->parser, field,
                             objects, max_objects, 0 );

  Exit:
    return error;
  }


  static int
  is_space( FT_Byte  c )
  {
    return ( c == ' ' || c == '\t' || c == '\r' || c == '\n' );
  }


  static int
  is_alpha( FT_Byte  c )
  {
    /* Note: we must accept "+" as a valid character, as it is used in */
    /*       embedded type1 fonts in PDF documents.                    */
    /*                                                                 */
    return ( isalnum( c ) || c == '.' || c == '_' || c == '-' || c == '+' );
  }


  static int
  read_binary_data( T1_ParserRec*  parser,
                    FT_Int*        size,
                    FT_Byte**      base )
  {
    FT_Byte*  cur;
    FT_Byte*  limit = parser->root.limit;


    /* the binary data has the following format */
    /*                                          */
    /* `size' [white*] RD white ....... ND      */
    /*                                          */

    T1_Skip_Spaces( parser );
    cur = parser->root.cursor;

    if ( cur < limit && (FT_Byte)( *cur - '0' ) < 10 )
    {
      *size = T1_ToInt( parser );

      T1_Skip_Spaces( parser );
      T1_Skip_Alpha ( parser );  /* `RD' or `-|' or something else */

      /* there is only one whitespace char after the */
      /* `RD' or `-|' token                          */
      *base = parser->root.cursor + 1;

      parser->root.cursor += *size + 1;
      return 1;
    }

    FT_ERROR(( "read_binary_data: invalid size field\n" ));
    parser->root.error = T1_Err_Invalid_File_Format;
    return 0;
  }


  /* we will now define the routines used to handle */
  /* the `/Encoding', `/Subrs', and `/CharStrings'  */
  /* dictionaries                                   */

  static void
  parse_font_name( T1_Face     face,
                   T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;
    FT_Error       error;
    FT_Memory      memory = parser->root.memory;
    FT_Int         len;
    FT_Byte*       cur;
    FT_Byte*       cur2;
    FT_Byte*       limit;


    if ( face->type1.font_name )
      /*  with synthetic fonts, it's possible we get here twice  */
      return;

    T1_Skip_Spaces( parser );

    cur   = parser->root.cursor;
    limit = parser->root.limit;

    if ( cur >= limit - 1 || *cur != '/' )
      return;

    cur++;
    cur2 = cur;
    while ( cur2 < limit && is_alpha( *cur2 ) )
      cur2++;

    len = (FT_Int)( cur2 - cur );
    if ( len > 0 )
    {
      if ( ALLOC( face->type1.font_name, len + 1 ) )
      {
        parser->root.error = error;
        return;
      }

      MEM_Copy( face->type1.font_name, cur, len );
      face->type1.font_name[len] = '\0';
    }
    parser->root.cursor = cur2;
  }


  static void
  parse_font_bbox( T1_Face     face,
                   T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;
    FT_Fixed       temp[4];
    FT_BBox*       bbox   = &face->type1.font_bbox;


    (void)T1_ToFixedArray( parser, 4, temp, 0 );
    bbox->xMin = FT_RoundFix( temp[0] );
    bbox->yMin = FT_RoundFix( temp[1] );
    bbox->xMax = FT_RoundFix( temp[2] );
    bbox->yMax = FT_RoundFix( temp[3] );
  }


  static void
  parse_font_matrix( T1_Face     face,
                     T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;
    FT_Matrix*     matrix = &face->type1.font_matrix;
    FT_Vector*     offset = &face->type1.font_offset;
    FT_Face        root   = (FT_Face)&face->root;
    FT_Fixed       temp[6];
    FT_Fixed       temp_scale;


    if ( matrix->xx || matrix->yx )
      /*  with synthetic fonts, it's possible we get here twice  */
      return;

    (void)T1_ToFixedArray( parser, 6, temp, 3 );

    temp_scale = ABS( temp[3] );

    /* Set Units per EM based on FontMatrix values.  We set the value to */
    /* 1000 / temp_scale, because temp_scale was already multiplied by   */
    /* 1000 (in t1_tofixed, from psobjs.c).                              */

    root->units_per_EM = (FT_UShort)( FT_DivFix( 1000 * 0x10000L,
                                                 temp_scale ) >> 16 );

    /* we need to scale the values by 1.0/temp_scale */
    if ( temp_scale != 0x10000L )
    {
      temp[0] = FT_DivFix( temp[0], temp_scale );
      temp[1] = FT_DivFix( temp[1], temp_scale );
      temp[2] = FT_DivFix( temp[2], temp_scale );
      temp[4] = FT_DivFix( temp[4], temp_scale );
      temp[5] = FT_DivFix( temp[5], temp_scale );
      temp[3] = 0x10000L;
    }

    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];

    /* note that the offsets must be expressed in integer font units */
    offset->x  = temp[4] >> 16;
    offset->y  = temp[5] >> 16;
  }


  static void
  parse_encoding( T1_Face     face,
                  T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;
    FT_Byte*       cur    = parser->root.cursor;
    FT_Byte*       limit  = parser->root.limit;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;


    /* skip whitespace */
    while ( is_space( *cur ) )
    {
      cur++;
      if ( cur >= limit )
      {
        FT_ERROR(( "parse_encoding: out of bounds!\n" ));
        parser->root.error = T1_Err_Invalid_File_Format;
        return;
      }
    }

    /* if we have a number, then the encoding is an array, */
    /* and we must load it now                             */
    if ( (FT_Byte)( *cur - '0' ) < 10 )
    {
      T1_Encoding*  encode     = &face->type1.encoding;
      FT_Int        count, n;
      PS_Table*     char_table = &loader->encoding_table;
      FT_Memory     memory     = parser->root.memory;
      FT_Error      error;


      if ( encode->char_index )
        /*  with synthetic fonts, it's possible we get here twice  */
        return;

      /* read the number of entries in the encoding, should be 256 */
      count = T1_ToInt( parser );
      if ( parser->root.error )
        return;

      /* we use a T1_Table to store our charnames */
      encode->num_chars = count;
      if ( ALLOC_ARRAY( encode->char_index, count, FT_Short   ) ||
           ALLOC_ARRAY( encode->char_name,  count, FT_String* ) ||
           ( error = psaux->ps_table_funcs->init(
                       char_table, count, memory ) ) != 0       )
      {
        parser->root.error = error;
        return;
      }

      /* We need to `zero' out encoding_table.elements          */
      for ( n = 0; n < count; n++ )
      {
        char*  notdef = (char *)".notdef";


        T1_Add_Table( char_table, n, notdef, 8 );
      }

      /* Now, we will need to read a record of the form         */
      /* ... charcode /charname ... for each entry in our table */
      /*                                                        */
      /* We simply look for a number followed by an immediate   */
      /* name.  Note that this ignores correctly the sequence   */
      /* that is often seen in type1 fonts:                     */
      /*                                                        */
      /*   0 1 255 { 1 index exch /.notdef put } for dup        */
      /*                                                        */
      /* used to clean the encoding array before anything else. */
      /*                                                        */
      /* We stop when we encounter a `def'.                     */

      cur   = parser->root.cursor;
      limit = parser->root.limit;
      n     = 0;

      for ( ; cur < limit; )
      {
        FT_Byte  c;


        c = *cur;

        /* we stop when we encounter a `def' */
        if ( c == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e' &&
               cur[2] == 'f' &&
               is_space(cur[-1]) &&
               is_space(cur[3]) )
          {
            FT_TRACE6(( "encoding end\n" ));
            break;
          }
        }

        /* otherwise, we must find a number before anything else */
        if ( (FT_Byte)( c - '0' ) < 10 )
        {
          FT_Int  charcode;


          parser->root.cursor = cur;
          charcode = T1_ToInt( parser );
          cur = parser->root.cursor;

          /* skip whitespace */
          while ( cur < limit && is_space( *cur ) )
            cur++;

          if ( cur < limit && *cur == '/' )
          {
            /* bingo, we have an immediate name -- it must be a */
            /* character name                                   */
            FT_Byte*  cur2 = cur + 1;
            FT_Int    len;


            while ( cur2 < limit && is_alpha( *cur2 ) )
              cur2++;

            len = (FT_Int)( cur2 - cur - 1 );

            parser->root.error = T1_Add_Table( char_table, charcode,
                                               cur + 1, len + 1 );
            char_table->elements[charcode][len] = '\0';
            if ( parser->root.error )
              return;

            cur = cur2;
          }
        }
        else
          cur++;
      }

      face->type1.encoding_type = t1_encoding_array;
      parser->root.cursor       = cur;
    }
    /* Otherwise, we should have either `StandardEncoding' or */
    /* `ExpertEncoding'                                       */
    else
    {
      if ( cur + 17 < limit &&
           strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = t1_encoding_standard;

      else if ( cur + 15 < limit &&
                strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = t1_encoding_expert;

      else
      {
        FT_ERROR(( "parse_encoding: invalid token!\n" ));
        parser->root.error = T1_Err_Invalid_File_Format;
      }
    }
  }


  static void
  parse_subrs( T1_Face     face,
               T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;
    PS_Table*      table  = &loader->subrs;
    FT_Memory      memory = parser->root.memory;
    FT_Error       error;
    FT_Int         n;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;


    if ( loader->num_subrs )
      /*  with synthetic fonts, it's possible we get here twice  */
      return;

    loader->num_subrs = T1_ToInt( parser );
    if ( parser->root.error )
      return;

    /* position the parser right before the `dup' of the first subr */
    T1_Skip_Spaces( parser );
    T1_Skip_Alpha( parser );      /* `array' */
    T1_Skip_Spaces( parser );

    /* initialize subrs array */
    error = psaux->ps_table_funcs->init( table, loader->num_subrs, memory );
    if ( error )
      goto Fail;

    /* the format is simple:                                 */
    /*                                                       */
    /*   `index' + binary data                               */
    /*                                                       */

    for ( n = 0; n < loader->num_subrs; n++ )
    {
      FT_Int    index, size;
      FT_Byte*  base;


      /* If the next token isn't `dup', we are also done.  This */
      /* happens when there are `holes' in the Subrs array.     */
      if ( strncmp( (char*)parser->root.cursor, "dup", 3 ) != 0 )
        break;

      index = T1_ToInt( parser );

      if ( !read_binary_data( parser, &size, &base ) )
        return;

      /* The binary string is followed by one token, e.g. `NP' */
      /* (bound to `noaccess put') or by two separate tokens:  */
      /* `noaccess' & `put'.  We position the parser right     */
      /* before the next `dup', if any.                        */
      T1_Skip_Spaces( parser );
      T1_Skip_Alpha( parser );    /* `NP' or `I' or `noaccess' */
      T1_Skip_Spaces( parser );

      if ( strncmp( (char*)parser->root.cursor, "put", 3 ) == 0 )
      {
        T1_Skip_Alpha( parser );  /* skip `put' */
        T1_Skip_Spaces( parser );
      }

      /* some fonts use a value of -1 for lenIV to indicate that */
      /* the charstrings are unencoded                           */
      /*                                                         */
      /* thanks to Tom Kacvinsky for pointing this out           */
      /*                                                         */
      if ( face->type1.private_dict.lenIV >= 0 )
      {
        psaux->t1_decrypt( base, size, 4330 );
        size -= face->type1.private_dict.lenIV;
        base += face->type1.private_dict.lenIV;
      }

      error = T1_Add_Table( table, index, base, size );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    parser->root.error = error;
  }


  static void
  parse_charstrings( T1_Face     face,
                     T1_Loader*  loader )
  {
    T1_ParserRec*  parser     = &loader->parser;
    PS_Table*      code_table = &loader->charstrings;
    PS_Table*      name_table = &loader->glyph_names;
    PS_Table*      swap_table = &loader->swap_table;
    FT_Memory      memory     = parser->root.memory;
    FT_Error       error;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;

    FT_Byte*    cur;
    FT_Byte*    limit = parser->root.limit;
    FT_Int      n;
    FT_UInt     notdef_index = 0;
    FT_Byte     notdef_found = 0;


    if ( loader->num_glyphs )
      /*  with synthetic fonts, it's possible we get here twice  */
      return;

    loader->num_glyphs = T1_ToInt( parser );
    if ( parser->root.error )
      return;

    /* initialize tables (leaving room for addition of .notdef, */
    /* if necessary).                                           */

    error = psaux->ps_table_funcs->init( code_table,
                                         loader->num_glyphs + 1,
                                         memory );
    if ( error )
      goto Fail;

    error = psaux->ps_table_funcs->init( name_table,
                                         loader->num_glyphs + 1,
                                         memory );
    if ( error )
      goto Fail;

    /* Initialize table for swapping index notdef_index and */
    /* index 0 names and codes (if necessary).              */

    error = psaux->ps_table_funcs->init( swap_table, 4, memory );

    if ( error )
      goto Fail;


    n = 0;
    for (;;)
    {
      FT_Int    size;
      FT_Byte*  base;


      /* the format is simple:                    */
      /*   `/glyphname' + binary data             */
      /*                                          */
      /* note that we stop when we find a `def'   */
      /*                                          */
      T1_Skip_Spaces( parser );

      cur = parser->root.cursor;
      if ( cur >= limit )
        break;

      /* we stop when we find a `def' or `end' keyword */
      if ( *cur   == 'd'   &&
           cur + 3 < limit &&
           cur[1] == 'e'   &&
           cur[2] == 'f'   )
        break;

      if ( *cur   == 'e'   &&
           cur + 3 < limit &&
           cur[1] == 'n'   &&
           cur[2] == 'd'   )
        break;

      if ( *cur != '/' )
        T1_Skip_Alpha( parser );
      else
      {
        FT_Byte*  cur2 = cur + 1;
        FT_Int    len;


        while ( cur2 < limit && is_alpha( *cur2 ) )
          cur2++;
        len = (FT_Int)( cur2 - cur - 1 );

        error = T1_Add_Table( name_table, n, cur + 1, len + 1 );
        if ( error )
          goto Fail;

        /* add a trailing zero to the name table */
        name_table->elements[n][len] = '\0';

        /* record index of /.notdef              */
        if ( strcmp( (const char*)".notdef",
                     (const char*)(name_table->elements[n]) ) == 0 )
        {
          notdef_index = n;
          notdef_found = 1;
        }

        parser->root.cursor = cur2;
        if ( !read_binary_data( parser, &size, &base ) )
          return;

        if ( face->type1.private_dict.lenIV >= 0 )
        {
          psaux->t1_decrypt( base, size, 4330 );
          size -= face->type1.private_dict.lenIV;
          base += face->type1.private_dict.lenIV;
        }

        error = T1_Add_Table( code_table, n, base, size );
        if ( error )
          goto Fail;

        n++;
        if ( n >= loader->num_glyphs )
          break;
      }
    }

    loader->num_glyphs = n;

    /* if /.notdef is found but does not occupy index 0, do our magic.      */
    if ( strcmp( (const char*)".notdef",
                 (const char*)name_table->elements[0] ) &&
         notdef_found                                      )
    {
      /* Swap glyph in index 0 with /.notdef glyph.  First, add index 0    */
      /* name and code entries to swap_table. Then place notdef_index name */
      /* and code entries into swap_table.  Then swap name and code        */
      /* entries at indices notdef_index and 0 using values stored in      */
      /* swap_table.                                                       */

      /* Index 0 name */
      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index 0 code */
      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      /* Index notdef_index name */
      error = T1_Add_Table( swap_table, 2,
                            name_table->elements[notdef_index],
                            name_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      /* Index notdef_index code */
      error = T1_Add_Table( swap_table, 3,
                            code_table->elements[notdef_index],
                            code_table->lengths [notdef_index] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, notdef_index,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, notdef_index,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0,
                            swap_table->elements[2],
                            swap_table->lengths [2] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0,
                            swap_table->elements[3],
                            swap_table->lengths [3] );
      if ( error )
        goto Fail;

    }
    else if ( !notdef_found )
    {

      /* notdef_index is already 0, or /.notdef is undefined in  */
      /* charstrings dictionary. Worry about /.notdef undefined. */
      /* We take index 0 and add it to the end of the table(s)   */
      /* and add our own /.notdef glyph to index 0.              */

      /* 0 333 hsbw endchar                                      */
      FT_Byte  notdef_glyph[] = {0x8B, 0xF7, 0xE1, 0x0D, 0x0E};
      char*    notdef_name    = (char *)".notdef";


      error = T1_Add_Table( swap_table, 0,
                            name_table->elements[0],
                            name_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( swap_table, 1,
                            code_table->elements[0],
                            code_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, 0, notdef_name, 8 );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, 0, notdef_glyph, 5 );

      if ( error )
        goto Fail;

      error = T1_Add_Table( name_table, n,
                            swap_table->elements[0],
                            swap_table->lengths [0] );
      if ( error )
        goto Fail;

      error = T1_Add_Table( code_table, n,
                            swap_table->elements[1],
                            swap_table->lengths [1] );
      if ( error )
        goto Fail;

      /* we added a glyph. */
      loader->num_glyphs = n + 1;

    }


    return;

  Fail:
    parser->root.error = error;
  }


  static
  const T1_Field  t1_keywords[] =
  {

#include "t1tokens.h"

    /* now add the special functions... */
    T1_FIELD_CALLBACK( "FontName", parse_font_name )
    T1_FIELD_CALLBACK( "FontBBox", parse_font_bbox )
    T1_FIELD_CALLBACK( "FontMatrix", parse_font_matrix )
    T1_FIELD_CALLBACK( "Encoding", parse_encoding )
    T1_FIELD_CALLBACK( "Subrs", parse_subrs )
    T1_FIELD_CALLBACK( "CharStrings", parse_charstrings )

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    T1_FIELD_CALLBACK( "BlendDesignPositions", parse_blend_design_positions )
    T1_FIELD_CALLBACK( "BlendDesignMap", parse_blend_design_map )
    T1_FIELD_CALLBACK( "BlendAxisTypes", parse_blend_axis_types )
    T1_FIELD_CALLBACK( "WeightVector", parse_weight_vector )
    T1_FIELD_CALLBACK( "shareddict", parse_shared_dict )
#endif

    { 0, t1_field_cid_info, t1_field_none, 0, 0, 0, 0, 0 }
  };


  static FT_Error
  parse_dict( T1_Face     face,
              T1_Loader*  loader,
              FT_Byte*    base,
              FT_Long     size )
  {
    T1_ParserRec*  parser = &loader->parser;


    parser->root.cursor = base;
    parser->root.limit  = base + size;
    parser->root.error  = 0;

    {
      FT_Byte*  cur   = base;
      FT_Byte*  limit = cur + size;


      for ( ; cur < limit; cur++ )
      {
        /* look for `FontDirectory', which causes problems on some fonts */
        if ( *cur == 'F' && cur + 25 < limit                 &&
             strncmp( (char*)cur, "FontDirectory", 13 ) == 0 )
        {
          FT_Byte*  cur2;


          /* skip the `FontDirectory' keyword */
          cur += 13;
          cur2 = cur;

          /* lookup the `known' keyword */
          while ( cur < limit && *cur != 'k'        &&
                  strncmp( (char*)cur, "known", 5 ) )
            cur++;

          if ( cur < limit )
          {
            T1_Token  token;


            /* skip the `known' keyword and the token following it */
            cur += 5;
            loader->parser.root.cursor = cur;
            T1_ToToken( &loader->parser, &token );

            /* if the last token was an array, skip it! */
            if ( token.type == t1_token_array )
              cur2 = parser->root.cursor;
          }
          cur = cur2;
        }
        /* look for immediates */
        else if ( *cur == '/' && cur + 2 < limit )
        {
          FT_Byte*  cur2;
          FT_Int    len;


          cur++;
          cur2 = cur;
          while ( cur2 < limit && is_alpha( *cur2 ) )
            cur2++;

          len  = (FT_Int)( cur2 - cur );
          if ( len > 0 && len < 22 )
          {
            {
              /* now, compare the immediate name to the keyword table */
              T1_Field*  keyword = (T1_Field*)t1_keywords;


              for (;;)
              {
                FT_Byte*  name;


                name = (FT_Byte*)keyword->ident;
                if ( !name )
                  break;

                if ( cur[0] == name[0]                          &&
                     len == (FT_Int)strlen( (const char*)name ) )
                {
                  FT_Int  n;


                  for ( n = 1; n < len; n++ )
                    if ( cur[n] != name[n] )
                      break;

                  if ( n >= len )
                  {
                    /* we found it -- run the parsing callback! */
                    parser->root.cursor = cur2;
                    T1_Skip_Spaces( parser );
                    parser->root.error = t1_load_keyword( face,
                                                          loader,
                                                          keyword );
                    if ( parser->root.error )
                      return parser->root.error;

                    cur = parser->root.cursor;
                    break;
                  }
                }
                keyword++;
              }
            }
          }
        }
      }
    }
    return parser->root.error;
  }


  static void
  t1_init_loader( T1_Loader*  loader,
                  T1_Face     face )
  {
    FT_UNUSED( face );

    MEM_Set( loader, 0, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    /* initialize the tables -- simply set their `init' field to 0 */
    loader->encoding_table.init = 0;
    loader->charstrings.init    = 0;
    loader->glyph_names.init    = 0;
    loader->subrs.init          = 0;
    loader->swap_table.init     = 0;
    loader->fontdata            = 0;
  }


  static void
  t1_done_loader( T1_Loader*  loader )
  {
    T1_ParserRec*  parser = &loader->parser;


    /* finalize tables */
    T1_Release_Table( &loader->encoding_table );
    T1_Release_Table( &loader->charstrings );
    T1_Release_Table( &loader->glyph_names );
    T1_Release_Table( &loader->swap_table );
    T1_Release_Table( &loader->subrs );

    /* finalize parser */
    T1_Finalize_Parser( parser );
  }


  FT_LOCAL_DEF FT_Error
  T1_Open_Face( T1_Face  face )
  {
    T1_Loader      loader;
    T1_ParserRec*  parser;
    T1_Font*       type1 = &face->type1;
    FT_Error       error;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;


    t1_init_loader( &loader, face );

    /* default lenIV */
    type1->private_dict.lenIV = 4;

    parser = &loader.parser;
    error = T1_New_Parser( parser,
                           face->root.stream,
                           face->root.memory,
                           psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader, parser->base_dict, parser->base_len );
    if ( error )
      goto Exit;

    error = T1_Get_Private_Dict( parser, psaux );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader, parser->private_dict,
                        parser->private_len );
    if ( error )
      goto Exit;

    /* now, propagate the subrs, charstrings, and glyphnames tables */
    /* to the Type1 data                                            */
    type1->num_glyphs = loader.num_glyphs;

    if ( loader.subrs.init )
    {
      loader.subrs.init  = 0;
      type1->num_subrs   = loader.num_subrs;
      type1->subrs_block = loader.subrs.block;
      type1->subrs       = loader.subrs.elements;
      type1->subrs_len   = loader.subrs.lengths;
    }

    if ( !loader.charstrings.init )
    {
      FT_ERROR(( "T1_Open_Face: no charstrings array in face!\n" ));
      error = T1_Err_Invalid_File_Format;
    }

    loader.charstrings.init  = 0;
    type1->charstrings_block = loader.charstrings.block;
    type1->charstrings       = loader.charstrings.elements;
    type1->charstrings_len   = loader.charstrings.lengths;

    /* we copy the glyph names `block' and `elements' fields; */
    /* the `lengths' field must be released later             */
    type1->glyph_names_block    = loader.glyph_names.block;
    type1->glyph_names          = (FT_String**)loader.glyph_names.elements;
    loader.glyph_names.block    = 0;
    loader.glyph_names.elements = 0;

    /* we must now build type1.encoding when we have a custom */
    /* array..                                                */
    if ( type1->encoding_type == t1_encoding_array )
    {
      FT_Int    charcode, index, min_char, max_char;
      FT_Byte*  char_name;
      FT_Byte*  glyph_name;


      /* OK, we do the following: for each element in the encoding  */
      /* table, look up the index of the glyph having the same name */
      /* the index is then stored in type1.encoding.char_index, and */
      /* a the name to type1.encoding.char_name                     */

      min_char = +32000;
      max_char = -32000;

      charcode = 0;
      for ( ; charcode < loader.encoding_table.max_elems; charcode++ )
      {
        type1->encoding.char_index[charcode] = 0;
        type1->encoding.char_name [charcode] = (char *)".notdef";

        char_name = loader.encoding_table.elements[charcode];
        if ( char_name )
          for ( index = 0; index < type1->num_glyphs; index++ )
          {
            glyph_name = (FT_Byte*)type1->glyph_names[index];
            if ( strcmp( (const char*)char_name,
                         (const char*)glyph_name ) == 0 )
            {
              type1->encoding.char_index[charcode] = (FT_UShort)index;
              type1->encoding.char_name [charcode] = (char*)glyph_name;

              /* Change min/max encoded char only if glyph name is */
              /* not /.notdef                                      */
              if ( strcmp( (const char*)".notdef",
                           (const char*)glyph_name ) != 0 )
              {
                if (charcode < min_char) min_char = charcode;
                if (charcode > max_char) max_char = charcode;
              }
              break;
            }
          }
      }
      type1->encoding.code_first = min_char;
      type1->encoding.code_last  = max_char;
      type1->encoding.num_chars  = loader.num_chars;
   }

  Exit:
    t1_done_loader( &loader );
    return error;
  }


/* END */
