TOPDIR = ..

include $(TOPDIR)/makefile.config

OBJECTS = \
 inferno.$(OBJ) game.$(OBJ) gauges.$(OBJ) mglobal.$(OBJ) \
 scores.$(OBJ) object.$(OBJ) laser.$(OBJ) physics.$(OBJ) bm.$(OBJ) \
 menu.$(OBJ) switch.$(OBJ) effects.$(OBJ) texmerge.$(OBJ) polyobj.$(OBJ) \
 fireball.$(OBJ) ai.$(OBJ) aipath.$(OBJ) hostage.$(OBJ) \
 powerup.$(OBJ) fuelcen.$(OBJ) render.$(OBJ) hash.$(OBJ) piggy.$(OBJ) args.$(OBJ) \
 gameseg.$(OBJ) wall.$(OBJ) collide.$(OBJ) lighting.$(OBJ) \
 robot.$(OBJ) morph.$(OBJ) vclip.$(OBJ) weapon.$(OBJ) fvi.$(OBJ) newdemo.$(OBJ) titles.$(OBJ) \
 gameseq.$(OBJ) controls.$(OBJ) automap.$(OBJ) text.$(OBJ) \
 network.$(OBJ) newmenu.$(OBJ) netmisc.$(OBJ) \
 gamefont.$(OBJ) hud.$(OBJ) playsave.$(OBJ) \
 endlevel.$(OBJ) terrain.$(OBJ) kconfig.$(OBJ) modem.$(OBJ)  \
 multi.$(OBJ) cntrlcen.$(OBJ) credits.$(OBJ) config.$(OBJ) kmatrix.$(OBJ) \
 paging.$(OBJ) mission.$(OBJ) songs.$(OBJ) bmread.$(OBJ) state.$(OBJ) \
 multibot.$(OBJ) nocomlib.$(OBJ) slew.$(OBJ) dumpmine.$(OBJ) \
 netlist.$(OBJ) nncoms.$(OBJ) ignore.$(OBJ) \
 custom.$(OBJ) netpkt.$(OBJ) multipow.$(OBJ) reorder.$(OBJ) \
 m_inspak.$(OBJ) radar.$(OBJ) \
 d_gamecv.$(OBJ) d_conv.$(OBJ) \
 mprofile.$(OBJ) hudlog.$(OBJ) \
 cdplay.$(OBJ) mlticntl.$(OBJ) \
 command.$(OBJ) ban.$(OBJ) \
 pingstat.$(OBJ) vlcnfire.$(OBJ) multiver.$(OBJ) reconfig.$(OBJ) \
 altsound.$(OBJ) key.$(OBJ)
# loadrl2.$(OBJ)
# gamesave.$(OBJ) gamemine.$(OBJ)

ifdef NETWORK
OBJECTS += ipx_drv.$(OBJ)
endif

ifdef SUPPORTS_NET_IP
OBJECTS += ip_base.$(OBJ) ipclient.$(OBJ) ipclienc.$(OBJ) 
OBJECTS_DONT_LINK = ipserver.$(OBJ)
endif

ifdef NO_RL2
OBJECTS += gamesave.$(OBJ) gamemine.$(OBJ)
else
OBJECTS += loadrl2.$(OBJ)
endif

ifdef SHAREWARE
OBJECTS += snddecom.$(OBJ)
endif

OUTPUT = $(LIBDIR)/main.$(LIB)
 
include $(TOPDIR)/makefile.lib

