SUBDIRS = @D2X_MAIN_SUBDIRS@

DIST_SUBDIRS = editor

EXTRA_SUBDIRS = old

noinst_LIBRARIES = libmain.a

INCLUDES = -I$(top_srcdir)/includes -I$(top_srcdir)/arch/include

CFLAGS += -fwritable-strings ${SDL_CFLAGS}

if USE_NETWORK
NETWORK_SRCS = ipx_drv.c kmatrix.c multi.c multibot.c network.c
else
NETWORK_SRCS =
endif

if EDITOR
EDITOR_SRCS = bmread.c
else
EDITOR_SRCS =
endif

libmain_a_SOURCES = ${EDITOR_SRCS} ${NETWORK_SRCS} \
ai.c	    gamerend.c  lighting.c  paging.c    switch.c \
ai2.c	    effects.c	gamesave.c  menu.c	physics.c   texmerge.c \
aipath.c    endlevel.c	gameseg.c   mglobal.c	piggy.c     text.c \
automap.c   escort.c	gameseq.c   mission.c	playsave.c  titles.c \
bm.c	    fireball.c	gauges.c    morph.c	polyobj.c   vclip.c \
fuelcen.c   hostage.c   powerup.c   wall.c \
cntrlcen.c  fvi.c	hud.c	    render.c    weapon.c \
collide.c   game.c	inferno.c   netmisc.c	robot.c \
config.c    gamecntl.c	kconfig.c   scores.c \
controls.c  gamefont.c	kludge.c    newdemo.c	slew.c \
credits.c   gamemine.c	newmenu.c   songs.c \
crypt.c     gamepal.c	laser.c     object.c	state.c \
console.c   cmd.c       terrain.c

EXTRA_DIST = ${EXTRA_SUBDIRS} \
descent.cfg inferno.ini \
link.bat \
ai.h aistruct.h automap.h bm.h bmread.h cdrom.h cntrlcen.h collide.h \
compbit.h config.h controls.h credits.h desc_id.h desw.h digi.h effects.h \
endlevel.h fireball.h fuelcen.h fvi.h fvi_a.h game.h gamefont.h gamemine.h \
gamepal.h gamesave.h gameseg.h gameseq.h gamestat.h gauges.h hostage.h \
hudmsg.h \
inferno.h joydefs.h kconfig.h kmatrix.h laser.h lighting.h menu.h mission.h \
modem.h morph.h movie.h multi.h multibot.h netmisc.h network.h newdemo.h \
newmenu.h object.h paging.h physics.h piggy.h player.h playsave.h polyobj.h \
powerup.h render.h robot.h scores.h screens.h segment.h segpoint.h slew.h \
songs.h sounds.h state.h switch.h terrain.h texmerge.h text.h textures.h \
titles.h vclip.h vers_id.h wall.h weapon.h \
dumpmine.c ipx_drv.c
