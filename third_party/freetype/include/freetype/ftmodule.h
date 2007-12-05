/***************************************************************************/
/*                                                                         */
/*  ftmodule.h                                                             */
/*                                                                         */
/*    FreeType modules public interface (specification).                   */
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


#ifndef __FTMODULE_H__
#define __FTMODULE_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    module_management                                                  */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Module Management                                                  */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How to add, upgrade, and remove modules from FreeType.             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The definitions below are used to manage modules within FreeType.  */
  /*    Modules can be added, upgraded, and removed at runtime.            */
  /*                                                                       */
  /*************************************************************************/


  /* module bit flags */
  typedef enum  FT_Module_Flags_
  {
    ft_module_font_driver         = 1,     /* this module is a font driver  */
    ft_module_renderer            = 2,     /* this module is a renderer     */
    ft_module_hinter              = 4,     /* this module is a glyph hinter */
    ft_module_styler              = 8,     /* this module is a styler       */

    ft_module_driver_scalable     = 0x100, /* the driver supports scalable  */
                                           /* fonts                         */
    ft_module_driver_no_outlines  = 0x200, /* the driver does not support   */
                                           /* vector outlines               */
    ft_module_driver_has_hinter   = 0x400  /* the driver provides its own   */
                                           /* hinter                        */

  } FT_Module_Flags;


  typedef void
  (*FT_Module_Interface)( void );

  typedef FT_Error
  (*FT_Module_Constructor)( FT_Module  module );

  typedef void
  (*FT_Module_Destructor)( FT_Module  module );

  typedef FT_Module_Interface
  (*FT_Module_Requester)( FT_Module    module,
                          const char*  name );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Module_Class                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The module class descriptor.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    module_flags      :: Bit flags describing the module.              */
  /*                                                                       */
  /*    module_size       :: The size of one module object/instance in     */
  /*                         bytes.                                        */
  /*                                                                       */
  /*    module_name       :: The name of the module.                       */
  /*                                                                       */
  /*    module_version    :: The version, as a 16.16 fixed number          */
  /*                         (major.minor).                                */
  /*                                                                       */
  /*    module_requires   :: The version of FreeType this module requires  */
  /*                         (starts at version 2.0, i.e 0x20000)          */
  /*                                                                       */
  /*    module_init       :: A function used to initialize (not create) a  */
  /*                         new module object.                            */
  /*                                                                       */
  /*    module_done       :: A function used to finalize (not destroy) a   */
  /*                         given module object                           */
  /*                                                                       */
  /*    get_interface     :: Queries a given module for a specific         */
  /*                         interface by name.                            */
  /*                                                                       */
  typedef struct  FT_Module_Class_
  {
    FT_ULong               module_flags;
    FT_Int                 module_size;
    const FT_String*       module_name;
    FT_Fixed               module_version;
    FT_Fixed               module_requires;

    const void*            module_interface;

    FT_Module_Constructor  module_init;
    FT_Module_Destructor   module_done;
    FT_Module_Requester    get_interface;

  } FT_Module_Class;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds a new module to a given library instance.                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to the library object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    clazz   :: A pointer to class descriptor for the module.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error will be returned if a module already exists by that name, */
  /*    or if the module requires a version of FreeType that is too great. */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Add_Module( FT_Library              library,
                 const FT_Module_Class*  clazz );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds a module by its name.                                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object.                     */
  /*                                                                       */
  /*    module_name :: The module's name (as an ASCII string).             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A module handle.  0 if none was found.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    You should better be familiar with FreeType internals to know      */
  /*    which module to look for :-)                                       */
  /*                                                                       */
  FT_EXPORT( FT_Module )
  FT_Get_Module( FT_Library   library,
                 const char*  module_name );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Remove_Module                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Removes a given module from a library instance.                    */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to a library object.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    module  :: A handle to a module object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The module object is destroyed by the function in case of success. */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Remove_Module( FT_Library  library,
                    FT_Module   module );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Library                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to create a new FreeType library instance    */
  /*    from a given memory object.  It is thus possible to use libraries  */
  /*    with distinct memory allocators within the same program.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A handle to the original memory object.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    alibrary :: A pointer to handle of a new library object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Library( FT_Memory    memory,
                  FT_Library  *alibrary );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Library                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given library object.  This closes all drivers and      */
  /*    discards all resource objects.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_Library( FT_Library  library );



  typedef void
  (*FT_DebugHook_Func)( void*  arg );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Debug_Hook                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets a debug hook function for debugging the interpreter of a font */
  /*    format.                                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    hook_index :: The index of the debug hook.  You should use the     */
  /*                  values defined in ftobjs.h, e.g.                     */
  /*                  FT_DEBUG_HOOK_TRUETYPE.                              */
  /*                                                                       */
  /*    debug_hook :: The function used to debug the interpreter.          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Currently, four debug hook slots are available, but only two (for  */
  /*    the TrueType and the Type 1 interpreter) are defined.              */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Set_Debug_Hook( FT_Library         library,
                     FT_UInt            hook_index,
                     FT_DebugHook_Func  debug_hook );



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Default_Modules                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adds the set of default drivers to a given library object.         */
  /*    This is only useful when you create a library object with          */
  /*    FT_New_Library() (usually to plug a custom memory manager).        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to a new library object.                       */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Add_Default_Modules( FT_Library  library );


  /* */


FT_END_HEADER

#endif /* __FTMODULE_H__ */


/* END */
