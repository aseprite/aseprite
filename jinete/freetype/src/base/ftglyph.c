/***************************************************************************/
/*                                                                         */
/*  ftglyph.c                                                              */
/*                                                                         */
/*    FreeType convenience functions to handle glyphs (body).              */
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
  /*  This file contains the definition of several convenience functions   */
  /*  that can be used by client applications to easily retrieve glyph     */
  /*  bitmaps and outlines from a given face.                              */
  /*                                                                       */
  /*  These functions should be optional if you are writing a font server  */
  /*  or text layout engine on top of FreeType.  However, they are pretty  */
  /*  handy for many other simple uses of the library.                     */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_INTERNAL_OBJECTS_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_glyph


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   Convenience functions                                         ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( void )
  FT_Matrix_Multiply( FT_Matrix*  a,
                      FT_Matrix*  b )
  {
    FT_Fixed  xx, xy, yx, yy;


    if ( !a || !b )
      return;

    xx = FT_MulFix( a->xx, b->xx ) + FT_MulFix( a->xy, b->yx );
    xy = FT_MulFix( a->xx, b->xy ) + FT_MulFix( a->xy, b->yy );
    yx = FT_MulFix( a->yx, b->xx ) + FT_MulFix( a->yy, b->yx );
    yy = FT_MulFix( a->yx, b->xy ) + FT_MulFix( a->yy, b->yy );

    b->xx = xx;  b->xy = xy;
    b->yx = yx;  b->yy = yy;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Matrix_Invert( FT_Matrix*  matrix )
  {
    FT_Pos  delta, xx, yy;


    if ( !matrix )
      return FT_Err_Invalid_Argument;

    /* compute discriminant */
    delta = FT_MulFix( matrix->xx, matrix->yy ) -
            FT_MulFix( matrix->xy, matrix->yx );

    if ( !delta )
      return FT_Err_Invalid_Argument;  /* matrix can't be inverted */

    matrix->xy = - FT_DivFix( matrix->xy, delta );
    matrix->yx = - FT_DivFix( matrix->yx, delta );

    xx = matrix->xx;
    yy = matrix->yy;

    matrix->xx = FT_DivFix( yy, delta );
    matrix->yy = FT_DivFix( xx, delta );

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_BitmapGlyph support                                        ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  ft_bitmap_copy( FT_Memory   memory,
                  FT_Bitmap*  source,
                  FT_Bitmap*  target )
  {
    FT_Error  error;
    FT_Int    pitch = source->pitch;
    FT_ULong  size;


    *target = *source;

    if ( pitch < 0 )
      pitch = -pitch;

    size = (FT_ULong)( pitch * source->rows );

    if ( !ALLOC( target->buffer, size ) )
      MEM_Copy( target->buffer, source->buffer, size );

    return error;
  }


  static FT_Error
  ft_bitmap_glyph_init( FT_BitmapGlyph  glyph,
                        FT_GlyphSlot    slot )
  {
    FT_Error    error   = FT_Err_Ok;
    FT_Library  library = FT_GLYPH(glyph)->library;
    FT_Memory   memory  = library->memory;


    if ( slot->format != ft_glyph_format_bitmap )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* grab the bitmap in the slot - do lazy copying whenever possible */
    glyph->bitmap = slot->bitmap;
    glyph->left   = slot->bitmap_left;
    glyph->top    = slot->bitmap_top;

    if ( slot->flags & ft_glyph_own_bitmap )
      slot->flags &= ~ft_glyph_own_bitmap;
    else
    {
      /* copy the bitmap into a new buffer */
      error = ft_bitmap_copy( memory, &slot->bitmap, &glyph->bitmap );
    }

  Exit:
    return error;
  }


  static FT_Error
  ft_bitmap_glyph_copy( FT_BitmapGlyph  source,
                        FT_BitmapGlyph  target )
  {
    FT_Memory  memory = source->root.library->memory;


    target->left = source->left;
    target->top  = source->top;

    return ft_bitmap_copy( memory, &source->bitmap, &target->bitmap );
  }


  static void
  ft_bitmap_glyph_done( FT_BitmapGlyph  glyph )
  {
    FT_Memory  memory = FT_GLYPH(glyph)->library->memory;


    FREE( glyph->bitmap.buffer );
  }


  static void
  ft_bitmap_glyph_bbox( FT_BitmapGlyph  glyph,
                        FT_BBox*        cbox )
  {
    cbox->xMin = glyph->left << 6;
    cbox->xMax = cbox->xMin + ( glyph->bitmap.width << 6 );
    cbox->yMax = glyph->top << 6;
    cbox->yMin = cbox->yMax - ( glyph->bitmap.rows << 6 );
  }


  const FT_Glyph_Class  ft_bitmap_glyph_class =
  {
    sizeof( FT_BitmapGlyphRec ),
    ft_glyph_format_bitmap,

    (FT_Glyph_Init_Func)     ft_bitmap_glyph_init,
    (FT_Glyph_Done_Func)     ft_bitmap_glyph_done,
    (FT_Glyph_Copy_Func)     ft_bitmap_glyph_copy,
    (FT_Glyph_Transform_Func)0,
    (FT_Glyph_BBox_Func)     ft_bitmap_glyph_bbox,
    (FT_Glyph_Prepare_Func)  0
  };


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_OutlineGlyph support                                       ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  ft_outline_glyph_init( FT_OutlineGlyph  glyph,
                         FT_GlyphSlot     slot )
  {
    FT_Error     error   = FT_Err_Ok;
    FT_Library   library = FT_GLYPH(glyph)->library;
    FT_Outline*  source  = &slot->outline;
    FT_Outline*  target  = &glyph->outline;


    /* check format in glyph slot */
    if ( slot->format != ft_glyph_format_outline )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* allocate new outline */
    error = FT_Outline_New( library, source->n_points, source->n_contours,
                            &glyph->outline );
    if ( error )
      goto Exit;

    /* copy it */
    MEM_Copy( target->points, source->points,
              source->n_points * sizeof ( FT_Vector ) );

    MEM_Copy( target->tags, source->tags,
              source->n_points * sizeof ( FT_Byte ) );

    MEM_Copy( target->contours, source->contours,
              source->n_contours * sizeof ( FT_Short ) );

    /* copy all flags, except the `ft_outline_owner' one */
    target->flags = source->flags | ft_outline_owner;

  Exit:
    return error;
  }


  static void
  ft_outline_glyph_done( FT_OutlineGlyph  glyph )
  {
    FT_Outline_Done( FT_GLYPH( glyph )->library, &glyph->outline );
  }


  static FT_Error
  ft_outline_glyph_copy( FT_OutlineGlyph  source,
                         FT_OutlineGlyph  target )
  {
    FT_Error    error;
    FT_Library  library = FT_GLYPH( source )->library;


    error = FT_Outline_New( library, source->outline.n_points,
                            source->outline.n_contours, &target->outline );
    if ( !error )
      FT_Outline_Copy( &source->outline, &target->outline );

    return error;
  }


  static void
  ft_outline_glyph_transform( FT_OutlineGlyph  glyph,
                              FT_Matrix*       matrix,
                              FT_Vector*       delta )
  {
    if ( matrix )
      FT_Outline_Transform( &glyph->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &glyph->outline, delta->x, delta->y );
  }


  static void
  ft_outline_glyph_bbox( FT_OutlineGlyph  glyph,
                         FT_BBox*         bbox )
  {
    FT_Outline_Get_CBox( &glyph->outline, bbox );
  }


  static FT_Error
  ft_outline_glyph_prepare( FT_OutlineGlyph  glyph,
                            FT_GlyphSlot     slot )
  {
    slot->format         = ft_glyph_format_outline;
    slot->outline        = glyph->outline;
    slot->outline.flags &= ~ft_outline_owner;

    return FT_Err_Ok;
  }


  const FT_Glyph_Class  ft_outline_glyph_class =
  {
    sizeof( FT_OutlineGlyphRec ),
    ft_glyph_format_outline,

    (FT_Glyph_Init_Func)     ft_outline_glyph_init,
    (FT_Glyph_Done_Func)     ft_outline_glyph_done,
    (FT_Glyph_Copy_Func)     ft_outline_glyph_copy,
    (FT_Glyph_Transform_Func)ft_outline_glyph_transform,
    (FT_Glyph_BBox_Func)     ft_outline_glyph_bbox,
    (FT_Glyph_Prepare_Func)  ft_outline_glyph_prepare
  };


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   FT_Glyph class and API                                        ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

   static FT_Error
   ft_new_glyph( FT_Library             library,
                 const FT_Glyph_Class*  clazz,
                 FT_Glyph*              aglyph )
   {
     FT_Memory  memory = library->memory;
     FT_Error   error;
     FT_Glyph   glyph;


     *aglyph = 0;

     if ( !ALLOC( glyph, clazz->glyph_size ) )
     {
       glyph->library = library;
       glyph->clazz   = clazz;
       glyph->format  = clazz->glyph_format;

       *aglyph = glyph;
     }

     return error;
   }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_Copy( FT_Glyph   source,
                 FT_Glyph  *target )
  {
    FT_Glyph               copy;
    FT_Error               error;
    const FT_Glyph_Class*  clazz;


    /* check arguments */
    if ( !target || !source || !source->clazz )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    *target = 0;

    clazz = source->clazz;
    error = ft_new_glyph( source->library, clazz, &copy );
    if ( error )
      goto Exit;

    copy->advance = source->advance;
    copy->format  = source->format;

    if ( clazz->glyph_copy )
      error = clazz->glyph_copy( source, copy );

    if ( error )
      FT_Done_Glyph( copy );
    else
      *target = copy;

  Exit:
    return error;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Glyph( FT_GlyphSlot  slot,
                FT_Glyph     *aglyph )
  {
    FT_Library  library = slot->library;
    FT_Error    error;
    FT_Glyph    glyph;

    const FT_Glyph_Class*  clazz = 0;


    if ( !slot )
      return FT_Err_Invalid_Slot_Handle;

    if ( !aglyph )
      return FT_Err_Invalid_Argument;

    /* if it is a bitmap, that's easy :-) */
    if ( slot->format == ft_glyph_format_bitmap )
      clazz = &ft_bitmap_glyph_class;

    /* it it is an outline too */
    else if ( slot->format == ft_glyph_format_outline )
      clazz = &ft_outline_glyph_class;

    else
    {
      /* try to find a renderer that supports the glyph image format */
      FT_Renderer  render = FT_Lookup_Renderer( library, slot->format, 0 );


      if ( render )
        clazz = &render->glyph_class;
    }

    if ( !clazz )
    {
      error = FT_Err_Invalid_Glyph_Format;
      goto Exit;
    }

    /* create FT_Glyph object */
    error = ft_new_glyph( library, clazz, &glyph );
    if ( error )
      goto Exit;

    /* copy advance while converting it to 16.16 format */
    glyph->advance.x = slot->advance.x << 10;
    glyph->advance.y = slot->advance.y << 10;

    /* now import the image from the glyph slot */
    error = clazz->glyph_init( glyph, slot );

    /* if an error occurred, destroy the glyph */
    if ( error )
      FT_Done_Glyph( glyph );
    else
      *aglyph = glyph;

  Exit:
    return error;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_Transform( FT_Glyph    glyph,
                      FT_Matrix*  matrix,
                      FT_Vector*  delta )
  {
    const FT_Glyph_Class*  clazz;
    FT_Error               error = FT_Err_Ok;


    if ( !glyph || !glyph->clazz )
      error = FT_Err_Invalid_Argument;
    else
    {
      clazz = glyph->clazz;
      if ( clazz->glyph_transform )
      {
        /* transform glyph image */
        clazz->glyph_transform( glyph, matrix, delta );

        /* transform advance vector */
        if ( matrix )
          FT_Vector_Transform( &glyph->advance, matrix );
      }
      else
        error = FT_Err_Invalid_Glyph_Format;
    }
    return error;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( void )
  FT_Glyph_Get_CBox( FT_Glyph  glyph,
                     FT_UInt   bbox_mode,
                     FT_BBox  *acbox )
  {
    const FT_Glyph_Class*  clazz;


    if ( !acbox )
      return;

    acbox->xMin = acbox->yMin = acbox->xMax = acbox->yMax = 0;

    if ( !glyph || !glyph->clazz )
      return;
    else
    {
      clazz = glyph->clazz;
      if ( !clazz->glyph_bbox )
        return;
      else
      {
        /* retrieve bbox in 26.6 coordinates */
        clazz->glyph_bbox( glyph, acbox );

        /* perform grid fitting if needed */
        if ( bbox_mode & ft_glyph_bbox_gridfit )
        {
          acbox->xMin &= -64;
          acbox->yMin &= -64;
          acbox->xMax  = ( acbox->xMax + 63 ) & -64;
          acbox->yMax  = ( acbox->yMax + 63 ) & -64;
        }

        /* convert to integer pixels if needed */
        if ( bbox_mode & ft_glyph_bbox_truncate )
        {
          acbox->xMin >>= 6;
          acbox->yMin >>= 6;
          acbox->xMax >>= 6;
          acbox->yMax >>= 6;
        }
      }
    }
    return;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_To_Bitmap( FT_Glyph*   the_glyph,
                      FT_ULong    render_mode,
                      FT_Vector*  origin,
                      FT_Bool     destroy )
  {
    FT_GlyphSlotRec  dummy;
    FT_Error         error;
    FT_Glyph         glyph;
    FT_BitmapGlyph   bitmap = NULL;

    const FT_Glyph_Class*  clazz;


    /* check argument */
    if ( !the_glyph )
      goto Bad;

    /* we render the glyph into a glyph bitmap using a `dummy' glyph slot */
    /* then calling FT_Render_Glyph_Internal()                            */

    glyph = *the_glyph;
    if ( !glyph )
      goto Bad;

    clazz = glyph->clazz;
    if ( !clazz || !clazz->glyph_prepare )
      goto Bad;

    MEM_Set( &dummy, 0, sizeof ( dummy ) );
    dummy.library = glyph->library;
    dummy.format  = clazz->glyph_format;

    /* create result bitmap glyph */
    error = ft_new_glyph( glyph->library, &ft_bitmap_glyph_class,
                          (FT_Glyph*)&bitmap );
    if ( error )
      goto Exit;

#if 0
    /* if `origin' is set, translate the glyph image */
    if ( origin )
      FT_Glyph_Transform( glyph, 0, origin );
#else
    FT_UNUSED( origin );
#endif

    /* prepare dummy slot for rendering */
    error = clazz->glyph_prepare( glyph, &dummy );
    if ( !error )
      error = FT_Render_Glyph_Internal( glyph->library, &dummy, render_mode );

#if 0
    if ( !destroy && origin )
    {
      FT_Vector  v;


      v.x = -origin->x;
      v.y = -origin->y;
      FT_Glyph_Transform( glyph, 0, &v );
    }
#endif

    if ( error )
      goto Exit;

    /* in case of success, copy the bitmap to the glyph bitmap */
    error = ft_bitmap_glyph_init( bitmap, &dummy );
    if ( error )
      goto Exit;

    /* copy advance */
    bitmap->root.advance = glyph->advance;

    if ( destroy )
      FT_Done_Glyph( glyph );

    *the_glyph = FT_GLYPH( bitmap );

  Exit:
    if ( error && bitmap )
      FT_Done_Glyph( FT_GLYPH( bitmap ) );

    return error;

  Bad:
    error = FT_Err_Invalid_Argument;
    goto Exit;
  }


  /* documentation is in ftglyph.h */

  FT_EXPORT_DEF( void )
  FT_Done_Glyph( FT_Glyph  glyph )
  {
    if ( glyph )
    {
      FT_Memory              memory = glyph->library->memory;
      const FT_Glyph_Class*  clazz  = glyph->clazz;


      if ( clazz->glyph_done )
        clazz->glyph_done( glyph );

      FREE( glyph );
    }
  }


/* END */
