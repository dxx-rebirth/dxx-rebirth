# Microsoft Developer Studio Project File - Name="d2xgl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=d2xgl - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "d2xgl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "d2xgl.mak" CFG="d2xgl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "d2xgl - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "d2xgl - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "d2xgl - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\main" /I "..\..\arch\include" /I "..\..\arch\win32\include" /I "..\..\..\..\sdl\SDL-1.2.7\include" /D "NDEBUG" /D "RELEASE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_GL_VIDEO" /D "FAST_FILE_IO" /D "CONSOLE" /D "NETWORK" /D "NATIVE_IPX" /D "OGL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib opengl32.lib glu32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "d2xgl - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "..\..\main" /I "..\..\arch\include" /I "..\..\arch\win32\include" /I "..\..\..\..\sdl\SDL-1.2.7\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /D "NMONO" /D "PIGGY_USE_PAGING" /D "NEWDEMO" /D "SDL_INPUT" /D "SDL_GL_VIDEO" /D "FAST_FILE_IO" /D "CONSOLE" /D "NETWORK" /D "NATIVE_IPX" /D "OGL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib opengl32.lib glu32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "d2xgl - Win32 Release"
# Name "d2xgl - Win32 Debug"
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

SOURCE=..\..\arch\ogl\gr.c
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

SOURCE=..\..\arch\ogl\ogl.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\sdl\rbaudio.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\ogl\sdlgl.c
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

SOURCE=..\..\arch\win32\ipx_mcast4.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\ipx_udp.c
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\ipx_win.c
# End Source File
# Begin Source File

SOURCE=..\..\main\kconfig.c
# End Source File
# Begin Source File

SOURCE=..\..\main\kludge.c
# End Source File
# Begin Source File

SOURCE=..\..\main\kmatrix.c
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

SOURCE=..\..\main\multi.c
# End Source File
# Begin Source File

SOURCE=..\..\main\multibot.c
# End Source File
# Begin Source File

SOURCE=..\..\main\netmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\main\network.c
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
# Begin Source File

SOURCE=..\..\arch\win32\winnet.c
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

SOURCE=..\..\main\ai.h
# End Source File
# Begin Source File

SOURCE=..\..\main\aistruct.h
# End Source File
# Begin Source File

SOURCE=..\..\include\args.h
# End Source File
# Begin Source File

SOURCE=..\..\main\automap.h
# End Source File
# Begin Source File

SOURCE=..\..\2d\bitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\main\bm.h
# End Source File
# Begin Source File

SOURCE=..\..\main\bmread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\byteswap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cfile.h
# End Source File
# Begin Source File

SOURCE=..\..\2d\clip.h
# End Source File
# Begin Source File

SOURCE=..\..\3d\clipper.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cmd.h
# End Source File
# Begin Source File

SOURCE=..\..\main\cntrlcen.h
# End Source File
# Begin Source File

SOURCE=..\..\main\collide.h
# End Source File
# Begin Source File

SOURCE=..\..\main\compbit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\CON_console.h
# End Source File
# Begin Source File

SOURCE=..\..\main\config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\console.h
# End Source File
# Begin Source File

SOURCE=..\..\main\controls.h
# End Source File
# Begin Source File

SOURCE=..\..\main\credits.h
# End Source File
# Begin Source File

SOURCE=..\..\include\d_io.h
# End Source File
# Begin Source File

SOURCE=..\..\main\desc_id.h
# End Source File
# Begin Source File

SOURCE=..\..\main\digi.h
# End Source File
# Begin Source File

SOURCE=..\..\main\effects.h
# End Source File
# Begin Source File

SOURCE=..\..\main\endlevel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\error.h
# End Source File
# Begin Source File

SOURCE=..\..\main\escort.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\include\event.h
# End Source File
# Begin Source File

SOURCE=..\..\include\findfile.h
# End Source File
# Begin Source File

SOURCE=..\..\main\fireball.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fix.h
# End Source File
# Begin Source File

SOURCE=..\..\main\fuelcen.h
# End Source File
# Begin Source File

SOURCE=..\..\main\fvi.h
# End Source File
# Begin Source File

SOURCE=..\..\main\fvi_a.h
# End Source File
# Begin Source File

SOURCE=..\..\main\game.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gamefont.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gamemine.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gamepal.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gamesave.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gameseg.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gameseq.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gamestat.h
# End Source File
# Begin Source File

SOURCE=..\..\main\gauges.h
# End Source File
# Begin Source File

SOURCE=..\..\3d\globvars.h
# End Source File
# Begin Source File

SOURCE=..\..\include\gr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\grdef.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\main\hostage.h
# End Source File
# Begin Source File

SOURCE=..\..\main\hudmsg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ibitblt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iff.h
# End Source File
# Begin Source File

SOURCE=..\..\main\inferno.h
# End Source File
# Begin Source File

SOURCE=..\..\include\interp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ipx.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\ipx_drv.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\include\ipx_mcast4.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\win32\include\ipx_udp.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\include\joy.h
# End Source File
# Begin Source File

SOURCE=..\..\main\joydefs.h
# End Source File
# Begin Source File

SOURCE=..\..\main\kconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\include\key.h
# End Source File
# Begin Source File

SOURCE=..\..\main\kmatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\main\laser.h
# End Source File
# Begin Source File

SOURCE=..\..\main\lighting.h
# End Source File
# Begin Source File

SOURCE=..\..\include\makesig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\maths.h
# End Source File
# Begin Source File

SOURCE=..\..\main\menu.h
# End Source File
# Begin Source File

SOURCE=..\..\main\mission.h
# End Source File
# Begin Source File

SOURCE=..\..\main\modem.h
# End Source File
# Begin Source File

SOURCE=..\..\include\modex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mono.h
# End Source File
# Begin Source File

SOURCE=..\..\main\morph.h
# End Source File
# Begin Source File

SOURCE=..\..\arch\include\mouse.h
# End Source File
# Begin Source File

SOURCE=..\..\main\movie.h
# End Source File
# Begin Source File

SOURCE=..\..\main\multi.h
# End Source File
# Begin Source File

SOURCE=..\..\main\multibot.h
# End Source File
# Begin Source File

SOURCE=..\..\main\netmisc.h
# End Source File
# Begin Source File

SOURCE=..\..\main\newdemo.h
# End Source File
# Begin Source File

SOURCE=..\..\main\newmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\main\object.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ogl_init.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pa_enabl.h
# End Source File
# Begin Source File

SOURCE=..\..\main\paging.h
# End Source File
# Begin Source File

SOURCE=..\..\include\palette.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pcx.h
# End Source File
# Begin Source File

SOURCE=..\..\main\physics.h
# End Source File
# Begin Source File

SOURCE=..\..\main\piggy.h
# End Source File
# Begin Source File

SOURCE=..\..\main\player.h
# End Source File
# Begin Source File

SOURCE=..\..\main\playsave.h
# End Source File
# Begin Source File

SOURCE=..\..\main\polyobj.h
# End Source File
# Begin Source File

SOURCE=..\..\main\powerup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pstypes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rbaudio.h
# End Source File
# Begin Source File

SOURCE=..\..\main\render.h
# End Source File
# Begin Source File

SOURCE=..\..\main\reorder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rle.h
# End Source File
# Begin Source File

SOURCE=..\..\main\robot.h
# End Source File
# Begin Source File

SOURCE=..\..\texmap\scanline.h
# End Source File
# Begin Source File

SOURCE=..\..\main\scores.h
# End Source File
# Begin Source File

SOURCE=..\..\main\screens.h
# End Source File
# Begin Source File

SOURCE=..\..\main\segment.h
# End Source File
# Begin Source File

SOURCE=..\..\main\segpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\main\slew.h
# End Source File
# Begin Source File

SOURCE=..\..\main\songs.h
# End Source File
# Begin Source File

SOURCE=..\..\main\sounds.h
# End Source File
# Begin Source File

SOURCE=..\..\main\state.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strutil.h
# End Source File
# Begin Source File

SOURCE=..\..\main\switch.h
# End Source File
# Begin Source File

SOURCE=..\..\main\terrain.h
# End Source File
# Begin Source File

SOURCE=..\..\include\texmap.h
# End Source File
# Begin Source File

SOURCE=..\..\texmap\texmapl.h
# End Source File
# Begin Source File

SOURCE=..\..\main\texmerge.h
# End Source File
# Begin Source File

SOURCE=..\..\main\text.h
# End Source File
# Begin Source File

SOURCE=..\..\main\textures.h
# End Source File
# Begin Source File

SOURCE=..\..\include\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\main\titles.h
# End Source File
# Begin Source File

SOURCE=..\..\include\u_dpmi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\u_mem.h
# End Source File
# Begin Source File

SOURCE=..\..\main\vclip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vecmat.h
# End Source File
# Begin Source File

SOURCE=..\..\main\vers_id.h
# End Source File
# Begin Source File

SOURCE=..\..\include\vesa.h
# End Source File
# Begin Source File

SOURCE=..\..\main\wall.h
# End Source File
# Begin Source File

SOURCE=..\..\main\weapon.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE="..\..\..\..\sdl\SDL-1.2.7\lib\SDL.lib"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\sdl\SDL-1.2.7\lib\SDLmain.lib"
# End Source File
# End Target
# End Project
