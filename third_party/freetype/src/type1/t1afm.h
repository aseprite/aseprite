/***************************************************************************/
/*                                                                         */
/*  t1afm.h                                                                */
/*                                                                         */
/*    AFM support for Type 1 fonts (specification).                        */
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


#ifndef __T1AFM_H__
#define __T1AFM_H__

#include <ft2build.h>
#include "t1objs.h"


FT_BEGIN_HEADER


  typedef struct  T1_Kern_Pair_
  {
    FT_UInt    glyph1;
    FT_UInt    glyph2;
    FT_Vector  kerning;

  } T1_Kern_Pair;


  typedef struct  T1_AFM_
  {
    FT_Int         num_pairs;
    T1_Kern_Pair*  kern_pairs;

  } T1_AFM;


  FT_LOCAL FT_Error
  T1_Read_AFM( FT_Face    face,
               FT_Stream  stream );

  FT_LOCAL void
  T1_Done_AFM( FT_Memory  memory,
               T1_AFM*    afm );

  FT_LOCAL void
  T1_Get_Kerning( T1_AFM*     afm,
                  FT_UInt     glyph1,
                  FT_UInt     glyph2,
                  FT_Vector*  kerning );


FT_END_HEADER

#endif /* __T1AFM_H__ */


/* END */
