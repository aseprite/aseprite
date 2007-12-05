/***************************************************************************/
/*                                                                         */
/*  ttinterp.c                                                             */
/*                                                                         */
/*    TrueType bytecode interpreter (body).                                */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_TRIGONOMETRY_H
#include FT_SYSTEM_H

#include "ttinterp.h"

#include "tterrors.h"


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER


#define TT_MULFIX  FT_MulFix
#define TT_MULDIV  FT_MulDiv
#define TT_INT64   FT_Int64

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttinterp

#undef  NO_APPLE_PATENT
#define APPLE_THRESHOLD  0x4000000L

  /*************************************************************************/
  /*                                                                       */
  /* In order to detect infinite loops in the code, we set up a counter    */
  /* within the run loop.  A single stroke of interpretation is now        */
  /* limitet to a maximal number of opcodes defined below.                 */
  /*                                                                       */
#define MAX_RUNNABLE_OPCODES  1000000L


  /*************************************************************************/
  /*                                                                       */
  /* There are two kinds of implementations:                               */
  /*                                                                       */
  /* a. static implementation                                              */
  /*                                                                       */
  /*    The current execution context is a static variable, which fields   */
  /*    are accessed directly by the interpreter during execution.  The    */
  /*    context is named `cur'.                                            */
  /*                                                                       */
  /*    This version is non-reentrant, of course.                          */
  /*                                                                       */
  /* b. indirect implementation                                            */
  /*                                                                       */
  /*    The current execution context is passed to _each_ function as its  */
  /*    first argument, and each field is thus accessed indirectly.        */
  /*                                                                       */
  /*    This version is fully re-entrant.                                  */
  /*                                                                       */
  /* The idea is that an indirect implementation may be slower to execute  */
  /* on low-end processors that are used in some systems (like 386s or     */
  /* even 486s).                                                           */
  /*                                                                       */
  /* As a consequence, the indirect implementation is now the default, as  */
  /* its performance costs can be considered negligible in our context.    */
  /* Note, however, that we kept the same source with macros because:      */
  /*                                                                       */
  /* - The code is kept very close in design to the Pascal code used for   */
  /*   development.                                                        */
  /*                                                                       */
  /* - It's much more readable that way!                                   */
  /*                                                                       */
  /* - It's still open to experimentation and tuning.                      */
  /*                                                                       */
  /*************************************************************************/


#ifndef TT_CONFIG_OPTION_STATIC_INTERPRETER     /* indirect implementation */

#define CUR  (*exc)                             /* see ttobjs.h */

#else                                           /* static implementation */

#define CUR  cur

  static
  TT_ExecContextRec  cur;   /* static exec. context variable */

  /* apparently, we have a _lot_ of direct indexing when accessing  */
  /* the static `cur', which makes the code bigger (due to all the  */
  /* four bytes addresses).                                         */

#endif /* TT_CONFIG_OPTION_STATIC_INTERPRETER */


  /*************************************************************************/
  /*                                                                       */
  /* The instruction argument stack.                                       */
  /*                                                                       */
#define INS_ARG  EXEC_OP_ FT_Long*  args    /* see ttobjs.h for EXEC_OP_ */


  /*************************************************************************/
  /*                                                                       */
  /* This macro is used whenever `exec' is unused in a function, to avoid  */
  /* stupid warnings from pedantic compilers.                              */
  /*                                                                       */
#define FT_UNUSED_EXEC  FT_UNUSED( CUR )


  /*************************************************************************/
  /*                                                                       */
  /* This macro is used whenever `args' is unused in a function, to avoid  */
  /* stupid warnings from pedantic compilers.                              */
  /*                                                                       */
#define FT_UNUSED_ARG  FT_UNUSED_EXEC; FT_UNUSED( args )


  /*************************************************************************/
  /*                                                                       */
  /* The following macros hide the use of EXEC_ARG and EXEC_ARG_ to        */
  /* increase readabilty of the code.                                      */
  /*                                                                       */
  /*************************************************************************/


#define SKIP_Code() \
          SkipCode( EXEC_ARG )

#define GET_ShortIns() \
          GetShortIns( EXEC_ARG )

#define NORMalize( x, y, v ) \
          Normalize( EXEC_ARG_ x, y, v )

#define SET_SuperRound( scale, flags ) \
          SetSuperRound( EXEC_ARG_ scale, flags )

#define ROUND_None( d, c ) \
          Round_None( EXEC_ARG_ d, c )

#define INS_Goto_CodeRange( range, ip ) \
          Ins_Goto_CodeRange( EXEC_ARG_ range, ip )

#define CUR_Func_project( x, y ) \
          CUR.func_project( EXEC_ARG_ x, y )

#define CUR_Func_move( z, p, d ) \
          CUR.func_move( EXEC_ARG_ z, p, d )

#define CUR_Func_dualproj( x, y ) \
          CUR.func_dualproj( EXEC_ARG_ x, y )

#define CUR_Func_freeProj( x, y ) \
          CUR.func_freeProj( EXEC_ARG_ x, y )

#define CUR_Func_round( d, c ) \
          CUR.func_round( EXEC_ARG_ d, c )

#define CUR_Func_read_cvt( index ) \
          CUR.func_read_cvt( EXEC_ARG_ index )

#define CUR_Func_write_cvt( index, val ) \
          CUR.func_write_cvt( EXEC_ARG_ index, val )

#define CUR_Func_move_cvt( index, val ) \
          CUR.func_move_cvt( EXEC_ARG_ index, val )

#define CURRENT_Ratio() \
          Current_Ratio( EXEC_ARG )

#define CURRENT_Ppem() \
          Current_Ppem( EXEC_ARG )

#define CUR_Ppem() \
          Cur_PPEM( EXEC_ARG )

#define INS_SxVTL( a, b, c, d ) \
          Ins_SxVTL( EXEC_ARG_ a, b, c, d )

#define COMPUTE_Funcs() \
          Compute_Funcs( EXEC_ARG )

#define COMPUTE_Round( a ) \
          Compute_Round( EXEC_ARG_ a )

#define COMPUTE_Point_Displacement( a, b, c, d ) \
          Compute_Point_Displacement( EXEC_ARG_ a, b, c, d )

#define MOVE_Zp2_Point( a, b, c, t ) \
          Move_Zp2_Point( EXEC_ARG_ a, b, c, t )


  /*************************************************************************/
  /*                                                                       */
  /* Instruction dispatch function, as used by the interpreter.            */
  /*                                                                       */
  typedef void  (*TInstruction_Function)( INS_ARG );


  /*************************************************************************/
  /*                                                                       */
  /* A simple bounds-checking macro.                                       */
  /*                                                                       */
#define BOUNDS( x, n )  ( (FT_UInt)(x) >= (FT_UInt)(n) )


#undef  SUCCESS
#define SUCCESS  0

#undef  FAILURE
#define FAILURE  1


  /*************************************************************************/
  /*                                                                       */
  /*                        CODERANGE FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Goto_CodeRange                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Switches to a new code range (updates the code related elements in */
  /*    `exec', and `IP').                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range :: The new execution code range.                             */
  /*                                                                       */
  /*    IP    :: The new IP in the new code range.                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec  :: The target execution context.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Goto_CodeRange( TT_ExecContext  exec,
                     FT_Int          range,
                     FT_Long         IP )
  {
    TT_CodeRange*  coderange;


    FT_Assert( range >= 1 && range <= 3 );

    coderange = &exec->codeRangeTable[range - 1];

    FT_Assert( coderange->base != NULL );

    /* NOTE: Because the last instruction of a program may be a CALL */
    /*       which will return to the first byte *after* the code    */
    /*       range, we test for IP <= Size instead of IP < Size.     */
    /*                                                               */
    FT_Assert( (FT_ULong)IP <= coderange->size );

    exec->code     = coderange->base;
    exec->codeSize = coderange->size;
    exec->IP       = IP;
    exec->curRange = range;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Set_CodeRange                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets a code range.                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The code range index.                                    */
  /*                                                                       */
  /*    base   :: The new code base.                                       */
  /*                                                                       */
  /*    length :: The range size in bytes.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec   :: The target execution context.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Set_CodeRange( TT_ExecContext  exec,
                    FT_Int          range,
                    void*           base,
                    FT_Long         length )
  {
    FT_Assert( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = (FT_Byte*)base;
    exec->codeRangeTable[range - 1].size = length;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Clear_CodeRange                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Clears a code range.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range :: The code range index.                                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec  :: The target execution context.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Does not set the Error variable.                                   */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Clear_CodeRange( TT_ExecContext  exec,
                      FT_Int          range )
  {
    FT_Assert( range >= 1 && range <= 3 );

    exec->codeRangeTable[range - 1].base = NULL;
    exec->codeRangeTable[range - 1].size = 0;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                   EXECUTION CONTEXT ROUTINES                          */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Destroy_Context                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given context.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec   :: A handle to the target execution context.                */
  /*                                                                       */
  /*    memory :: A handle to the parent memory object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Destroy_Context( TT_ExecContext  exec,
                      FT_Memory       memory )
  {
    /* free composite load stack */
    FREE( exec->loadStack );
    exec->loadSize = 0;

    /* points zone */
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    /* free stack */
    FREE( exec->stack );
    exec->stackSize = 0;

    /* free call stack */
    FREE( exec->callStack );
    exec->callSize = 0;
    exec->callTop  = 0;

    /* free glyph code range */
    FREE( exec->glyphIns );
    exec->glyphSize = 0;

    exec->size = NULL;
    exec->face = NULL;

    FREE( exec );
    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Init_Context                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a context object.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to the parent memory object.                    */
  /*                                                                       */
  /*    face   :: A handle to the source TrueType face object.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec   :: A handle to the target execution context.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Init_Context( TT_ExecContext  exec,
                TT_Face         face,
                FT_Memory       memory )
  {
    FT_Error  error;


    FT_TRACE1(( "Init_Context: new object at 0x%08p, parent = 0x%08p\n",
                exec, face ));

    exec->memory   = memory;
    exec->callSize = 32;

    if ( ALLOC_ARRAY( exec->callStack, exec->callSize, TT_CallRec ) )
      goto Fail_Memory;

    /* all values in the context are set to 0 already, but this is */
    /* here as a remainder                                         */
    exec->maxPoints   = 0;
    exec->maxContours = 0;

    exec->stackSize = 0;
    exec->loadSize  = 0;
    exec->glyphSize = 0;

    exec->stack     = NULL;
    exec->loadStack = NULL;
    exec->glyphIns  = NULL;

    exec->face = face;
    exec->size = NULL;

    return TT_Err_Ok;

  Fail_Memory:
    FT_ERROR(( "Init_Context: not enough memory for 0x%08lx\n",
               (FT_Long)exec ));
    TT_Destroy_Context( exec, memory );

    return error;
 }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Update_Max                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks the size of a buffer and reallocates it if necessary.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory     :: A handle to the parent memory object.                */
  /*                                                                       */
  /*    multiplier :: The size in bytes of each element in the buffer.     */
  /*                                                                       */
  /*    new_max    :: The new capacity (size) of the buffer.               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size       :: The address of the buffer's current size expressed   */
  /*                  in elements.                                         */
  /*                                                                       */
  /*    buff       :: The address of the buffer base pointer.              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Update_Max( FT_Memory  memory,
              FT_ULong*  size,
              FT_Long    multiplier,
              void**     buff,
              FT_ULong   new_max )
  {
    FT_Error  error;


    if ( *size < new_max )
    {
      FREE( *buff );
      if ( ALLOC( *buff, new_max * multiplier ) )
        return error;
      *size = new_max;
    }

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Prepare an execution context for glyph hinting.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*                                                                       */
  /*    size :: A handle to the source size object.                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    exec :: A handle to the target execution context.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_Context( TT_ExecContext  exec,
                   TT_Face         face,
                   TT_Size         size )
  {
    FT_Int          i;
    FT_ULong        tmp;
    TT_MaxProfile*  maxp;
    FT_Error        error;


    exec->face = face;
    maxp       = &face->max_profile;
    exec->size = size;

    if ( size )
    {
      exec->numFDefs   = size->num_function_defs;
      exec->maxFDefs   = size->max_function_defs;
      exec->numIDefs   = size->num_instruction_defs;
      exec->maxIDefs   = size->max_instruction_defs;
      exec->FDefs      = size->function_defs;
      exec->IDefs      = size->instruction_defs;
      exec->tt_metrics = size->ttmetrics;
      exec->metrics    = size->root.metrics;

      exec->maxFunc    = size->max_func;
      exec->maxIns     = size->max_ins;

      for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
        exec->codeRangeTable[i] = size->codeRangeTable[i];

      /* set graphics state */
      exec->GS = size->GS;

      exec->cvtSize = size->cvt_size;
      exec->cvt     = size->cvt;

      exec->storeSize = size->storage_size;
      exec->storage   = size->storage;

      exec->twilight  = size->twilight;
    }

    error = Update_Max( exec->memory,
                        &exec->loadSize,
                        sizeof ( TT_SubGlyphRec ),
                        (void**)&exec->loadStack,
                        exec->face->max_components + 1 );
    if ( error )
      return error;

    /* XXX: We reserve a little more elements on the stack to deal safely */
    /*      with broken fonts like arialbs, courbs, timesbs, etc.         */
    tmp = exec->stackSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_F26Dot6 ),
                        (void**)&exec->stack,
                        maxp->maxStackElements + 32 );
    exec->stackSize = (FT_UInt)tmp;
    if ( error )
      return error;

    tmp = exec->glyphSize;
    error = Update_Max( exec->memory,
                        &tmp,
                        sizeof ( FT_Byte ),
                        (void**)&exec->glyphIns,
                        maxp->maxSizeOfInstructions );
    exec->glyphSize = (FT_UShort)tmp;
    if ( error )
      return error;

    exec->pts.n_points   = 0;
    exec->pts.n_contours = 0;

    exec->instruction_trap = FALSE;

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Save_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Saves the code ranges in a `size' object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec :: A handle to the source execution context.                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size :: A handle to the target size object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Save_Context( TT_ExecContext  exec,
                   TT_Size         size )
  {
    FT_Int  i;


    /* XXXX: Will probably disappear soon with all the code range */
    /*       management, which is now rather obsolete.            */
    /*                                                            */
    size->num_function_defs    = exec->numFDefs;
    size->num_instruction_defs = exec->numIDefs;

    size->max_func = exec->maxFunc;
    size->max_ins  = exec->maxIns;

    for ( i = 0; i < TT_MAX_CODE_RANGES; i++ )
      size->codeRangeTable[i] = exec->codeRangeTable[i];

    return TT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Run_Context                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Executes one or more instructions in the execution context.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    debug :: A Boolean flag.  If set, the function sets some internal  */
  /*             variables and returns immediately, otherwise TT_RunIns()  */
  /*             is called.                                                */
  /*                                                                       */
  /*             This is commented out currently.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec  :: A handle to the target execution context.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    TrueTyoe error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Run_Context( TT_ExecContext  exec,
                  FT_Bool         debug )
  {
    FT_Error  error;


    if ( ( error = TT_Goto_CodeRange( exec, tt_coderange_glyph, 0  ) )
           != TT_Err_Ok )
      return error;

    exec->zp0 = exec->pts;
    exec->zp1 = exec->pts;
    exec->zp2 = exec->pts;

    exec->GS.gep0 = 1;
    exec->GS.gep1 = 1;
    exec->GS.gep2 = 1;

    exec->GS.projVector.x = 0x4000;
    exec->GS.projVector.y = 0x0000;

    exec->GS.freeVector = exec->GS.projVector;
    exec->GS.dualVector = exec->GS.projVector;

    exec->GS.round_state = 1;
    exec->GS.loop        = 1;

    /* some glyphs leave something on the stack. so we clean it */
    /* before a new execution.                                  */
    exec->top     = 0;
    exec->callTop = 0;

#if 1
    FT_UNUSED( debug );

    return exec->face->interpreter( exec );
#else
    if ( !debug )
      return TT_RunIns( exec );
    else
      return TT_Err_Ok;
#endif
  }


  const TT_GraphicsState  tt_default_graphics_state =
  {
    0, 0, 0,
    { 0x4000, 0 },
    { 0x4000, 0 },
    { 0x4000, 0 },
    1, 64, 1,
    TRUE, 68, 0, 0, 9, 3,
    0, FALSE, 2, 1, 1, 1
  };


  /* documentation is in ttinterp.h */

  FT_EXPORT_DEF( TT_ExecContext )
  TT_New_Context( TT_Face  face )
  {
    TT_Driver       driver;
    TT_ExecContext  exec;
    FT_Memory       memory;


    if ( !face )
      return 0;

    driver = (TT_Driver)face->root.driver;

    memory = driver->root.root.memory;
    exec   = driver->context;

    if ( !driver->context )
    {
      FT_Error  error;


      /* allocate object */
      if ( ALLOC( exec, sizeof ( *exec ) ) )
        goto Exit;

      /* initialize it */
      error = Init_Context( exec, face, memory );
      if ( error )
        goto Fail;

      /* store it into the driver */
      driver->context = exec;
    }

  Exit:
    return driver->context;

  Fail:
    FREE( exec );

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Done_Context                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards an execution context.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec :: A handle to the target execution context.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Done_Context( TT_ExecContext  exec )
  {
    /* Nothing at all for now */
    FT_UNUSED( exec );

    return TT_Err_Ok;
  }


 /* return length of given vector */
#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static FT_F26Dot6
  Norm( FT_F26Dot6  X,
        FT_F26Dot6  Y )
  {
    TT_INT64  T1, T2;


    MUL_64( X, X, T1 );
    MUL_64( Y, Y, T2 );

    ADD_64( T1, T2, T1 );

    return (FT_F26Dot6)SQRT_64( T1 );
  }

#else  /* !FT_CONFIG_OPTION_OLD_CALCS */

  static FT_F26Dot6
  Norm( FT_F26Dot6  X,
        FT_F26Dot6  Y )
  {
    FT_Vector  v;

    v.x = X;
    v.y = Y;
    return FT_Vector_Length( &v );
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


  /*************************************************************************/
  /*                                                                       */
  /* Before an opcode is executed, the interpreter verifies that there are */
  /* enough arguments on the stack, with the help of the Pop_Push_Count    */
  /* table.                                                                */
  /*                                                                       */
  /* For each opcode, the first column gives the number of arguments that  */
  /* are popped from the stack; the second one gives the number of those   */
  /* that are pushed in result.                                            */
  /*                                                                       */
  /* Note that for opcodes with a varying number of parameters, either 0   */
  /* or 1 arg is verified before execution, depending on the nature of the */
  /* instruction:                                                          */
  /*                                                                       */
  /* - if the number of arguments is given by the bytecode stream or the   */
  /*   loop variable, 0 is chosen.                                         */
  /*                                                                       */
  /* - if the first argument is a count n that is followed by arguments    */
  /*   a1 .. an, then 1 is chosen.                                         */
  /*                                                                       */
  /*************************************************************************/


#undef  PACK
#define PACK( x, y )  ( ( x << 4 ) | y )


  static
  const FT_Byte  Pop_Push_Count[256] =
  {
    /* opcodes are gathered in groups of 16 */
    /* please keep the spaces as they are   */

    /*  SVTCA  y  */  PACK( 0, 0 ),
    /*  SVTCA  x  */  PACK( 0, 0 ),
    /*  SPvTCA y  */  PACK( 0, 0 ),
    /*  SPvTCA x  */  PACK( 0, 0 ),
    /*  SFvTCA y  */  PACK( 0, 0 ),
    /*  SFvTCA x  */  PACK( 0, 0 ),
    /*  SPvTL //  */  PACK( 2, 0 ),
    /*  SPvTL +   */  PACK( 2, 0 ),
    /*  SFvTL //  */  PACK( 2, 0 ),
    /*  SFvTL +   */  PACK( 2, 0 ),
    /*  SPvFS     */  PACK( 2, 0 ),
    /*  SFvFS     */  PACK( 2, 0 ),
    /*  GPV       */  PACK( 0, 2 ),
    /*  GFV       */  PACK( 0, 2 ),
    /*  SFvTPv    */  PACK( 0, 0 ),
    /*  ISECT     */  PACK( 5, 0 ),

    /*  SRP0      */  PACK( 1, 0 ),
    /*  SRP1      */  PACK( 1, 0 ),
    /*  SRP2      */  PACK( 1, 0 ),
    /*  SZP0      */  PACK( 1, 0 ),
    /*  SZP1      */  PACK( 1, 0 ),
    /*  SZP2      */  PACK( 1, 0 ),
    /*  SZPS      */  PACK( 1, 0 ),
    /*  SLOOP     */  PACK( 1, 0 ),
    /*  RTG       */  PACK( 0, 0 ),
    /*  RTHG      */  PACK( 0, 0 ),
    /*  SMD       */  PACK( 1, 0 ),
    /*  ELSE      */  PACK( 0, 0 ),
    /*  JMPR      */  PACK( 1, 0 ),
    /*  SCvTCi    */  PACK( 1, 0 ),
    /*  SSwCi     */  PACK( 1, 0 ),
    /*  SSW       */  PACK( 1, 0 ),

    /*  DUP       */  PACK( 1, 2 ),
    /*  POP       */  PACK( 1, 0 ),
    /*  CLEAR     */  PACK( 0, 0 ),
    /*  SWAP      */  PACK( 2, 2 ),
    /*  DEPTH     */  PACK( 0, 1 ),
    /*  CINDEX    */  PACK( 1, 1 ),
    /*  MINDEX    */  PACK( 1, 0 ),
    /*  AlignPTS  */  PACK( 2, 0 ),
    /*  INS_$28   */  PACK( 0, 0 ),
    /*  UTP       */  PACK( 1, 0 ),
    /*  LOOPCALL  */  PACK( 2, 0 ),
    /*  CALL      */  PACK( 1, 0 ),
    /*  FDEF      */  PACK( 1, 0 ),
    /*  ENDF      */  PACK( 0, 0 ),
    /*  MDAP[0]   */  PACK( 1, 0 ),
    /*  MDAP[1]   */  PACK( 1, 0 ),

    /*  IUP[0]    */  PACK( 0, 0 ),
    /*  IUP[1]    */  PACK( 0, 0 ),
    /*  SHP[0]    */  PACK( 0, 0 ),
    /*  SHP[1]    */  PACK( 0, 0 ),
    /*  SHC[0]    */  PACK( 1, 0 ),
    /*  SHC[1]    */  PACK( 1, 0 ),
    /*  SHZ[0]    */  PACK( 1, 0 ),
    /*  SHZ[1]    */  PACK( 1, 0 ),
    /*  SHPIX     */  PACK( 1, 0 ),
    /*  IP        */  PACK( 0, 0 ),
    /*  MSIRP[0]  */  PACK( 2, 0 ),
    /*  MSIRP[1]  */  PACK( 2, 0 ),
    /*  AlignRP   */  PACK( 0, 0 ),
    /*  RTDG      */  PACK( 0, 0 ),
    /*  MIAP[0]   */  PACK( 2, 0 ),
    /*  MIAP[1]   */  PACK( 2, 0 ),

    /*  NPushB    */  PACK( 0, 0 ),
    /*  NPushW    */  PACK( 0, 0 ),
    /*  WS        */  PACK( 2, 0 ),
    /*  RS        */  PACK( 1, 1 ),
    /*  WCvtP     */  PACK( 2, 0 ),
    /*  RCvt      */  PACK( 1, 1 ),
    /*  GC[0]     */  PACK( 1, 1 ),
    /*  GC[1]     */  PACK( 1, 1 ),
    /*  SCFS      */  PACK( 2, 0 ),
    /*  MD[0]     */  PACK( 2, 1 ),
    /*  MD[1]     */  PACK( 2, 1 ),
    /*  MPPEM     */  PACK( 0, 1 ),
    /*  MPS       */  PACK( 0, 1 ),
    /*  FlipON    */  PACK( 0, 0 ),
    /*  FlipOFF   */  PACK( 0, 0 ),
    /*  DEBUG     */  PACK( 1, 0 ),

    /*  LT        */  PACK( 2, 1 ),
    /*  LTEQ      */  PACK( 2, 1 ),
    /*  GT        */  PACK( 2, 1 ),
    /*  GTEQ      */  PACK( 2, 1 ),
    /*  EQ        */  PACK( 2, 1 ),
    /*  NEQ       */  PACK( 2, 1 ),
    /*  ODD       */  PACK( 1, 1 ),
    /*  EVEN      */  PACK( 1, 1 ),
    /*  IF        */  PACK( 1, 0 ),
    /*  EIF       */  PACK( 0, 0 ),
    /*  AND       */  PACK( 2, 1 ),
    /*  OR        */  PACK( 2, 1 ),
    /*  NOT       */  PACK( 1, 1 ),
    /*  DeltaP1   */  PACK( 1, 0 ),
    /*  SDB       */  PACK( 1, 0 ),
    /*  SDS       */  PACK( 1, 0 ),

    /*  ADD       */  PACK( 2, 1 ),
    /*  SUB       */  PACK( 2, 1 ),
    /*  DIV       */  PACK( 2, 1 ),
    /*  MUL       */  PACK( 2, 1 ),
    /*  ABS       */  PACK( 1, 1 ),
    /*  NEG       */  PACK( 1, 1 ),
    /*  FLOOR     */  PACK( 1, 1 ),
    /*  CEILING   */  PACK( 1, 1 ),
    /*  ROUND[0]  */  PACK( 1, 1 ),
    /*  ROUND[1]  */  PACK( 1, 1 ),
    /*  ROUND[2]  */  PACK( 1, 1 ),
    /*  ROUND[3]  */  PACK( 1, 1 ),
    /*  NROUND[0] */  PACK( 1, 1 ),
    /*  NROUND[1] */  PACK( 1, 1 ),
    /*  NROUND[2] */  PACK( 1, 1 ),
    /*  NROUND[3] */  PACK( 1, 1 ),

    /*  WCvtF     */  PACK( 2, 0 ),
    /*  DeltaP2   */  PACK( 1, 0 ),
    /*  DeltaP3   */  PACK( 1, 0 ),
    /*  DeltaCn[0] */ PACK( 1, 0 ),
    /*  DeltaCn[1] */ PACK( 1, 0 ),
    /*  DeltaCn[2] */ PACK( 1, 0 ),
    /*  SROUND    */  PACK( 1, 0 ),
    /*  S45Round  */  PACK( 1, 0 ),
    /*  JROT      */  PACK( 2, 0 ),
    /*  JROF      */  PACK( 2, 0 ),
    /*  ROFF      */  PACK( 0, 0 ),
    /*  INS_$7B   */  PACK( 0, 0 ),
    /*  RUTG      */  PACK( 0, 0 ),
    /*  RDTG      */  PACK( 0, 0 ),
    /*  SANGW     */  PACK( 1, 0 ),
    /*  AA        */  PACK( 1, 0 ),

    /*  FlipPT    */  PACK( 0, 0 ),
    /*  FlipRgON  */  PACK( 2, 0 ),
    /*  FlipRgOFF */  PACK( 2, 0 ),
    /*  INS_$83   */  PACK( 0, 0 ),
    /*  INS_$84   */  PACK( 0, 0 ),
    /*  ScanCTRL  */  PACK( 1, 0 ),
    /*  SDVPTL[0] */  PACK( 2, 0 ),
    /*  SDVPTL[1] */  PACK( 2, 0 ),
    /*  GetINFO   */  PACK( 1, 1 ),
    /*  IDEF      */  PACK( 1, 0 ),
    /*  ROLL      */  PACK( 3, 3 ),
    /*  MAX       */  PACK( 2, 1 ),
    /*  MIN       */  PACK( 2, 1 ),
    /*  ScanTYPE  */  PACK( 1, 0 ),
    /*  InstCTRL  */  PACK( 2, 0 ),
    /*  INS_$8F   */  PACK( 0, 0 ),

    /*  INS_$90  */   PACK( 0, 0 ),
    /*  INS_$91  */   PACK( 0, 0 ),
    /*  INS_$92  */   PACK( 0, 0 ),
    /*  INS_$93  */   PACK( 0, 0 ),
    /*  INS_$94  */   PACK( 0, 0 ),
    /*  INS_$95  */   PACK( 0, 0 ),
    /*  INS_$96  */   PACK( 0, 0 ),
    /*  INS_$97  */   PACK( 0, 0 ),
    /*  INS_$98  */   PACK( 0, 0 ),
    /*  INS_$99  */   PACK( 0, 0 ),
    /*  INS_$9A  */   PACK( 0, 0 ),
    /*  INS_$9B  */   PACK( 0, 0 ),
    /*  INS_$9C  */   PACK( 0, 0 ),
    /*  INS_$9D  */   PACK( 0, 0 ),
    /*  INS_$9E  */   PACK( 0, 0 ),
    /*  INS_$9F  */   PACK( 0, 0 ),

    /*  INS_$A0  */   PACK( 0, 0 ),
    /*  INS_$A1  */   PACK( 0, 0 ),
    /*  INS_$A2  */   PACK( 0, 0 ),
    /*  INS_$A3  */   PACK( 0, 0 ),
    /*  INS_$A4  */   PACK( 0, 0 ),
    /*  INS_$A5  */   PACK( 0, 0 ),
    /*  INS_$A6  */   PACK( 0, 0 ),
    /*  INS_$A7  */   PACK( 0, 0 ),
    /*  INS_$A8  */   PACK( 0, 0 ),
    /*  INS_$A9  */   PACK( 0, 0 ),
    /*  INS_$AA  */   PACK( 0, 0 ),
    /*  INS_$AB  */   PACK( 0, 0 ),
    /*  INS_$AC  */   PACK( 0, 0 ),
    /*  INS_$AD  */   PACK( 0, 0 ),
    /*  INS_$AE  */   PACK( 0, 0 ),
    /*  INS_$AF  */   PACK( 0, 0 ),

    /*  PushB[0]  */  PACK( 0, 1 ),
    /*  PushB[1]  */  PACK( 0, 2 ),
    /*  PushB[2]  */  PACK( 0, 3 ),
    /*  PushB[3]  */  PACK( 0, 4 ),
    /*  PushB[4]  */  PACK( 0, 5 ),
    /*  PushB[5]  */  PACK( 0, 6 ),
    /*  PushB[6]  */  PACK( 0, 7 ),
    /*  PushB[7]  */  PACK( 0, 8 ),
    /*  PushW[0]  */  PACK( 0, 1 ),
    /*  PushW[1]  */  PACK( 0, 2 ),
    /*  PushW[2]  */  PACK( 0, 3 ),
    /*  PushW[3]  */  PACK( 0, 4 ),
    /*  PushW[4]  */  PACK( 0, 5 ),
    /*  PushW[5]  */  PACK( 0, 6 ),
    /*  PushW[6]  */  PACK( 0, 7 ),
    /*  PushW[7]  */  PACK( 0, 8 ),

    /*  MDRP[00]  */  PACK( 1, 0 ),
    /*  MDRP[01]  */  PACK( 1, 0 ),
    /*  MDRP[02]  */  PACK( 1, 0 ),
    /*  MDRP[03]  */  PACK( 1, 0 ),
    /*  MDRP[04]  */  PACK( 1, 0 ),
    /*  MDRP[05]  */  PACK( 1, 0 ),
    /*  MDRP[06]  */  PACK( 1, 0 ),
    /*  MDRP[07]  */  PACK( 1, 0 ),
    /*  MDRP[08]  */  PACK( 1, 0 ),
    /*  MDRP[09]  */  PACK( 1, 0 ),
    /*  MDRP[10]  */  PACK( 1, 0 ),
    /*  MDRP[11]  */  PACK( 1, 0 ),
    /*  MDRP[12]  */  PACK( 1, 0 ),
    /*  MDRP[13]  */  PACK( 1, 0 ),
    /*  MDRP[14]  */  PACK( 1, 0 ),
    /*  MDRP[15]  */  PACK( 1, 0 ),

    /*  MDRP[16]  */  PACK( 1, 0 ),
    /*  MDRP[17]  */  PACK( 1, 0 ),
    /*  MDRP[18]  */  PACK( 1, 0 ),
    /*  MDRP[19]  */  PACK( 1, 0 ),
    /*  MDRP[20]  */  PACK( 1, 0 ),
    /*  MDRP[21]  */  PACK( 1, 0 ),
    /*  MDRP[22]  */  PACK( 1, 0 ),
    /*  MDRP[23]  */  PACK( 1, 0 ),
    /*  MDRP[24]  */  PACK( 1, 0 ),
    /*  MDRP[25]  */  PACK( 1, 0 ),
    /*  MDRP[26]  */  PACK( 1, 0 ),
    /*  MDRP[27]  */  PACK( 1, 0 ),
    /*  MDRP[28]  */  PACK( 1, 0 ),
    /*  MDRP[29]  */  PACK( 1, 0 ),
    /*  MDRP[30]  */  PACK( 1, 0 ),
    /*  MDRP[31]  */  PACK( 1, 0 ),

    /*  MIRP[00]  */  PACK( 2, 0 ),
    /*  MIRP[01]  */  PACK( 2, 0 ),
    /*  MIRP[02]  */  PACK( 2, 0 ),
    /*  MIRP[03]  */  PACK( 2, 0 ),
    /*  MIRP[04]  */  PACK( 2, 0 ),
    /*  MIRP[05]  */  PACK( 2, 0 ),
    /*  MIRP[06]  */  PACK( 2, 0 ),
    /*  MIRP[07]  */  PACK( 2, 0 ),
    /*  MIRP[08]  */  PACK( 2, 0 ),
    /*  MIRP[09]  */  PACK( 2, 0 ),
    /*  MIRP[10]  */  PACK( 2, 0 ),
    /*  MIRP[11]  */  PACK( 2, 0 ),
    /*  MIRP[12]  */  PACK( 2, 0 ),
    /*  MIRP[13]  */  PACK( 2, 0 ),
    /*  MIRP[14]  */  PACK( 2, 0 ),
    /*  MIRP[15]  */  PACK( 2, 0 ),

    /*  MIRP[16]  */  PACK( 2, 0 ),
    /*  MIRP[17]  */  PACK( 2, 0 ),
    /*  MIRP[18]  */  PACK( 2, 0 ),
    /*  MIRP[19]  */  PACK( 2, 0 ),
    /*  MIRP[20]  */  PACK( 2, 0 ),
    /*  MIRP[21]  */  PACK( 2, 0 ),
    /*  MIRP[22]  */  PACK( 2, 0 ),
    /*  MIRP[23]  */  PACK( 2, 0 ),
    /*  MIRP[24]  */  PACK( 2, 0 ),
    /*  MIRP[25]  */  PACK( 2, 0 ),
    /*  MIRP[26]  */  PACK( 2, 0 ),
    /*  MIRP[27]  */  PACK( 2, 0 ),
    /*  MIRP[28]  */  PACK( 2, 0 ),
    /*  MIRP[29]  */  PACK( 2, 0 ),
    /*  MIRP[30]  */  PACK( 2, 0 ),
    /*  MIRP[31]  */  PACK( 2, 0 )
  };


  static
  const FT_Char  opcode_length[256] =
  {
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

   -1,-1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    2, 3, 4, 5,  6, 7, 8, 9,  3, 5, 7, 9, 11,13,15,17,

    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,
    1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1
  };

  static
  const FT_Vector  Null_Vector = {0,0};


#undef PACK


#undef  NULL_Vector
#define NULL_Vector  (FT_Vector*)&Null_Vector


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Current_Ratio                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the current aspect ratio scaling factor depending on the   */
  /*    projection vector's state and device resolutions.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The aspect ratio in 16.16 format, always <= 1.0 .                  */
  /*                                                                       */
  static FT_Long
  Current_Ratio( EXEC_OP )
  {
    if ( CUR.tt_metrics.ratio )
      return CUR.tt_metrics.ratio;

    if ( CUR.GS.projVector.y == 0 )
      CUR.tt_metrics.ratio = CUR.tt_metrics.x_ratio;

    else if ( CUR.GS.projVector.x == 0 )
      CUR.tt_metrics.ratio = CUR.tt_metrics.y_ratio;

    else
    {
      FT_Long  x, y;

      x = TT_MULDIV( CUR.GS.projVector.x, CUR.tt_metrics.x_ratio, 0x4000 );
      y = TT_MULDIV( CUR.GS.projVector.y, CUR.tt_metrics.y_ratio, 0x4000 );
      CUR.tt_metrics.ratio = Norm( x, y );
    }

    return CUR.tt_metrics.ratio;
  }


  static FT_Long
  Current_Ppem( EXEC_OP )
  {
    return TT_MULFIX( CUR.tt_metrics.ppem, CURRENT_Ratio() );
  }


  /*************************************************************************/
  /*                                                                       */
  /* Functions related to the control value table (CVT).                   */
  /*                                                                       */
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT( EXEC_OP_ FT_ULong  index )
  {
    return CUR.cvt[index];
  }


  FT_CALLBACK_DEF( FT_F26Dot6 )
  Read_CVT_Stretched( EXEC_OP_ FT_ULong  index )
  {
    return TT_MULFIX( CUR.cvt[index], CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Write_CVT( EXEC_OP_ FT_ULong    index,
                      FT_F26Dot6  value )
  {
    CUR.cvt[index] = value;
  }


  FT_CALLBACK_DEF( void )
  Write_CVT_Stretched( EXEC_OP_ FT_ULong    index,
                                FT_F26Dot6  value )
  {
    CUR.cvt[index] = FT_DivFix( value, CURRENT_Ratio() );
  }


  FT_CALLBACK_DEF( void )
  Move_CVT( EXEC_OP_ FT_ULong    index,
                     FT_F26Dot6  value )
  {
    CUR.cvt[index] += value;
  }


  FT_CALLBACK_DEF( void )
  Move_CVT_Stretched( EXEC_OP_ FT_ULong    index,
                               FT_F26Dot6  value )
  {
    CUR.cvt[index] += FT_DivFix( value, CURRENT_Ratio() );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    GetShortIns                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a short integer taken from the instruction stream at       */
  /*    address IP.                                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Short read at code[IP].                                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This one could become a macro.                                     */
  /*                                                                       */
  static FT_Short
  GetShortIns( EXEC_OP )
  {
    /* Reading a byte stream so there is no endianess (DaveP) */
    CUR.IP += 2;
    return (FT_Short)( ( CUR.code[CUR.IP - 2] << 8 ) +
                         CUR.code[CUR.IP - 1]      );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Ins_Goto_CodeRange                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Goes to a certain code range in the instruction stream.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    aRange :: The index of the code range.                             */
  /*                                                                       */
  /*    aIP    :: The new IP address in the code range.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS or FAILURE.                                                */
  /*                                                                       */
  static FT_Bool
  Ins_Goto_CodeRange( EXEC_OP_ FT_Int    aRange,
                               FT_ULong  aIP )
  {
    TT_CodeRange*  range;


    if ( aRange < 1 || aRange > 3 )
    {
      CUR.error = TT_Err_Bad_Argument;
      return FAILURE;
    }

    range = &CUR.codeRangeTable[aRange - 1];

    if ( range->base == NULL )     /* invalid coderange */
    {
      CUR.error = TT_Err_Invalid_CodeRange;
      return FAILURE;
    }

    /* NOTE: Because the last instruction of a program may be a CALL */
    /*       which will return to the first byte *after* the code    */
    /*       range, we test for AIP <= Size, instead of AIP < Size.  */

    if ( aIP > range->size )
    {
      CUR.error = TT_Err_Code_Overflow;
      return FAILURE;
    }

    CUR.code     = range->base;
    CUR.codeSize = range->size;
    CUR.IP       = aIP;
    CUR.curRange = aRange;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Direct_Move                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Moves a point by a given distance along the freedom vector.  The   */
  /*    point will be `touched'.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    point    :: The index of the point to move.                        */
  /*                                                                       */
  /*    distance :: The distance to apply.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    zone     :: The affected glyph zone.                               */
  /*                                                                       */
  static void
  Direct_Move( EXEC_OP_ TT_GlyphZone*  zone,
                        FT_UShort      point,
                        FT_F26Dot6     distance )
  {
    FT_F26Dot6 v;


    v = CUR.GS.freeVector.x;

    if ( v != 0 )
    {

#ifdef NO_APPLE_PATENT

      if ( ABS( CUR.F_dot_P ) > APPLE_THRESHOLD )
        zone->cur[point].x += distance;

#else

      zone->cur[point].x += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

#endif

      zone->tags[point] |= FT_Curve_Tag_Touch_X;
    }

    v = CUR.GS.freeVector.y;

    if ( v != 0 )
    {

#ifdef NO_APPLE_PATENT

      if ( ABS( CUR.F_dot_P ) > APPLE_THRESHOLD )
        zone->cur[point].y += distance;

#else

      zone->cur[point].y += TT_MULDIV( distance,
                                       v * 0x10000L,
                                       CUR.F_dot_P );

#endif

      zone->tags[point] |= FT_Curve_Tag_Touch_Y;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* Special versions of Direct_Move()                                     */
  /*                                                                       */
  /*   The following versions are used whenever both vectors are both      */
  /*   along one of the coordinate unit vectors, i.e. in 90% of the cases. */
  /*                                                                       */
  /*************************************************************************/


  static void
  Direct_Move_X( EXEC_OP_ TT_GlyphZone*  zone,
                          FT_UShort      point,
                          FT_F26Dot6     distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].x += distance;
    zone->tags[point]  |= FT_Curve_Tag_Touch_X;
  }


  static void
  Direct_Move_Y( EXEC_OP_ TT_GlyphZone*  zone,
                          FT_UShort      point,
                          FT_F26Dot6     distance )
  {
    FT_UNUSED_EXEC;

    zone->cur[point].y += distance;
    zone->tags[point]  |= FT_Curve_Tag_Touch_Y;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_None                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Does not round, but adds engine compensation.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance (not) to round.                       */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The compensated distance.                                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The TrueType specification says very few about the relationship    */
  /*    between rounding and engine compensation.  However, it seems from  */
  /*    the description of super round that we should add the compensation */
  /*    before rounding.                                                   */
  /*                                                                       */
  static FT_F26Dot6
  Round_None( EXEC_OP_ FT_F26Dot6  distance,
                       FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( val < 0 )
        val = 0;
    }
    else {
      val = distance - compensation;
      if ( val > 0 )
        val = 0;
    }
    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Grid                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to grid after adding engine compensation.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                          FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 32;
      if ( val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance + 32 ) & -64 );
      if ( val > 0 )
        val = 0;
    }

    return  val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Half_Grid                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to half grid after adding engine compensation.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Half_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = ( ( distance + compensation ) & -64 ) + 32;
      if ( val < 0 )
        val = 0;
    }
    else
    {
      val = -( ( (compensation - distance) & -64 ) + 32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Down_To_Grid                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value down to grid after adding engine compensation.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_Down_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                               FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation;
      if ( val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance ) & -64 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Up_To_Grid                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value up to grid after adding engine compensation.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_Up_To_Grid( EXEC_OP_ FT_F26Dot6  distance,
                             FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    FT_UNUSED_EXEC;

    if ( distance >= 0 )
    {
      val = distance + compensation + 63;
      if ( val > 0 )
        val &= ~63;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance + 63 ) & -64 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_To_Double_Grid                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Rounds value to double grid after adding engine compensation.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  static FT_F26Dot6
  Round_To_Double_Grid( EXEC_OP_ FT_F26Dot6  distance,
                                 FT_F26Dot6  compensation )
  {
    FT_F26Dot6 val;

    FT_UNUSED_EXEC;


    if ( distance >= 0 )
    {
      val = distance + compensation + 16;
      if ( val > 0 )
        val &= ~31;
      else
        val = 0;
    }
    else
    {
      val = -( ( compensation - distance + 16 ) & -32 );
      if ( val > 0 )
        val = 0;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Super                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Super-rounds value to grid after adding engine compensation.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The TrueType specification says very few about the relationship    */
  /*    between rounding and engine compensation.  However, it seems from  */
  /*    the description of super round that we should add the compensation */
  /*    before rounding.                                                   */
  /*                                                                       */
  static FT_F26Dot6
  Round_Super( EXEC_OP_ FT_F26Dot6  distance,
                        FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( distance - CUR.phase + CUR.threshold + compensation ) &
              -CUR.period;
      if ( val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( CUR.threshold - CUR.phase - distance + compensation ) &
               -CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Round_Super_45                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Super-rounds value to grid after adding engine compensation.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    distance     :: The distance to round.                             */
  /*                                                                       */
  /*    compensation :: The engine compensation.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Rounded distance.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    There is a separate function for Round_Super_45() as we may need   */
  /*    greater precision.                                                 */
  /*                                                                       */
  static FT_F26Dot6
  Round_Super_45( EXEC_OP_ FT_F26Dot6  distance,
                           FT_F26Dot6  compensation )
  {
    FT_F26Dot6  val;


    if ( distance >= 0 )
    {
      val = ( ( distance - CUR.phase + CUR.threshold + compensation ) /
                CUR.period ) * CUR.period;
      if ( val < 0 )
        val = 0;
      val += CUR.phase;
    }
    else
    {
      val = -( ( ( CUR.threshold - CUR.phase - distance + compensation ) /
                   CUR.period ) * CUR.period );
      if ( val > 0 )
        val = 0;
      val -= CUR.phase;
    }

    return val;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Compute_Round                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the rounding mode.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    round_mode :: The rounding mode to be used.                        */
  /*                                                                       */
  static void
  Compute_Round( EXEC_OP_ FT_Byte  round_mode )
  {
    switch ( round_mode )
    {
    case TT_Round_Off:
      CUR.func_round = (TT_Round_Func)Round_None;
      break;

    case TT_Round_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Grid;
      break;

    case TT_Round_Up_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;
      break;

    case TT_Round_Down_To_Grid:
      CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;
      break;

    case TT_Round_To_Half_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;
      break;

    case TT_Round_To_Double_Grid:
      CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;
      break;

    case TT_Round_Super:
      CUR.func_round = (TT_Round_Func)Round_Super;
      break;

    case TT_Round_Super_45:
      CUR.func_round = (TT_Round_Func)Round_Super_45;
      break;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    SetSuperRound                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets Super Round parameters.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    GridPeriod :: Grid period                                          */
  /*    selector   :: SROUND opcode                                        */
  /*                                                                       */
  static void
  SetSuperRound( EXEC_OP_ FT_F26Dot6  GridPeriod,
                          FT_Long     selector )
  {
    switch ( (FT_Int)( selector & 0xC0 ) )
    {
      case 0:
        CUR.period = GridPeriod / 2;
        break;

      case 0x40:
        CUR.period = GridPeriod;
        break;

      case 0x80:
        CUR.period = GridPeriod * 2;
        break;

      /* This opcode is reserved, but... */

      case 0xC0:
        CUR.period = GridPeriod;
        break;
    }

    switch ( (FT_Int)( selector & 0x30 ) )
    {
    case 0:
      CUR.phase = 0;
      break;

    case 0x10:
      CUR.phase = CUR.period / 4;
      break;

    case 0x20:
      CUR.phase = CUR.period / 2;
      break;

    case 0x30:
      CUR.phase = GridPeriod * 3 / 4;
      break;
    }

    if ( (selector & 0x0F) == 0 )
      CUR.threshold = CUR.period - 1;
    else
      CUR.threshold = ( (FT_Int)( selector & 0x0F ) - 4 ) * CUR.period / 8;

    CUR.period    /= 256;
    CUR.phase     /= 256;
    CUR.threshold /= 256;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of vector given by (v2-v1) along the       */
  /*    current projection vector.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project( EXEC_OP_ FT_Vector*  v1,
                    FT_Vector*  v2 )
  {
    return TT_MULDIV( v1->x - v2->x, CUR.GS.projVector.x, 0x4000 ) +
           TT_MULDIV( v1->y - v2->y, CUR.GS.projVector.y, 0x4000 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Dual_Project                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    current dual vector.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Dual_Project( EXEC_OP_ FT_Vector*  v1,
                         FT_Vector*  v2 )
  {
    return TT_MULDIV( v1->x - v2->x, CUR.GS.dualVector.x, 0x4000 ) +
           TT_MULDIV( v1->y - v2->y, CUR.GS.dualVector.y, 0x4000 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Free_Project                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    current freedom vector.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Free_Project( EXEC_OP_ FT_Vector*  v1,
                         FT_Vector*  v2 )
  {
    return TT_MULDIV( v1->x - v2->x, CUR.GS.freeVector.x, 0x4000 ) +
           TT_MULDIV( v1->y - v2->y, CUR.GS.freeVector.y, 0x4000 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project_x                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    horizontal axis.                                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project_x( EXEC_OP_ FT_Vector*  v1,
                      FT_Vector*  v2 )
  {
    FT_UNUSED_EXEC;

    return ( v1->x - v2->x );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Project_y                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection of the vector given by (v2-v1) along the   */
  /*    vertical axis.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    v1 :: First input vector.                                          */
  /*    v2 :: Second input vector.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The distance in F26dot6 format.                                    */
  /*                                                                       */
  static FT_F26Dot6
  Project_y( EXEC_OP_ FT_Vector*  v1,
                      FT_Vector*  v2 )
  {
    FT_UNUSED_EXEC;

   return ( v1->y - v2->y );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Compute_Funcs                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the projection and movement function pointers according   */
  /*    to the current graphics state.                                     */
  /*                                                                       */
  static void
  Compute_Funcs( EXEC_OP )
  {
    if ( CUR.GS.freeVector.x == 0x4000 )
    {
      CUR.func_freeProj = (TT_Project_Func)Project_x;
      CUR.F_dot_P       = CUR.GS.projVector.x * 0x10000L;
    }
    else
    {
      if ( CUR.GS.freeVector.y == 0x4000 )
      {
        CUR.func_freeProj = (TT_Project_Func)Project_y;
        CUR.F_dot_P       = CUR.GS.projVector.y * 0x10000L;
      }
      else
      {
        CUR.func_freeProj = (TT_Project_Func)Free_Project;
        CUR.F_dot_P = (FT_Long)CUR.GS.projVector.x * CUR.GS.freeVector.x * 4 +
                      (FT_Long)CUR.GS.projVector.y * CUR.GS.freeVector.y * 4;
      }
    }

    if ( CUR.GS.projVector.x == 0x4000 )
      CUR.func_project = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.projVector.y == 0x4000 )
        CUR.func_project = (TT_Project_Func)Project_y;
      else
        CUR.func_project = (TT_Project_Func)Project;
    }

    if ( CUR.GS.dualVector.x == 0x4000 )
      CUR.func_dualproj = (TT_Project_Func)Project_x;
    else
    {
      if ( CUR.GS.dualVector.y == 0x4000 )
        CUR.func_dualproj = (TT_Project_Func)Project_y;
      else
        CUR.func_dualproj = (TT_Project_Func)Dual_Project;
    }

    CUR.func_move = (TT_Move_Func)Direct_Move;

    if ( CUR.F_dot_P == 0x40000000L )
    {
      if ( CUR.GS.freeVector.x == 0x4000 )
        CUR.func_move = (TT_Move_Func)Direct_Move_X;
      else
      {
        if ( CUR.GS.freeVector.y == 0x4000 )
          CUR.func_move = (TT_Move_Func)Direct_Move_Y;
      }
    }

    /* at small sizes, F_dot_P can become too small, resulting   */
    /* in overflows and `spikes' in a number of glyphs like `w'. */

    if ( ABS( CUR.F_dot_P ) < 0x4000000L )
      CUR.F_dot_P = 0x40000000L;

    /* Disable cached aspect ratio */
    CUR.tt_metrics.ratio = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Normalize                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Norms a vector.                                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    Vx :: The horizontal input vector coordinate.                      */
  /*    Vy :: The vertical input vector coordinate.                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    R  :: The normed unit vector.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Returns FAILURE if a vector parameter is zero.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In case Vx and Vy are both zero, Normalize() returns SUCCESS, and  */
  /*    R is undefined.                                                    */
  /*                                                                       */

#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static FT_Bool
  Normalize( EXEC_OP_ FT_F26Dot6      Vx,
                      FT_F26Dot6      Vy,
                      FT_UnitVector*  R )
  {
    FT_F26Dot6  W;
    FT_Bool     S1, S2;

    FT_UNUSED_EXEC;


    if ( ABS( Vx ) < 0x10000L && ABS( Vy ) < 0x10000L )
    {
      Vx *= 0x100;
      Vy *= 0x100;

      W = Norm( Vx, Vy );

      if ( W == 0 )
      {
        /* XXX: UNDOCUMENTED! It seems that it is possible to try   */
        /*      to normalize the vector (0,0).  Return immediately. */
        return SUCCESS;
      }

      R->x = (FT_F2Dot14)FT_MulDiv( Vx, 0x4000L, W );
      R->y = (FT_F2Dot14)FT_MulDiv( Vy, 0x4000L, W );

      return SUCCESS;
    }

    W = Norm( Vx, Vy );

    Vx = FT_MulDiv( Vx, 0x4000L, W );
    Vy = FT_MulDiv( Vy, 0x4000L, W );

    W = Vx * Vx + Vy * Vy;

    /* Now, we want that Sqrt( W ) = 0x4000 */
    /* Or 0x1000000 <= W < 0x1004000        */

    if ( Vx < 0 )
    {
      Vx = -Vx;
      S1 = TRUE;
    }
    else
      S1 = FALSE;

    if ( Vy < 0 )
    {
      Vy = -Vy;
      S2 = TRUE;
    }
    else
      S2 = FALSE;

    while ( W < 0x1000000L )
    {
      /* We need to increase W by a minimal amount */
      if ( Vx < Vy )
        Vx++;
      else
        Vy++;

      W = Vx * Vx + Vy * Vy;
    }

    while ( W >= 0x1004000L )
    {
      /* We need to decrease W by a minimal amount */
      if ( Vx < Vy )
        Vx--;
      else
        Vy--;

      W = Vx * Vx + Vy * Vy;
    }

    /* Note that in various cases, we can only  */
    /* compute a Sqrt(W) of 0x3FFF, eg. Vx = Vy */

    if ( S1 )
      Vx = -Vx;

    if ( S2 )
      Vy = -Vy;

    R->x = (FT_F2Dot14)Vx;   /* Type conversion */
    R->y = (FT_F2Dot14)Vy;   /* Type conversion */

    return SUCCESS;
  }

#else

  static FT_Bool
  Normalize( EXEC_OP_ FT_F26Dot6      Vx,
                      FT_F26Dot6      Vy,
                      FT_UnitVector*  R )
  {
    FT_Vector  v;
    FT_Angle   angle;

    FT_UNUSED_EXEC;

    angle = FT_Atan2( Vx, Vy );

    FT_Vector_Unit( &v, angle );

    R->x = (short)(v.x >> 2);
    R->y = (short)(v.y >> 2);

    return SUCCESS;
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


  /*************************************************************************/
  /*                                                                       */
  /* Here we start with the implementation of the various opcodes.         */
  /*                                                                       */
  /*************************************************************************/


  static FT_Bool
  Ins_SxVTL( EXEC_OP_ FT_UShort       aIdx1,
                      FT_UShort       aIdx2,
                      FT_Int          aOpc,
                      FT_UnitVector*  Vec )
  {
    FT_Long     A, B, C;
    FT_Vector*  p1;
    FT_Vector*  p2;


    if ( BOUNDS( aIdx1, CUR.zp2.n_points ) ||
         BOUNDS( aIdx2, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    p1 = CUR.zp1.cur + aIdx2;
    p2 = CUR.zp2.cur + aIdx1;

    A = p1->x - p2->x;
    B = p1->y - p2->y;

    if ( ( aOpc & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, Vec );

    return SUCCESS;
  }


  /* When not using the big switch statements, the interpreter uses a */
  /* call table defined later below in this source.  Each opcode must */
  /* thus have a corresponding function, even trivial ones.           */
  /*                                                                  */
  /* They are all defined there.                                      */

#define DO_SVTCA                            \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.freeVector.y = B;                \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.projVector.x = A;                \
    CUR.GS.dualVector.x = A;                \
                                            \
    CUR.GS.projVector.y = B;                \
    CUR.GS.dualVector.y = B;                \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SFVTCA                           \
  {                                         \
    FT_Short  A, B;                         \
                                            \
                                            \
    A = (FT_Short)( CUR.opcode & 1 ) << 14; \
    B = A ^ (FT_Short)0x4000;               \
                                            \
    CUR.GS.freeVector.x = A;                \
    CUR.GS.freeVector.y = B;                \
                                            \
    COMPUTE_Funcs();                        \
  }


#define DO_SPVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.projVector ) == SUCCESS ) \
    {                                                 \
      CUR.GS.dualVector = CUR.GS.projVector;          \
      COMPUTE_Funcs();                                \
    }


#define DO_SFVTL                                      \
    if ( INS_SxVTL( (FT_UShort)args[1],               \
                    (FT_UShort)args[0],               \
                    CUR.opcode,                       \
                    &CUR.GS.freeVector ) == SUCCESS ) \
      COMPUTE_Funcs();


#define DO_SFVTPV                          \
    CUR.GS.freeVector = CUR.GS.projVector; \
    COMPUTE_Funcs();


#define DO_SPVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = (FT_Long)S;                             \
                                                \
    NORMalize( X, Y, &CUR.GS.projVector );      \
                                                \
    CUR.GS.dualVector = CUR.GS.projVector;      \
    COMPUTE_Funcs();                            \
  }


#define DO_SFVFS                                \
  {                                             \
    FT_Short  S;                                \
    FT_Long   X, Y;                             \
                                                \
                                                \
    /* Only use low 16bits, then sign extend */ \
    S = (FT_Short)args[1];                      \
    Y = (FT_Long)S;                             \
    S = (FT_Short)args[0];                      \
    X = S;                                      \
                                                \
    NORMalize( X, Y, &CUR.GS.freeVector );      \
    COMPUTE_Funcs();                            \
  }


#define DO_GPV                     \
    args[0] = CUR.GS.projVector.x; \
    args[1] = CUR.GS.projVector.y;


#define DO_GFV                     \
    args[0] = CUR.GS.freeVector.x; \
    args[1] = CUR.GS.freeVector.y;


#define DO_SRP0                      \
    CUR.GS.rp0 = (FT_UShort)args[0];


#define DO_SRP1                      \
    CUR.GS.rp1 = (FT_UShort)args[0];


#define DO_SRP2                      \
    CUR.GS.rp2 = (FT_UShort)args[0];


#define DO_RTHG                                         \
    CUR.GS.round_state = TT_Round_To_Half_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Half_Grid;


#define DO_RTG                                     \
    CUR.GS.round_state = TT_Round_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Grid;


#define DO_RTDG                                           \
    CUR.GS.round_state = TT_Round_To_Double_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_To_Double_Grid;


#define DO_RUTG                                       \
    CUR.GS.round_state = TT_Round_Up_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Up_To_Grid;


#define DO_RDTG                                         \
    CUR.GS.round_state = TT_Round_Down_To_Grid;         \
    CUR.func_round = (TT_Round_Func)Round_Down_To_Grid;


#define DO_ROFF                                 \
    CUR.GS.round_state = TT_Round_Off;          \
    CUR.func_round = (TT_Round_Func)Round_None;


#define DO_SROUND                                \
    SET_SuperRound( 0x4000, args[0] );           \
    CUR.GS.round_state = TT_Round_Super;         \
    CUR.func_round = (TT_Round_Func)Round_Super;


#define DO_S45ROUND                                 \
    SET_SuperRound( 0x2D41, args[0] );              \
    CUR.GS.round_state = TT_Round_Super_45;         \
    CUR.func_round = (TT_Round_Func)Round_Super_45;


#define DO_SLOOP                       \
    if ( args[0] < 0 )                 \
      CUR.error = TT_Err_Bad_Argument; \
    else                               \
      CUR.GS.loop = args[0];


#define DO_SMD                         \
    CUR.GS.minimum_distance = args[0];


#define DO_SCVTCI                                     \
    CUR.GS.control_value_cutin = (FT_F26Dot6)args[0];


#define DO_SSWCI                                     \
    CUR.GS.single_width_cutin = (FT_F26Dot6)args[0];


    /* XXX: UNDOCUMENTED! or bug in the Windows engine? */
    /*                                                  */
    /* It seems that the value that is read here is     */
    /* expressed in 16.16 format rather than in font    */
    /* units.                                           */
    /*                                                  */
#define DO_SSW                                                 \
    CUR.GS.single_width_value = (FT_F26Dot6)( args[0] >> 10 );


#define DO_FLIPON            \
    CUR.GS.auto_flip = TRUE;


#define DO_FLIPOFF            \
    CUR.GS.auto_flip = FALSE;


#define DO_SDB                             \
    CUR.GS.delta_base = (FT_Short)args[0];


#define DO_SDS                              \
    CUR.GS.delta_shift = (FT_Short)args[0];


#define DO_MD  /* nothing */


#define DO_MPPEM              \
    args[0] = CURRENT_Ppem();


  /* Note: The pointSize should be irrelevant in a given font program; */
  /*       we thus decide to return only the ppem.                     */
#if 0

#define DO_MPS                       \
    args[0] = CUR.metrics.pointSize;

#else

#define DO_MPS                \
    args[0] = CURRENT_Ppem();

#endif /* 0 */


#define DO_DUP         \
    args[1] = args[0];


#define DO_CLEAR     \
    CUR.new_top = 0;


#define DO_SWAP        \
  {                    \
    FT_Long  L;        \
                       \
                       \
    L       = args[0]; \
    args[0] = args[1]; \
    args[1] = L;       \
  }


#define DO_DEPTH       \
    args[0] = CUR.top;


#define DO_CINDEX                           \
  {                                         \
    FT_Long  L;                             \
                                            \
                                            \
    L = args[0];                            \
                                            \
    if ( L <= 0 || L > CUR.args )           \
      CUR.error = TT_Err_Invalid_Reference; \
    else                                    \
      args[0] = CUR.stack[CUR.args - L];    \
  }


#define DO_JROT               \
    if ( args[1] != 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_JMPR             \
    CUR.IP      += args[0]; \
    CUR.step_ins = FALSE;


#define DO_JROF               \
    if ( args[1] == 0 )       \
    {                         \
      CUR.IP      += args[0]; \
      CUR.step_ins = FALSE;   \
    }


#define DO_LT                        \
    args[0] = ( args[0] < args[1] );


#define DO_LTEQ                       \
    args[0] = ( args[0] <= args[1] );


#define DO_GT                        \
    args[0] = ( args[0] > args[1] );


#define DO_GTEQ                       \
    args[0] = ( args[0] >= args[1] );


#define DO_EQ                         \
    args[0] = ( args[0] == args[1] );


#define DO_NEQ                        \
    args[0] = ( args[0] != args[1] );


#define DO_ODD                                                  \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 64 );


#define DO_EVEN                                                \
    args[0] = ( ( CUR_Func_round( args[0], 0 ) & 127 ) == 0 );


#define DO_AND                        \
    args[0] = ( args[0] && args[1] );


#define DO_OR                         \
    args[0] = ( args[0] || args[1] );


#define DO_NOT          \
    args[0] = !args[0];


#define DO_ADD          \
    args[0] += args[1];


#define DO_SUB          \
    args[0] -= args[1];


#define DO_DIV                                      \
    if ( args[1] == 0 )                             \
      CUR.error = TT_Err_Divide_By_Zero;            \
    else                                            \
      args[0] = TT_MULDIV( args[0], 64L, args[1] );


#define DO_MUL                                    \
    args[0] = TT_MULDIV( args[0], args[1], 64L );


#define DO_ABS                \
    args[0] = ABS( args[0] );


#define DO_NEG          \
    args[0] = -args[0];


#define DO_FLOOR    \
    args[0] &= -64;


#define DO_CEILING                    \
    args[0] = ( args[0] + 63 ) & -64;


#define DO_RS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
       else                            \
         args[0] = 0;                  \
     }                                 \
     else                              \
       args[0] = CUR.storage[I];       \
   }


#define DO_WS                          \
   {                                   \
     FT_ULong  I = (FT_ULong)args[0];  \
                                       \
                                       \
     if ( BOUNDS( I, CUR.storeSize ) ) \
     {                                 \
       if ( CUR.pedantic_hinting )     \
       {                               \
         ARRAY_BOUND_ERROR;            \
       }                               \
     }                                 \
     else                              \
       CUR.storage[I] = args[1];       \
   }


#define DO_RCVT                          \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
       else                              \
         args[0] = 0;                    \
     }                                   \
     else                                \
       args[0] = CUR_Func_read_cvt( I ); \
   }


#define DO_WCVTP                         \
   {                                     \
     FT_ULong  I = (FT_ULong)args[0];    \
                                         \
                                         \
     if ( BOUNDS( I, CUR.cvtSize ) )     \
     {                                   \
       if ( CUR.pedantic_hinting )       \
       {                                 \
         ARRAY_BOUND_ERROR;              \
       }                                 \
     }                                   \
     else                                \
       CUR_Func_write_cvt( I, args[1] ); \
   }


#define DO_WCVTF                                                \
   {                                                            \
     FT_ULong  I = (FT_ULong)args[0];                           \
                                                                \
                                                                \
     if ( BOUNDS( I, CUR.cvtSize ) )                            \
     {                                                          \
       if ( CUR.pedantic_hinting )                              \
       {                                                        \
         ARRAY_BOUND_ERROR;                                     \
       }                                                        \
     }                                                          \
     else                                                       \
       CUR.cvt[I] = TT_MULFIX( args[1], CUR.tt_metrics.scale ); \
   }


#define DO_DEBUG                     \
    CUR.error = TT_Err_Debug_OpCode;


#define DO_ROUND                                                   \
    args[0] = CUR_Func_round(                                      \
                args[0],                                           \
                CUR.tt_metrics.compensations[CUR.opcode - 0x68] );


#define DO_NROUND                                                            \
    args[0] = ROUND_None( args[0],                                           \
                          CUR.tt_metrics.compensations[CUR.opcode - 0x6C] );


#define DO_MAX               \
    if ( args[1] > args[0] ) \
      args[0] = args[1];


#define DO_MIN               \
    if ( args[1] < args[0] ) \
      args[0] = args[1];


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR                   \
    {                                       \
      CUR.error = TT_Err_Invalid_Reference; \
      return;                               \
    }


  /*************************************************************************/
  /*                                                                       */
  /* SVTCA[a]:     Set (F and P) Vectors to Coordinate Axis                */
  /* Opcode range: 0x00-0x01                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SVTCA( INS_ARG )
  {
    DO_SVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVTCA[a]:    Set PVector to Coordinate Axis                          */
  /* Opcode range: 0x02-0x03                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SPVTCA( INS_ARG )
  {
    DO_SPVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTCA[a]:    Set FVector to Coordinate Axis                          */
  /* Opcode range: 0x04-0x05                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SFVTCA( INS_ARG )
  {
    DO_SFVTCA
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVTL[a]:     Set PVector To Line                                     */
  /* Opcode range: 0x06-0x07                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SPVTL( INS_ARG )
  {
    DO_SPVTL
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTL[a]:     Set FVector To Line                                     */
  /* Opcode range: 0x08-0x09                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SFVTL( INS_ARG )
  {
    DO_SFVTL
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVTPV[]:     Set FVector To PVector                                  */
  /* Opcode range: 0x0E                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_SFVTPV( INS_ARG )
  {
    DO_SFVTPV
  }


  /*************************************************************************/
  /*                                                                       */
  /* SPVFS[]:      Set PVector From Stack                                  */
  /* Opcode range: 0x0A                                                    */
  /* Stack:        f2.14 f2.14 -->                                         */
  /*                                                                       */
  static void
  Ins_SPVFS( INS_ARG )
  {
    DO_SPVFS
  }


  /*************************************************************************/
  /*                                                                       */
  /* SFVFS[]:      Set FVector From Stack                                  */
  /* Opcode range: 0x0B                                                    */
  /* Stack:        f2.14 f2.14 -->                                         */
  /*                                                                       */
  static void
  Ins_SFVFS( INS_ARG )
  {
    DO_SFVFS
  }


  /*************************************************************************/
  /*                                                                       */
  /* GPV[]:        Get Projection Vector                                   */
  /* Opcode range: 0x0C                                                    */
  /* Stack:        ef2.14 --> ef2.14                                       */
  /*                                                                       */
  static void
  Ins_GPV( INS_ARG )
  {
    DO_GPV
  }


  /*************************************************************************/
  /* GFV[]:        Get Freedom Vector                                      */
  /* Opcode range: 0x0D                                                    */
  /* Stack:        ef2.14 --> ef2.14                                       */
  /*                                                                       */
  static void
  Ins_GFV( INS_ARG )
  {
    DO_GFV
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP0[]:       Set Reference Point 0                                   */
  /* Opcode range: 0x10                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP0( INS_ARG )
  {
    DO_SRP0
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP1[]:       Set Reference Point 1                                   */
  /* Opcode range: 0x11                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP1( INS_ARG )
  {
    DO_SRP1
  }


  /*************************************************************************/
  /*                                                                       */
  /* SRP2[]:       Set Reference Point 2                                   */
  /* Opcode range: 0x12                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SRP2( INS_ARG )
  {
    DO_SRP2
  }


  /*************************************************************************/
  /*                                                                       */
  /* RTHG[]:       Round To Half Grid                                      */
  /* Opcode range: 0x19                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTHG( INS_ARG )
  {
    DO_RTHG
  }


  /*************************************************************************/
  /*                                                                       */
  /* RTG[]:        Round To Grid                                           */
  /* Opcode range: 0x18                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTG( INS_ARG )
  {
    DO_RTG
  }


  /*************************************************************************/
  /* RTDG[]:       Round To Double Grid                                    */
  /* Opcode range: 0x3D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RTDG( INS_ARG )
  {
    DO_RTDG
  }


  /*************************************************************************/
  /* RUTG[]:       Round Up To Grid                                        */
  /* Opcode range: 0x7C                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RUTG( INS_ARG )
  {
    DO_RUTG
  }


  /*************************************************************************/
  /*                                                                       */
  /* RDTG[]:       Round Down To Grid                                      */
  /* Opcode range: 0x7D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_RDTG( INS_ARG )
  {
    DO_RDTG
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROFF[]:       Round OFF                                               */
  /* Opcode range: 0x7A                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ROFF( INS_ARG )
  {
    DO_ROFF
  }


  /*************************************************************************/
  /*                                                                       */
  /* SROUND[]:     Super ROUND                                             */
  /* Opcode range: 0x76                                                    */
  /* Stack:        Eint8 -->                                               */
  /*                                                                       */
  static void
  Ins_SROUND( INS_ARG )
  {
    DO_SROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* S45ROUND[]:   Super ROUND 45 degrees                                  */
  /* Opcode range: 0x77                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_S45ROUND( INS_ARG )
  {
    DO_S45ROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* SLOOP[]:      Set LOOP variable                                       */
  /* Opcode range: 0x17                                                    */
  /* Stack:        int32? -->                                              */
  /*                                                                       */
  static void
  Ins_SLOOP( INS_ARG )
  {
    DO_SLOOP
  }


  /*************************************************************************/
  /*                                                                       */
  /* SMD[]:        Set Minimum Distance                                    */
  /* Opcode range: 0x1A                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SMD( INS_ARG )
  {
    DO_SMD
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCVTCI[]:     Set Control Value Table Cut In                          */
  /* Opcode range: 0x1D                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SCVTCI( INS_ARG )
  {
    DO_SCVTCI
  }


  /*************************************************************************/
  /*                                                                       */
  /* SSWCI[]:      Set Single Width Cut In                                 */
  /* Opcode range: 0x1E                                                    */
  /* Stack:        f26.6 -->                                               */
  /*                                                                       */
  static void
  Ins_SSWCI( INS_ARG )
  {
    DO_SSWCI
  }


  /*************************************************************************/
  /*                                                                       */
  /* SSW[]:        Set Single Width                                        */
  /* Opcode range: 0x1F                                                    */
  /* Stack:        int32? -->                                              */
  /*                                                                       */
  static void
  Ins_SSW( INS_ARG )
  {
    DO_SSW
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPON[]:     Set auto-FLIP to ON                                     */
  /* Opcode range: 0x4D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_FLIPON( INS_ARG )
  {
    DO_FLIPON
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPOFF[]:    Set auto-FLIP to OFF                                    */
  /* Opcode range: 0x4E                                                    */
  /* Stack: -->                                                            */
  /*                                                                       */
  static void
  Ins_FLIPOFF( INS_ARG )
  {
    DO_FLIPOFF
  }


  /*************************************************************************/
  /*                                                                       */
  /* SANGW[]:      Set ANGle Weight                                        */
  /* Opcode range: 0x7E                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SANGW( INS_ARG )
  {
    /* instruction not supported anymore */
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDB[]:        Set Delta Base                                          */
  /* Opcode range: 0x5E                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SDB( INS_ARG )
  {
    DO_SDB
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDS[]:        Set Delta Shift                                         */
  /* Opcode range: 0x5F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SDS( INS_ARG )
  {
    DO_SDS
  }


  /*************************************************************************/
  /*                                                                       */
  /* MPPEM[]:      Measure Pixel Per EM                                    */
  /* Opcode range: 0x4B                                                    */
  /* Stack:        --> Euint16                                             */
  /*                                                                       */
  static void
  Ins_MPPEM( INS_ARG )
  {
    DO_MPPEM
  }


  /*************************************************************************/
  /*                                                                       */
  /* MPS[]:        Measure Point Size                                      */
  /* Opcode range: 0x4C                                                    */
  /* Stack:        --> Euint16                                             */
  /*                                                                       */
  static void
  Ins_MPS( INS_ARG )
  {
    DO_MPS
  }


  /*************************************************************************/
  /*                                                                       */
  /* DUP[]:        DUPlicate the top stack's element                       */
  /* Opcode range: 0x20                                                    */
  /* Stack:        StkElt --> StkElt StkElt                                */
  /*                                                                       */
  static void
  Ins_DUP( INS_ARG )
  {
    DO_DUP
  }


  /*************************************************************************/
  /*                                                                       */
  /* POP[]:        POP the stack's top element                             */
  /* Opcode range: 0x21                                                    */
  /* Stack:        StkElt -->                                              */
  /*                                                                       */
  static void
  Ins_POP( INS_ARG )
  {
    /* nothing to do */
  }


  /*************************************************************************/
  /*                                                                       */
  /* CLEAR[]:      CLEAR the entire stack                                  */
  /* Opcode range: 0x22                                                    */
  /* Stack:        StkElt... -->                                           */
  /*                                                                       */
  static void
  Ins_CLEAR( INS_ARG )
  {
    DO_CLEAR
  }


  /*************************************************************************/
  /*                                                                       */
  /* SWAP[]:       SWAP the stack's top two elements                       */
  /* Opcode range: 0x23                                                    */
  /* Stack:        2 * StkElt --> 2 * StkElt                               */
  /*                                                                       */
  static void
  Ins_SWAP( INS_ARG )
  {
    DO_SWAP
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEPTH[]:      return the stack DEPTH                                  */
  /* Opcode range: 0x24                                                    */
  /* Stack:        --> uint32                                              */
  /*                                                                       */
  static void
  Ins_DEPTH( INS_ARG )
  {
    DO_DEPTH
  }


  /*************************************************************************/
  /*                                                                       */
  /* CINDEX[]:     Copy INDEXed element                                    */
  /* Opcode range: 0x25                                                    */
  /* Stack:        int32 --> StkElt                                        */
  /*                                                                       */
  static void
  Ins_CINDEX( INS_ARG )
  {
    DO_CINDEX
  }


  /*************************************************************************/
  /*                                                                       */
  /* EIF[]:        End IF                                                  */
  /* Opcode range: 0x59                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_EIF( INS_ARG )
  {
    /* nothing to do */
  }


  /*************************************************************************/
  /*                                                                       */
  /* JROT[]:       Jump Relative On True                                   */
  /* Opcode range: 0x78                                                    */
  /* Stack:        StkElt int32 -->                                        */
  /*                                                                       */
  static void
  Ins_JROT( INS_ARG )
  {
    DO_JROT
  }


  /*************************************************************************/
  /*                                                                       */
  /* JMPR[]:       JuMP Relative                                           */
  /* Opcode range: 0x1C                                                    */
  /* Stack:        int32 -->                                               */
  /*                                                                       */
  static void
  Ins_JMPR( INS_ARG )
  {
    DO_JMPR
  }


  /*************************************************************************/
  /*                                                                       */
  /* JROF[]:       Jump Relative On False                                  */
  /* Opcode range: 0x79                                                    */
  /* Stack:        StkElt int32 -->                                        */
  /*                                                                       */
  static void
  Ins_JROF( INS_ARG )
  {
    DO_JROF
  }


  /*************************************************************************/
  /*                                                                       */
  /* LT[]:         Less Than                                               */
  /* Opcode range: 0x50                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_LT( INS_ARG )
  {
    DO_LT
  }


  /*************************************************************************/
  /*                                                                       */
  /* LTEQ[]:       Less Than or EQual                                      */
  /* Opcode range: 0x51                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_LTEQ( INS_ARG )
  {
    DO_LTEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* GT[]:         Greater Than                                            */
  /* Opcode range: 0x52                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_GT( INS_ARG )
  {
    DO_GT
  }


  /*************************************************************************/
  /*                                                                       */
  /* GTEQ[]:       Greater Than or EQual                                   */
  /* Opcode range: 0x53                                                    */
  /* Stack:        int32? int32? --> bool                                  */
  /*                                                                       */
  static void
  Ins_GTEQ( INS_ARG )
  {
    DO_GTEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* EQ[]:         EQual                                                   */
  /* Opcode range: 0x54                                                    */
  /* Stack:        StkElt StkElt --> bool                                  */
  /*                                                                       */
  static void
  Ins_EQ( INS_ARG )
  {
    DO_EQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* NEQ[]:        Not EQual                                               */
  /* Opcode range: 0x55                                                    */
  /* Stack:        StkElt StkElt --> bool                                  */
  /*                                                                       */
  static void
  Ins_NEQ( INS_ARG )
  {
    DO_NEQ
  }


  /*************************************************************************/
  /*                                                                       */
  /* ODD[]:        Is ODD                                                  */
  /* Opcode range: 0x56                                                    */
  /* Stack:        f26.6 --> bool                                          */
  /*                                                                       */
  static void
  Ins_ODD( INS_ARG )
  {
    DO_ODD
  }


  /*************************************************************************/
  /*                                                                       */
  /* EVEN[]:       Is EVEN                                                 */
  /* Opcode range: 0x57                                                    */
  /* Stack:        f26.6 --> bool                                          */
  /*                                                                       */
  static void
  Ins_EVEN( INS_ARG )
  {
    DO_EVEN
  }


  /*************************************************************************/
  /*                                                                       */
  /* AND[]:        logical AND                                             */
  /* Opcode range: 0x5A                                                    */
  /* Stack:        uint32 uint32 --> uint32                                */
  /*                                                                       */
  static void
  Ins_AND( INS_ARG )
  {
    DO_AND
  }


  /*************************************************************************/
  /*                                                                       */
  /* OR[]:         logical OR                                              */
  /* Opcode range: 0x5B                                                    */
  /* Stack:        uint32 uint32 --> uint32                                */
  /*                                                                       */
  static void
  Ins_OR( INS_ARG )
  {
    DO_OR
  }


  /*************************************************************************/
  /*                                                                       */
  /* NOT[]:        logical NOT                                             */
  /* Opcode range: 0x5C                                                    */
  /* Stack:        StkElt --> uint32                                       */
  /*                                                                       */
  static void
  Ins_NOT( INS_ARG )
  {
    DO_NOT
  }


  /*************************************************************************/
  /*                                                                       */
  /* ADD[]:        ADD                                                     */
  /* Opcode range: 0x60                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_ADD( INS_ARG )
  {
    DO_ADD
  }


  /*************************************************************************/
  /*                                                                       */
  /* SUB[]:        SUBtract                                                */
  /* Opcode range: 0x61                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_SUB( INS_ARG )
  {
    DO_SUB
  }


  /*************************************************************************/
  /*                                                                       */
  /* DIV[]:        DIVide                                                  */
  /* Opcode range: 0x62                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_DIV( INS_ARG )
  {
    DO_DIV
  }


  /*************************************************************************/
  /*                                                                       */
  /* MUL[]:        MULtiply                                                */
  /* Opcode range: 0x63                                                    */
  /* Stack:        f26.6 f26.6 --> f26.6                                   */
  /*                                                                       */
  static void
  Ins_MUL( INS_ARG )
  {
    DO_MUL
  }


  /*************************************************************************/
  /*                                                                       */
  /* ABS[]:        ABSolute value                                          */
  /* Opcode range: 0x64                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_ABS( INS_ARG )
  {
    DO_ABS
  }


  /*************************************************************************/
  /*                                                                       */
  /* NEG[]:        NEGate                                                  */
  /* Opcode range: 0x65                                                    */
  /* Stack: f26.6 --> f26.6                                                */
  /*                                                                       */
  static void
  Ins_NEG( INS_ARG )
  {
    DO_NEG
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLOOR[]:      FLOOR                                                   */
  /* Opcode range: 0x66                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_FLOOR( INS_ARG )
  {
    DO_FLOOR
  }


  /*************************************************************************/
  /*                                                                       */
  /* CEILING[]:    CEILING                                                 */
  /* Opcode range: 0x67                                                    */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_CEILING( INS_ARG )
  {
    DO_CEILING
  }


  /*************************************************************************/
  /*                                                                       */
  /* RS[]:         Read Store                                              */
  /* Opcode range: 0x43                                                    */
  /* Stack:        uint32 --> uint32                                       */
  /*                                                                       */
  static void
  Ins_RS( INS_ARG )
  {
    DO_RS
  }


  /*************************************************************************/
  /*                                                                       */
  /* WS[]:         Write Store                                             */
  /* Opcode range: 0x42                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_WS( INS_ARG )
  {
    DO_WS
  }


  /*************************************************************************/
  /*                                                                       */
  /* WCVTP[]:      Write CVT in Pixel units                                */
  /* Opcode range: 0x44                                                    */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  static void
  Ins_WCVTP( INS_ARG )
  {
    DO_WCVTP
  }


  /*************************************************************************/
  /*                                                                       */
  /* WCVTF[]:      Write CVT in Funits                                     */
  /* Opcode range: 0x70                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_WCVTF( INS_ARG )
  {
    DO_WCVTF
  }


  /*************************************************************************/
  /*                                                                       */
  /* RCVT[]:       Read CVT                                                */
  /* Opcode range: 0x45                                                    */
  /* Stack:        uint32 --> f26.6                                        */
  /*                                                                       */
  static void
  Ins_RCVT( INS_ARG )
  {
    DO_RCVT
  }


  /*************************************************************************/
  /*                                                                       */
  /* AA[]:         Adjust Angle                                            */
  /* Opcode range: 0x7F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_AA( INS_ARG )
  {
    /* intentionally no longer supported */
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEBUG[]:      DEBUG.  Unsupported.                                    */
  /* Opcode range: 0x4F                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  /* Note: The original instruction pops a value from the stack.           */
  /*                                                                       */
  static void
  Ins_DEBUG( INS_ARG )
  {
    DO_DEBUG
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROUND[ab]:    ROUND value                                             */
  /* Opcode range: 0x68-0x6B                                               */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_ROUND( INS_ARG )
  {
    DO_ROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* NROUND[ab]:   No ROUNDing of value                                    */
  /* Opcode range: 0x6C-0x6F                                               */
  /* Stack:        f26.6 --> f26.6                                         */
  /*                                                                       */
  static void
  Ins_NROUND( INS_ARG )
  {
    DO_NROUND
  }


  /*************************************************************************/
  /*                                                                       */
  /* MAX[]:        MAXimum                                                 */
  /* Opcode range: 0x68                                                    */
  /* Stack:        int32? int32? --> int32                                 */
  /*                                                                       */
  static void
  Ins_MAX( INS_ARG )
  {
    DO_MAX
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIN[]:        MINimum                                                 */
  /* Opcode range: 0x69                                                    */
  /* Stack:        int32? int32? --> int32                                 */
  /*                                                                       */
  static void
  Ins_MIN( INS_ARG )
  {
    DO_MIN
  }


#endif  /* !TT_CONFIG_OPTION_INTERPRETER_SWITCH */


  /*************************************************************************/
  /*                                                                       */
  /* The following functions are called as is within the switch statement. */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* MINDEX[]:     Move INDEXed element                                    */
  /* Opcode range: 0x26                                                    */
  /* Stack:        int32? --> StkElt                                       */
  /*                                                                       */
  static void
  Ins_MINDEX( INS_ARG )
  {
    FT_Long  L, K;


    L = args[0];

    if ( L <= 0 || L > CUR.args )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR.stack[CUR.args - L];

    MEM_Move( &CUR.stack[CUR.args - L    ],
              &CUR.stack[CUR.args - L + 1],
              ( L - 1 ) * sizeof ( FT_Long ) );

    CUR.stack[CUR.args - 1] = K;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ROLL[]:       ROLL top three elements                                 */
  /* Opcode range: 0x8A                                                    */
  /* Stack:        3 * StkElt --> 3 * StkElt                               */
  /*                                                                       */
  static void
  Ins_ROLL( INS_ARG )
  {
    FT_Long  A, B, C;

    FT_UNUSED_EXEC;


    A = args[2];
    B = args[1];
    C = args[0];

    args[2] = C;
    args[1] = A;
    args[0] = B;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING THE FLOW OF CONTROL                                          */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  static FT_Bool
  SkipCode( EXEC_OP )
  {
    CUR.IP += CUR.length;

    if ( CUR.IP < CUR.codeSize )
    {
      CUR.opcode = CUR.code[CUR.IP];

      CUR.length = opcode_length[CUR.opcode];
      if ( CUR.length < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto Fail_Overflow;
        CUR.length = CUR.code[CUR.IP + 1] + 2;
      }

      if ( CUR.IP + CUR.length <= CUR.codeSize )
        return SUCCESS;
    }

  Fail_Overflow:
    CUR.error = TT_Err_Code_Overflow;
    return FAILURE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* IF[]:         IF test                                                 */
  /* Opcode range: 0x58                                                    */
  /* Stack:        StkElt -->                                              */
  /*                                                                       */
  static void
  Ins_IF( INS_ARG )
  {
    FT_Int   nIfs;
    FT_Bool  Out;


    if ( args[0] != 0 )
      return;

    nIfs = 1;
    Out = 0;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:      /* IF */
        nIfs++;
        break;

      case 0x1B:      /* ELSE */
        Out = FT_BOOL( nIfs == 1 );
        break;

      case 0x59:      /* EIF */
        nIfs--;
        Out = FT_BOOL( nIfs == 0 );
        break;
      }
    } while ( Out == 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* ELSE[]:       ELSE                                                    */
  /* Opcode range: 0x1B                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ELSE( INS_ARG )
  {
    FT_Int  nIfs;

    FT_UNUSED_ARG;


    nIfs = 1;

    do
    {
      if ( SKIP_Code() == FAILURE )
        return;

      switch ( CUR.opcode )
      {
      case 0x58:    /* IF */
        nIfs++;
        break;

      case 0x59:    /* EIF */
        nIfs--;
        break;
      }
    } while ( nIfs != 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* DEFINING AND USING FUNCTIONS AND INSTRUCTIONS                         */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* FDEF[]:       Function DEFinition                                     */
  /* Opcode range: 0x2C                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_FDEF( INS_ARG )
  {
    FT_ULong       n;
    TT_DefRecord*  rec;
    TT_DefRecord*  limit;


    /* some font programs are broken enough to redefine functions! */
    /* We will then parse the current table.                       */

    rec   = CUR.FDefs;
    limit = rec + CUR.numFDefs;
    n     = args[0];

    for ( ; rec < limit; rec++ )
    {
      if ( rec->opc == n )
        break;
    }

    if ( rec == limit )
    {
      /* check that there is enough room for new functions */
      if ( CUR.numFDefs >= CUR.maxFDefs )
      {
        CUR.error = TT_Err_Too_Many_Function_Defs;
        return;
      }
      CUR.numFDefs++;
    }

    rec->range  = CUR.curRange;
    rec->opc    = n;
    rec->start  = CUR.IP + 1;
    rec->active = TRUE;

    if ( n > CUR.maxFunc )
      CUR.maxFunc = n;

    /* Now skip the whole function definition. */
    /* We don't allow nested IDEFS & FDEFs.    */

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:    /* IDEF */
      case 0x2C:    /* FDEF */
        CUR.error = TT_Err_Nested_DEFS;
        return;

      case 0x2D:   /* ENDF */
        return;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* ENDF[]:       END Function definition                                 */
  /* Opcode range: 0x2D                                                    */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_ENDF( INS_ARG )
  {
    TT_CallRec*  pRec;

    FT_UNUSED_ARG;


    if ( CUR.callTop <= 0 )     /* We encountered an ENDF without a call */
    {
      CUR.error = TT_Err_ENDF_In_Exec_Stream;
      return;
    }

    CUR.callTop--;

    pRec = &CUR.callStack[CUR.callTop];

    pRec->Cur_Count--;

    CUR.step_ins = FALSE;

    if ( pRec->Cur_Count > 0 )
    {
      CUR.callTop++;
      CUR.IP = pRec->Cur_Restart;
    }
    else
      /* Loop through the current function */
      INS_Goto_CodeRange( pRec->Caller_Range,
                          pRec->Caller_IP );

    /* Exit the current call frame.                      */

    /* NOTE: If the last intruction of a program is a    */
    /*       CALL or LOOPCALL, the return address is     */
    /*       always out of the code range.  This is a    */
    /*       valid address, and it is why we do not test */
    /*       the result of Ins_Goto_CodeRange() here!    */
  }


  /*************************************************************************/
  /*                                                                       */
  /* CALL[]:       CALL function                                           */
  /* Opcode range: 0x2B                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_CALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    /* first of all, check the index */

    F = args[0];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    /* Except for some old Apple fonts, all functions in a TrueType */
    /* font are defined in increasing order, starting from 0.  This */
    /* means that we normally have                                  */
    /*                                                              */
    /*    CUR.maxFunc+1 == CUR.numFDefs                             */
    /*    CUR.FDefs[n].opc == n for n in 0..CUR.maxFunc             */
    /*                                                              */
    /* If this isn't true, we need to look up the function table.   */

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      /* look up the FDefs table */
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    /* check that the function is active */
    if ( !def->active )
      goto Fail;

    /* check the call stack */
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    pCrec = CUR.callStack + CUR.callTop;

    pCrec->Caller_Range = CUR.curRange;
    pCrec->Caller_IP    = CUR.IP + 1;
    pCrec->Cur_Count    = 1;
    pCrec->Cur_Restart  = def->start;

    CUR.callTop++;

    INS_Goto_CodeRange( def->range,
                        def->start );

    CUR.step_ins = FALSE;
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  /*************************************************************************/
  /*                                                                       */
  /* LOOPCALL[]:   LOOP and CALL function                                  */
  /* Opcode range: 0x2A                                                    */
  /* Stack:        uint32? Eint16? -->                                     */
  /*                                                                       */
  static void
  Ins_LOOPCALL( INS_ARG )
  {
    FT_ULong       F;
    TT_CallRec*    pCrec;
    TT_DefRecord*  def;


    /* first of all, check the index */
    F = args[1];
    if ( BOUNDS( F, CUR.maxFunc + 1 ) )
      goto Fail;

    /* Except for some old Apple fonts, all functions in a TrueType */
    /* font are defined in increasing order, starting from 0.  This */
    /* means that we normally have                                  */
    /*                                                              */
    /*    CUR.maxFunc+1 == CUR.numFDefs                             */
    /*    CUR.FDefs[n].opc == n for n in 0..CUR.maxFunc             */
    /*                                                              */
    /* If this isn't true, we need to look up the function table.   */

    def = CUR.FDefs + F;
    if ( CUR.maxFunc + 1 != CUR.numFDefs || def->opc != F )
    {
      /* look up the FDefs table */
      TT_DefRecord*  limit;


      def   = CUR.FDefs;
      limit = def + CUR.numFDefs;

      while ( def < limit && def->opc != F )
        def++;

      if ( def == limit )
        goto Fail;
    }

    /* check that the function is active */
    if ( !def->active )
      goto Fail;

    /* check stack */
    if ( CUR.callTop >= CUR.callSize )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    if ( args[0] > 0 )
    {
      pCrec = CUR.callStack + CUR.callTop;

      pCrec->Caller_Range = CUR.curRange;
      pCrec->Caller_IP    = CUR.IP + 1;
      pCrec->Cur_Count    = (FT_Int)args[0];
      pCrec->Cur_Restart  = def->start;

      CUR.callTop++;

      INS_Goto_CodeRange( def->range, def->start );

      CUR.step_ins = FALSE;
    }
    return;

  Fail:
    CUR.error = TT_Err_Invalid_Reference;
  }


  /*************************************************************************/
  /*                                                                       */
  /* IDEF[]:       Instruction DEFinition                                  */
  /* Opcode range: 0x89                                                    */
  /* Stack:        Eint8 -->                                               */
  /*                                                                       */
  static void
  Ins_IDEF( INS_ARG )
  {
    TT_DefRecord*  def;
    TT_DefRecord*  limit;


    /*  First of all, look for the same function in our table */

    def   = CUR.IDefs;
    limit = def + CUR.numIDefs;

    for ( ; def < limit; def++ )
      if ( def->opc == (FT_ULong)args[0] )
        break;

    if ( def == limit )
    {
      /* check that there is enough room for a new instruction */
      if ( CUR.numIDefs >= CUR.maxIDefs )
      {
        CUR.error = TT_Err_Too_Many_Instruction_Defs;
        return;
      }
      CUR.numIDefs++;
    }

    def->opc    = args[0];
    def->start  = CUR.IP+1;
    def->range  = CUR.curRange;
    def->active = TRUE;

    if ( (FT_ULong)args[0] > CUR.maxIns )
      CUR.maxIns = args[0];

    /* Now skip the whole function definition. */
    /* We don't allow nested IDEFs & FDEFs.    */

    while ( SKIP_Code() == SUCCESS )
    {
      switch ( CUR.opcode )
      {
      case 0x89:   /* IDEF */
      case 0x2C:   /* FDEF */
        CUR.error = TT_Err_Nested_DEFS;
        return;
      case 0x2D:   /* ENDF */
        return;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHING DATA ONTO THE INTERPRETER STACK                               */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* NPUSHB[]:     PUSH N Bytes                                            */
  /* Opcode range: 0x40                                                    */
  /* Stack:        --> uint32...                                           */
  /*                                                                       */
  static void
  Ins_NPUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K + 1];

    CUR.new_top += L;
  }


  /*************************************************************************/
  /*                                                                       */
  /* NPUSHW[]:     PUSH N Words                                            */
  /* Opcode range: 0x41                                                    */
  /* Stack:        --> int32...                                            */
  /*                                                                       */
  static void
  Ins_NPUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)CUR.code[CUR.IP + 1];

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP += 2;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
    CUR.new_top += L;
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHB[abc]:   PUSH Bytes                                              */
  /* Opcode range: 0xB0-0xB7                                               */
  /* Stack:        --> uint32...                                           */
  /*                                                                       */
  static void
  Ins_PUSHB( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)(CUR.opcode - 0xB0 + 1);

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    for ( K = 1; K <= L; K++ )
      args[K - 1] = CUR.code[CUR.IP + K];
  }


  /*************************************************************************/
  /*                                                                       */
  /* PUSHW[abc]:   PUSH Words                                              */
  /* Opcode range: 0xB8-0xBF                                               */
  /* Stack:        --> int32...                                            */
  /*                                                                       */
  static void
  Ins_PUSHW( INS_ARG )
  {
    FT_UShort  L, K;


    L = (FT_UShort)(CUR.opcode - 0xB8 + 1);

    if ( BOUNDS( L, CUR.stackSize + 1 - CUR.top ) )
    {
      CUR.error = TT_Err_Stack_Overflow;
      return;
    }

    CUR.IP++;

    for ( K = 0; K < L; K++ )
      args[K] = GET_ShortIns();

    CUR.step_ins = FALSE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING THE GRAPHICS STATE                                           */
  /*                                                                       */
  /*  Instructions appear in the specs' order.                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* GC[a]:        Get Coordinate projected onto                           */
  /* Opcode range: 0x46-0x47                                               */
  /* Stack:        uint32 --> f26.6                                        */
  /*                                                                       */
  /* BULLSHIT: Measures from the original glyph must be taken along the    */
  /*           dual projection vector!                                     */
  /*                                                                       */
  static void
  Ins_GC( INS_ARG )
  {
    FT_ULong    L;
    FT_F26Dot6  R;


    L = (FT_ULong)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      else
        R = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        R = CUR_Func_dualproj( CUR.zp2.org + L, NULL_Vector );
      else
        R = CUR_Func_project( CUR.zp2.cur + L, NULL_Vector );
    }

    args[0] = R;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCFS[]:       Set Coordinate From Stack                               */
  /* Opcode range: 0x48                                                    */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  /* Formula:                                                              */
  /*                                                                       */
  /*   OA := OA + ( value - OA.p )/( f.p ) * f                             */
  /*                                                                       */
  static void
  Ins_SCFS( INS_ARG )
  {
    FT_Long    K;
    FT_UShort  L;


    L = (FT_UShort)args[0];

    if ( BOUNDS( L, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    K = CUR_Func_project( CUR.zp2.cur + L, NULL_Vector );

    CUR_Func_move( &CUR.zp2, L, args[1] - K );

    /* not part of the specs, but here for safety */

    if ( CUR.GS.gep2 == 0 )
      CUR.zp2.org[L] = CUR.zp2.cur[L];
  }


  /*************************************************************************/
  /*                                                                       */
  /* MD[a]:        Measure Distance                                        */
  /* Opcode range: 0x49-0x4A                                               */
  /* Stack:        uint32 uint32 --> f26.6                                 */
  /*                                                                       */
  /* BULLSHIT: Measure taken in the original glyph must be along the dual  */
  /*           projection vector.                                          */
  /*                                                                       */
  /* Second BULLSHIT: Flag attributes are inverted!                        */
  /*                  0 => measure distance in original outline            */
  /*                  1 => measure distance in grid-fitted outline         */
  /*                                                                       */
  /* Third one: `zp0 - zp1', and not `zp2 - zp1!                           */
  /*                                                                       */
  static void
  Ins_MD( INS_ARG )
  {
    FT_UShort   K, L;
    FT_F26Dot6  D;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if( BOUNDS( L, CUR.zp0.n_points ) ||
        BOUNDS( K, CUR.zp1.n_points ) )
    {
      if ( CUR.pedantic_hinting )
      {
        CUR.error = TT_Err_Invalid_Reference;
        return;
      }
      D = 0;
    }
    else
    {
      if ( CUR.opcode & 1 )
        D = CUR_Func_project( CUR.zp0.cur + L, CUR.zp1.cur + K );
      else
        D = CUR_Func_dualproj( CUR.zp0.org + L, CUR.zp1.org + K );
    }

    args[0] = D;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SDPVTL[a]:    Set Dual PVector to Line                                */
  /* Opcode range: 0x86-0x87                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_SDPVTL( INS_ARG )
  {
    FT_Long    A, B, C;
    FT_UShort  p1, p2;   /* was FT_Int in pas type ERROR */


    p1 = (FT_UShort)args[1];
    p2 = (FT_UShort)args[0];

    if ( BOUNDS( p2, CUR.zp1.n_points ) ||
         BOUNDS( p1, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    {
      FT_Vector* v1 = CUR.zp1.org + p2;
      FT_Vector* v2 = CUR.zp2.org + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.dualVector );

    {
      FT_Vector*  v1 = CUR.zp1.cur + p2;
      FT_Vector*  v2 = CUR.zp2.cur + p1;


      A = v1->x - v2->x;
      B = v1->y - v2->y;
    }

    if ( ( CUR.opcode & 1 ) != 0 )
    {
      C =  B;   /* counter clockwise rotation */
      B =  A;
      A = -C;
    }

    NORMalize( A, B, &CUR.GS.projVector );

    COMPUTE_Funcs();
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP0[]:       Set Zone Pointer 0                                      */
  /* Opcode range: 0x13                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP0( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep0 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP1[]:       Set Zone Pointer 1                                      */
  /* Opcode range: 0x14                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP1( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp1 = CUR.twilight;
      break;

    case 1:
      CUR.zp1 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep1 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZP2[]:       Set Zone Pointer 2                                      */
  /* Opcode range: 0x15                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZP2( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp2 = CUR.twilight;
      break;

    case 1:
      CUR.zp2 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* SZPS[]:       Set Zone PointerS                                       */
  /* Opcode range: 0x16                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SZPS( INS_ARG )
  {
    switch ( (FT_Int)args[0] )
    {
    case 0:
      CUR.zp0 = CUR.twilight;
      break;

    case 1:
      CUR.zp0 = CUR.pts;
      break;

    default:
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    CUR.zp1 = CUR.zp0;
    CUR.zp2 = CUR.zp0;

    CUR.GS.gep0 = (FT_UShort)args[0];
    CUR.GS.gep1 = (FT_UShort)args[0];
    CUR.GS.gep2 = (FT_UShort)args[0];
  }


  /*************************************************************************/
  /*                                                                       */
  /* INSTCTRL[]:   INSTruction ConTRoL                                     */
  /* Opcode range: 0x8e                                                    */
  /* Stack:        int32 int32 -->                                         */
  /*                                                                       */
  static void
  Ins_INSTCTRL( INS_ARG )
  {
    FT_Long  K, L;


    K = args[1];
    L = args[0];

    if ( K < 1 || K > 2 )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( L != 0 )
        L = K;

    CUR.GS.instruct_control = FT_BOOL(
      ( (FT_Byte)CUR.GS.instruct_control & ~(FT_Byte)K ) | (FT_Byte)L );
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCANCTRL[]:   SCAN ConTRoL                                            */
  /* Opcode range: 0x85                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_SCANCTRL( INS_ARG )
  {
    FT_Int  A;


    /* Get Threshold */
    A = (FT_Int)( args[0] & 0xFF );

    if ( A == 0xFF )
    {
      CUR.GS.scan_control = TRUE;
      return;
    }
    else if ( A == 0 )
    {
      CUR.GS.scan_control = FALSE;
      return;
    }

    A *= 64;

#if 0
    if ( (args[0] & 0x100) != 0 && CUR.metrics.pointSize <= A )
      CUR.GS.scan_control = TRUE;
#endif

    if ( (args[0] & 0x200) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = TRUE;

    if ( (args[0] & 0x400) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = TRUE;

#if 0
    if ( (args[0] & 0x800) != 0 && CUR.metrics.pointSize > A )
      CUR.GS.scan_control = FALSE;
#endif

    if ( (args[0] & 0x1000) != 0 && CUR.tt_metrics.rotated )
      CUR.GS.scan_control = FALSE;

    if ( (args[0] & 0x2000) != 0 && CUR.tt_metrics.stretched )
      CUR.GS.scan_control = FALSE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SCANTYPE[]:   SCAN TYPE                                               */
  /* Opcode range: 0x8D                                                    */
  /* Stack:        uint32? -->                                             */
  /*                                                                       */
  static void
  Ins_SCANTYPE( INS_ARG )
  {
    /* for compatibility with future enhancements, */
    /* we must ignore new modes                    */

    if ( args[0] >= 0 && args[0] <= 5 )
    {
      if ( args[0] == 3 )
        args[0] = 2;

      CUR.GS.scan_type = (FT_Int)args[0];
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* MANAGING OUTLINES                                                     */
  /*                                                                       */
  /*   Instructions appear in the specification's order.                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* FLIPPT[]:     FLIP PoinT                                              */
  /* Opcode range: 0x80                                                    */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_FLIPPT( INS_ARG )
  {
    FT_UShort  point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Too_Few_Arguments;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.pts.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        CUR.pts.tags[point] ^= FT_Curve_Tag_On;

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPRGON[]:   FLIP RanGe ON                                           */
  /* Opcode range: 0x81                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_FLIPRGON( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] |= FT_Curve_Tag_On;
  }


  /*************************************************************************/
  /*                                                                       */
  /* FLIPRGOFF:    FLIP RanGe OFF                                          */
  /* Opcode range: 0x82                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_FLIPRGOFF( INS_ARG )
  {
    FT_UShort  I, K, L;


    K = (FT_UShort)args[1];
    L = (FT_UShort)args[0];

    if ( BOUNDS( K, CUR.pts.n_points ) ||
         BOUNDS( L, CUR.pts.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    for ( I = L; I <= K; I++ )
      CUR.pts.tags[I] &= ~FT_Curve_Tag_On;
  }


  static FT_Bool
  Compute_Point_Displacement( EXEC_OP_ FT_F26Dot6*    x,
                                       FT_F26Dot6*    y,
                                       TT_GlyphZone*  zone,
                                       FT_UShort*     refp )
  {
    TT_GlyphZone  zp;
    FT_UShort     p;
    FT_F26Dot6    d;


    if ( CUR.opcode & 1 )
    {
      zp = CUR.zp0;
      p  = CUR.GS.rp1;
    }
    else
    {
      zp = CUR.zp1;
      p  = CUR.GS.rp2;
    }

    if ( BOUNDS( p, zp.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return FAILURE;
    }

    *zone = zp;
    *refp = p;

    d = CUR_Func_project( zp.cur + p, zp.org + p );

#ifdef NO_APPLE_PATENT

    *x = TT_MULDIV( d, CUR.GS.freeVector.x, 0x4000 );
    *y = TT_MULDIV( d, CUR.GS.freeVector.y, 0x4000 );

#else

    *x = TT_MULDIV( d,
                    (FT_Long)CUR.GS.freeVector.x * 0x10000L,
                    CUR.F_dot_P );
    *y = TT_MULDIV( d,
                    (FT_Long)CUR.GS.freeVector.y * 0x10000L,
                    CUR.F_dot_P );

#endif /* NO_APPLE_PATENT */

    return SUCCESS;
  }


  static void
  Move_Zp2_Point( EXEC_OP_ FT_UShort   point,
                           FT_F26Dot6  dx,
                           FT_F26Dot6  dy,
                           FT_Bool     touch )
  {
    if ( CUR.GS.freeVector.x != 0 )
    {
      CUR.zp2.cur[point].x += dx;
      if ( touch )
        CUR.zp2.tags[point] |= FT_Curve_Tag_Touch_X;
    }

    if ( CUR.GS.freeVector.y != 0 )
    {
      CUR.zp2.cur[point].y += dy;
      if ( touch )
        CUR.zp2.tags[point] |= FT_Curve_Tag_Touch_Y;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHP[a]:       SHift Point by the last point                           */
  /* Opcode range: 0x32-0x33                                               */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_SHP( INS_ARG )
  {
    TT_GlyphZone  zp;
    FT_UShort     refp;

    FT_F26Dot6    dx,
                  dy;
    FT_UShort     point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;
      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        /* XXX: UNDOCUMENTED! SHP touches the points */
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHC[a]:       SHift Contour                                           */
  /* Opcode range: 0x34-35                                                 */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SHC( INS_ARG )
  {
    TT_GlyphZone zp;
    FT_UShort    refp;
    FT_F26Dot6   dx,
                 dy;

    FT_Short     contour;
    FT_UShort    first_point, last_point, i;


    contour = (FT_UShort)args[0];

    if ( BOUNDS( contour, CUR.pts.n_contours ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( contour == 0 )
      first_point = 0;
    else
      first_point = (FT_UShort)(CUR.pts.contours[contour - 1] + 1);

    last_point = CUR.pts.contours[contour];

    /* XXX: this is probably wrong... at least it prevents memory */
    /*      corruption when zp2 is the twilight zone              */
    if ( last_point > CUR.zp2.n_points )
    {
      if ( CUR.zp2.n_points > 0 )
        last_point = (FT_UShort)(CUR.zp2.n_points - 1);
      else
        last_point = 0;
    }

    /* XXX: UNDOCUMENTED! SHC doesn't touch the points */
    for ( i = first_point; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHZ[a]:       SHift Zone                                              */
  /* Opcode range: 0x36-37                                                 */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_SHZ( INS_ARG )
  {
    TT_GlyphZone zp;
    FT_UShort    refp;
    FT_F26Dot6   dx,
                 dy;

    FT_UShort    last_point, i;


    if ( BOUNDS( args[0], 2 ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( COMPUTE_Point_Displacement( &dx, &dy, &zp, &refp ) )
      return;

    if ( CUR.zp2.n_points > 0 )
      last_point = (FT_UShort)(CUR.zp2.n_points - 1);
    else
      last_point = 0;

    /* XXX: UNDOCUMENTED! SHZ doesn't touch the points */
    for ( i = 0; i <= last_point; i++ )
    {
      if ( zp.cur != CUR.zp2.cur || refp != i )
        MOVE_Zp2_Point( i, dx, dy, FALSE );
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* SHPIX[]:      SHift points by a PIXel amount                          */
  /* Opcode range: 0x38                                                    */
  /* Stack:        f26.6 uint32... -->                                     */
  /*                                                                       */
  static void
  Ins_SHPIX( INS_ARG )
  {
    FT_F26Dot6  dx, dy;
    FT_UShort   point;


    if ( CUR.top < CUR.GS.loop + 1 )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dx = TT_MULDIV( args[0],
                    (FT_Long)CUR.GS.freeVector.x,
                    0x4000 );
    dy = TT_MULDIV( args[0],
                    (FT_Long)CUR.GS.freeVector.y,
                    0x4000 );

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
        MOVE_Zp2_Point( point, dx, dy, TRUE );

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MSIRP[a]:     Move Stack Indirect Relative Position                   */
  /* Opcode range: 0x3A-0x3B                                               */
  /* Stack:        f26.6 uint32 -->                                        */
  /*                                                                       */
  static void
  Ins_MSIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: UNDOCUMENTED! behaviour */
    if ( CUR.GS.gep0 == 0 )   /* if in twilight zone */
    {
      CUR.zp1.org[point] = CUR.zp0.org[CUR.GS.rp0];
      CUR.zp1.cur[point] = CUR.zp1.org[point];
    }

    distance = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, args[1] - distance );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( (CUR.opcode & 1) != 0 )
      CUR.GS.rp0 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MDAP[a]:      Move Direct Absolute Point                              */
  /* Opcode range: 0x2E-0x2F                                               */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_MDAP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  cur_dist,
                distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone? ?                                */
    if ( ( CUR.opcode & 1 ) != 0 )
    {
      cur_dist = CUR_Func_project( CUR.zp0.cur + point, NULL_Vector );
      distance = CUR_Func_round( cur_dist,
                                 CUR.tt_metrics.compensations[0] ) - cur_dist;
    }
    else
      distance = 0;

    CUR_Func_move( &CUR.zp0, point, distance );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIAP[a]:      Move Indirect Absolute Point                            */
  /* Opcode range: 0x3E-0x3F                                               */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_MIAP( INS_ARG )
  {
    FT_ULong    cvtEntry;
    FT_UShort   point;
    FT_F26Dot6  distance,
                org_dist;


    cvtEntry = (FT_ULong)args[1];
    point    = (FT_UShort)args[0];

    if ( BOUNDS( point,    CUR.zp0.n_points ) ||
         BOUNDS( cvtEntry, CUR.cvtSize )      )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* UNDOCUMENTED!                                     */
    /*                                                   */
    /* The behaviour of an MIAP instruction is quite     */
    /* different when used in the twilight zone.         */
    /*                                                   */
    /* First, no control value cutin test is performed   */
    /* as it would fail anyway.  Second, the original    */
    /* point, i.e. (org_x,org_y) of zp0.point, is set    */
    /* to the absolute, unrounded distance found in      */
    /* the CVT.                                          */
    /*                                                   */
    /* This is used in the CVT programs of the Microsoft */
    /* fonts Arial, Times, etc., in order to re-adjust   */
    /* some key font heights.  It allows the use of the  */
    /* IP instruction in the twilight zone, which        */
    /* otherwise would be `illegal' according to the     */
    /* specification.                                    */
    /*                                                   */
    /* We implement it with a special sequence for the   */
    /* twilight zone.  This is a bad hack, but it seems  */
    /* to work.                                          */

    distance = CUR_Func_read_cvt( cvtEntry );

    if ( CUR.GS.gep0 == 0 )   /* If in twilight zone */
    {
      CUR.zp0.org[point].x = TT_MULDIV( CUR.GS.freeVector.x,
                                        distance, 0x4000 );
      CUR.zp0.org[point].y = TT_MULDIV( CUR.GS.freeVector.y,
                                        distance, 0x4000 );
      CUR.zp0.cur[point] = CUR.zp0.org[point];
    }

    org_dist = CUR_Func_project( CUR.zp0.cur + point, NULL_Vector );

    if ( ( CUR.opcode & 1 ) != 0 )   /* rounding and control cutin flag */
    {
      if ( ABS( distance - org_dist ) > CUR.GS.control_value_cutin )
        distance = org_dist;

      distance = CUR_Func_round( distance, CUR.tt_metrics.compensations[0] );
    }

    CUR_Func_move( &CUR.zp0, point, distance - org_dist );

    CUR.GS.rp0 = point;
    CUR.GS.rp1 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MDRP[abcde]:  Move Direct Relative Point                              */
  /* Opcode range: 0xC0-0xDF                                               */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_MDRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  org_dist, distance;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: Is there some undocumented feature while in the */
    /*      twilight zone?                                  */

    org_dist = CUR_Func_dualproj( CUR.zp1.org + point,
                                  CUR.zp0.org + CUR.GS.rp0 );

    /* single width cutin test */

    if ( ABS( org_dist ) < CUR.GS.single_width_cutin )
    {
      if ( org_dist >= 0 )
        org_dist = CUR.GS.single_width_value;
      else
        org_dist = -CUR.GS.single_width_value;
    }

    /* round flag */

    if ( ( CUR.opcode & 4 ) != 0 )
      distance = CUR_Func_round(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    else
      distance = ROUND_None(
                   org_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    /* minimum distance flag */

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    /* now move the point */

    org_dist = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    CUR_Func_move( &CUR.zp1, point, distance - org_dist );

    CUR.GS.rp1 = CUR.GS.rp0;
    CUR.GS.rp2 = point;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MIRP[abcde]:  Move Indirect Relative Point                            */
  /* Opcode range: 0xE0-0xFF                                               */
  /* Stack:        int32? uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_MIRP( INS_ARG )
  {
    FT_UShort   point;
    FT_ULong    cvtEntry;

    FT_F26Dot6  cvt_dist,
                distance,
                cur_dist,
                org_dist;


    point    = (FT_UShort)args[0];
    cvtEntry = (FT_ULong)( args[1] + 1 );

    /* XXX: UNDOCUMENTED! cvt[-1] = 0 always */

    if ( BOUNDS( point,      CUR.zp1.n_points ) ||
         BOUNDS( cvtEntry,   CUR.cvtSize + 1 )  ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    if ( !cvtEntry )
      cvt_dist = 0;
    else
      cvt_dist = CUR_Func_read_cvt( cvtEntry - 1 );

    /* single width test */

    if ( ABS( cvt_dist ) < CUR.GS.single_width_cutin )
    {
      if ( cvt_dist >= 0 )
        cvt_dist =  CUR.GS.single_width_value;
      else
        cvt_dist = -CUR.GS.single_width_value;
    }

    /* XXX: UNDOCUMENTED! -- twilight zone */

    if ( CUR.GS.gep1 == 0 )
    {
      CUR.zp1.org[point].x = CUR.zp0.org[CUR.GS.rp0].x +
                             TT_MULDIV( cvt_dist,
                                        CUR.GS.freeVector.x,
                                        0x4000 );

      CUR.zp1.org[point].y = CUR.zp0.org[CUR.GS.rp0].y +
                             TT_MULDIV( cvt_dist,
                                        CUR.GS.freeVector.y,
                                        0x4000 );

      CUR.zp1.cur[point] = CUR.zp1.org[point];
    }

    org_dist = CUR_Func_dualproj( CUR.zp1.org + point,
                                  CUR.zp0.org + CUR.GS.rp0 );

    cur_dist = CUR_Func_project( CUR.zp1.cur + point,
                                 CUR.zp0.cur + CUR.GS.rp0 );

    /* auto-flip test */

    if ( CUR.GS.auto_flip )
    {
      if ( ( org_dist ^ cvt_dist ) < 0 )
        cvt_dist = -cvt_dist;
    }

    /* control value cutin and round */

    if ( ( CUR.opcode & 4 ) != 0 )
    {
      /* XXX: UNDOCUMENTED!  Only perform cut-in test when both points */
      /*      refer to the same zone.                                  */

      if ( CUR.GS.gep0 == CUR.GS.gep1 )
        if ( ABS( cvt_dist - org_dist ) >= CUR.GS.control_value_cutin )
          cvt_dist = org_dist;

      distance = CUR_Func_round(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );
    }
    else
      distance = ROUND_None(
                   cvt_dist,
                   CUR.tt_metrics.compensations[CUR.opcode & 3] );

    /* minimum distance test */

    if ( ( CUR.opcode & 8 ) != 0 )
    {
      if ( org_dist >= 0 )
      {
        if ( distance < CUR.GS.minimum_distance )
          distance = CUR.GS.minimum_distance;
      }
      else
      {
        if ( distance > -CUR.GS.minimum_distance )
          distance = -CUR.GS.minimum_distance;
      }
    }

    CUR_Func_move( &CUR.zp1, point, distance - cur_dist );

    CUR.GS.rp1 = CUR.GS.rp0;

    if ( ( CUR.opcode & 16 ) != 0 )
      CUR.GS.rp0 = point;

    /* XXX: UNDOCUMENTED! */

    CUR.GS.rp2 = point;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ALIGNRP[]:    ALIGN Relative Point                                    */
  /* Opcode range: 0x3C                                                    */
  /* Stack:        uint32 uint32... -->                                    */
  /*                                                                       */
  static void
  Ins_ALIGNRP( INS_ARG )
  {
    FT_UShort   point;
    FT_F26Dot6  distance;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop ||
         BOUNDS( CUR.GS.rp0, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];

      if ( BOUNDS( point, CUR.zp1.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        distance = CUR_Func_project( CUR.zp1.cur + point,
                                     CUR.zp0.cur + CUR.GS.rp0 );

        CUR_Func_move( &CUR.zp1, point, -distance );
      }

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* ISECT[]:      moves point to InterSECTion                             */
  /* Opcode range: 0x0F                                                    */
  /* Stack:        5 * uint32 -->                                          */
  /*                                                                       */
  static void
  Ins_ISECT( INS_ARG )
  {
    FT_UShort   point,
                a0, a1,
                b0, b1;

    FT_F26Dot6  discriminant;

    FT_F26Dot6  dx,  dy,
                dax, day,
                dbx, dby;

    FT_F26Dot6  val;

    FT_Vector   R;


    point = (FT_UShort)args[0];

    a0 = (FT_UShort)args[1];
    a1 = (FT_UShort)args[2];
    b0 = (FT_UShort)args[3];
    b1 = (FT_UShort)args[4];

    if ( BOUNDS( b0, CUR.zp0.n_points )  ||
         BOUNDS( b1, CUR.zp0.n_points )  ||
         BOUNDS( a0, CUR.zp1.n_points )  ||
         BOUNDS( a1, CUR.zp1.n_points )  ||
         BOUNDS( point, CUR.zp2.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    dbx = CUR.zp0.cur[b1].x - CUR.zp0.cur[b0].x;
    dby = CUR.zp0.cur[b1].y - CUR.zp0.cur[b0].y;

    dax = CUR.zp1.cur[a1].x - CUR.zp1.cur[a0].x;
    day = CUR.zp1.cur[a1].y - CUR.zp1.cur[a0].y;

    dx = CUR.zp0.cur[b0].x - CUR.zp1.cur[a0].x;
    dy = CUR.zp0.cur[b0].y - CUR.zp1.cur[a0].y;

    CUR.zp2.tags[point] |= FT_Curve_Tag_Touch_Both;

    discriminant = TT_MULDIV( dax, -dby, 0x40 ) +
                   TT_MULDIV( day, dbx, 0x40 );

    if ( ABS( discriminant ) >= 0x40 )
    {
      val = TT_MULDIV( dx, -dby, 0x40 ) + TT_MULDIV( dy, dbx, 0x40 );

      R.x = TT_MULDIV( val, dax, discriminant );
      R.y = TT_MULDIV( val, day, discriminant );

      CUR.zp2.cur[point].x = CUR.zp1.cur[a0].x + R.x;
      CUR.zp2.cur[point].y = CUR.zp1.cur[a0].y + R.y;
    }
    else
    {
      /* else, take the middle of the middles of A and B */

      CUR.zp2.cur[point].x = ( CUR.zp1.cur[a0].x +
                               CUR.zp1.cur[a1].x +
                               CUR.zp0.cur[b0].x +
                               CUR.zp0.cur[b1].x ) / 4;
      CUR.zp2.cur[point].y = ( CUR.zp1.cur[a0].y +
                               CUR.zp1.cur[a1].y +
                               CUR.zp0.cur[b0].y +
                               CUR.zp0.cur[b1].y ) / 4;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* ALIGNPTS[]:   ALIGN PoinTS                                            */
  /* Opcode range: 0x27                                                    */
  /* Stack:        uint32 uint32 -->                                       */
  /*                                                                       */
  static void
  Ins_ALIGNPTS( INS_ARG )
  {
    FT_UShort   p1, p2;
    FT_F26Dot6  distance;


    p1 = (FT_UShort)args[0];
    p2 = (FT_UShort)args[1];

    if ( BOUNDS( args[0], CUR.zp1.n_points ) ||
         BOUNDS( args[1], CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    distance = CUR_Func_project( CUR.zp0.cur + p2,
                                 CUR.zp1.cur + p1 ) / 2;

    CUR_Func_move( &CUR.zp1, p1, distance );
    CUR_Func_move( &CUR.zp0, p2, -distance );
  }


  /*************************************************************************/
  /*                                                                       */
  /* IP[]:         Interpolate Point                                       */
  /* Opcode range: 0x39                                                    */
  /* Stack:        uint32... -->                                           */
  /*                                                                       */
  static void
  Ins_IP( INS_ARG )
  {
    FT_F26Dot6  org_a, org_b, org_x,
                cur_a, cur_b, cur_x,
                distance;
    FT_UShort   point;

    FT_UNUSED_ARG;


    if ( CUR.top < CUR.GS.loop )
    {
      CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    /* XXX: There are some glyphs in some braindead but popular  */
    /*      fonts out there (e.g. [aeu]grave in monotype.ttf)    */
    /*      calling IP[] with bad values of rp[12].              */
    /*      Do something sane when this odd thing happens.       */

    if ( BOUNDS( CUR.GS.rp1, CUR.zp0.n_points ) ||
         BOUNDS( CUR.GS.rp2, CUR.zp1.n_points ) )
    {
      org_a = cur_a = 0;
      org_b = cur_b = 0;
    }
    else
    {
      org_a = CUR_Func_dualproj( CUR.zp0.org + CUR.GS.rp1, NULL_Vector );
      org_b = CUR_Func_dualproj( CUR.zp1.org + CUR.GS.rp2, NULL_Vector );

      cur_a = CUR_Func_project( CUR.zp0.cur + CUR.GS.rp1, NULL_Vector );
      cur_b = CUR_Func_project( CUR.zp1.cur + CUR.GS.rp2, NULL_Vector );
    }

    while ( CUR.GS.loop > 0 )
    {
      CUR.args--;

      point = (FT_UShort)CUR.stack[CUR.args];
      if ( BOUNDS( point, CUR.zp2.n_points ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        org_x = CUR_Func_dualproj( CUR.zp2.org + point, NULL_Vector );
        cur_x = CUR_Func_project ( CUR.zp2.cur + point, NULL_Vector );

        if ( ( org_a <= org_b && org_x <= org_a ) ||
             ( org_a >  org_b && org_x >= org_a ) )

          distance = ( cur_a - org_a ) + ( org_x - cur_x );

        else if ( ( org_a <= org_b  &&  org_x >= org_b ) ||
                  ( org_a >  org_b  &&  org_x <  org_b ) )

          distance = ( cur_b - org_b ) + ( org_x - cur_x );

        else
           /* note: it seems that rounding this value isn't a good */
           /*       idea (cf. width of capital `S' in Times)       */

           distance = TT_MULDIV( cur_b - cur_a,
                                 org_x - org_a,
                                 org_b - org_a ) + ( cur_a - cur_x );

        CUR_Func_move( &CUR.zp2, point, distance );
      }

      CUR.GS.loop--;
    }

    CUR.GS.loop = 1;
    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* UTP[a]:       UnTouch Point                                           */
  /* Opcode range: 0x29                                                    */
  /* Stack:        uint32 -->                                              */
  /*                                                                       */
  static void
  Ins_UTP( INS_ARG )
  {
    FT_UShort  point;
    FT_Byte    mask;


    point = (FT_UShort)args[0];

    if ( BOUNDS( point, CUR.zp0.n_points ) )
    {
      if ( CUR.pedantic_hinting )
        CUR.error = TT_Err_Invalid_Reference;
      return;
    }

    mask = 0xFF;

    if ( CUR.GS.freeVector.x != 0 )
      mask &= ~FT_Curve_Tag_Touch_X;

    if ( CUR.GS.freeVector.y != 0 )
      mask &= ~FT_Curve_Tag_Touch_Y;

    CUR.zp0.tags[point] &= mask;
  }


  /* Local variables for Ins_IUP: */
  struct  LOC_Ins_IUP
  {
    FT_Vector*  orgs;   /* original and current coordinate */
    FT_Vector*  curs;   /* arrays                          */
  };


  static void
  Shift( FT_UInt              p1,
         FT_UInt              p2,
         FT_UInt              p,
         struct LOC_Ins_IUP*  LINK )
  {
    FT_UInt     i;
    FT_F26Dot6  x;


    x = LINK->curs[p].x - LINK->orgs[p].x;

    for ( i = p1; i < p; i++ )
      LINK->curs[i].x += x;

    for ( i = p + 1; i <= p2; i++ )
      LINK->curs[i].x += x;
  }


  static void
  Interp( FT_UInt              p1,
          FT_UInt              p2,
          FT_UInt              ref1,
          FT_UInt              ref2,
          struct LOC_Ins_IUP*  LINK )
  {
    FT_UInt     i;
    FT_F26Dot6  x, x1, x2, d1, d2;


    if ( p1 > p2 )
      return;

    x1 = LINK->orgs[ref1].x;
    d1 = LINK->curs[ref1].x - LINK->orgs[ref1].x;
    x2 = LINK->orgs[ref2].x;
    d2 = LINK->curs[ref2].x - LINK->orgs[ref2].x;

    if ( x1 == x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i].x;

        if ( x <= x1 )
          x += d1;
        else
          x += d2;

        LINK->curs[i].x = x;
      }
      return;
    }

    if ( x1 < x2 )
    {
      for ( i = p1; i <= p2; i++ )
      {
        x = LINK->orgs[i].x;

        if ( x <= x1 )
          x += d1;
        else
        {
          if ( x >= x2 )
            x += d2;
          else
            x = LINK->curs[ref1].x +
                  TT_MULDIV( x - x1,
                             LINK->curs[ref2].x - LINK->curs[ref1].x,
                             x2 - x1 );
        }
        LINK->curs[i].x = x;
      }
      return;
    }

    /* x2 < x1 */

    for ( i = p1; i <= p2; i++ )
    {
      x = LINK->orgs[i].x;
      if ( x <= x2 )
        x += d2;
      else
      {
        if ( x >= x1 )
          x += d1;
        else
          x = LINK->curs[ref1].x +
              TT_MULDIV( x - x1,
                         LINK->curs[ref2].x - LINK->curs[ref1].x,
                         x2 - x1 );
      }
      LINK->curs[i].x = x;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* IUP[a]:       Interpolate Untouched Points                            */
  /* Opcode range: 0x30-0x31                                               */
  /* Stack:        -->                                                     */
  /*                                                                       */
  static void
  Ins_IUP( INS_ARG )
  {
    struct LOC_Ins_IUP  V;
    FT_Byte             mask;

    FT_UInt   first_point;   /* first point of contour        */
    FT_UInt   end_point;     /* end point (last+1) of contour */

    FT_UInt   first_touched; /* first touched point in contour   */
    FT_UInt   cur_touched;   /* current touched point in contour */

    FT_UInt   point;         /* current point   */
    FT_Short  contour;       /* current contour */

    FT_UNUSED_ARG;


    if ( CUR.opcode & 1 )
    {
      mask   = FT_Curve_Tag_Touch_X;
      V.orgs = CUR.pts.org;
      V.curs = CUR.pts.cur;
    }
    else
    {
      mask   = FT_Curve_Tag_Touch_Y;
      V.orgs = (FT_Vector*)( (FT_Pos*)CUR.pts.org + 1 );
      V.curs = (FT_Vector*)( (FT_Pos*)CUR.pts.cur + 1 );
    }

    contour = 0;
    point   = 0;

    do
    {
      end_point   = CUR.pts.contours[contour];
      first_point = point;

      while ( point <= end_point && (CUR.pts.tags[point] & mask) == 0 )
        point++;

      if ( point <= end_point )
      {
        first_touched = point;
        cur_touched   = point;

        point++;

        while ( point <= end_point )
        {
          if ( ( CUR.pts.tags[point] & mask ) != 0 )
          {
            if ( point > 0 )
              Interp( cur_touched + 1,
                      point - 1,
                      cur_touched,
                      point,
                      &V );
            cur_touched = point;
          }

          point++;
        }

        if ( cur_touched == first_touched )
          Shift( first_point, end_point, cur_touched, &V );
        else
        {
          Interp( (FT_UShort)( cur_touched + 1 ),
                  end_point,
                  cur_touched,
                  first_touched,
                  &V );

          if ( first_touched > 0 )
            Interp( first_point,
                    first_touched - 1,
                    cur_touched,
                    first_touched,
                    &V );
        }
      }
      contour++;
    } while ( contour < CUR.pts.n_contours );
  }


  /*************************************************************************/
  /*                                                                       */
  /* DELTAPn[]:    DELTA exceptions P1, P2, P3                             */
  /* Opcode range: 0x5D,0x71,0x72                                          */
  /* Stack:        uint32 (2 * uint32)... -->                              */
  /*                                                                       */
  static void
  Ins_DELTAP( INS_ARG )
  {
    FT_ULong   k, nump;
    FT_UShort  A;
    FT_ULong   C;
    FT_Long    B;


    nump = (FT_ULong)args[0];   /* some points theoretically may occur more
                                   than once, thus UShort isn't enough */

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_UShort)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      /* XXX: Because some popular fonts contain some invalid DeltaP */
      /*      instructions, we simply ignore them when the stacked   */
      /*      point reference is off limit, rather than returning an */
      /*      error.  As a delta instruction doesn't change a glyph  */
      /*      in great ways, this shouldn't be a problem.            */

      if ( !BOUNDS( A, CUR.zp0.n_points ) )
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x5D:
          break;

        case 0x71:
          C += 16;
          break;

        case 0x72:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move( &CUR.zp0, A, B );
        }
      }
      else
        if ( CUR.pedantic_hinting )
          CUR.error = TT_Err_Invalid_Reference;
    }

    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* DELTACn[]:    DELTA exceptions C1, C2, C3                             */
  /* Opcode range: 0x73,0x74,0x75                                          */
  /* Stack:        uint32 (2 * uint32)... -->                              */
  /*                                                                       */
  static void
  Ins_DELTAC( INS_ARG )
  {
    FT_ULong  nump, k;
    FT_ULong  A, C;
    FT_Long   B;


    nump = (FT_ULong)args[0];

    for ( k = 1; k <= nump; k++ )
    {
      if ( CUR.args < 2 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        return;
      }

      CUR.args -= 2;

      A = (FT_ULong)CUR.stack[CUR.args + 1];
      B = CUR.stack[CUR.args];

      if ( BOUNDS( A, CUR.cvtSize ) )
      {
        if ( CUR.pedantic_hinting )
        {
          CUR.error = TT_Err_Invalid_Reference;
          return;
        }
      }
      else
      {
        C = ( (FT_ULong)B & 0xF0 ) >> 4;

        switch ( CUR.opcode )
        {
        case 0x73:
          break;

        case 0x74:
          C += 16;
          break;

        case 0x75:
          C += 32;
          break;
        }

        C += CUR.GS.delta_base;

        if ( CURRENT_Ppem() == (FT_Long)C )
        {
          B = ( (FT_ULong)B & 0xF ) - 8;
          if ( B >= 0 )
            B++;
          B = B * 64 / ( 1L << CUR.GS.delta_shift );

          CUR_Func_move_cvt( A, B );
        }
      }
    }

    CUR.new_top = CUR.args;
  }


  /*************************************************************************/
  /*                                                                       */
  /* MISC. INSTRUCTIONS                                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* GETINFO[]:    GET INFOrmation                                         */
  /* Opcode range: 0x88                                                    */
  /* Stack:        uint32 --> uint32                                       */
  /*                                                                       */
  /* XXX: According to Apple specs, bits 1 & 2 of the argument ought to be */
  /*      consulted before rotated/stretched info is returned.             */
  static void
  Ins_GETINFO( INS_ARG )
  {
    FT_Long  K;


    K = 0;

    /* We return then Windows 3.1 version number */
    /* for the font scaler                       */
    if ( ( args[0] & 1 ) != 0 )
      K = 3;

    /* Has the glyph been rotated ? */
    if ( CUR.tt_metrics.rotated )
      K |= 0x80;

    /* Has the glyph been stretched ? */
    if ( CUR.tt_metrics.stretched )
      K |= 0x100;

    args[0] = K;
  }


  static void
  Ins_UNKNOWN( INS_ARG )
  {
    TT_DefRecord*  def   = CUR.IDefs;
    TT_DefRecord*  limit = def + CUR.numIDefs;

    FT_UNUSED_ARG;


    for ( ; def < limit; def++ )
    {
      if ( (FT_Byte)def->opc == CUR.opcode && def->active )
      {
        TT_CallRec*  call;


        if ( CUR.callTop >= CUR.callSize )
        {
          CUR.error = TT_Err_Stack_Overflow;
          return;
        }

        call = CUR.callStack + CUR.callTop++;

        call->Caller_Range = CUR.curRange;
        call->Caller_IP    = CUR.IP+1;
        call->Cur_Count    = 1;
        call->Cur_Restart  = def->start;

        INS_Goto_CodeRange( def->range, def->start );

        CUR.step_ins = FALSE;
        return;
      }
    }

    CUR.error = TT_Err_Invalid_Opcode;
  }


#ifndef TT_CONFIG_OPTION_INTERPRETER_SWITCH


  static
  TInstruction_Function  Instruct_Dispatch[256] =
  {
    /* Opcodes are gathered in groups of 16. */
    /* Please keep the spaces as they are.   */

    /*  SVTCA  y  */  Ins_SVTCA,
    /*  SVTCA  x  */  Ins_SVTCA,
    /*  SPvTCA y  */  Ins_SPVTCA,
    /*  SPvTCA x  */  Ins_SPVTCA,
    /*  SFvTCA y  */  Ins_SFVTCA,
    /*  SFvTCA x  */  Ins_SFVTCA,
    /*  SPvTL //  */  Ins_SPVTL,
    /*  SPvTL +   */  Ins_SPVTL,
    /*  SFvTL //  */  Ins_SFVTL,
    /*  SFvTL +   */  Ins_SFVTL,
    /*  SPvFS     */  Ins_SPVFS,
    /*  SFvFS     */  Ins_SFVFS,
    /*  GPV       */  Ins_GPV,
    /*  GFV       */  Ins_GFV,
    /*  SFvTPv    */  Ins_SFVTPV,
    /*  ISECT     */  Ins_ISECT,

    /*  SRP0      */  Ins_SRP0,
    /*  SRP1      */  Ins_SRP1,
    /*  SRP2      */  Ins_SRP2,
    /*  SZP0      */  Ins_SZP0,
    /*  SZP1      */  Ins_SZP1,
    /*  SZP2      */  Ins_SZP2,
    /*  SZPS      */  Ins_SZPS,
    /*  SLOOP     */  Ins_SLOOP,
    /*  RTG       */  Ins_RTG,
    /*  RTHG      */  Ins_RTHG,
    /*  SMD       */  Ins_SMD,
    /*  ELSE      */  Ins_ELSE,
    /*  JMPR      */  Ins_JMPR,
    /*  SCvTCi    */  Ins_SCVTCI,
    /*  SSwCi     */  Ins_SSWCI,
    /*  SSW       */  Ins_SSW,

    /*  DUP       */  Ins_DUP,
    /*  POP       */  Ins_POP,
    /*  CLEAR     */  Ins_CLEAR,
    /*  SWAP      */  Ins_SWAP,
    /*  DEPTH     */  Ins_DEPTH,
    /*  CINDEX    */  Ins_CINDEX,
    /*  MINDEX    */  Ins_MINDEX,
    /*  AlignPTS  */  Ins_ALIGNPTS,
    /*  INS_0x28  */  Ins_UNKNOWN,
    /*  UTP       */  Ins_UTP,
    /*  LOOPCALL  */  Ins_LOOPCALL,
    /*  CALL      */  Ins_CALL,
    /*  FDEF      */  Ins_FDEF,
    /*  ENDF      */  Ins_ENDF,
    /*  MDAP[0]   */  Ins_MDAP,
    /*  MDAP[1]   */  Ins_MDAP,

    /*  IUP[0]    */  Ins_IUP,
    /*  IUP[1]    */  Ins_IUP,
    /*  SHP[0]    */  Ins_SHP,
    /*  SHP[1]    */  Ins_SHP,
    /*  SHC[0]    */  Ins_SHC,
    /*  SHC[1]    */  Ins_SHC,
    /*  SHZ[0]    */  Ins_SHZ,
    /*  SHZ[1]    */  Ins_SHZ,
    /*  SHPIX     */  Ins_SHPIX,
    /*  IP        */  Ins_IP,
    /*  MSIRP[0]  */  Ins_MSIRP,
    /*  MSIRP[1]  */  Ins_MSIRP,
    /*  AlignRP   */  Ins_ALIGNRP,
    /*  RTDG      */  Ins_RTDG,
    /*  MIAP[0]   */  Ins_MIAP,
    /*  MIAP[1]   */  Ins_MIAP,

    /*  NPushB    */  Ins_NPUSHB,
    /*  NPushW    */  Ins_NPUSHW,
    /*  WS        */  Ins_WS,
    /*  RS        */  Ins_RS,
    /*  WCvtP     */  Ins_WCVTP,
    /*  RCvt      */  Ins_RCVT,
    /*  GC[0]     */  Ins_GC,
    /*  GC[1]     */  Ins_GC,
    /*  SCFS      */  Ins_SCFS,
    /*  MD[0]     */  Ins_MD,
    /*  MD[1]     */  Ins_MD,
    /*  MPPEM     */  Ins_MPPEM,
    /*  MPS       */  Ins_MPS,
    /*  FlipON    */  Ins_FLIPON,
    /*  FlipOFF   */  Ins_FLIPOFF,
    /*  DEBUG     */  Ins_DEBUG,

    /*  LT        */  Ins_LT,
    /*  LTEQ      */  Ins_LTEQ,
    /*  GT        */  Ins_GT,
    /*  GTEQ      */  Ins_GTEQ,
    /*  EQ        */  Ins_EQ,
    /*  NEQ       */  Ins_NEQ,
    /*  ODD       */  Ins_ODD,
    /*  EVEN      */  Ins_EVEN,
    /*  IF        */  Ins_IF,
    /*  EIF       */  Ins_EIF,
    /*  AND       */  Ins_AND,
    /*  OR        */  Ins_OR,
    /*  NOT       */  Ins_NOT,
    /*  DeltaP1   */  Ins_DELTAP,
    /*  SDB       */  Ins_SDB,
    /*  SDS       */  Ins_SDS,

    /*  ADD       */  Ins_ADD,
    /*  SUB       */  Ins_SUB,
    /*  DIV       */  Ins_DIV,
    /*  MUL       */  Ins_MUL,
    /*  ABS       */  Ins_ABS,
    /*  NEG       */  Ins_NEG,
    /*  FLOOR     */  Ins_FLOOR,
    /*  CEILING   */  Ins_CEILING,
    /*  ROUND[0]  */  Ins_ROUND,
    /*  ROUND[1]  */  Ins_ROUND,
    /*  ROUND[2]  */  Ins_ROUND,
    /*  ROUND[3]  */  Ins_ROUND,
    /*  NROUND[0] */  Ins_NROUND,
    /*  NROUND[1] */  Ins_NROUND,
    /*  NROUND[2] */  Ins_NROUND,
    /*  NROUND[3] */  Ins_NROUND,

    /*  WCvtF     */  Ins_WCVTF,
    /*  DeltaP2   */  Ins_DELTAP,
    /*  DeltaP3   */  Ins_DELTAP,
    /*  DeltaCn[0] */ Ins_DELTAC,
    /*  DeltaCn[1] */ Ins_DELTAC,
    /*  DeltaCn[2] */ Ins_DELTAC,
    /*  SROUND    */  Ins_SROUND,
    /*  S45Round  */  Ins_S45ROUND,
    /*  JROT      */  Ins_JROT,
    /*  JROF      */  Ins_JROF,
    /*  ROFF      */  Ins_ROFF,
    /*  INS_0x7B  */  Ins_UNKNOWN,
    /*  RUTG      */  Ins_RUTG,
    /*  RDTG      */  Ins_RDTG,
    /*  SANGW     */  Ins_SANGW,
    /*  AA        */  Ins_AA,

    /*  FlipPT    */  Ins_FLIPPT,
    /*  FlipRgON  */  Ins_FLIPRGON,
    /*  FlipRgOFF */  Ins_FLIPRGOFF,
    /*  INS_0x83  */  Ins_UNKNOWN,
    /*  INS_0x84  */  Ins_UNKNOWN,
    /*  ScanCTRL  */  Ins_SCANCTRL,
    /*  SDPVTL[0] */  Ins_SDPVTL,
    /*  SDPVTL[1] */  Ins_SDPVTL,
    /*  GetINFO   */  Ins_GETINFO,
    /*  IDEF      */  Ins_IDEF,
    /*  ROLL      */  Ins_ROLL,
    /*  MAX       */  Ins_MAX,
    /*  MIN       */  Ins_MIN,
    /*  ScanTYPE  */  Ins_SCANTYPE,
    /*  InstCTRL  */  Ins_INSTCTRL,
    /*  INS_0x8F  */  Ins_UNKNOWN,

    /*  INS_0x90  */   Ins_UNKNOWN,
    /*  INS_0x91  */   Ins_UNKNOWN,
    /*  INS_0x92  */   Ins_UNKNOWN,
    /*  INS_0x93  */   Ins_UNKNOWN,
    /*  INS_0x94  */   Ins_UNKNOWN,
    /*  INS_0x95  */   Ins_UNKNOWN,
    /*  INS_0x96  */   Ins_UNKNOWN,
    /*  INS_0x97  */   Ins_UNKNOWN,
    /*  INS_0x98  */   Ins_UNKNOWN,
    /*  INS_0x99  */   Ins_UNKNOWN,
    /*  INS_0x9A  */   Ins_UNKNOWN,
    /*  INS_0x9B  */   Ins_UNKNOWN,
    /*  INS_0x9C  */   Ins_UNKNOWN,
    /*  INS_0x9D  */   Ins_UNKNOWN,
    /*  INS_0x9E  */   Ins_UNKNOWN,
    /*  INS_0x9F  */   Ins_UNKNOWN,

    /*  INS_0xA0  */   Ins_UNKNOWN,
    /*  INS_0xA1  */   Ins_UNKNOWN,
    /*  INS_0xA2  */   Ins_UNKNOWN,
    /*  INS_0xA3  */   Ins_UNKNOWN,
    /*  INS_0xA4  */   Ins_UNKNOWN,
    /*  INS_0xA5  */   Ins_UNKNOWN,
    /*  INS_0xA6  */   Ins_UNKNOWN,
    /*  INS_0xA7  */   Ins_UNKNOWN,
    /*  INS_0xA8  */   Ins_UNKNOWN,
    /*  INS_0xA9  */   Ins_UNKNOWN,
    /*  INS_0xAA  */   Ins_UNKNOWN,
    /*  INS_0xAB  */   Ins_UNKNOWN,
    /*  INS_0xAC  */   Ins_UNKNOWN,
    /*  INS_0xAD  */   Ins_UNKNOWN,
    /*  INS_0xAE  */   Ins_UNKNOWN,
    /*  INS_0xAF  */   Ins_UNKNOWN,

    /*  PushB[0]  */  Ins_PUSHB,
    /*  PushB[1]  */  Ins_PUSHB,
    /*  PushB[2]  */  Ins_PUSHB,
    /*  PushB[3]  */  Ins_PUSHB,
    /*  PushB[4]  */  Ins_PUSHB,
    /*  PushB[5]  */  Ins_PUSHB,
    /*  PushB[6]  */  Ins_PUSHB,
    /*  PushB[7]  */  Ins_PUSHB,
    /*  PushW[0]  */  Ins_PUSHW,
    /*  PushW[1]  */  Ins_PUSHW,
    /*  PushW[2]  */  Ins_PUSHW,
    /*  PushW[3]  */  Ins_PUSHW,
    /*  PushW[4]  */  Ins_PUSHW,
    /*  PushW[5]  */  Ins_PUSHW,
    /*  PushW[6]  */  Ins_PUSHW,
    /*  PushW[7]  */  Ins_PUSHW,

    /*  MDRP[00]  */  Ins_MDRP,
    /*  MDRP[01]  */  Ins_MDRP,
    /*  MDRP[02]  */  Ins_MDRP,
    /*  MDRP[03]  */  Ins_MDRP,
    /*  MDRP[04]  */  Ins_MDRP,
    /*  MDRP[05]  */  Ins_MDRP,
    /*  MDRP[06]  */  Ins_MDRP,
    /*  MDRP[07]  */  Ins_MDRP,
    /*  MDRP[08]  */  Ins_MDRP,
    /*  MDRP[09]  */  Ins_MDRP,
    /*  MDRP[10]  */  Ins_MDRP,
    /*  MDRP[11]  */  Ins_MDRP,
    /*  MDRP[12]  */  Ins_MDRP,
    /*  MDRP[13]  */  Ins_MDRP,
    /*  MDRP[14]  */  Ins_MDRP,
    /*  MDRP[15]  */  Ins_MDRP,

    /*  MDRP[16]  */  Ins_MDRP,
    /*  MDRP[17]  */  Ins_MDRP,
    /*  MDRP[18]  */  Ins_MDRP,
    /*  MDRP[19]  */  Ins_MDRP,
    /*  MDRP[20]  */  Ins_MDRP,
    /*  MDRP[21]  */  Ins_MDRP,
    /*  MDRP[22]  */  Ins_MDRP,
    /*  MDRP[23]  */  Ins_MDRP,
    /*  MDRP[24]  */  Ins_MDRP,
    /*  MDRP[25]  */  Ins_MDRP,
    /*  MDRP[26]  */  Ins_MDRP,
    /*  MDRP[27]  */  Ins_MDRP,
    /*  MDRP[28]  */  Ins_MDRP,
    /*  MDRP[29]  */  Ins_MDRP,
    /*  MDRP[30]  */  Ins_MDRP,
    /*  MDRP[31]  */  Ins_MDRP,

    /*  MIRP[00]  */  Ins_MIRP,
    /*  MIRP[01]  */  Ins_MIRP,
    /*  MIRP[02]  */  Ins_MIRP,
    /*  MIRP[03]  */  Ins_MIRP,
    /*  MIRP[04]  */  Ins_MIRP,
    /*  MIRP[05]  */  Ins_MIRP,
    /*  MIRP[06]  */  Ins_MIRP,
    /*  MIRP[07]  */  Ins_MIRP,
    /*  MIRP[08]  */  Ins_MIRP,
    /*  MIRP[09]  */  Ins_MIRP,
    /*  MIRP[10]  */  Ins_MIRP,
    /*  MIRP[11]  */  Ins_MIRP,
    /*  MIRP[12]  */  Ins_MIRP,
    /*  MIRP[13]  */  Ins_MIRP,
    /*  MIRP[14]  */  Ins_MIRP,
    /*  MIRP[15]  */  Ins_MIRP,

    /*  MIRP[16]  */  Ins_MIRP,
    /*  MIRP[17]  */  Ins_MIRP,
    /*  MIRP[18]  */  Ins_MIRP,
    /*  MIRP[19]  */  Ins_MIRP,
    /*  MIRP[20]  */  Ins_MIRP,
    /*  MIRP[21]  */  Ins_MIRP,
    /*  MIRP[22]  */  Ins_MIRP,
    /*  MIRP[23]  */  Ins_MIRP,
    /*  MIRP[24]  */  Ins_MIRP,
    /*  MIRP[25]  */  Ins_MIRP,
    /*  MIRP[26]  */  Ins_MIRP,
    /*  MIRP[27]  */  Ins_MIRP,
    /*  MIRP[28]  */  Ins_MIRP,
    /*  MIRP[29]  */  Ins_MIRP,
    /*  MIRP[30]  */  Ins_MIRP,
    /*  MIRP[31]  */  Ins_MIRP
  };


#endif /* !TT_CONFIG_OPTION_INTERPRETER_SWITCH */


  /*************************************************************************/
  /*                                                                       */
  /* RUN                                                                   */
  /*                                                                       */
  /*  This function executes a run of opcodes.  It will exit in the        */
  /*  following cases:                                                     */
  /*                                                                       */
  /*  - Errors (in which case it returns FALSE).                           */
  /*                                                                       */
  /*  - Reaching the end of the main code range (returns TRUE).            */
  /*    Reaching the end of a code range within a function call is an      */
  /*    error.                                                             */
  /*                                                                       */
  /*  - After executing one single opcode, if the flag `Instruction_Trap'  */
  /*    is set to TRUE (returns TRUE).                                     */
  /*                                                                       */
  /*  On exit whith TRUE, test IP < CodeSize to know wether it comes from  */
  /*  an instruction trap or a normal termination.                         */
  /*                                                                       */
  /*                                                                       */
  /*  Note: The documented DEBUG opcode pops a value from the stack.  This */
  /*        behaviour is unsupported; here a DEBUG opcode is always an     */
  /*        error.                                                         */
  /*                                                                       */
  /*                                                                       */
  /* THIS IS THE INTERPRETER'S MAIN LOOP.                                  */
  /*                                                                       */
  /*  Instructions appear in the specification's order.                    */
  /*                                                                       */
  /*************************************************************************/


  /* documentation is in ttinterp.h */

  FT_EXPORT_DEF( FT_Error )
  TT_RunIns( TT_ExecContext  exc )
  {
    FT_Long  ins_counter = 0;  /* executed instructions counter */


#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    cur = *exc;
#endif

    /* set CVT functions */
    CUR.tt_metrics.ratio = 0;
    if ( CUR.metrics.x_ppem != CUR.metrics.y_ppem )
    {
      /* non-square pixels, use the stretched routines */
      CUR.func_read_cvt  = Read_CVT_Stretched;
      CUR.func_write_cvt = Write_CVT_Stretched;
      CUR.func_move_cvt  = Move_CVT_Stretched;
    }
    else
    {
      /* square pixels, use normal routines */
      CUR.func_read_cvt  = Read_CVT;
      CUR.func_write_cvt = Write_CVT;
      CUR.func_move_cvt  = Move_CVT;
    }

    COMPUTE_Funcs();
    COMPUTE_Round( (FT_Byte)exc->GS.round_state );

    do
    {
      CUR.opcode = CUR.code[CUR.IP];

      if ( ( CUR.length = opcode_length[CUR.opcode] ) < 0 )
      {
        if ( CUR.IP + 1 > CUR.codeSize )
          goto LErrorCodeOverflow_;

        CUR.length = CUR.code[CUR.IP + 1] + 2;
      }

      if ( CUR.IP + CUR.length > CUR.codeSize )
        goto LErrorCodeOverflow_;

      /* First, let's check for empty stack and overflow */
      CUR.args = CUR.top - ( Pop_Push_Count[CUR.opcode] >> 4 );

      /* `args' is the top of the stack once arguments have been popped. */
      /* One can also interpret it as the index of the last argument.    */
      if ( CUR.args < 0 )
      {
        CUR.error = TT_Err_Too_Few_Arguments;
        goto LErrorLabel_;
      }

      CUR.new_top = CUR.args + ( Pop_Push_Count[CUR.opcode] & 15 );

      /* `new_top' is the new top of the stack, after the instruction's */
      /* execution.  `top' will be set to `new_top' after the `switch'  */
      /* statement.                                                     */
      if ( CUR.new_top > CUR.stackSize )
      {
        CUR.error = TT_Err_Stack_Overflow;
        goto LErrorLabel_;
      }

      CUR.step_ins = TRUE;
      CUR.error    = TT_Err_Ok;

#ifdef TT_CONFIG_OPTION_INTERPRETER_SWITCH

      {
        FT_Long*  args   = CUR.stack + CUR.args;
        FT_Byte   opcode = CUR.opcode;


#undef  ARRAY_BOUND_ERROR
#define ARRAY_BOUND_ERROR  goto Set_Invalid_Ref


        switch ( opcode )
        {
        case 0x00:  /* SVTCA y  */
        case 0x01:  /* SVTCA x  */
        case 0x02:  /* SPvTCA y */
        case 0x03:  /* SPvTCA x */
        case 0x04:  /* SFvTCA y */
        case 0x05:  /* SFvTCA x */
          {
            FT_Short AA, BB;


            AA = (FT_Short)( ( opcode & 1 ) << 14 );
            BB = (FT_Short)( AA ^ 0x4000 );

            if ( opcode < 4 )
            {
              CUR.GS.projVector.x = AA;
              CUR.GS.projVector.y = BB;

              CUR.GS.dualVector.x = AA;
              CUR.GS.dualVector.y = BB;
            }

            if ( ( opcode & 2 ) == 0 )
            {
              CUR.GS.freeVector.x = AA;
              CUR.GS.freeVector.y = BB;
            }

            COMPUTE_Funcs();
          }
          break;

        case 0x06:  /* SPvTL // */
        case 0x07:  /* SPvTL +  */
          DO_SPVTL
          break;

        case 0x08:  /* SFvTL // */
        case 0x09:  /* SFvTL +  */
          DO_SFVTL
          break;

        case 0x0A:  /* SPvFS */
          DO_SPVFS
          break;

        case 0x0B:  /* SFvFS */
          DO_SFVFS
          break;

        case 0x0C:  /* GPV */
          DO_GPV
          break;

        case 0x0D:  /* GFV */
          DO_GFV
          break;

        case 0x0E:  /* SFvTPv */
          DO_SFVTPV
          break;

        case 0x0F:  /* ISECT  */
          Ins_ISECT( EXEC_ARG_ args );
          break;

        case 0x10:  /* SRP0 */
          DO_SRP0
          break;

        case 0x11:  /* SRP1 */
          DO_SRP1
          break;

        case 0x12:  /* SRP2 */
          DO_SRP2
          break;

        case 0x13:  /* SZP0 */
          Ins_SZP0( EXEC_ARG_ args );
          break;

        case 0x14:  /* SZP1 */
          Ins_SZP1( EXEC_ARG_ args );
          break;

        case 0x15:  /* SZP2 */
          Ins_SZP2( EXEC_ARG_ args );
          break;

        case 0x16:  /* SZPS */
          Ins_SZPS( EXEC_ARG_ args );
          break;

        case 0x17:  /* SLOOP */
          DO_SLOOP
          break;

        case 0x18:  /* RTG */
          DO_RTG
          break;

        case 0x19:  /* RTHG */
          DO_RTHG
          break;

        case 0x1A:  /* SMD */
          DO_SMD
          break;

        case 0x1B:  /* ELSE */
          Ins_ELSE( EXEC_ARG_ args );
          break;

        case 0x1C:  /* JMPR */
          DO_JMPR
          break;

        case 0x1D:  /* SCVTCI */
          DO_SCVTCI
          break;

        case 0x1E:  /* SSWCI */
          DO_SSWCI
          break;

        case 0x1F:  /* SSW */
          DO_SSW
          break;

        case 0x20:  /* DUP */
          DO_DUP
          break;

        case 0x21:  /* POP */
          /* nothing :-) */
          break;

        case 0x22:  /* CLEAR */
          DO_CLEAR
          break;

        case 0x23:  /* SWAP */
          DO_SWAP
          break;

        case 0x24:  /* DEPTH */
          DO_DEPTH
          break;

        case 0x25:  /* CINDEX */
          DO_CINDEX
          break;

        case 0x26:  /* MINDEX */
          Ins_MINDEX( EXEC_ARG_ args );
          break;

        case 0x27:  /* ALIGNPTS */
          Ins_ALIGNPTS( EXEC_ARG_ args );
          break;

        case 0x28:  /* ???? */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x29:  /* UTP */
          Ins_UTP( EXEC_ARG_ args );
          break;

        case 0x2A:  /* LOOPCALL */
          Ins_LOOPCALL( EXEC_ARG_ args );
          break;

        case 0x2B:  /* CALL */
          Ins_CALL( EXEC_ARG_ args );
          break;

        case 0x2C:  /* FDEF */
          Ins_FDEF( EXEC_ARG_ args );
          break;

        case 0x2D:  /* ENDF */
          Ins_ENDF( EXEC_ARG_ args );
          break;

        case 0x2E:  /* MDAP */
        case 0x2F:  /* MDAP */
          Ins_MDAP( EXEC_ARG_ args );
          break;


        case 0x30:  /* IUP */
        case 0x31:  /* IUP */
          Ins_IUP( EXEC_ARG_ args );
          break;

        case 0x32:  /* SHP */
        case 0x33:  /* SHP */
          Ins_SHP( EXEC_ARG_ args );
          break;

        case 0x34:  /* SHC */
        case 0x35:  /* SHC */
          Ins_SHC( EXEC_ARG_ args );
          break;

        case 0x36:  /* SHZ */
        case 0x37:  /* SHZ */
          Ins_SHZ( EXEC_ARG_ args );
          break;

        case 0x38:  /* SHPIX */
          Ins_SHPIX( EXEC_ARG_ args );
          break;

        case 0x39:  /* IP    */
          Ins_IP( EXEC_ARG_ args );
          break;

        case 0x3A:  /* MSIRP */
        case 0x3B:  /* MSIRP */
          Ins_MSIRP( EXEC_ARG_ args );
          break;

        case 0x3C:  /* AlignRP */
          Ins_ALIGNRP( EXEC_ARG_ args );
          break;

        case 0x3D:  /* RTDG */
          DO_RTDG
          break;

        case 0x3E:  /* MIAP */
        case 0x3F:  /* MIAP */
          Ins_MIAP( EXEC_ARG_ args );
          break;

        case 0x40:  /* NPUSHB */
          Ins_NPUSHB( EXEC_ARG_ args );
          break;

        case 0x41:  /* NPUSHW */
          Ins_NPUSHW( EXEC_ARG_ args );
          break;

        case 0x42:  /* WS */
          DO_WS
          break;

      Set_Invalid_Ref:
            CUR.error = TT_Err_Invalid_Reference;
          break;

        case 0x43:  /* RS */
          DO_RS
          break;

        case 0x44:  /* WCVTP */
          DO_WCVTP
          break;

        case 0x45:  /* RCVT */
          DO_RCVT
          break;

        case 0x46:  /* GC */
        case 0x47:  /* GC */
          Ins_GC( EXEC_ARG_ args );
          break;

        case 0x48:  /* SCFS */
          Ins_SCFS( EXEC_ARG_ args );
          break;

        case 0x49:  /* MD */
        case 0x4A:  /* MD */
          Ins_MD( EXEC_ARG_ args );
          break;

        case 0x4B:  /* MPPEM */
          DO_MPPEM
          break;

        case 0x4C:  /* MPS */
          DO_MPS
          break;

        case 0x4D:  /* FLIPON */
          DO_FLIPON
          break;

        case 0x4E:  /* FLIPOFF */
          DO_FLIPOFF
          break;

        case 0x4F:  /* DEBUG */
          DO_DEBUG
          break;

        case 0x50:  /* LT */
          DO_LT
          break;

        case 0x51:  /* LTEQ */
          DO_LTEQ
          break;

        case 0x52:  /* GT */
          DO_GT
          break;

        case 0x53:  /* GTEQ */
          DO_GTEQ
          break;

        case 0x54:  /* EQ */
          DO_EQ
          break;

        case 0x55:  /* NEQ */
          DO_NEQ
          break;

        case 0x56:  /* ODD */
          DO_ODD
          break;

        case 0x57:  /* EVEN */
          DO_EVEN
          break;

        case 0x58:  /* IF */
          Ins_IF( EXEC_ARG_ args );
          break;

        case 0x59:  /* EIF */
          /* do nothing */
          break;

        case 0x5A:  /* AND */
          DO_AND
          break;

        case 0x5B:  /* OR */
          DO_OR
          break;

        case 0x5C:  /* NOT */
          DO_NOT
          break;

        case 0x5D:  /* DELTAP1 */
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x5E:  /* SDB */
          DO_SDB
          break;

        case 0x5F:  /* SDS */
          DO_SDS
          break;

        case 0x60:  /* ADD */
          DO_ADD
          break;

        case 0x61:  /* SUB */
          DO_SUB
          break;

        case 0x62:  /* DIV */
          DO_DIV
          break;

        case 0x63:  /* MUL */
          DO_MUL
          break;

        case 0x64:  /* ABS */
          DO_ABS
          break;

        case 0x65:  /* NEG */
          DO_NEG
          break;

        case 0x66:  /* FLOOR */
          DO_FLOOR
          break;

        case 0x67:  /* CEILING */
          DO_CEILING
          break;

        case 0x68:  /* ROUND */
        case 0x69:  /* ROUND */
        case 0x6A:  /* ROUND */
        case 0x6B:  /* ROUND */
          DO_ROUND
          break;

        case 0x6C:  /* NROUND */
        case 0x6D:  /* NROUND */
        case 0x6E:  /* NRRUND */
        case 0x6F:  /* NROUND */
          DO_NROUND
          break;

        case 0x70:  /* WCVTF */
          DO_WCVTF
          break;

        case 0x71:  /* DELTAP2 */
        case 0x72:  /* DELTAP3 */
          Ins_DELTAP( EXEC_ARG_ args );
          break;

        case 0x73:  /* DELTAC0 */
        case 0x74:  /* DELTAC1 */
        case 0x75:  /* DELTAC2 */
          Ins_DELTAC( EXEC_ARG_ args );
          break;

        case 0x76:  /* SROUND */
          DO_SROUND
          break;

        case 0x77:  /* S45Round */
          DO_S45ROUND
          break;

        case 0x78:  /* JROT */
          DO_JROT
          break;

        case 0x79:  /* JROF */
          DO_JROF
          break;

        case 0x7A:  /* ROFF */
          DO_ROFF
          break;

        case 0x7B:  /* ???? */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x7C:  /* RUTG */
          DO_RUTG
          break;

        case 0x7D:  /* RDTG */
          DO_RDTG
          break;

        case 0x7E:  /* SANGW */
        case 0x7F:  /* AA    */
          /* nothing - obsolete */
          break;

        case 0x80:  /* FLIPPT */
          Ins_FLIPPT( EXEC_ARG_ args );
          break;

        case 0x81:  /* FLIPRGON */
          Ins_FLIPRGON( EXEC_ARG_ args );
          break;

        case 0x82:  /* FLIPRGOFF */
          Ins_FLIPRGOFF( EXEC_ARG_ args );
          break;

        case 0x83:  /* UNKNOWN */
        case 0x84:  /* UNKNOWN */
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        case 0x85:  /* SCANCTRL */
          Ins_SCANCTRL( EXEC_ARG_ args );
          break;

        case 0x86:  /* SDPVTL */
        case 0x87:  /* SDPVTL */
          Ins_SDPVTL( EXEC_ARG_ args );
          break;

        case 0x88:  /* GETINFO */
          Ins_GETINFO( EXEC_ARG_ args );
          break;

        case 0x89:  /* IDEF */
          Ins_IDEF( EXEC_ARG_ args );
          break;

        case 0x8A:  /* ROLL */
          Ins_ROLL( EXEC_ARG_ args );
          break;

        case 0x8B:  /* MAX */
          DO_MAX
          break;

        case 0x8C:  /* MIN */
          DO_MIN
          break;

        case 0x8D:  /* SCANTYPE */
          Ins_SCANTYPE( EXEC_ARG_ args );
          break;

        case 0x8E:  /* INSTCTRL */
          Ins_INSTCTRL( EXEC_ARG_ args );
          break;

        case 0x8F:
          Ins_UNKNOWN( EXEC_ARG_ args );
          break;

        default:
          if ( opcode >= 0xE0 )
            Ins_MIRP( EXEC_ARG_ args );
          else if ( opcode >= 0xC0 )
            Ins_MDRP( EXEC_ARG_ args );
          else if ( opcode >= 0xB8 )
            Ins_PUSHW( EXEC_ARG_ args );
          else if ( opcode >= 0xB0 )
            Ins_PUSHB( EXEC_ARG_ args );
          else
            Ins_UNKNOWN( EXEC_ARG_ args );
        }

      }

#else

      Instruct_Dispatch[CUR.opcode]( EXEC_ARG_ &CUR.stack[CUR.args] );

#endif /* TT_CONFIG_OPTION_INTERPRETER_SWITCH */

      if ( CUR.error != TT_Err_Ok )
      {
        switch ( CUR.error )
        {
        case TT_Err_Invalid_Opcode: /* looking for redefined instructions */
          {
            TT_DefRecord*  def   = CUR.IDefs;
            TT_DefRecord*  limit = def + CUR.numIDefs;


            for ( ; def < limit; def++ )
            {
              if ( def->active && CUR.opcode == (FT_Byte)def->opc )
              {
                TT_CallRec*  callrec;


                if ( CUR.callTop >= CUR.callSize )
                {
                  CUR.error = TT_Err_Invalid_Reference;
                  goto LErrorLabel_;
                }

                callrec = &CUR.callStack[CUR.callTop];

                callrec->Caller_Range = CUR.curRange;
                callrec->Caller_IP    = CUR.IP + 1;
                callrec->Cur_Count    = 1;
                callrec->Cur_Restart  = def->start;

                if ( INS_Goto_CodeRange( def->range, def->start ) == FAILURE )
                  goto LErrorLabel_;

                goto LSuiteLabel_;
              }
            }
          }

          CUR.error = TT_Err_Invalid_Opcode;
          goto LErrorLabel_;

#if 0
          break;   /* Unreachable code warning suppression.             */
                   /* Leave to remind in case a later change the editor */
                   /* to consider break;                                */
#endif

        default:
          goto LErrorLabel_;

#if 0
        break;
#endif
        }
      }

      CUR.top = CUR.new_top;

      if ( CUR.step_ins )
        CUR.IP += CUR.length;

      /* increment instruction counter and check if we didn't */
      /* run this program for too long (e.g. infinite loops). */
      if ( ++ins_counter > MAX_RUNNABLE_OPCODES )
        return TT_Err_Execution_Too_Long;

    LSuiteLabel_:
      if ( CUR.IP >= CUR.codeSize )
      {
        if ( CUR.callTop > 0 )
        {
          CUR.error = TT_Err_Code_Overflow;
          goto LErrorLabel_;
        }
        else
          goto LNo_Error_;
      }
    } while ( !CUR.instruction_trap );

  LNo_Error_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return TT_Err_Ok;

  LErrorCodeOverflow_:
    CUR.error = TT_Err_Code_Overflow;

  LErrorLabel_:

#ifdef TT_CONFIG_OPTION_STATIC_RASTER
    *exc = cur;
#endif

    return CUR.error;
  }


#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


/* END */
