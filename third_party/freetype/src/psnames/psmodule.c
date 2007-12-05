/***************************************************************************/
/*                                                                         */
/*  psmodule.c                                                             */
/*                                                                         */
/*    PSNames module implementation (body).                                */
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
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_OBJECTS_H

#include "psmodule.h"
#include "pstables.h"

#include "psnamerr.h"

#include <stdlib.h>     /* for qsort()             */
#include <string.h>     /* for strcmp(), strncpy() */


#ifndef FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES


#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST


  /* return the Unicode value corresponding to a given glyph.  Note that */
  /* we do deal with glyph variants by detecting a non-initial dot in    */
  /* the name, as in `A.swash' or `e.final', etc.                        */
  /*                                                                     */
  static FT_ULong
  PS_Unicode_Value( const char*  glyph_name )
  {
    FT_Int  n;
    char    first = glyph_name[0];
    char    temp[64];


    /* if the name begins with `uni', then the glyph name may be a */
    /* hard-coded unicode character code.                          */
    if ( glyph_name[0] == 'u' &&
         glyph_name[1] == 'n' &&
         glyph_name[2] == 'i' )
    {
      /* determine whether the next four characters following are */
      /* hexadecimal.                                             */

      /* XXX: Add code to deal with ligatures, i.e. glyph names like */
      /*      `uniXXXXYYYYZZZZ'...                                   */

      FT_Int       count;
      FT_ULong     value = 0;
      const char*  p     = glyph_name + 3;


      for ( count = 4; count > 0; count--, p++ )
      {
        char          c = *p;
        unsigned int  d;


        d = (unsigned char)c - '0';
        if ( d >= 10 )
        {
          d = (unsigned char)c - 'A';
          if ( d >= 6 )
            d = 16;
          else
            d += 10;
        }

        /* exit if a non-uppercase hexadecimal character was found */
        if ( d >= 16 )
          break;

        value = ( value << 4 ) + d;
      }
      if ( count == 0 )
        return value;
    }

    /* look for a non-initial dot in the glyph name in order to */
    /* sort-out variants like `A.swash', `e.final', etc.        */
    {
      const char*  p;
      int          len;


      p = glyph_name;

      while ( *p && *p != '.' )
        p++;

      len = (int)( p - glyph_name );

      if ( *p && len < 64 )
      {
        strncpy( temp, glyph_name, len );
        temp[len]  = 0;
        glyph_name = temp;
      }
    }

    /* now, look up the glyph in the Adobe Glyph List */
    for ( n = 0; n < NUM_ADOBE_GLYPHS; n++ )
    {
      const char*  name = sid_standard_names[n];


      if ( first == name[0] && strcmp( glyph_name, name ) == 0 )
        return ps_names_to_unicode[n];
    }

    /* not found, there is probably no Unicode value for this glyph name */
    return 0;
  }


  /* qsort callback to sort the unicode map */
  FT_CALLBACK_DEF( int )
  compare_uni_maps( const void*  a,
                    const void*  b )
  {
    PS_UniMap*  map1 = (PS_UniMap*)a;
    PS_UniMap*  map2 = (PS_UniMap*)b;


    return ( map1->unicode - map2->unicode );
  }


  /* Builds a table that maps Unicode values to glyph indices */
  static FT_Error
  PS_Build_Unicode_Table( FT_Memory     memory,
                          FT_UInt       num_glyphs,
                          const char**  glyph_names,
                          PS_Unicodes*  table )
  {
    FT_Error  error;


    /* we first allocate the table */
    table->num_maps = 0;
    table->maps     = 0;

    if ( !ALLOC_ARRAY( table->maps, num_glyphs, PS_UniMap ) )
    {
      FT_UInt     n;
      FT_UInt     count;
      PS_UniMap*  map;
      FT_ULong    uni_char;


      map = table->maps;

      for ( n = 0; n < num_glyphs; n++ )
      {
        const char*  gname = glyph_names[n];


        if ( gname )
        {
          uni_char = PS_Unicode_Value( gname );

          if ( uni_char != 0 && uni_char != 0xFFFF )
          {
            map->unicode     = uni_char;
            map->glyph_index = n;
            map++;
          }
        }
      }

      /* now, compress the table a bit */
      count = (FT_UInt)( map - table->maps );

      if ( count > 0 && REALLOC( table->maps,
                                 num_glyphs * sizeof ( PS_UniMap ),
                                 count * sizeof ( PS_UniMap ) ) )
        count = 0;

      if ( count == 0 )
      {
        FREE( table->maps );
        if ( !error )
          error = PSnames_Err_Invalid_Argument;  /* no unicode chars here! */
      }
      else
        /* sort the table in increasing order of unicode values */
        qsort( table->maps, count, sizeof ( PS_UniMap ), compare_uni_maps );

      table->num_maps = count;
    }

    return error;
  }


  static FT_UInt
  PS_Lookup_Unicode( PS_Unicodes*  table,
                     FT_ULong      unicode )
  {
    PS_UniMap  *min, *max, *mid;


    /* perform a binary search on the table */

    min = table->maps;
    max = min + table->num_maps - 1;

    while ( min <= max )
    {
      mid = min + ( max - min ) / 2;
      if ( mid->unicode == unicode )
        return mid->glyph_index;

      if ( min == max )
        break;

      if ( mid->unicode < unicode )
        min = mid + 1;
      else
        max = mid - 1;
    }

    return 0xFFFF;
  }


#endif /* FT_CONFIG_OPTION_ADOBE_GLYPH_LIST */


  static const char*
  PS_Macintosh_Name( FT_UInt  name_index )
  {
    if ( name_index >= 258 )
      name_index = 0;

    return ps_glyph_names[mac_standard_names[name_index]];
  }


  static const char*
  PS_Standard_Strings( FT_UInt  sid )
  {
    return ( sid < NUM_SID_GLYPHS ? sid_standard_names[sid] : 0 );
  }


  static
  const PSNames_Interface  psnames_interface =
  {
#ifdef FT_CONFIG_OPTION_ADOBE_GLYPH_LIST

    (PS_Unicode_Value_Func)    PS_Unicode_Value,
    (PS_Build_Unicodes_Func)   PS_Build_Unicode_Table,
    (PS_Lookup_Unicode_Func)   PS_Lookup_Unicode,

#else

    0,
    0,
    0,

#endif /* FT_CONFIG_OPTION_ADOBE_GLYPH_LIST */

    (PS_Macintosh_Name_Func)   PS_Macintosh_Name,
    (PS_Adobe_Std_Strings_Func)PS_Standard_Strings,

    t1_standard_encoding,
    t1_expert_encoding
  };


#endif /* !FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES */


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  psnames_module_class =
  {
    0,  /* this is not a font driver, nor a renderer */
    sizeof ( FT_ModuleRec ),

    "psnames",  /* driver name                         */
    0x10000L,   /* driver version                      */
    0x20000L,   /* driver requires FreeType 2 or above */

#ifdef FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES
    0,
#else
    (void*)&psnames_interface,   /* module specific interface */
#endif

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  0
  };


/* END */
