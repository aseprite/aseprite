/***************************************************************************/
/*                                                                         */
/*  ftnames.c                                                              */
/*                                                                         */
/*    Simple interface to access SFNT name tables (which are used          */
/*    to hold font names, copyright info, notices, etc.) (body).           */
/*                                                                         */
/*    This is _not_ used to retrieve glyph names!                          */
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
#include FT_SFNT_NAMES_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


#ifdef TT_CONFIG_OPTION_SFNT_NAMES


  /* documentation is in ftnames.h */

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Sfnt_Name_Count( FT_Face  face )
  {
    return (face && FT_IS_SFNT( face )) ? ((TT_Face)face)->num_names : 0;
  }


  /* documentation is in ftnames.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Sfnt_Name( FT_Face       face,
                    FT_UInt       index,
                    FT_SfntName  *aname )
  {
    FT_Error  error = FT_Err_Invalid_Argument;


    if ( aname && face && FT_IS_SFNT( face ) )
    {
      TT_Face  ttface = (TT_Face)face;


      if ( index < (FT_UInt)ttface->num_names )
      {
        TT_NameRec*  name = ttface->name_table.names + index;


        aname->platform_id = name->platformID;
        aname->encoding_id = name->encodingID;
        aname->language_id = name->languageID;
        aname->name_id     = name->nameID;
        aname->string      = (FT_Byte*)name->string;
        aname->string_len  = name->stringLength;

        error = FT_Err_Ok;
      }
    }

    return error;
  }


#endif /* TT_CONFIG_OPTION_SFNT_NAMES */


/* END */
