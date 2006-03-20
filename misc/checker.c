//added 05/17/99 Matt Mueller - checker stubs for various functions.

/* Needed for CHKR_PREFIX.  */
#include "checker_api.h"
#include <setjmp.h>
#include <glob.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "gr.h"

#ifdef __SDL__
//#undef _SDL_STATIC_LIB
#include "SDL.h"
#endif

void chcksetwritable(char * p, int size)__asm__(CHKR_PREFIX ("chcksetwritable"));
void chcksetwritable(char * p, int size){
	stubs_chkr_set_right (p,size,CHKR_RW);
}
void chcksetunwritable(char * p, int size)__asm__(CHKR_PREFIX ("chcksetunwritable"));
void chcksetunwritable(char * p, int size){
	stubs_chkr_set_right (p,size,CHKR_RO);
}

int chkr_stub_glob(const char *pattern, int flags,int errfunc(const char * epath, int eerrno),glob_t *pglob)__asm__ (CHKR_PREFIX ("glob"));
int
chkr_stub_glob(const char *pattern, int flags,int errfunc(const char * epath, int eerrno),glob_t *pglob)
{
	int g;
	stubs_chkr_check_str(pattern,CHKR_RO,"pattern");
	stubs_chkr_check_addr(pglob,sizeof(glob_t),CHKR_WO,"pglob");
	g=glob(pattern,flags,errfunc,pglob);
	if (g==0){
		int i;
		stubs_chkr_set_right (pglob, sizeof(glob_t), CHKR_RW);
		stubs_chkr_set_right (pglob->gl_pathv, sizeof(char*) * pglob->gl_pathc, CHKR_RW);
		for (i=0;i<pglob->gl_pathc;i++)
			stubs_chkr_set_right (pglob->gl_pathv[i], strlen(pglob->gl_pathv[i])+1, CHKR_RW);

	}
	return g;
}

#ifdef __SDL__	   
int chkr_stub_SDL_VideoModeOK (int width, int height, int bpp, Uint32 flags)__asm__(CHKR_PREFIX ("SDL_VideoModeOK"));
int chkr_stub_SDL_VideoModeOK (int width, int height, int bpp, Uint32 flags)
{
	return SDL_VideoModeOK(width,height,bpp,flags);
}
SDL_Surface * chkr_stub_SDL_SetVideoMode (int width, int height, int bpp, Uint32 flags)__asm__(CHKR_PREFIX ("SDL_SetVideoMode"));
SDL_Surface * chkr_stub_SDL_SetVideoMode (int width, int height, int bpp, Uint32 flags)
{
	SDL_Surface * s;
	s=SDL_SetVideoMode(width,height,bpp,flags);
	stubs_chkr_set_right (s,sizeof(SDL_Surface),CHKR_RO);
	stubs_chkr_set_right (s->format,sizeof(SDL_PixelFormat),CHKR_RO);
	stubs_chkr_set_right (s->format->palette,256*3,CHKR_RO);
#ifdef TEST_GR_LOCK
	stubs_chkr_set_right (s->pixels,s->w*s->h*s->format->BytesPerPixel,CHKR_RO);
#else
	stubs_chkr_set_right (s->pixels,s->w*s->h*s->format->BytesPerPixel,CHKR_RW);
#endif
	return s;
}
int chkr_stub_SDL_SetColors (SDL_Surface *surface,SDL_Color *colors, int firstcolor, int ncolors)__asm__ (CHKR_PREFIX ("SDL_SetColors"));
int chkr_stub_SDL_SetColors (SDL_Surface *surface,SDL_Color *colors, int firstcolor, int ncolors){
	int i;

	stubs_chkr_check_addr(surface,sizeof(SDL_Surface),CHKR_RO,"surface");
//	for (i=firstcolor;i<ncolors;i++){
//		stubs_chkr_check_addr(colors,sizeof(Uint8)*3,CHKR_RO,"colors");
//	}
	i=SDL_SetColors (surface,colors,firstcolor,ncolors);
	return i;
}
void chkr_stub_SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)__asm__ (CHKR_PREFIX ("SDL_UpdateRect"));
void chkr_stub_SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h){
	stubs_chkr_check_addr(screen,sizeof(SDL_Surface),CHKR_RO,"screen");
	SDL_UpdateRect(screen,x,y,w,h);
}
void chkr_stub_SDL_WM_SetCaption (const char *title, const char *icon)__asm__ (CHKR_PREFIX ("SDL_WM_SetCaption"));
void chkr_stub_SDL_WM_SetCaption (const char *title, const char *icon){
	stubs_chkr_check_str(title,CHKR_RO,"title");
	if (icon)
		stubs_chkr_check_str(icon,CHKR_RO,"icon");
	SDL_WM_SetCaption(title,icon);
}

void chkr_stub_SDL_PauseAudio (int pause_on)__asm__ (CHKR_PREFIX ("SDL_PauseAudio"));
void chkr_stub_SDL_PauseAudio (int pause_on){
	SDL_PauseAudio(pause_on);
}
void chkr_stub_SDL_CloseAudio (void)__asm__ (CHKR_PREFIX ("SDL_CloseAudio"));
void chkr_stub_SDL_CloseAudio (void){
	SDL_CloseAudio();
}
int chkr_stub_SDL_OpenAudio (SDL_AudioSpec *desired, SDL_AudioSpec *obtained)__asm__ (CHKR_PREFIX ("SDL_OpenAudio"));
int chkr_stub_SDL_OpenAudio (SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
	int i;
	stubs_chkr_check_addr(desired,sizeof(SDL_AudioSpec),CHKR_RO,"desired");
	i=SDL_OpenAudio(desired,obtained);
	if (i && obtained){
		stubs_chkr_set_right (obtained,sizeof(SDL_AudioSpec),CHKR_RO);
	}
	return i;
}
#define CD_Stub(name) int chkr_stub_SDL_CD ## name  (SDL_CD *cdrom)__asm__ (CHKR_PREFIX ("SDL_CD" ## #name));\
int chkr_stub_SDL_CD ## name (SDL_CD *cdrom){\
	return SDL_CD ## name(cdrom);\
}
CD_Stub(Stop);
CD_Stub(Pause);
CD_Stub(Resume);
CDstatus chkr_stub_SDL_CDStatus(SDL_CD *cdrom)__asm__ (CHKR_PREFIX ("SDL_CDStatus"));
CDstatus chkr_stub_SDL_CDStatus(SDL_CD *cdrom){
	return SDL_CDStatus(cdrom);
}
SDL_CD * chkr_stub_SDL_CDOpen (int drive)__asm__ (CHKR_PREFIX ("SDL_CDOpen"));
SDL_CD * chkr_stub_SDL_CDOpen (int drive){
	SDL_CD *c;
	c=SDL_CDOpen(drive);
	if (c)
		stubs_chkr_set_right(c,sizeof(SDL_CD),CHKR_RO);
	return c;
}
int chkr_stub_SDL_CDPlayTracks (SDL_CD *cdrom,int start_track, int start_frame, int ntracks, int nframes)__asm__ (CHKR_PREFIX ("SDL_CDPlayTracks"));
int chkr_stub_SDL_CDPlayTracks (SDL_CD *cdrom,int start_track, int start_frame, int ntracks, int nframes){
	return SDL_CDPlayTracks(cdrom,start_track,start_frame,ntracks,nframes);
}
						

int chkr_stub_SDL_PollEvent (SDL_Event *event)__asm__ (CHKR_PREFIX ("SDL_PollEvent"));
int chkr_stub_SDL_PollEvent (SDL_Event *event){
	int i;
	i=SDL_PollEvent(event);
	if(i && event){
		stubs_chkr_set_right(event,sizeof(SDL_Event),CHKR_RO);
	}
	return i;
}

void chkr_stub_SDL_ShowCursor (int tog)__asm__ (CHKR_PREFIX ("SDL_ShowCursor"));
void chkr_stub_SDL_ShowCursor (int tog){
	SDL_ShowCursor(tog);
}

Uint32 chkr_stub_SDL_GetTicks (void)__asm__ (CHKR_PREFIX ("SDL_GetTicks"));
Uint32 chkr_stub_SDL_GetTicks (void){
	return SDL_GetTicks();
}

char * chkr_stub_SDL_GetError (void)__asm__ (CHKR_PREFIX ("SDL_GetError"));
char * chkr_stub_SDL_GetError (void){
	char * t=SDL_GetError();
	stubs_chkr_set_right(t,strlen(t)+1,CHKR_RO);
	return t;
}

void chkr_stub_SDL_Quit(void)__asm__ (CHKR_PREFIX ("SDL_Quit"));
void chkr_stub_SDL_Quit(void){
	SDL_Quit();
}
int chkr_stub_SDL_Init(Uint32 flags)__asm__ (CHKR_PREFIX ("SDL_Init"));
int chkr_stub_SDL_Init(Uint32 flags){
	return SDL_Init(flags);
}
#endif /* __SDL__ */

int chkr_stub_tcflush(int fd, int queue_selector)__asm__ (CHKR_PREFIX ("tcflush"));
int chkr_stub_tcflush(int fd, int queue_selector){
	fd_used_by_prog(fd);
	return tcflush(fd,queue_selector);
}


int chkr_stub_longjmp(jmp_buf buf,int val)__asm__ (CHKR_PREFIX ("__chcklongjmp"));
int
chkr_stub_longjmp(jmp_buf buf,int val)
{
//	stubs_chkr_check_addr(buf,sizeof(jmp_buf),CHKR_WO,"jmp_buf");
	//return __siglongjmp(buf,val);
	longjmp(buf,val);
}
int chkr_stub_setjmp(jmp_buf buf)__asm__ (CHKR_PREFIX ("__chcksetjmp"));
int
chkr_stub_setjmp(jmp_buf buf)
{
//	stubs_chkr_check_addr(buf,sizeof(jmp_buf),CHKR_WO,"jmp_buf");
	return setjmp(buf);
}
int chkr_stub_sigsetjmp(jmp_buf buf,int save)__asm__ (CHKR_PREFIX ("__chcksigsetjmp"));
int
chkr_stub_sigsetjmp(jmp_buf buf,int save)
{
//	stubs_chkr_check_addr(buf,sizeof(jmp_buf),CHKR_WO,"jmp_buf");
	//return __sigsetjmp(buf,save);
	return sigsetjmp(buf,save);
}

