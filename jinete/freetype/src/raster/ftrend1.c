/***************************************************************************/
/*                                                                         */
/*  ftrend1.c                                                              */
/*                                                                         */
/*    The FreeType glyph rasterizer interface (body).                      */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include "ftrend1.h"
#include "ftraster.h"

#include "rasterrs.h"


  /* initialize renderer -- init its raster */
  static FT_Error
  ft_raster1_init( FT_Renderer  render )
  {
    FT_Library  library = FT_MODULE_LIBRARY( render );


    render->clazz->raster_class->raster_reset( render->raster,
                                               library->raster_pool,
                                               library->raster_pool_size );

    return Raster_Err_Ok;
  }


  /* set render-specific mode */
  static FT_Error
  ft_raster1_set_mode( FT_Renderer  render,
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
  ft_raster1_transform( FT_Renderer   render,
                        FT_GlyphSlot  slot,
                        FT_Matrix*    matrix,
                        FT_Vector*    delta )
  {
    FT_Error error = Raster_Err_Ok;


    if ( slot->format != render->glyph_format )
    {
      error = Raster_Err_Invalid_Argument;
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
  ft_raster1_get_cbox( FT_Renderer   render,
                       FT_GlyphSlot  slot,
                       FT_BBox*      cbox )
  {
    MEM_Set( cbox, 0, sizeof ( *cbox ) );

    if ( slot->format == render->glyph_format )
      FT_Outline_Get_CBox( &slot->outline, cbox );
  }


  /* convert a slot's glyph image into a bitmap */
  static FT_Error
  ft_raster1_render( FT_Renderer   render,
                     FT_GlyphSlot  slot,
                     FT_UInt       mode,
                     FT_Vector*    origin )
  {
    FT_Error     error;
    FT_Outline*  outline;
    FT_BBox      cbox;
    FT_UInt      width, height, pitch;
    FT_Bitmap*   bitmap;
    FT_Memory    memory;

    FT_Raster_Params  params;


    /* check glyph image format */
    if ( slot->format != render->glyph_format )
    {
      error = Raster_Err_Invalid_Argument;
      goto Exit;
    }

    /* check rendering mode */
    if ( mode != ft_render_mode_mono )
    {
      /* raster1 is only capable of producing monochrome bitmaps */
      if ( render->clazz == &ft_raster1_renderer_class )
        return Raster_Err_Cannot_Render_Glyph;
    }
    else
    {
      /* raster5 is only capable of producing 5-gray-levels bitmaps */
      if ( render->clazz == &ft_raster5_renderer_class )
        return Raster_Err_Cannot_Render_Glyph;
    }

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
    if ( !( mode & ft_render_mode_mono ) )
    {
      /* we pad to 32 bits, only for backwards compatibility with FT 1.x */
      pitch = ( width + 3 ) & -4;
      bitmap->pixel_mode = ft_pixel_mode_grays;
      bitmap->num_grays  = 256;
    }
    else
    {
      pitch = ( width + 7 ) >> 3;
      bitmap->pixel_mode = ft_pixel_mode_mono;
    }

    bitmap->width = width;
    bitmap->rows  = height;
    bitmap->pitch = pitch;

    if ( ALLOC( bitmap->buffer, (FT_ULong)pitch * height ) )
      goto Exit;

    slot->flags |= ft_glyph_own_bitmap;

    /* translate outline to render it into the bitmap */
    FT_Outline_Translate( outline, -cbox.xMin, -cbox.yMin );

    /* set up parameters */
    params.target = bitmap;
    params.source = outline;
    params.flags  = 0;

    if ( bitmap->pixel_mode == ft_pixel_mode_grays )
      params.flags |= ft_raster_flag_aa;

    /* render outline into the bitmap */
    error = render->raster_render( render->raster, &params );
    
    FT_Outline_Translate( outline, cbox.xMin, cbox.yMin );
    
    if ( error )
      goto Exit;

    slot->format      = ft_glyph_format_bitmap;
    slot->bitmap_left = cbox.xMin >> 6;
    slot->bitmap_top  = cbox.yMax >> 6;

  Exit:
    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_raster1_renderer_class =
  {
    {
      ft_module_renderer,
      sizeof( FT_RendererRec ),

      "raster1",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_raster1_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    ft_glyph_format_outline,

    (FTRenderer_render)   ft_raster1_render,
    (FTRenderer_transform)ft_raster1_transform,
    (FTRenderer_getCBox)  ft_raster1_get_cbox,
    (FTRenderer_setMode)  ft_raster1_set_mode,

    (FT_Raster_Funcs*)    &ft_standard_raster
  };


  /* This renderer is _NOT_ part of the default modules; you will need */
  /* to register it by hand in your application.  It should only be    */
  /* used for backwards-compatibility with FT 1.x anyway.              */
  /*                                                                   */
  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_raster5_renderer_class =
  {
    {
      ft_module_renderer,
      sizeof( FT_RendererRec ),

      "raster5",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_raster1_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    ft_glyph_format_outline,

    (FTRenderer_render)   ft_raster1_render,
    (FTRenderer_transform)ft_raster1_transform,
    (FTRenderer_getCBox)  ft_raster1_get_cbox,
    (FTRenderer_setMode)  ft_raster1_set_mode,

    (FT_Raster_Funcs*)    &ft_standard_raster
  };


/* END */
