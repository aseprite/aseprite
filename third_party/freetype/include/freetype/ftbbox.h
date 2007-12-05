/***************************************************************************/
/*                                                                         */
/*  ftbbox.h                                                               */
/*                                                                         */
/*    FreeType exact bbox computation (specification).                     */
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
  /* This component has a _single_ role: to compute exact outline bounding */
  /* boxes.                                                                */
  /*                                                                       */
  /* It is separated from the rest of the engine for various technical     */
  /* reasons.  It may well be integrated in `ftoutln' later.               */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTBBOX_H__
#define __FTBBOX_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    outline_processing                                                 */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Get_BBox                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the exact bounding box of an outline.  This is slower     */
  /*    than computing the control box.  However, it uses an advanced      */
  /*    algorithm which returns _very_ quickly when the two boxes          */
  /*    coincide.  Otherwise, the outline Bezier arcs are walked over to   */
  /*    extract their extrema.                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline :: A pointer to the source outline.                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    abbox   :: The outline's exact bounding box.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Outline_Get_BBox( FT_Outline*  outline,
                       FT_BBox     *abbox );


  /* */


FT_END_HEADER

#endif /* __FTBBOX_H__ */


/* END */
