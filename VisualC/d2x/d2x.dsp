# Microsoft Developer Studio Project File - Name="d2x" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=d2x - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "d2x.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "d2x.mak" CFG="d2x - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "d2x - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "d2x - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "d2x - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\main" /I "..\..\arch\include" /I "..\..\..\..\sdl\SDL-1.2.6\include" /D "NDEBUG" /D "RELEASE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_VIDEO" /D "FAST_FILE_IO" /D "CONSOLE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "d2x - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\main" /I "..\..\arch\include" /I "..\..\..\..\sdl\SDL-1.2.6\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_VIDEO" /D "FAST_FILE_IO" /D "CONSOLE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "d2x - Win32 Release"
# Name "d2x - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "2d"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\2d\2dsline.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\bitblt.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\bitmap.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\box.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\canvas.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\circle.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\disc.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\font.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\gpixel.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\ibitblt.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\line.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\palette.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\pcx.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\pixel.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\poly.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\rect.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\rle.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\scalec.c
# End Source File
# Begin Source File

SOURCE=..\..\2d\tmerge.c
# End Source File
# End Group
# Begin Group "3d"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\3d\clipper.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\draw.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\globvars.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\instance.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\interp.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\points.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\rod.c
# End Source File
# Begin Source File

SOURCE=..\..\3d\setup.c
# End Source File
# End Group
# Begin Group "arch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\arch\sdl\digi.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\event.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\findfile.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\gr.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\init.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\joy.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\joydefs.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\key.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\mingw_init.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\mouse.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\rbaudio.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\timer.c
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\misc\args.c
# End Source File
# Begin Source File

SOURCE=..\..\cfile\cfile.c
# End Source File
# Begin Source File

SOURCE=..\..\console\CON_console.c
# End Source File
# Begin Source File

SOURCE=..\..\misc\d_io.c
# End Source File
# Begin Source File

SOURCE=..\..\misc\error.c
# End Source File
# Begin Source File

SOURCE=..\..\misc\hash.c
# End Source File
# Begin Source File

SOURCE=..\..\iff\iff.c
# End Source File
# Begin Source File

SOURCE=..\..\mem\mem.c
# End Source File
# Begin Source File

SOURCE=..\..\misc\strio.c
# End Source File
# Begin Source File

SOURCE=..\..\misc\strutil.c
# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\main\ai.c
# End Source File
# Begin Source File

SOURCE=..\..\main\ai2.c
# End Source File
# Begin Source File

SOURCE=..\..\main\aipath.c
# End Source File
# Begin Source File

SOURCE=..\..\main\automap.c
# End Source File
# Begin Source File

SOURCE=..\..\main\bm.c
# End Source File
# Begin Source File

SOURCE=..\..\main\cmd.c
# End Source File
# Begin Source File

SOURCE=..\..\main\cntrlcen.c
# End Source File
# Begin Source File

SOURCE=..\..\main\collide.c
# End Source File
# Begin Source File

SOURCE=..\..\main\config.c
# End Source File
# Begin Source File

SOURCE=..\..\main\console.c
# End Source File
# Begin Source File

SOURCE=..\..\main\controls.c
# End Source File
# Begin Source File

SOURCE=..\..\main\credits.c
# End Source File
# Begin Source File

SOURCE=..\..\main\crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\main\effects.c
# End Source File
# Begin Source File

SOURCE=..\..\main\endlevel.c
# End Source File
# Begin Source File

SOURCE=..\..\main\escort.c
# End Source File
# Begin Source File

SOURCE=..\..\main\fireball.c
# End Source File
# Begin Source File

SOURCE=..\..\main\fuelcen.c
# End Source File
# Begin Source File

SOURCE=..\..\main\fvi.c
# End Source File
# Begin Source File

SOURCE=..\..\main\game.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamecntl.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamefont.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamemine.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamepal.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamerend.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gamesave.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gameseg.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gameseq.c
# End Source File
# Begin Source File

SOURCE=..\..\main\gauges.c
# End Source File
# Begin Source File

SOURCE=..\..\main\hostage.c
# End Source File
# Begin Source File

SOURCE=..\..\main\hud.c
# End Source File
# Begin Source File

SOURCE=..\..\main\inferno.c
# End Source File
# Begin Source File

SOURCE=..\..\main\kconfig.c
# End Source File
# Begin Source File

SOURCE=..\..\main\kludge.c
# End Source File
# Begin Source File

SOURCE=..\..\main\laser.c
# End Source File
# Begin Source File

SOURCE=..\..\main\lighting.c
# End Source File
# Begin Source File

SOURCE=..\..\main\menu.c
# End Source File
# Begin Source File

SOURCE=..\..\main\mglobal.c
# End Source File
# Begin Source File

SOURCE=..\..\main\mission.c
# End Source File
# Begin Source File

SOURCE=..\..\main\morph.c
# End Source File
# Begin Source File

SOURCE=..\..\main\movie.c
# End Source File
# Begin Source File

SOURCE=..\..\main\newdemo.c
# End Source File
# Begin Source File

SOURCE=..\..\main\newmenu.c
# End Source File
# Begin Source File

SOURCE=..\..\main\object.c
# End Source File
# Begin Source File

SOURCE=..\..\main\paging.c
# End Source File
# Begin Source File

SOURCE=..\..\main\physics.c
# End Source File
# Begin Source File

SOURCE=..\..\main\piggy.c
# End Source File
# Begin Source File

SOURCE=..\..\main\player.c
# End Source File
# Begin Source File

SOURCE=..\..\main\playsave.c
# End Source File
# Begin Source File

SOURCE=..\..\main\polyobj.c
# End Source File
# Begin Source File

SOURCE=..\..\main\powerup.c
# End Source File
# Begin Source File

SOURCE=..\..\main\render.c
# End Source File
# Begin Source File

SOURCE=..\..\main\robot.c
# End Source File
# Begin Source File

SOURCE=..\..\main\scores.c
# End Source File
# Begin Source File

SOURCE=..\..\main\segment.c
# End Source File
# Begin Source File

SOURCE=..\..\main\slew.c
# End Source File
# Begin Source File

SOURCE=..\..\main\songs.c
# End Source File
# Begin Source File

SOURCE=..\..\main\state.c
# End Source File
# Begin Source File

SOURCE=..\..\main\switch.c
# End Source File
# Begin Source File

SOURCE=..\..\main\terrain.c
# End Source File
# Begin Source File

SOURCE=..\..\main\texmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\main\text.c
# End Source File
# Begin Source File

SOURCE=..\..\main\titles.c
# End Source File
# Begin Source File

SOURCE=..\..\main\vclip.c
# End Source File
# Begin Source File

SOURCE=..\..\main\wall.c
# End Source File
# Begin Source File

SOURCE=..\..\main\weapon.c
# End Source File
# End Group
# Begin Group "maths"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\maths\fixc.c
# End Source File
# Begin Source File

SOURCE=..\..\maths\rand.c
# End Source File
# Begin Source File

SOURCE=..\..\maths\tables.c
# End Source File
# Begin Source File

SOURCE=..\..\maths\vecmat.c
# End Source File
# End Group
# Begin Group "texmap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\texmap\ntmap.c
# End Source File
# Begin Source File

SOURCE=..\..\texmap\scanline.c
# End Source File
# Begin Source File

SOURCE=..\..\texmap\tmapflat.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\args.h
# End Source File
# Begin Source File

SOURCE=..\..\include\byteswap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cmd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\CON_console.h
# End Source File
# Begin Source File

SOURCE=..\..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\..\include\d_io.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\maths.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pstypes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strutil.h
# End Source File
# Begin Source File

SOURCE=..\..\include\u_mem.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE="..\..\..\..\sdl\SDL-1.2.6\lib\SDL.lib"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\sdl\SDL-1.2.6\lib\SDLmain.lib"
# End Source File
# End Target
# End Project
