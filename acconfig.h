/* Define if you want an assembler free build */
#undef NO_ASM

/* Define if you want a network build */
#undef NETWORK

/* Define if you want an OpenGL build */
#undef OGL

/* Define if you want an SVGALib build */
#undef SVGA

/* Define if you want a GGI build */
#undef GGI

/* Define if building under linux */
#undef __ENV_LINUX__

/* Define to disable asserts, int3, etc. */
#undef NDEBUG

@BOTTOM@

/* General defines */
#define NMONO 1
#define PIGGY_USE_PAGING 1
#define NEWDEMO 1

#ifdef __ENV_LINUX__
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
