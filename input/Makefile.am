SUBDIRS = linux 
if USE_OPENGL
 SUBDIRS += ggi
else
 SUBDIRS += sdl
endif

noinst_LIBRARIES = libinput.a

libinput_a_SOURCES =

if USE_OPENGL
libinput_a_LIBADD = ggi/event.o ggi/key.o ggi/mouse.o linux/joydefs.o linux/joystick.o
else
libinput_a_LIBADD = sdl/event.o sdl/key.o sdl/mouse.o linux/joydefs.o linux/joystick.o
endif 

