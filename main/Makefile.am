SUBDIRS = @D2X_MAIN_SUBDIRS@

DIST_SUBDIRS = editor

EXTRA_SUBDIRS = old

noinst_LIBRARIES = libmain.a

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/arch/include

if USE_NETWORK
NETWORK_SRCS = ipx_drv.c kmatrix.c multi.c multibot.c network.c
#               ipclienc.c ip_base.cpp ipclient.cpp netlist.c netpkt.c multipow.c
endif

if EDITOR
EDITOR_SRCS = bmread.c
endif

if SHAREWARE
SHAREWARE_SRCS = terrain.c
endif

MOVIE_SRCS = movie.c mvelib.c mve_audio.c mveplay.c

libmain_a_SOURCES = \
${EDITOR_SRCS} ${NETWORK_SRCS} ${SHAREWARE_SRCS} ${MOVIE_SRCS} \
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
console.c   cmd.c       player.c    segment.c

EXTRA_libmain_a_SOURCES = \
ipx_drv.c kmatrix.c multi.c multibot.c network.c netpkt.c \
bmread.c terrain.c movie.c ipclienc.c netlist.c multipow.c \
ip_base.cpp ipclient.cpp

EXTRA_DIST = ${EXTRA_SUBDIRS} \
descent.cfg inferno.ini \
link.bat \
ai.h aistruct.h automap.h bm.h bmread.h cntrlcen.h collide.h \
compbit.h config.h controls.h credits.h desc_id.h desw.h digi.h effects.h \
endlevel.h fireball.h fuelcen.h fvi.h fvi_a.h game.h gamefont.h gamemine.h \
gamepal.h gamesave.h gameseg.h gameseq.h gamestat.h gauges.h hostage.h \
hudmsg.h inferno.h ip_base.h ip_basec.h ipclient.h joydefs.h kconfig.h \
kmatrix.h laser.h lighting.h menu.h mission.h modem.h morph.h movie.h multi.h \
multibot.h multipow.h netmisc.h netpkt.h network.h newdemo.h newmenu.h \
object.h paging.h physics.h piggy.h player.h playsave.h polyobj.h powerup.h \
render.h reorder.h robot.h scores.h screens.h segment.h segpoint.h slew.h \
songs.h sounds.h state.h switch.h terrain.h texmerge.h text.h textures.h \
titles.h vclip.h vers_id.h wall.h weapon.h \
dumpmine.c \
ipserver.cpp

