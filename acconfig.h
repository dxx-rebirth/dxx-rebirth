/* Define if you want an assembler free build */
#undef NO_ASM

/* Define if you want a network build */
#undef NETWORK

/* Define if you want to build the editor */
#undef EDITOR

/* Define if you want to build the demo version */
#undef SHAREWARE

/* Define if you want an OpenGL build */
#undef OGL

/* Define if you want an SVGALib build */
#undef SVGA

/* Define if you want a GGI build */
#undef GGI

/* Define to disable asserts, int3, etc. */
#undef NDEBUG

/* Define for a "release" build */
#undef RELEASE

/* Define to enable trick to show movies */
#undef MOVIE_TRICK

/* Define to render endlevel flythrough sequences (instead of movies) */
#undef NMOVIES

/* Define to use SDL Joystick */
#undef SDL_JOYSTICK

/* Define this to be the shared game directory root */
#undef SHAREPATH

/* d2x major version */
#undef D2XMAJOR

/* d2x minor version */
#undef D2XMINOR

/* d2x micro version */
#undef D2XMICRO

@BOTTOM@
/* General defines */
#define NMONO 1
#define PIGGY_USE_PAGING 1
#define NEWDEMO 1

#ifdef __linux__
# define __SDL__ 1
# define SDL_AUDIO 1

# ifdef OGL
#  define SDL_GL_VIDEO 1
#  define SDL_INPUT 1
# else
#  ifdef GGI
#   define GGI_VIDEO 1
#   define GII_INPUT 1
#  else
#   ifdef SVGA
#    define SVGALIB_VIDEO 1
#    define SVGALIB_INPUT 1
#   else
#    define SDL_VIDEO 1
#    define SDL_INPUT 1
#   endif
#  endif
# endif
#endif

#ifdef __MINGW32__
# define __SDL__ 1
# define SDL_AUDIO 1
# define SDL_INPUT 1
# ifdef OGL
#  define SDL_GL_VIDEO 1
# else
#  define SDL_VIDEO 1
# endif
#endif
