SUBDIRS = editor old

INCLUDES = -I $(top_srcdir)/includes -fwritable-strings

bin_PROGRAMS = d2x

d2x_SOURCES = \
ai.c	    gamerend.c  lighting.c	paging.c    switch.c \
ai2.c	    effects.c	gamesave.c  menu.c	physics.c   texmerge.c \
aipath.c    endlevel.c	gameseg.c   mglobal.c	piggy.c     text.c \
automap.c   escort.c	gameseq.c   mission.c	playsave.c  titles.c \
bm.c	    fireball.c	gauges.c    morph.c	polyobj.c   vclip.c \
bmread.c    fuelcen.c	hostage.c   multi.c	powerup.c   wall.c \
cntrlcen.c  fvi.c	hud.c	    multibot.c	render.c    weapon.c \
collide.c   game.c	inferno.c   netmisc.c	robot.c \
config.c    gamecntl.c	kconfig.c   network.c	scores.c \
controls.c  gamefont.c	kludge.c    newdemo.c	slew.c \
credits.c   gamemine.c	kmatrix.c   newmenu.c	songs.c \
crypt.c     gamepal.c	laser.c     object.c	state.c \
console.c   cmd.c


d2x_LDFLAGS = -export-dynamic

if ENV_DJGPP
 d2x_LDADD = ../3d/lib3d.a ../2d/lib2d.a ../mem/libmem.a ../cfile/libcfile.a ../iff/libiff.a ../texmap/libtexmap.a ../misc/libmisc.a ../maths/libmaths.a  ../video/libvideo.a ../arch/libarch.a ../sound/libsound.a ../input/libinput.a

else
 d2x_LDADD = ../3d/lib3d.a ../2d/lib2d.a ../mem/libmem.a ../cfile/libcfile.a ../iff/libiff.a ../texmap/libtexmap.a ../misc/libmisc.a ../maths/libmaths.a ../arch/libarch.a ../video/libvideo.a ../sound/libsound.a ../input/libinput.a -lm -ldl -lpthread ${SDL_LIBS}

if USE_OPENGL
 d2x_LDADD += ${OGL_LIBS} -lgii -L/usr/X11R6/lib -lXext
endif

if USE_SVGALIB
 d2x_LDADD += ${SVGA_LIBS}
endif

if USE_GGI
 d2x_LDADD += ${GGI_LIBS}
endif

endif

EXTRA_DIST = \
descent.cfg inferno.ini \
link.bat \
ai.h aistruct.h automap.h bm.h bmread.h cdrom.h cntrlcen.h collide.h \
compbit.h config.h controls.h credits.h desc_id.h desw.h digi.h effects.h \
endlevel.h fireball.h fuelcen.h fvi.h fvi_a.h game.h gamefont.h gamemine.h \
gamepal.h gamesave.h gameseg.h gameseq.h gamestat.h gauges.h hostage.h \
inferno.h joydefs.h kconfig.h kmatrix.h laser.h lighting.h menu.h mission.h \
modem.h morph.h movie.h multi.h multibot.h netmisc.h network.h newdemo.h \
newmenu.h object.h paging.h physics.h piggy.h player.h playsave.h polyobj.h \
powerup.h render.h robot.h scores.h screens.h segment.h segpoint.h slew.h \
songs.h sounds.h state.h switch.h terrain.h texmerge.h text.h textures.h \
titles.h vclip.h vers_id.h wall.h weapon.h \
dumpmine.c
