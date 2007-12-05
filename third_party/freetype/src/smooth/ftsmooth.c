/***************************************************************************/
/*                                                                         */
/*  ftsmooth.c                                                             */
/*                                                                         */
/*    Anti-aliasing renderer interface (body).                             */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include "ftsmooth.h"
#include "ftgrays.h"

#include "ftsmerrs.h"


  /* initialize renderer -- init its raster */
  static FT_Error
  ft_smooth_init( FT_Renderer  render )
  {
    FT_Library  library = FT_MODULE_LIBRARY( render );


    render->clazz->raster_class->raster_reset( render->raster,
                                               library->raster_pool,
                                               library->raster_pool_size );

    return 0;
  }


  /* sets render-specific mode */
  static FT_Error
  ft_smooth_set_mode( FT_Renderer  render,
                      FT_ULong     mode_tag,
                      FT_Pointer   data )
  {
    /* we simply pass it to the raster */
    return render->clazz->raster_class->raster_set_mode( render->raster,
                                                         mode_tag,
                                                         data );
  }

  /* transform a given glyph image */
  static FT_Error
  ft_smooth_transform( FT_Renderer   render,
                       FT_GlyphSlot  slot,
                       FT_Matrix*    matrix,
                       FT_Vector*    delta )
  {
    FT_Error  error = Smooth_Err_Ok;


    if ( slot->format != render->glyph_format )
    {
      error = Smooth_Err_Invalid_Argument;
      goto Exit;
    }

    if ( matrix )
      FT_Outline_Transform( &slot->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &slot->outline, delta->x, delta->y );

  Exit:
    return error;
  }


  /* return the glyph's control box */
  static void
  ft_smooth_get_cbox( FT_Renderer   render,
                      FT_GlyphSlot  slot,
                      FT_BBox*      cbox )
  {
    MEM_Set( cbox, 0, sizeof ( *cbox ) );

    if ( slot->format == render->glyph_format )
      FT_Outline_Get_CBox( &slot->outline, cbox );
  }


  /* convert a slot's glyph image into a bitmap */
  static FT_Error
  ft_smooth_render( FT_Renderer   render,
                    FT_GlyphSlot  slot,
                    FT_UInt       mode,
                    FT_Vector*    origin )
  {
    FT_Error     error;
    FT_Outline*  outline = NULL;
    FT_BBox      cbox;
    FT_UInt      width, height, pitch;
    FT_Bitmap*   bitmap;
    FT_Memory    memory;

    FT_Raster_Params  params;


    /* check glyph image format */
    if ( slot->format != render->glyph_format )
    {
      error = Smooth_Err_Invalid_Argument;
      goto Exit;
    }

    /* check mode */
    if ( mode != ft_render_mode_normal )
      return Smooth_Err_Cannot_Render_Glyph;

    outline = &slot->outline;

    /* translate the outline to the new origin if needed */
    if ( origin )
      FT_Outline_Translate( outline, origin->x, origin->y );

    /* compute the control box, and grid fit it */
    FT_Outline_Get_CBox( outline, &cbox );

    cbox.xMin &= -64;
    cbox.yMin &= -64;
    cbox.xMax  = ( cbox.xMax + 63 ) & -64;
    cbox.yMax  = ( cbox.yMax + 63 ) & -64;

    width  = ( cbox.xMax - cbox.xMin ) >> 6;
    height = ( cbox.yMax - cbox.yMin ) >> 6;
    bitmap = &slot->bitmap;
    memory = render->root.memory;

    /* release old bitmap buffer */
    if ( slot->flags & ft_glyph_own_bitmap )
    {
      FREE( bitmap->buffer );
      slot->flags &= ~ft_glyph_own_bitmap;
    }

    /* allocate new one, depends on pixel format */
    pitch = width;
    bitmap->pixel_mode = ft_pixel_mode_grays;
    bitmap->num_grays  = 256;
    bitmap->width      = width;
    bitmap->rows       = height;
    bitmap->pitch      = pitch;

    if ( ALLOC( bitmap->buffer, (FT_ULong)pitch * height ) )
      goto Exit;

    slot->flags |= ft_glyph_own_bitmap;

    /* translate outline to render it into the bitmap */
    FT_Outline_Translate( outline, -cbox.xMin, -cbox.yMin );

    /* set up parameters */
    params.target = bitmap;
    params.source = outline;
    params.flags  = ft_raster_flag_aa;

    /* render outline into the bitmap */
    error = render->raster_render( render->raster, &params );
    
    FT_Outline_Translate( outline, cbox.xMin, cbox.yMin );

    if ( error )
      goto Exit;

    slot->format      = ft_glyph_format_bitmap;
    slot->bitmap_left = cbox.xMin >> 6;
    slot->bitmap_top  = cbox.yMax >> 6;

  Exit:
    if ( outline && origin )
      FT_Outline_Translate( outline, -origin->x, -origin->y );

    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_smooth_renderer_class =
  {
    {
      ft_module_renderer,
      sizeof( FT_RendererRec ),

      "smooth",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_smooth_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    ft_glyph_format_outline,

    (FTRenderer_render)   ft_smooth_render,
    (FTRenderer_transform)ft_smooth_transform,
    (FTRenderer_getCBox)  ft_smooth_get_cbox,
    (FTRenderer_setMode)  ft_smooth_set_mode,

    (FT_Raster_Funcs*)    &ft_grays_raster
  };


/* END */
