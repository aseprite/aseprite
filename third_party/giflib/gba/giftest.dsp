# Microsoft Developer Studio Project File - Name="giftest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=giftest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "giftest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "giftest.mak" CFG="giftest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "giftest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "giftest - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "giftest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f giftest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "giftest.exe"
# PROP BASE Bsc_Name "giftest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "vmake -f "giftest.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "giftest.mb"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "giftest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f giftest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "giftest.exe"
# PROP BASE Bsc_Name "giftest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "vmake -f "giftest.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "giftest.mb"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "giftest - Win32 Release"
# Name "giftest - Win32 Debug"

!IF  "$(CFG)" == "giftest - Win32 Release"

!ELSEIF  "$(CFG)" == "giftest - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\dgif_lib.c
# End Source File
# Begin Source File

SOURCE=..\lib\gif_err.c
# End Source File
# Begin Source File

SOURCE=..\lib\gifalloc.c
# End Source File
# Begin Source File

SOURCE=.\giftest.c
# End Source File
# Begin Source File

SOURCE=.\giftest.mak
# End Source File
# Begin Source File

SOURCE=".\res\porsche-240x160.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\gif_lib.h
# End Source File
# Begin Source File

SOURCE=..\lib\gif_lib_private.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\cover.gif
# End Source File
# Begin Source File

SOURCE=".\res\porsche-240x160.gif"
# End Source File
# Begin Source File

SOURCE=".\res\x-trans.gif"
# End Source File
# End Group
# Begin Group "Output Files"

# PROP Default_Filter "*.gba;*.mb"
# Begin Source File

SOURCE=.\giftest.gba
# End Source File
# Begin Source File

SOURCE=.\giftest.map
# End Source File
# Begin Source File

SOURCE=.\giftest.mb
# End Source File
# End Group
# End Target
# End Project
