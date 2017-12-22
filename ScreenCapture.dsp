# Microsoft Developer Studio Project File - Name="ScreenCapture" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ScreenCapture - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ScreenCapture.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ScreenCapture.mak" CFG="ScreenCapture - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ScreenCapture - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ScreenCapture - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ScreenCapture - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCREENCAPTURE_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCREENCAPTURE_EXPORTS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"E:/ScreenCaptureExe/ScreenCapture.dll"

!ELSEIF  "$(CFG)" == "ScreenCapture - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCREENCAPTURE_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCREENCAPTURE_EXPORTS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"E:/ScreenCaptureExe/ScreenCapture.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ScreenCapture - Win32 Release"
# Name "ScreenCapture - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ScreenCapture.cpp
# End Source File
# Begin Source File

SOURCE=.\ScreenCaptureDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ScreenCapture.h
# End Source File
# Begin Source File

SOURCE=.\ScreenCaptureDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sc_arrow.cur
# End Source File
# Begin Source File

SOURCE=.\ScreenCapture.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\resPNG\arrow_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\arrow_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\arrow_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\back_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\back_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\back_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\brush_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\brush_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\brush_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\cancel_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\cancel_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\cancel_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ellipse_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ellipse_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ellipse_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ok_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ok_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\ok_normal.png
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\resPNG\rectangle_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\rectangle_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\rectangle_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\save_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\save_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\save_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_big_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_big_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_big_normal.png
# End Source File
# Begin Source File

SOURCE=".\resPNG\size_mid_ normal.png"
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_mid_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_mid_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_small_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_small_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\size_small_normal.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\word_down.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\word_hover.png
# End Source File
# Begin Source File

SOURCE=.\resPNG\word_normal.png
# End Source File
# End Target
# End Project
