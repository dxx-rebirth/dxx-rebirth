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
if USE_OPENGL
  d2x_LDADD = ../3d/lib3d.a ../2d/lib2d.a ../mem/libmem.a ../cfile/libcfile.a ../iff/libiff.a ../texmap/libtexmap.a ../misc/libmisc.a ../maths/libmaths.a ../arch/libarch.a ../video/libvideo.a ../sound/libsound.a ../input/libinput.a -lm -lSDL -ldl -lpthread -lGL -lGLU -lgii -L/usr/X11R6/lib -lXext
else
  d2x_LDADD = ../3d/lib3d.a ../2d/lib2d.a ../mem/libmem.a ../cfile/libcfile.a ../iff/libiff.a ../texmap/libtexmap.a ../misc/libmisc.a ../maths/libmaths.a ../arch/libarch.a ../video/libvideo.a ../sound/libsound.a ../input/libinput.a -lm -lSDL -ldl -lpthread
endif
endif
