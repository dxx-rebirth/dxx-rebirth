#ifndef _internal_h_
#define _internal_h_


#define PRINT_ERROR(X) fprintf(stderr, "ERROR in %s:%s(): %s\n", __FILE__, __FUNCTION__, X)

Uint32	DT_GetPixel(SDL_Surface *surface, int x, int y);
void	DT_PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);


#endif /* _internal_h_ */


