noinst_LIBRARIES = libvideo.a

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/includes -I$(top_srcdir)/main -I$(top_srcdir)/arch/include

if ENV_DJGPP
libvideo_a_SOURCES = dos_gr.c
else
if USE_OPENGL
libvideo_a_SOURCES = ogl_gr.c ogl.c ogl_sdl.c
else
if USE_GGI
libvideo_a_SOURCES = ggi_gr.c
else
if USE_SVGALIB
libvideo_a_SOURCES = svgalib_gr.c
else
libvideo_a_SOURCES = sdl_gr.c
endif
endif
endif
endif

EXTRA_libvideo_a_SOURCES = \
dos_gr.c ogl_gr.c ogl.c ogl_sdl.c ggi_gr.c svgalib_gr.c sdl_gr.c

# Not currently used
EXTRA_DIST = \
ogl_wgl.c ogl_glx.c
