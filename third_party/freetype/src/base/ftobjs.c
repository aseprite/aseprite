/***************************************************************************/
/*                                                                         */
/*  ftobjs.c                                                               */
/*                                                                         */
/*    The FreeType private base classes (body).                            */
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
#include FT_LIST_H
#include FT_OUTLINE_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H

#include <string.h>     /* for strcmp() */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_memory


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_Alloc( FT_Memory  memory,
            FT_Long    size,
            void*     *P )
  {
    FT_Assert( P != 0 );

    if ( size > 0 )
    {
      *P = memory->alloc( memory, size );
      if ( !*P )
      {
        FT_ERROR(( "FT_Alloc:" ));
        FT_ERROR(( " Out of memory? (%ld requested)\n",
                   size ));

        return FT_Err_Out_Of_Memory;
      }
      MEM_Set( *P, 0, size );
    }
    else
      *P = NULL;

    FT_TRACE7(( "FT_Alloc:" ));
    FT_TRACE7(( " size = %ld, block = 0x%08p, ref = 0x%08p\n",
                size, *P, P ));

    return FT_Err_Ok;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_Realloc( FT_Memory  memory,
              FT_Long    current,
              FT_Long    size,
              void**     P )
  {
    void*  Q;


    FT_Assert( P != 0 );

    /* if the original pointer is NULL, call FT_Alloc() */
    if ( !*P )
      return FT_Alloc( memory, size, P );

    /* if the new block if zero-sized, clear the current one */
    if ( size <= 0 )
    {
      FT_Free( memory, P );
      return FT_Err_Ok;
    }

    Q = memory->realloc( memory, current, size, *P );
    if ( !Q )
      goto Fail;

    if ( size > current )
      memset( (char*)Q + current, 0, size - current );

    *P = Q;
    return FT_Err_Ok;

  Fail:
    FT_ERROR(( "FT_Realloc:" ));
    FT_ERROR(( " Failed (current %ld, requested %ld)\n",
               current, size ));
    return FT_Err_Out_Of_Memory;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( void )
  FT_Free( FT_Memory  memory,
           void**     P )
  {
    FT_TRACE7(( "FT_Free:" ));
    FT_TRACE7(( " Freeing block 0x%08p, ref 0x%08p\n",
                P, P ? *P : (void*)0 ));

    if ( P && *P )
    {
      memory->free( memory, *P );
      *P = 0;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           S T R E A M                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_new_input_stream                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new input stream object from an FT_Open_Args structure.  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function expects a valid `astream' parameter.                  */
  /*                                                                       */
  static FT_Error
  ft_new_input_stream( FT_Library     library,
                       FT_Open_Args*  args,
                       FT_Stream*     astream )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Stream  stream;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !args )
      return FT_Err_Invalid_Argument;

    *astream = 0;
    memory   = library->memory;
    if ( ALLOC( stream, sizeof ( *stream ) ) )
      goto Exit;

    stream->memory = memory;

    /* now, look at the stream flags */
    if ( args->flags & ft_open_memory )
    {
      error = 0;
      FT_New_Memory_Stream( library,
                            (FT_Byte*)args->memory_base,
                            args->memory_size,
                            stream );
    }
    else if ( args->flags & ft_open_pathname )
    {
      error = FT_New_Stream( args->pathname, stream );
      stream->pathname.pointer = args->pathname;
    }
    else if ( ( args->flags & ft_open_stream ) && args->stream )
    {
      /* in this case, we do not need to allocate a new stream object */
      /* since the caller is responsible for closing it himself       */
      FREE( stream );
      stream = args->stream;
    }
    else
      error = FT_Err_Invalid_Argument;

    if ( error )
      FREE( stream );

    *astream = stream;

  Exit:
    return error;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( void )
  FT_Done_Stream( FT_Stream  stream )
  {
    if ( stream && stream->close )
    {
      stream->close( stream );
      stream->close = 0;
    }
  }


  static void
  ft_done_stream( FT_Stream*  astream,
                  FT_Int      external )
  {
    FT_Stream  stream = *astream;


    if ( stream->close )
      stream->close( stream );

    if ( !external )
    {
      FT_Memory  memory = stream->memory;


      FREE( stream );
    }
    *astream = 0;
  }


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_objs


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                     G L Y P H   L O A D E R                     ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The glyph loader is a simple object which is used to load a set of    */
  /* glyphs easily.  It is critical for the correct loading of composites. */
  /*                                                                       */
  /* Ideally, one can see it as a stack of abstract `glyph' objects.       */
  /*                                                                       */
  /*   loader.base     Is really the bottom of the stack.  It describes a  */
  /*                   single glyph image made of the juxtaposition of     */
  /*                   several glyphs (those `in the stack').              */
  /*                                                                       */
  /*   loader.current  Describes the top of the stack, on which a new      */
  /*                   glyph can be loaded.                                */
  /*                                                                       */
  /*   Rewind          Clears the stack.                                   */
  /*   Prepare         Set up `loader.current' for addition of a new glyph */
  /*                   image.                                              */
  /*   Add             Add the `current' glyph image to the `base' one,    */
  /*                   and prepare for another one.                        */
  /*                                                                       */
  /* The glyph loader is now a base object.  Each driver used to           */
  /* re-implement it in one way or the other, which wasted code and        */
  /* energy.                                                               */
  /*                                                                       */
  /*************************************************************************/


  /* create a new glyph loader */
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_New( FT_Memory         memory,
                      FT_GlyphLoader*  *aloader )
  {
    FT_GlyphLoader*  loader;
    FT_Error         error;


    if ( !ALLOC( loader, sizeof ( *loader ) ) )
    {
      loader->memory = memory;
      *aloader       = loader;
    }
    return error;
  }


  /* rewind the glyph loader - reset counters to 0 */
  FT_BASE_DEF( void )
  FT_GlyphLoader_Rewind( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;


    base->outline.n_points   = 0;
    base->outline.n_contours = 0;
    base->num_subglyphs      = 0;

    *current = *base;
  }


  /* reset the glyph loader, frees all allocated tables */
  /* and starts from zero                               */
  FT_BASE_DEF( void )
  FT_GlyphLoader_Reset( FT_GlyphLoader*  loader )
  {
    FT_Memory memory = loader->memory;


    FREE( loader->base.outline.points );
    FREE( loader->base.outline.tags );
    FREE( loader->base.outline.contours );
    FREE( loader->base.extra_points );
    FREE( loader->base.subglyphs );

    loader->max_points    = 0;
    loader->max_contours  = 0;
    loader->max_subglyphs = 0;

    FT_GlyphLoader_Rewind( loader );
  }


  /* delete a glyph loader */
  FT_BASE_DEF( void )
  FT_GlyphLoader_Done( FT_GlyphLoader*  loader )
  {
    if ( loader )
    {
      FT_Memory memory = loader->memory;


      FT_GlyphLoader_Reset( loader );
      FREE( loader );
    }
  }


  /* re-adjust the `current' outline fields */
  static void
  FT_GlyphLoader_Adjust_Points( FT_GlyphLoader*  loader )
  {
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;


    current->points   = base->points   + base->n_points;
    current->tags     = base->tags     + base->n_points;
    current->contours = base->contours + base->n_contours;

    /* handle extra points table - if any */
    if ( loader->use_extra )
      loader->current.extra_points =
        loader->base.extra_points + base->n_points;
  }


  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_Create_Extra( FT_GlyphLoader*  loader )
  {
    FT_Error   error;
    FT_Memory  memory = loader->memory;


    if ( !ALLOC_ARRAY( loader->base.extra_points,
                       loader->max_points, FT_Vector ) )
    {
      loader->use_extra = 1;
      FT_GlyphLoader_Adjust_Points( loader );
    }
    return error;
  }


  /* re-adjust the `current' subglyphs field */
  static void
  FT_GlyphLoader_Adjust_Subglyphs( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad* base    = &loader->base;
    FT_GlyphLoad* current = &loader->current;


    current->subglyphs = base->subglyphs + base->num_subglyphs;
  }


  /* Ensure that we can add `n_points' and `n_contours' to our glyph. this */
  /* function reallocates its outline tables if necessary.  Note that it   */
  /* DOESN'T change the number of points within the loader!                */
  /*                                                                       */
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_Check_Points( FT_GlyphLoader*  loader,
                               FT_UInt          n_points,
                               FT_UInt          n_contours )
  {
    FT_Memory    memory  = loader->memory;
    FT_Error     error   = FT_Err_Ok;
    FT_Outline*  base    = &loader->base.outline;
    FT_Outline*  current = &loader->current.outline;
    FT_Bool      adjust  = 1;

    FT_UInt      new_max, old_max;


    /* check points & tags */
    new_max = base->n_points + current->n_points + n_points;
    old_max = loader->max_points;

    if ( new_max > old_max )
    {
      new_max = ( new_max + 7 ) & -8;

      if ( REALLOC_ARRAY( base->points, old_max, new_max, FT_Vector ) ||
           REALLOC_ARRAY( base->tags,   old_max, new_max, FT_Byte   ) )
       goto Exit;

      if ( loader->use_extra &&
           REALLOC_ARRAY( loader->base.extra_points, old_max,
                          new_max, FT_Vector ) )
       goto Exit;

      adjust = 1;
      loader->max_points = new_max;
    }

    /* check contours */
    old_max = loader->max_contours;
    new_max = base->n_contours + current->n_contours +
              n_contours;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 3 ) & -4;
      if ( REALLOC_ARRAY( base->contours, old_max, new_max, FT_Short ) )
        goto Exit;

      adjust = 1;
      loader->max_contours = new_max;
    }

    if ( adjust )
      FT_GlyphLoader_Adjust_Points( loader );

  Exit:
    return error;
  }


  /* Ensure that we can add `n_subglyphs' to our glyph. this function */
  /* reallocates its subglyphs table if necessary.  Note that it DOES */
  /* NOT change the number of subglyphs within the loader!            */
  /*                                                                  */
  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_Check_Subglyphs( FT_GlyphLoader*  loader,
                                  FT_UInt          n_subs )
  {
    FT_Memory  memory = loader->memory;
    FT_Error   error  = FT_Err_Ok;
    FT_UInt    new_max, old_max;

    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;


    new_max = base->num_subglyphs + current->num_subglyphs + n_subs;
    old_max = loader->max_subglyphs;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 1 ) & -2;
      if ( REALLOC_ARRAY( base->subglyphs, old_max, new_max, FT_SubGlyph ) )
        goto Exit;

      loader->max_subglyphs = new_max;

      FT_GlyphLoader_Adjust_Subglyphs( loader );
    }

  Exit:
    return error;
  }


  /* prepare loader for the addition of a new glyph on top of the base one */
  FT_BASE_DEF( void )
  FT_GlyphLoader_Prepare( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  current = &loader->current;


    current->outline.n_points   = 0;
    current->outline.n_contours = 0;
    current->num_subglyphs      = 0;

    FT_GlyphLoader_Adjust_Points   ( loader );
    FT_GlyphLoader_Adjust_Subglyphs( loader );
  }


  /* add current glyph to the base image - and prepare for another */
  FT_BASE_DEF( void )
  FT_GlyphLoader_Add( FT_GlyphLoader*  loader )
  {
    FT_GlyphLoad*  base    = &loader->base;
    FT_GlyphLoad*  current = &loader->current;

    FT_UInt        n_curr_contours = current->outline.n_contours;
    FT_UInt        n_base_points   = base->outline.n_points;
    FT_UInt        n;


    base->outline.n_points =
      (short)( base->outline.n_points + current->outline.n_points );
    base->outline.n_contours =
      (short)( base->outline.n_contours + current->outline.n_contours );

    base->num_subglyphs += current->num_subglyphs;

    /* adjust contours count in newest outline */
    for ( n = 0; n < n_curr_contours; n++ )
      current->outline.contours[n] =
        (short)( current->outline.contours[n] + n_base_points );

    /* prepare for another new glyph image */
    FT_GlyphLoader_Prepare( loader );
  }


  FT_BASE_DEF( FT_Error )
  FT_GlyphLoader_Copy_Points( FT_GlyphLoader*  target,
                              FT_GlyphLoader*  source )
  {
    FT_Error  error;
    FT_UInt   num_points   = source->base.outline.n_points;
    FT_UInt   num_contours = source->base.outline.n_contours;


    error = FT_GlyphLoader_Check_Points( target, num_points, num_contours );
    if ( !error )
    {
      FT_Outline*  out = &target->base.outline;
      FT_Outline*  in  = &source->base.outline;


      MEM_Copy( out->points, in->points,
                num_points * sizeof ( FT_Vector ) );
      MEM_Copy( out->tags, in->tags,
                num_points * sizeof ( char ) );
      MEM_Copy( out->contours, in->contours,
                num_contours * sizeof ( short ) );

      /* do we need to copy the extra points? */
      if ( target->use_extra && source->use_extra )
        MEM_Copy( target->base.extra_points, source->base.extra_points,
                  num_points * sizeof ( FT_Vector ) );

      out->n_points   = (short)num_points;
      out->n_contours = (short)num_contours;

      FT_GlyphLoader_Adjust_Points( target );
    }

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****               FACE, SIZE & GLYPH SLOT OBJECTS                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  ft_glyphslot_init( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class*  clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;
    FT_Error          error  = FT_Err_Ok;
    FT_Slot_Internal  internal;


    slot->library = driver->root.library;

    if ( ALLOC( internal, sizeof ( *internal ) ) )
      goto Exit;

    slot->internal = internal;

    if ( FT_DRIVER_USES_OUTLINES( driver ) )
      error = FT_GlyphLoader_New( memory, &internal->loader );

    if ( !error && clazz->init_slot )
      error = clazz->init_slot( slot );

  Exit:
    return error;
  }


  static void
  ft_glyphslot_clear( FT_GlyphSlot  slot )
  {
    /* free bitmap if needed */
    if ( slot->flags & ft_glyph_own_bitmap )
    {
      FT_Memory  memory = FT_FACE_MEMORY( slot->face );


      FREE( slot->bitmap.buffer );
      slot->flags &= ~ft_glyph_own_bitmap;
    }

    /* clear all public fields in the glyph slot */
    MEM_Set( &slot->metrics, 0, sizeof ( slot->metrics ) );
    MEM_Set( &slot->outline, 0, sizeof ( slot->outline ) );
    MEM_Set( &slot->bitmap,  0, sizeof ( slot->bitmap )  );

    slot->bitmap_left   = 0;
    slot->bitmap_top    = 0;
    slot->num_subglyphs = 0;
    slot->subglyphs     = 0;
    slot->control_data  = 0;
    slot->control_len   = 0;
    slot->other         = 0;
    slot->format        = ft_glyph_format_none;

    slot->linearHoriAdvance = 0;
    slot->linearVertAdvance = 0;
  }


  static void
  ft_glyphslot_done( FT_GlyphSlot  slot )
  {
    FT_Driver         driver = slot->face->driver;
    FT_Driver_Class*  clazz  = driver->clazz;
    FT_Memory         memory = driver->root.memory;


    if ( clazz->done_slot )
      clazz->done_slot( slot );

    /* free bitmap buffer if needed */
    if ( slot->flags & ft_glyph_own_bitmap )
      FREE( slot->bitmap.buffer );

    /* free glyph loader */
    if ( FT_DRIVER_USES_OUTLINES( driver ) )
    {
      FT_GlyphLoader_Done( slot->internal->loader );
      slot->internal->loader = 0;
    }

    FREE( slot->internal );
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( FT_Error )
  FT_New_GlyphSlot( FT_Face        face,
                    FT_GlyphSlot  *aslot )
  {
    FT_Error          error;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;
    FT_Memory         memory;
    FT_GlyphSlot      slot;


    if ( !face || !aslot || !face->driver )
      return FT_Err_Invalid_Argument;

    *aslot = 0;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = driver->root.memory;

    FT_TRACE4(( "FT_New_GlyphSlot: Creating new slot object\n" ));
    if ( !ALLOC( slot, clazz->slot_object_size ) )
    {
      slot->face = face;

      error = ft_glyphslot_init( slot );
      if ( error )
      {
        ft_glyphslot_done( slot );
        FREE( slot );
        goto Exit;
      }

      *aslot = slot;
    }

  Exit:
    FT_TRACE4(( "FT_New_GlyphSlot: Return %d\n", error ));
    return error;
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( void )
  FT_Done_GlyphSlot( FT_GlyphSlot  slot )
  {
    if ( slot )
    {
      FT_Driver         driver = slot->face->driver;
      FT_Memory         memory = driver->root.memory;
      FT_GlyphSlot*     parent;
      FT_GlyphSlot      cur;


      /* Remove slot from its parent face's list */
      parent = &slot->face->glyph;
      cur    = *parent;

      while ( cur )
      {
        if ( cur == slot )
        {
          *parent = cur->next;
          ft_glyphslot_done( slot );
          FREE( slot );
          break;
        }
        cur = cur->next;
      }
    }
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( void )
  FT_Set_Transform( FT_Face     face,
                    FT_Matrix*  matrix,
                    FT_Vector*  delta )
  {
    FT_Face_Internal  internal;


    if ( !face )
      return;

    internal = face->internal;

    internal->transform_flags = 0;

    if ( !matrix )
    {
      internal->transform_matrix.xx = 0x10000L;
      internal->transform_matrix.xy = 0;
      internal->transform_matrix.yx = 0;
      internal->transform_matrix.yy = 0x10000L;
      matrix = &internal->transform_matrix;
    }
    else
      internal->transform_matrix = *matrix;

    /* set transform_flags bit flag 0 if `matrix' isn't the identity */
    if ( ( matrix->xy | matrix->yx ) ||
         matrix->xx != 0x10000L      ||
         matrix->yy != 0x10000L      )
      internal->transform_flags |= 1;

    if ( !delta )
    {
      internal->transform_delta.x = 0;
      internal->transform_delta.y = 0;
      delta = &internal->transform_delta;
    }
    else
      internal->transform_delta = *delta;

    /* set transform_flags bit flag 1 if `delta' isn't the null vector */
    if ( delta->x | delta->y )
      internal->transform_flags |= 2;
  }


  static FT_Renderer
  ft_lookup_glyph_renderer( FT_GlyphSlot  slot );


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Load_Glyph( FT_Face  face,
                 FT_UInt  glyph_index,
                 FT_Int   load_flags )
  {
    FT_Error      error;
    FT_Driver     driver;
    FT_GlyphSlot  slot;
    FT_Library    library;
    FT_Bool       autohint;
    FT_Module     hinter;


    if ( !face || !face->size || !face->glyph )
      return FT_Err_Invalid_Face_Handle;

    if ( glyph_index >= (FT_UInt)face->num_glyphs )
      return FT_Err_Invalid_Argument;

    slot = face->glyph;
    ft_glyphslot_clear( slot );

    driver = face->driver;

    /* if the flag NO_RECURSE is set, we disable hinting and scaling */
    if ( load_flags & FT_LOAD_NO_RECURSE )
    {
      /* disable scaling, hinting, and transformation */
      load_flags |= FT_LOAD_NO_SCALE         |
                    FT_LOAD_NO_HINTING       |
                    FT_LOAD_IGNORE_TRANSFORM;

      /* disable bitmap rendering */
      load_flags &= ~FT_LOAD_RENDER;
    }

    /* do we need to load the glyph through the auto-hinter? */
    library  = driver->root.library;
    hinter   = library->auto_hinter;
    autohint =
      FT_BOOL( hinter                                                      &&
               !( load_flags & ( FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) ) &&
               FT_DRIVER_IS_SCALABLE( driver )                             &&
               FT_DRIVER_USES_OUTLINES( driver )                           );
    if ( autohint )
    {
      if ( FT_DRIVER_HAS_HINTER( driver ) &&
           !( load_flags & FT_LOAD_FORCE_AUTOHINT ) )
        autohint = 0;
    }

    if ( autohint )
    {
      FT_AutoHinter_Interface*  hinting;


      /* try to load embedded bitmaps first if available            */
      /*                                                            */
      /* XXX: This is really a temporary hack that should disappear */
      /*      promptly with FreeType 2.1!                           */
      /*                                                            */
      if ( FT_HAS_FIXED_SIZES( face ) )
      {
        error = driver->clazz->load_glyph( slot, face->size,
                                           glyph_index,
                                           load_flags | FT_LOAD_SBITS_ONLY );

        if ( !error && slot->format == ft_glyph_format_bitmap )
          goto Load_Ok;
      }

      /* load auto-hinted outline */
      hinting = (FT_AutoHinter_Interface*)hinter->clazz->module_interface;

      error   = hinting->load_glyph( (FT_AutoHinter)hinter,
                                     slot, face->size,
                                     glyph_index, load_flags );
    }
    else
    {
      error = driver->clazz->load_glyph( slot,
                                         face->size,
                                         glyph_index,
                                         load_flags );
      if ( error )
        goto Exit;

      /* check that the loaded outline is correct */
      error = FT_Outline_Check( &slot->outline );
      if ( error )
        goto Exit;
    }

  Load_Ok:
    /* compute the advance */
    if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
    {
      slot->advance.x = 0;
      slot->advance.y = slot->metrics.vertAdvance;
    }
    else
    {
      slot->advance.x = slot->metrics.horiAdvance;
      slot->advance.y = 0;
    }

    /* compute the linear advance in 16.16 pixels */
    if ( ( load_flags & FT_LOAD_LINEAR_DESIGN ) == 0 )
    {
      FT_UInt           EM      = face->units_per_EM;
      FT_Size_Metrics*  metrics = &face->size->metrics;

      slot->linearHoriAdvance = FT_MulDiv( slot->linearHoriAdvance,
                                           (FT_Long)metrics->x_ppem << 16, EM );

      slot->linearVertAdvance = FT_MulDiv( slot->linearVertAdvance,
                                           (FT_Long)metrics->y_ppem << 16, EM );
    }

    if ( ( load_flags & FT_LOAD_IGNORE_TRANSFORM ) == 0 )
    {
      FT_Face_Internal  internal = face->internal;


      /* now, transform the glyph image if needed */
      if ( internal->transform_flags )
      {
        /* get renderer */
        FT_Renderer  renderer = ft_lookup_glyph_renderer( slot );


        if ( renderer )
          error = renderer->clazz->transform_glyph(
                                     renderer, slot,
                                     &internal->transform_matrix,
                                     &internal->transform_delta );
        /* transform advance */
        FT_Vector_Transform( &slot->advance, &internal->transform_matrix );
      }
    }

    /* do we need to render the image now? */
    if ( !error                                    &&
         slot->format != ft_glyph_format_bitmap    &&
         slot->format != ft_glyph_format_composite &&
         load_flags & FT_LOAD_RENDER )
    {
      error = FT_Render_Glyph( slot,
                               ( load_flags & FT_LOAD_MONOCHROME )
                                  ? ft_render_mode_mono
                                  : ft_render_mode_normal );
    }

  Exit:
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Load_Char( FT_Face   face,
                FT_ULong  char_code,
                FT_Int    load_flags )
  {
    FT_UInt  glyph_index;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    glyph_index = (FT_UInt)char_code;
    if ( face->charmap )
      glyph_index = FT_Get_Char_Index( face, char_code );

    return FT_Load_Glyph( face, glyph_index, load_flags );
  }


  /* destructor for sizes list */
  static void
  destroy_size( FT_Memory  memory,
                FT_Size    size,
                FT_Driver  driver )
  {
    /* finalize client-specific data */
    if ( size->generic.finalizer )
      size->generic.finalizer( size );

    /* finalize format-specific stuff */
    if ( driver->clazz->done_size )
      driver->clazz->done_size( size );

    FREE( size->internal );
    FREE( size );
  }


  /* destructor for faces list */
  static void
  destroy_face( FT_Memory  memory,
                FT_Face    face,
                FT_Driver  driver )
  {
    FT_Driver_Class*  clazz = driver->clazz;


    /* discard auto-hinting data */
    if ( face->autohint.finalizer )
      face->autohint.finalizer( face->autohint.data );

    /* Discard glyph slots for this face                            */
    /* Beware!  FT_Done_GlyphSlot() changes the field `face->glyph' */
    while ( face->glyph )
      FT_Done_GlyphSlot( face->glyph );

    /* Discard all sizes for this face */
    FT_List_Finalize( &face->sizes_list,
                     (FT_List_Destructor)destroy_size,
                     memory,
                     driver );
    face->size = 0;

    /* Now discard client data */
    if ( face->generic.finalizer )
      face->generic.finalizer( face );

    /* finalize format-specific stuff */
    if ( clazz->done_face )
      clazz->done_face( face );

    /* close the stream for this face if needed */
    ft_done_stream(
      &face->stream,
      ( face->face_flags & FT_FACE_FLAG_EXTERNAL_STREAM ) != 0 );

    /* get rid of it */
    if ( face->internal )
    {
      FREE( face->internal->postscript_name );
      FREE( face->internal );
    }
    FREE( face );
  }


  static void
  Destroy_Driver( FT_Driver  driver )
  {
    FT_List_Finalize( &driver->faces_list,
                      (FT_List_Destructor)destroy_face,
                      driver->root.memory,
                      driver );

    /* check whether we need to drop the driver's glyph loader */
    if ( FT_DRIVER_USES_OUTLINES( driver ) )
      FT_GlyphLoader_Done( driver->glyph_loader );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    open_face                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function does some work for FT_Open_Face().                   */
  /*                                                                       */
  static FT_Error
  open_face( FT_Driver      driver,
             FT_Stream      stream,
             FT_Long        face_index,
             FT_Int         num_params,
             FT_Parameter*  params,
             FT_Face*       aface )
  {
    FT_Memory         memory;
    FT_Driver_Class*  clazz;
    FT_Face           face = 0;
    FT_Error          error;
    FT_Face_Internal  internal;


    clazz  = driver->clazz;
    memory = driver->root.memory;

    /* allocate the face object and perform basic initialization */
    if ( ALLOC( face, clazz->face_object_size ) )
      goto Fail;

    if ( ALLOC( internal, sizeof ( *internal ) ) )
      goto Fail;

    face->internal = internal;

    face->driver   = driver;
    face->memory   = memory;
    face->stream   = stream;

    error = clazz->init_face( stream,
                              face,
                              face_index,
                              num_params,
                              params );
    if ( error )
      goto Fail;

    *aface = face;

  Fail:
    if ( error )
    {
      clazz->done_face( face );
      FREE( face->internal );
      FREE( face );
      *aface = 0;
    }

    return error;
  }


  /* there's a Mac-specific extended implementation of FT_New_Face() */
  /* in src/mac/ftmac.c                                              */

#ifndef macintosh

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  pathname,
               FT_Long      face_index,
               FT_Face     *aface )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `aface' delayed to FT_Open_Face() */
    if ( !pathname )
      return FT_Err_Invalid_Argument;

    args.flags    = ft_open_pathname;
    args.pathname = (char*)pathname;

    return FT_Open_Face( library, &args, face_index, aface );
  }

#endif  /* !macintosh */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Memory_Face( FT_Library      library,
                      const FT_Byte*  file_base,
                      FT_Long         file_size,
                      FT_Long         face_index,
                      FT_Face        *aface )
  {
    FT_Open_Args  args;


    /* test for valid `library' and `face' delayed to FT_Open_Face() */
    if ( !file_base )
      return FT_Err_Invalid_Argument;

    args.flags       = ft_open_memory;
    args.memory_base = file_base;
    args.memory_size = file_size;

    return FT_Open_Face( library, &args, face_index, aface );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Open_Face( FT_Library     library,
                FT_Open_Args*  args,
                FT_Long        face_index,
                FT_Face       *aface )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_Stream    stream;
    FT_Face      face = 0;
    FT_ListNode  node = 0;
    FT_Bool      external_stream;


    /* test for valid `library' delayed to */
    /* ft_new_input_stream()               */

    if ( !aface || !args )
      return FT_Err_Invalid_Argument;

    *aface = 0;

    external_stream = FT_BOOL( ( args->flags & ft_open_stream ) &&
                               args->stream                     );

    /* create input stream */
    error = ft_new_input_stream( library, args, &stream );
    if ( error )
      goto Exit;

    memory = library->memory;

    /* If the font driver is specified in the `args' structure, use */
    /* it.  Otherwise, we scan the list of registered drivers.      */
    if ( ( args->flags & ft_open_driver ) && args->driver )
    {
      driver = FT_DRIVER( args->driver );

      /* not all modules are drivers, so check... */
      if ( FT_MODULE_IS_DRIVER( driver ) )
      {
        FT_Int         num_params = 0;
        FT_Parameter*  params     = 0;


        if ( args->flags & ft_open_params )
        {
          num_params = args->num_params;
          params     = args->params;
        }

        error = open_face( driver, stream, face_index,
                           num_params, params, &face );
        if ( !error )
          goto Success;
      }
      else
        error = FT_Err_Invalid_Handle;

      ft_done_stream( &stream, external_stream );
      goto Fail;
    }
    else
    {
      /* check each font driver for an appropriate format */
      FT_Module*  cur   = library->modules;
      FT_Module*  limit = cur + library->num_modules;


      for ( ; cur < limit; cur++ )
      {
        /* not all modules are font drivers, so check... */
        if ( FT_MODULE_IS_DRIVER( cur[0] ) )
        {
          FT_Int         num_params = 0;
          FT_Parameter*  params     = 0;


          driver = FT_DRIVER( cur[0] );

          if ( args->flags & ft_open_params )
          {
            num_params = args->num_params;
            params     = args->params;
          }

          error = open_face( driver, stream, face_index,
                             num_params, params, &face );
          if ( !error )
            goto Success;

          if ( FT_ERROR_BASE( error ) != FT_Err_Unknown_File_Format )
            goto Fail2;
        }
      }

      /* no driver is able to handle this format */
      error = FT_Err_Unknown_File_Format;

  Fail2:
      ft_done_stream( &stream, external_stream );
      goto Fail;
    }

  Success:
    FT_TRACE4(( "FT_Open_Face: New face object, adding to list\n" ));

    /* set the FT_FACE_FLAG_EXTERNAL_STREAM bit for FT_Done_Face */
    if ( external_stream )
      face->face_flags |= FT_FACE_FLAG_EXTERNAL_STREAM;

    /* add the face object to its driver's list */
    if ( ALLOC( node, sizeof ( *node ) ) )
      goto Fail;

    node->data = face;
    /* don't assume driver is the same as face->driver, so use */
    /* face->driver instead.                                   */
    FT_List_Add( &face->driver->faces_list, node );

    /* now allocate a glyph slot object for the face */
    {
      FT_GlyphSlot  slot;


      FT_TRACE4(( "FT_Open_Face: Creating glyph slot\n" ));

      error = FT_New_GlyphSlot( face, &slot );
      if ( error )
        goto Fail;

      face->glyph = slot;
    }

    /* finally, allocate a size object for the face */
    {
      FT_Size  size;


      FT_TRACE4(( "FT_Open_Face: Creating size object\n" ));

      error = FT_New_Size( face, &size );
      if ( error )
        goto Fail;

      face->size = size;
    }

    /* initialize internal face data */
    {
      FT_Face_Internal  internal = face->internal;


      internal->transform_matrix.xx = 0x10000L;
      internal->transform_matrix.xy = 0;
      internal->transform_matrix.yx = 0;
      internal->transform_matrix.yy = 0x10000L;

      internal->transform_delta.x = 0;
      internal->transform_delta.y = 0;
    }

    *aface = face;
    goto Exit;

  Fail:
    FT_Done_Face( face );

  Exit:
    FT_TRACE4(( "FT_Open_Face: Return %d\n", error ));

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Attach_File( FT_Face      face,
                  const char*  filepathname )
  {
    FT_Open_Args  open;


    /* test for valid `face' delayed to FT_Attach_Stream() */

    if ( !filepathname )
      return FT_Err_Invalid_Argument;

    open.flags    = ft_open_pathname;
    open.pathname = (char*)filepathname;

    return FT_Attach_Stream( face, &open );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Attach_Stream( FT_Face        face,
                    FT_Open_Args*  parameters )
  {
    FT_Stream  stream;
    FT_Error   error;
    FT_Driver  driver;

    FT_Driver_Class*  clazz;


    /* test for valid `parameters' delayed to ft_new_input_stream() */

    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    error = ft_new_input_stream( driver->root.library, parameters, &stream );
    if ( error )
      goto Exit;

    /* we implement FT_Attach_Stream in each driver through the */
    /* `attach_file' interface                                  */

    error = FT_Err_Unimplemented_Feature;
    clazz = driver->clazz;
    if ( clazz->attach_file )
      error = clazz->attach_file( face, stream );

    /* close the attached stream */
    ft_done_stream( &stream,
                    (FT_Bool)( parameters->stream &&
                               ( parameters->flags & ft_open_stream ) ) );

  Exit:
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Face( FT_Face  face )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_ListNode  node;


    error = FT_Err_Invalid_Face_Handle;
    if ( face && face->driver )
    {
      driver = face->driver;
      memory = driver->root.memory;

      /* find face in driver's list */
      node = FT_List_Find( &driver->faces_list, face );
      if ( node )
      {
        /* remove face object from the driver's list */
        FT_List_Remove( &driver->faces_list, node );
        FREE( node );

        /* now destroy the object proper */
        destroy_face( memory, face, driver );
        error = FT_Err_Ok;
      }
    }
    return error;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Size( FT_Face   face,
               FT_Size  *asize )
  {
    FT_Error          error;
    FT_Memory         memory;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;

    FT_Size           size = 0;
    FT_ListNode       node = 0;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    if ( !asize )
      return FT_Err_Invalid_Size_Handle;

    if ( !face->driver )
      return FT_Err_Invalid_Driver_Handle;

    *asize = 0;

    driver = face->driver;
    clazz  = driver->clazz;
    memory = face->memory;

    /* Allocate new size object and perform basic initialisation */
    if ( ALLOC( size, clazz->size_object_size ) ||
         ALLOC( node, sizeof ( FT_ListNodeRec ) ) )
      goto Exit;

    size->face = face;

    /* for now, do not use any internal fields in size objects */
    size->internal = 0;

    if ( clazz->init_size )
      error = clazz->init_size( size );

    /* in case of success, add to the face's list */
    if ( !error )
    {
      *asize     = size;
      node->data = size;
      FT_List_Add( &face->sizes_list, node );
    }

  Exit:
    if ( error )
    {
      FREE( node );
      FREE( size );
    }

    return error;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Size( FT_Size  size )
  {
    FT_Error     error;
    FT_Driver    driver;
    FT_Memory    memory;
    FT_Face      face;
    FT_ListNode  node;


    if ( !size )
      return FT_Err_Invalid_Size_Handle;

    face = size->face;
    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    memory = driver->root.memory;

    error = FT_Err_Ok;
    node  = FT_List_Find( &face->sizes_list, size );
    if ( node )
    {
      FT_List_Remove( &face->sizes_list, node );
      FREE( node );

      if ( face->size == size )
      {
        face->size = 0;
        if ( face->sizes_list.head )
          face->size = (FT_Size)(face->sizes_list.head->data);
      }

      destroy_size( memory, size, driver );
    }
    else
      error = FT_Err_Invalid_Size_Handle;

    return error;
  }


  static void
  ft_recompute_scaled_metrics( FT_Face           face,
                               FT_Size_Metrics*  metrics )
  {
    /* Compute root ascender, descender, test height, and max_advance */

    metrics->ascender    = ( FT_MulFix( face->ascender,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->descender   = ( FT_MulFix( face->descender,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->height      = ( FT_MulFix( face->height,
                                        metrics->y_scale ) + 32 ) & -64;

    metrics->max_advance = ( FT_MulFix( face->max_advance_width,
                                        metrics->x_scale ) + 32 ) & -64;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Char_Size( FT_Face     face,
                    FT_F26Dot6  char_width,
                    FT_F26Dot6  char_height,
                    FT_UInt     horz_resolution,
                    FT_UInt     vert_resolution )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;
    FT_Size_Metrics*  metrics;
    FT_Long           dim_x, dim_y;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver  = face->driver;
    metrics = &face->size->metrics;

    if ( !char_width )
      char_width = char_height;

    else if ( !char_height )
      char_height = char_width;

    if ( !horz_resolution )
      horz_resolution = 72;

    if ( !vert_resolution )
      vert_resolution = 72;

    driver = face->driver;
    clazz  = driver->clazz;

    /* default processing -- this can be overridden by the driver */
    if ( char_width  < 1 * 64 )
      char_width  = 1 * 64;
    if ( char_height < 1 * 64 )
      char_height = 1 * 64;

    /* Compute pixel sizes in 26.6 units */
    dim_x = ( ( ( char_width  * horz_resolution ) / 72 ) + 32 ) & -64;
    dim_y = ( ( ( char_height * vert_resolution ) / 72 ) + 32 ) & -64;

    metrics->x_ppem  = (FT_UShort)( dim_x >> 6 );
    metrics->y_ppem  = (FT_UShort)( dim_y >> 6 );

    metrics->x_scale = 0x10000L;
    metrics->y_scale = 0x10000L;

    if ( face->face_flags & FT_FACE_FLAG_SCALABLE )
    {
      metrics->x_scale = FT_DivFix( dim_x, face->units_per_EM );
      metrics->y_scale = FT_DivFix( dim_y, face->units_per_EM );

      ft_recompute_scaled_metrics( face, metrics );
    }

    if ( clazz->set_char_sizes )
      error = clazz->set_char_sizes( face->size,
                                     char_width,
                                     char_height,
                                     horz_resolution,
                                     vert_resolution );
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Pixel_Sizes( FT_Face  face,
                      FT_UInt  pixel_width,
                      FT_UInt  pixel_height )
  {
    FT_Error          error = FT_Err_Ok;
    FT_Driver         driver;
    FT_Driver_Class*  clazz;
    FT_Size_Metrics*  metrics = &face->size->metrics;


    if ( !face || !face->size || !face->driver )
      return FT_Err_Invalid_Face_Handle;

    driver = face->driver;
    clazz  = driver->clazz;

    /* default processing -- this can be overridden by the driver */
    if ( pixel_width == 0 )
      pixel_width = pixel_height;

    else if ( pixel_height == 0 )
      pixel_height = pixel_width;

    if ( pixel_width  < 1 )
      pixel_width  = 1;
    if ( pixel_height < 1 )
      pixel_height = 1;

    metrics->x_ppem = (FT_UShort)pixel_width;
    metrics->y_ppem = (FT_UShort)pixel_height;

    if ( face->face_flags & FT_FACE_FLAG_SCALABLE )
    {
      metrics->x_scale = FT_DivFix( metrics->x_ppem << 6,
                                    face->units_per_EM );

      metrics->y_scale = FT_DivFix( metrics->y_ppem << 6,
                                    face->units_per_EM );

      ft_recompute_scaled_metrics( face, metrics );
    }

    if ( clazz->set_pixel_sizes )
      error = clazz->set_pixel_sizes( face->size,
                                      pixel_width,
                                      pixel_height );
    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Kerning( FT_Face     face,
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_UInt     kern_mode,
                  FT_Vector  *akerning )
  {
    FT_Error   error = FT_Err_Ok;
    FT_Driver  driver;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    if ( !akerning )
      return FT_Err_Invalid_Argument;

    driver = face->driver;

    akerning->x = 0;
    akerning->y = 0;

    if ( driver->clazz->get_kerning )
    {
      error = driver->clazz->get_kerning( face,
                                          left_glyph,
                                          right_glyph,
                                          akerning );
      if ( !error )
      {
        if ( kern_mode != ft_kerning_unscaled )
        {
          akerning->x = FT_MulFix( akerning->x, face->size->metrics.x_scale );
          akerning->y = FT_MulFix( akerning->y, face->size->metrics.y_scale );

          if ( kern_mode != ft_kerning_unfitted )
          {
            akerning->x = ( akerning->x + 32 ) & -64;
            akerning->y = ( akerning->y + 32 ) & -64;
          }
        }
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Select_Charmap( FT_Face      face,
                     FT_Encoding  encoding )
  {
    FT_CharMap*  cur;
    FT_CharMap*  limit;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    cur = face->charmaps;
    if ( !cur )
      return FT_Err_Invalid_CharMap_Handle;

    limit = cur + face->num_charmaps;

    for ( ; cur < limit; cur++ )
    {
      if ( cur[0]->encoding == encoding )
      {
        face->charmap = cur[0];
        return 0;
      }
    }

    return FT_Err_Invalid_Argument;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Charmap( FT_Face     face,
                  FT_CharMap  charmap )
  {
    FT_CharMap*  cur;
    FT_CharMap*  limit;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    cur = face->charmaps;
    if ( !cur )
      return FT_Err_Invalid_CharMap_Handle;

    limit = cur + face->num_charmaps;

    for ( ; cur < limit; cur++ )
    {
      if ( cur[0] == charmap )
      {
        face->charmap = cur[0];
        return 0;
      }
    }
    return FT_Err_Invalid_Argument;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Char_Index( FT_Face   face,
                     FT_ULong  charcode )
  {
    FT_UInt    result;
    FT_Driver  driver;


    result = 0;
    if ( face && face->charmap )
    {
      driver = face->driver;
      result = driver->clazz->get_char_index( face->charmap, charcode );
    }
    return result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_UInt )
  FT_Get_Name_Index( FT_Face     face,
                     FT_String*  glyph_name )
  {
    FT_UInt  result = 0;


    if ( face && FT_HAS_GLYPH_NAMES( face ) )
    {
      /* now, lookup for glyph name */
      FT_Driver         driver = face->driver;
      FT_Module_Class*  clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Name_Index_Requester  requester;


        requester = (FT_Name_Index_Requester)clazz->get_interface(
                      FT_MODULE( driver ), "name_index" );
        if ( requester )
          result = requester( face, glyph_name );
      }
    }

    return result;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Glyph_Name( FT_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max )
  {
    FT_Error  error = FT_Err_Invalid_Argument;


    /* clean up buffer */
    if ( buffer && buffer_max > 0 )
      ((FT_Byte*)buffer)[0] = 0;

    if ( face                                    &&
         glyph_index < (FT_UInt)face->num_glyphs &&
         FT_HAS_GLYPH_NAMES( face )              )
    {
      /* now, lookup for glyph name */
      FT_Driver        driver = face->driver;
      FT_Module_Class* clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_Glyph_Name_Requester  requester;


        requester = (FT_Glyph_Name_Requester)clazz->get_interface(
                      FT_MODULE( driver ), "glyph_name" );
        if ( requester )
          error = requester( face, glyph_index, buffer, buffer_max );
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( const char* )
  FT_Get_Postscript_Name( FT_Face  face )
  {
    const char*  result = NULL;


    if ( !face )
      goto Exit;

    result = face->internal->postscript_name;
    if ( !result )
    {
      /* now, lookup for glyph name */
      FT_Driver         driver = face->driver;
      FT_Module_Class*  clazz  = FT_MODULE_CLASS( driver );


      if ( clazz->get_interface )
      {
        FT_PSName_Requester  requester;


        requester = (FT_PSName_Requester)clazz->get_interface(
                      FT_MODULE( driver ), "postscript_name" );
        if ( requester )
          result = requester( face );
      }
    }
  Exit:
    return result;
  }


  /* documentation is in tttables.h */

  FT_EXPORT_DEF( void* )
  FT_Get_Sfnt_Table( FT_Face      face,
                     FT_Sfnt_Tag  tag )
  {
    void*                   table = 0;
    FT_Get_Sfnt_Table_Func  func;
    FT_Driver               driver;


    if ( !face || !FT_IS_SFNT( face ) )
      goto Exit;

    driver = face->driver;
    func = (FT_Get_Sfnt_Table_Func)driver->root.clazz->get_interface(
                                     FT_MODULE( driver ), "get_sfnt" );
    if ( func )
      table = func( face, tag );

  Exit:
    return table;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Activate_Size( FT_Size  size )
  {
    FT_Face  face;


    if ( size == NULL )
      return FT_Err_Bad_Argument;

    face = size->face;
    if ( face == NULL || face->driver == NULL )
      return FT_Err_Bad_Argument;

    /* we don't need anything more complex than that; all size objects */
    /* are already listed by the face                                  */
    face->size = size;

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                        R E N D E R E R S                        ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /* lookup a renderer by glyph format in the library's list */
  FT_BASE_DEF( FT_Renderer )
  FT_Lookup_Renderer( FT_Library       library,
                      FT_Glyph_Format  format,
                      FT_ListNode*     node )
  {
    FT_ListNode   cur;
    FT_Renderer   result = 0;


    if ( !library )
      goto Exit;

    cur = library->renderers.head;

    if ( node )
    {
      if ( *node )
        cur = (*node)->next;
      *node = 0;
    }

    while ( cur )
    {
      FT_Renderer  renderer = FT_RENDERER( cur->data );


      if ( renderer->glyph_format == format )
      {
        if ( node )
          *node = cur;

        result = renderer;
        break;
      }
      cur = cur->next;
    }

  Exit:
    return result;
  }


  static FT_Renderer
  ft_lookup_glyph_renderer( FT_GlyphSlot  slot )
  {
    FT_Face      face    = slot->face;
    FT_Library   library = FT_FACE_LIBRARY( face );
    FT_Renderer  result  = library->cur_renderer;


    if ( !result || result->glyph_format != slot->format )
      result = FT_Lookup_Renderer( library, slot->format, 0 );

    return result;
  }


  static void
  ft_set_current_renderer( FT_Library  library )
  {
    FT_Renderer  renderer;


    renderer = FT_Lookup_Renderer( library, ft_glyph_format_outline, 0 );
    library->cur_renderer = renderer;
  }


  static FT_Error
  ft_add_renderer( FT_Module  module )
  {
    FT_Library   library = module->library;
    FT_Memory    memory  = library->memory;
    FT_Error     error;
    FT_ListNode  node;


    if ( ALLOC( node, sizeof ( *node ) ) )
      goto Exit;

    {
      FT_Renderer         render = FT_RENDERER( module );
      FT_Renderer_Class*  clazz  = (FT_Renderer_Class*)module->clazz;


      render->clazz        = clazz;
      render->glyph_format = clazz->glyph_format;

      /* allocate raster object if needed */
      if ( clazz->glyph_format == ft_glyph_format_outline &&
           clazz->raster_class->raster_new )
      {
        error = clazz->raster_class->raster_new( memory, &render->raster );
        if ( error )
          goto Fail;

        render->raster_render = clazz->raster_class->raster_render;
        render->render        = clazz->render_glyph;
      }

      /* add to list */
      node->data = module;
      FT_List_Add( &library->renderers, node );

      ft_set_current_renderer( library );
    }

  Fail:
    if ( error )
      FREE( node );

  Exit:
    return error;
  }


  static void
  ft_remove_renderer( FT_Module  module )
  {
    FT_Library   library = module->library;
    FT_Memory    memory  = library->memory;
    FT_ListNode  node;


    node = FT_List_Find( &library->renderers, module );
    if ( node )
    {
      FT_Renderer  render = FT_RENDERER( module );


      /* release raster object, if any */
      if ( render->raster )
        render->clazz->raster_class->raster_done( render->raster );

      /* remove from list */
      FT_List_Remove( &library->renderers, node );
      FREE( node );

      ft_set_current_renderer( library );
    }
  }


  /* documentation is in ftrender.h */

  FT_EXPORT_DEF( FT_Renderer )
  FT_Get_Renderer( FT_Library       library,
                   FT_Glyph_Format  format )
  {
    /* test for valid `library' delayed to FT_Lookup_Renderer() */

    return  FT_Lookup_Renderer( library, format, 0 );
  }


  /* documentation is in ftrender.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_Renderer( FT_Library     library,
                   FT_Renderer    renderer,
                   FT_UInt        num_params,
                   FT_Parameter*  parameters )
  {
    FT_ListNode  node;
    FT_Error     error = FT_Err_Ok;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !renderer )
      return FT_Err_Invalid_Argument;

    node = FT_List_Find( &library->renderers, renderer );
    if ( !node )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    FT_List_Up( &library->renderers, node );

    if ( renderer->glyph_format == ft_glyph_format_outline )
      library->cur_renderer = renderer;

    if ( num_params > 0 )
    {
      FTRenderer_setMode  set_mode = renderer->clazz->set_mode;


      for ( ; num_params > 0; num_params-- )
      {
        error = set_mode( renderer, parameters->tag, parameters->data );
        if ( error )
          break;
      }
    }

  Exit:
    return error;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Render_Glyph_Internal( FT_Library    library,
                            FT_GlyphSlot  slot,
                            FT_UInt       render_mode )
  {
    FT_Error     error = FT_Err_Ok;
    FT_Renderer  renderer;


    /* if it is already a bitmap, no need to do anything */
    switch ( slot->format )
    {
    case ft_glyph_format_bitmap:   /* already a bitmap, don't do anything */
      break;

    default:
      {
        FT_ListNode  node   = 0;
        FT_Bool      update = 0;


        /* small shortcut for the very common case */
        if ( slot->format == ft_glyph_format_outline )
        {
          renderer = library->cur_renderer;
          node     = library->renderers.head;
        }
        else
          renderer = FT_Lookup_Renderer( library, slot->format, &node );

        error = FT_Err_Unimplemented_Feature;
        while ( renderer )
        {
          error = renderer->render( renderer, slot, render_mode, NULL );
          if ( !error ||
               FT_ERROR_BASE( error ) != FT_Err_Cannot_Render_Glyph )
            break;

          /* FT_Err_Cannot_Render_Glyph is returned if the render mode   */
          /* is unsupported by the current renderer for this glyph image */
          /* format.                                                     */

          /* now, look for another renderer that supports the same */
          /* format.                                               */
          renderer = FT_Lookup_Renderer( library, slot->format, &node );
          update   = 1;
        }

        /* if we changed the current renderer for the glyph image format */
        /* we need to select it as the next current one                  */
        if ( !error && update && renderer )
          FT_Set_Renderer( library, renderer, 0, 0 );
      }
    }

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Render_Glyph( FT_GlyphSlot  slot,
                   FT_UInt       render_mode )
  {
    FT_Library   library;


    if ( !slot )
      return FT_Err_Invalid_Argument;

    library = FT_FACE_LIBRARY( slot->face );

    return FT_Render_Glyph_Internal( library, slot, render_mode );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         M O D U L E S                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Destroy_Module                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given module object.  For drivers, this also destroys   */
  /*    all child faces.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*     module :: A handle to the target driver object.                   */
  /*                                                                       */
  /* <Note>                                                                */
  /*     The driver _must_ be LOCKED!                                      */
  /*                                                                       */
  static void
  Destroy_Module( FT_Module  module )
  {
    FT_Memory         memory  = module->memory;
    FT_Module_Class*  clazz   = module->clazz;
    FT_Library        library = module->library;


    /* finalize client-data - before anything else */
    if ( module->generic.finalizer )
      module->generic.finalizer( module );

    if ( library && library->auto_hinter == module )
      library->auto_hinter = 0;

    /* if the module is a renderer */
    if ( FT_MODULE_IS_RENDERER( module ) )
      ft_remove_renderer( module );

    /* if the module is a font driver, add some steps */
    if ( FT_MODULE_IS_DRIVER( module ) )
      Destroy_Driver( FT_DRIVER( module ) );

    /* finalize the module object */
    if ( clazz->module_done )
      clazz->module_done( module );

    /* discard it */
    FREE( module );
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Add_Module( FT_Library              library,
                 const FT_Module_Class*  clazz )
  {
    FT_Error   error;
    FT_Memory  memory;
    FT_Module  module;
    FT_UInt    nn;


#define FREETYPE_VER_FIXED  ( ( (FT_Long)FREETYPE_MAJOR << 16 ) | \
                                FREETYPE_MINOR                  )

    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( !clazz )
      return FT_Err_Invalid_Argument;

    /* check freetype version */
    if ( clazz->module_requires > FREETYPE_VER_FIXED )
      return FT_Err_Invalid_Version;

    /* look for a module with the same name in the library's table */
    for ( nn = 0; nn < library->num_modules; nn++ )
    {
      module = library->modules[nn];
      if ( strcmp( module->clazz->module_name, clazz->module_name ) == 0 )
      {
        /* this installed module has the same name, compare their versions */
        if ( clazz->module_version <= module->clazz->module_version )
          return FT_Err_Lower_Module_Version;

        /* remove the module from our list, then exit the loop to replace */
        /* it by our new version..                                        */
        FT_Remove_Module( library, module );
        break;
      }
    }

    memory = library->memory;
    error  = FT_Err_Ok;

    if ( library->num_modules >= FT_MAX_MODULES )
    {
      error = FT_Err_Too_Many_Drivers;
      goto Exit;
    }

    /* allocate module object */
    if ( ALLOC( module,clazz->module_size ) )
      goto Exit;

    /* base initialization */
    module->library = library;
    module->memory  = memory;
    module->clazz   = (FT_Module_Class*)clazz;

    /* check whether the module is a renderer - this must be performed */
    /* before the normal module initialization                         */
    if ( FT_MODULE_IS_RENDERER( module ) )
    {
      /* add to the renderers list */
      error = ft_add_renderer( module );
      if ( error )
        goto Fail;
    }

    /* is the module a auto-hinter? */
    if ( FT_MODULE_IS_HINTER( module ) )
      library->auto_hinter = module;

    /* if the module is a font driver */
    if ( FT_MODULE_IS_DRIVER( module ) )
    {
      /* allocate glyph loader if needed */
      FT_Driver   driver = FT_DRIVER( module );


      driver->clazz = (FT_Driver_Class*)module->clazz;
      if ( FT_DRIVER_USES_OUTLINES( driver ) )
      {
        error = FT_GlyphLoader_New( memory, &driver->glyph_loader );
        if ( error )
          goto Fail;
      }
    }

    if ( clazz->module_init )
    {
      error = clazz->module_init( module );
      if ( error )
        goto Fail;
    }

    /* add module to the library's table */
    library->modules[library->num_modules++] = module;

  Exit:
    return error;

  Fail:
    if ( FT_MODULE_IS_DRIVER( module ) )
    {
      FT_Driver  driver = FT_DRIVER( module );


      if ( FT_DRIVER_USES_OUTLINES( driver ) )
        FT_GlyphLoader_Done( driver->glyph_loader );
    }

    if ( FT_MODULE_IS_RENDERER( module ) )
    {
      FT_Renderer  renderer = FT_RENDERER( module );


      if ( renderer->raster )
        renderer->clazz->raster_class->raster_done( renderer->raster );
    }

    FREE( module );
    goto Exit;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Module )
  FT_Get_Module( FT_Library   library,
                 const char*  module_name )
  {
    FT_Module   result = 0;
    FT_Module*  cur;
    FT_Module*  limit;


    if ( !library || !module_name )
      return result;

    cur   = library->modules;
    limit = cur + library->num_modules;

    for ( ; cur < limit; cur++ )
      if ( strcmp( cur[0]->clazz->module_name, module_name ) == 0 )
      {
        result = cur[0];
        break;
      }

    return result;
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( const void* )
  FT_Get_Module_Interface( FT_Library   library,
                           const char*  mod_name )
  {
    FT_Module  module;


    /* test for valid `library' delayed to FT_Get_Module() */

    module = FT_Get_Module( library, mod_name );

    return module ? module->clazz->module_interface : 0;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Remove_Module( FT_Library  library,
                    FT_Module   module )
  {
    /* try to find the module from the table, then remove it from there */

    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    if ( module )
    {
      FT_Module*  cur   = library->modules;
      FT_Module*  limit = cur + library->num_modules;


      for ( ; cur < limit; cur++ )
      {
        if ( cur[0] == module )
        {
          /* remove it from the table */
          library->num_modules--;
          limit--;
          while ( cur < limit )
          {
            cur[0] = cur[1];
            cur++;
          }
          limit[0] = 0;

          /* destroy the module */
          Destroy_Module( module );

          return FT_Err_Ok;
        }
      }
    }
    return FT_Err_Invalid_Driver_Handle;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         L I B R A R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Library( FT_Memory    memory,
                  FT_Library  *alibrary )
  {
    FT_Library  library = 0;
    FT_Error    error;


    if ( !memory )
      return FT_Err_Invalid_Argument;

    /* first of all, allocate the library object */
    if ( ALLOC( library, sizeof ( *library ) ) )
      return error;

    library->memory = memory;

    /* allocate the render pool */
    library->raster_pool_size = FT_RENDER_POOL_SIZE;
    if ( ALLOC( library->raster_pool, FT_RENDER_POOL_SIZE ) )
      goto Fail;

    /* That's ok now */
    *alibrary = library;

    return FT_Err_Ok;

  Fail:
    FREE( library );
    return error;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_Library( FT_Library  library )
  {
    FT_Memory  memory;


    if ( !library )
      return FT_Err_Invalid_Library_Handle;

    memory = library->memory;

    /* Discard client-data */
    if ( library->generic.finalizer )
      library->generic.finalizer( library );

    /* Close all modules in the library */
#if 1
    while ( library->num_modules > 0 )
      FT_Remove_Module( library, library->modules[0] );
#else
    {
      FT_UInt  n;


      for ( n = 0; n < library->num_modules; n++ )
      {
        FT_Module  module = library->modules[n];


        if ( module )
        {
          Destroy_Module( module );
          library->modules[n] = 0;
        }
      }
    }
#endif

    /* Destroy raster objects */
    FREE( library->raster_pool );
    library->raster_pool_size = 0;

    FREE( library );
    return FT_Err_Ok;
  }


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( void )
  FT_Set_Debug_Hook( FT_Library         library,
                     FT_UInt            hook_index,
                     FT_DebugHook_Func  debug_hook )
  {
    if ( library && debug_hook &&
         hook_index <
           ( sizeof ( library->debug_hooks ) / sizeof ( void* ) ) )
      library->debug_hooks[hook_index] = debug_hook;
  }


/* END */
