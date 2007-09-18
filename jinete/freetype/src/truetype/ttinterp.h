/***************************************************************************/
/*                                                                         */
/*  ttinterp.h                                                             */
/*                                                                         */
/*    TrueType bytecode interpreter (specification).                       */
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


#ifndef __TTINTERP_H__
#define __TTINTERP_H__


#include <ft2build.h>
#include "ttobjs.h"


FT_BEGIN_HEADER


#ifndef TT_CONFIG_OPTION_STATIC_INTEPRETER  /* indirect implementation */

#define EXEC_OP_   TT_ExecContext  exc,
#define EXEC_OP    TT_ExecContext  exc
#define EXEC_ARG_  exc,
#define EXEC_ARG   exc

#else                                       /* static implementation */

#define EXEC_OP_   /* void */
#define EXEC_OP    /* void */
#define EXEC_ARG_  /* void */
#define EXEC_ARG   /* void */

#endif /* TT_CONFIG_OPTION_STATIC_INTERPRETER */


  /*************************************************************************/
  /*                                                                       */
  /* Rounding mode constants.                                              */
  /*                                                                       */
#define TT_Round_Off             5
#define TT_Round_To_Half_Grid    0
#define TT_Round_To_Grid         1
#define TT_Round_To_Double_Grid  2
#define TT_Round_Up_To_Grid      4
#define TT_Round_Down_To_Grid    3
#define TT_Round_Super           6
#define TT_Round_Super_45        7


  /*************************************************************************/
  /*                                                                       */
  /* Function types used by the interpreter, depending on various modes    */
  /* (e.g. the rounding mode, whether to render a vertical or horizontal   */
  /* line etc).                                                            */
  /*                                                                       */
  /*************************************************************************/

  /* Rounding function */
  typedef FT_F26Dot6
  (*TT_Round_Func)( EXEC_OP_ FT_F26Dot6  distance,
                             FT_F26Dot6  compensation );

  /* Point displacement along the freedom vector routine */
  typedef void
  (*TT_Move_Func)( EXEC_OP_ TT_GlyphZone*  zone,
                            FT_UShort      point,
                            FT_F26Dot6     distance );

  /* Distance projection along one of the projection vectors */
  typedef FT_F26Dot6
  (*TT_Project_Func)( EXEC_OP_ FT_Vector*  v1,
                               FT_Vector*  v2 );

  /* reading a cvt value.  Take care of non-square pixels if necessary */
  typedef FT_F26Dot6
  (*TT_Get_CVT_Func)( EXEC_OP_ FT_ULong  index );

  /* setting or moving a cvt value.  Take care of non-square pixels  */
  /* if necessary                                                    */
  typedef void
  (*TT_Set_CVT_Func)( EXEC_OP_ FT_ULong    index,
                               FT_F26Dot6  value );


  /*************************************************************************/
  /*                                                                       */
  /* This structure defines a call record, used to manage function calls.  */
  /*                                                                       */
  typedef struct  TT_CallRec_
  {
    FT_Int   Caller_Range;
    FT_Long  Caller_IP;
    FT_Long  Cur_Count;
    FT_Long  Cur_Restart;

  } TT_CallRec, *TT_CallStack;


  /*************************************************************************/
  /*                                                                       */
  /* The main structure for the interpreter which collects all necessary   */
  /* variables and states.                                                 */
  /*                                                                       */
  typedef struct  TT_ExecContextRec_
  {
    TT_Face         face;
    TT_Size         size;
    FT_Memory       memory;

    /* instructions state */

    FT_Error        error;      /* last execution error */

    FT_Long         top;        /* top of exec. stack   */

    FT_UInt         stackSize;  /* size of exec. stack  */
    FT_Long*        stack;      /* current exec. stack  */

    FT_Long         args;
    FT_UInt         new_top;    /* new top after exec.  */

    TT_GlyphZone    zp0,        /* zone records */
                    zp1,
                    zp2,
                    pts,
                    twilight;

    FT_Size_Metrics  metrics;
    TT_Size_Metrics  tt_metrics; /* size metrics */

    TT_GraphicsState  GS;         /* current graphics state */

    FT_Int          curRange;  /* current code range number   */
    FT_Byte*        code;      /* current code range          */
    FT_Long         IP;        /* current instruction pointer */
    FT_Long         codeSize;  /* size of current range       */

    FT_Byte         opcode;    /* current opcode              */
    FT_Int          length;    /* length of current opcode    */

    FT_Bool         step_ins;  /* true if the interpreter must */
                               /* increment IP after ins. exec */
    FT_Long         cvtSize;
    FT_Long*        cvt;

    FT_UInt         glyphSize; /* glyph instructions buffer size */
    FT_Byte*        glyphIns;  /* glyph instructions buffer */

    FT_UInt         numFDefs;  /* number of function defs         */
    FT_UInt         maxFDefs;  /* maximum number of function defs */
    TT_DefArray     FDefs;     /* table of FDefs entries          */

    FT_UInt         numIDefs;  /* number of instruction defs */
    FT_UInt         maxIDefs;  /* maximum number of ins defs */
    TT_DefArray     IDefs;     /* table of IDefs entries     */

    FT_UInt         maxFunc;   /* maximum function index     */
    FT_UInt         maxIns;    /* maximum instruction index  */

    FT_Int          callTop,    /* top of call stack during execution */
                    callSize;   /* size of call stack */
    TT_CallStack    callStack;  /* call stack */

    FT_UShort       maxPoints;    /* capacity of this context's `pts' */
    FT_Short        maxContours;  /* record, expressed in points and  */
                                  /* contours.                        */

    TT_CodeRangeTable  codeRangeTable;  /* table of valid code ranges */
                                        /* useful for the debugger   */

    FT_UShort       storeSize;  /* size of current storage */
    FT_Long*        storage;    /* storage area            */

    FT_F26Dot6      period;     /* values used for the */
    FT_F26Dot6      phase;      /* `SuperRounding'     */
    FT_F26Dot6      threshold;

#if 0
    /* this seems to be unused */
    FT_Int          cur_ppem;       /* ppem along the current proj vector */
#endif

    FT_Bool         instruction_trap;  /* If `True', the interpreter will */
                                       /* exit after each instruction     */

    TT_GraphicsState  default_GS;      /* graphics state resulting from    */
                                       /* the prep program                 */
    FT_Bool          is_composite;     /* true if the glyph is composite   */
    FT_Bool          pedantic_hinting; /* true for pedantic interpretation */

    /* latest interpreter additions */

    FT_Long            F_dot_P;    /* dot product of freedom and projection */
                                   /* vectors                               */
    TT_Round_Func      func_round; /* current rounding function             */

    TT_Project_Func    func_project,   /* current projection function */
                       func_dualproj,  /* current dual proj. function */
                       func_freeProj;  /* current freedom proj. func  */

    TT_Move_Func       func_move;      /* current point move function */

    TT_Get_CVT_Func    func_read_cvt;  /* read a cvt entry              */
    TT_Set_CVT_Func    func_write_cvt; /* write a cvt entry (in pixels) */
    TT_Set_CVT_Func    func_move_cvt;  /* incr a cvt entry (in pixels)  */

    FT_ULong           loadSize;
    TT_SubGlyph_Stack  loadStack;      /* loading subglyph stack */

  } TT_ExecContextRec;


  extern const TT_GraphicsState  tt_default_graphics_state;


  FT_LOCAL FT_Error
  TT_Goto_CodeRange( TT_ExecContext  exec,
                     FT_Int          range,
                     FT_Long         IP );

  FT_LOCAL FT_Error
  TT_Set_CodeRange( TT_ExecContext  exec,
                    FT_Int          range,
                    void*           base,
                    FT_Long         length );

  FT_LOCAL FT_Error
  TT_Clear_CodeRange( TT_ExecContext  exec,
                      FT_Int          range );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_New_Context                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Queries the face context for a given font.  Note that there is     */
  /*    now a _single_ execution context in the TrueType driver which is   */
  /*    shared among faces.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A handle to the execution context.  Initialized for `face'.        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the glyph loader and debugger should call this function.      */
  /*                                                                       */
  FT_EXPORT( TT_ExecContext )
  TT_New_Context( TT_Face  face );


  FT_LOCAL FT_Error
  TT_Done_Context( TT_ExecContext  exec );

  FT_LOCAL FT_Error
  TT_Destroy_Context( TT_ExecContext  exec,
                      FT_Memory       memory );

  FT_LOCAL FT_Error
  TT_Load_Context( TT_ExecContext  exec,
                   TT_Face         face,
                   TT_Size         size );

  FT_LOCAL FT_Error
  TT_Save_Context( TT_ExecContext  exec,
                   TT_Size         ins );

  FT_LOCAL FT_Error
  TT_Run_Context( TT_ExecContext  exec,
                  FT_Bool         debug );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_RunIns                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Executes one or more instruction in the execution context.  This   */
  /*    is the main function of the TrueType opcode interpreter.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    exec :: A handle to the target execution context.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only the object manager and debugger should call this function.    */
  /*                                                                       */
  /*    This function is publicly exported because it is directly          */
  /*    invoked by the TrueType debugger.                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  TT_RunIns( TT_ExecContext  exec );


FT_END_HEADER

#endif /* __TTINTERP_H__ */


/* END */
