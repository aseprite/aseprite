/***************************************************************************/
/*                                                                         */
/*  ftmm.h                                                                 */
/*                                                                         */
/*    FreeType Multiple Master font interface (specification).             */
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


#ifndef __FTMM_H__
#define __FTMM_H__


#include <ft2build.h>
#include FT_TYPE1_TABLES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    multiple_masters                                                   */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Multiple Masters                                                   */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How to manage Multiple Masters fonts.                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The following types and functions are used to manage Multiple      */
  /*    Master fonts, i.e. the selection of specific design instances by   */
  /*    setting design axis coordinates.                                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_MM_Axis                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to model a given axis in design space for  */
  /*    Multiple Masters fonts.                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    name    :: The axis's name.                                        */
  /*                                                                       */
  /*    minimum :: The axis's minimum design coordinate.                   */
  /*                                                                       */
  /*    maximum :: The axis's maximum design coordinate.                   */
  /*                                                                       */
  typedef struct  FT_MM_Axis_
  {
    FT_String*  name;
    FT_Long     minimum;
    FT_Long     maximum;

  } FT_MM_Axis;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Multi_Master                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model the axes and space of a Multiple Masters */
  /*    font.                                                              */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_axis    :: Number of axes.  Cannot exceed 4.                   */
  /*                                                                       */
  /*    num_designs :: Number of designs; should ne normally 2^num_axis    */
  /*                   even though the Type 1 specification strangely      */
  /*                   allows for intermediate designs to be present. This */
  /*                   number cannot exceed 16.                            */
  /*                                                                       */
  /*    axis        :: A table of axis descriptors.                        */
  /*                                                                       */
  typedef struct  FT_Multi_Master_
  {
    FT_UInt     num_axis;
    FT_UInt     num_designs;
    FT_MM_Axis  axis[T1_MAX_MM_AXIS];

  } FT_Multi_Master;

  /* */

  typedef FT_Error
  (*FT_Get_MM_Func)( FT_Face           face,
                     FT_Multi_Master*  master );

  typedef FT_Error
  (*FT_Set_MM_Design_Func)( FT_Face   face,
                            FT_UInt   num_coords,
                            FT_Long*  coords );

  typedef FT_Error
  (*FT_Set_MM_Blend_Func)( FT_Face   face,
                           FT_UInt   num_coords,
                           FT_Long*  coords );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Multi_Master                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the Multiple Master descriptor of a given font.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: A handle to the source face.                            */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amaster :: The Multiple Masters descriptor.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Get_Multi_Master( FT_Face           face,
                       FT_Multi_Master  *amaster );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_MM_Design_Coordinates                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    For Multiple Masters fonts, choose an interpolated font design     */
  /*    through design coordinates.                                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: A handle to the source face.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_coords :: The number of design coordinates (must be equal to   */
  /*                  the number of axes in the font).                     */
  /*                                                                       */
  /*    coords     :: An array of design coordinates.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_MM_Design_Coordinates( FT_Face   face,
                                FT_UInt   num_coords,
                                FT_Long*  coords );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_MM_Blend_Coordinates                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    For Multiple Masters fonts, choose an interpolated font design     */
  /*    through normalized blend coordinates.                              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: A handle to the source face.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_coords :: The number of design coordinates (must be equal to   */
  /*                  the number of axes in the font).                     */
  /*                                                                       */
  /*    coords     :: The design coordinates array (each element must be   */
  /*                  between 0 and 1.0).                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_MM_Blend_Coordinates( FT_Face    face,
                               FT_UInt    num_coords,
                               FT_Fixed*  coords );


  /* */


FT_END_HEADER

#endif /* __FTMM_H__ */


/* END */
