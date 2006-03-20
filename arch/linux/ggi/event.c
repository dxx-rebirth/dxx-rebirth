#include <stdio.h>
#include <sys/time.h>
#include <ggi/gii.h>
#include <string.h>
#include "event.h"
#include "error.h"
#include "fix.h"

static int initialised=0;

#ifdef GGI_VIDEO
#include <ggi/ggi.h>
extern ggi_visual_t *screenvis;
#endif
gii_input_t inputs;

extern void keyboard_handler(int key, ubyte state);
extern void mouse_handler_button(int button, ubyte state);
extern void mouse_handler_relative(int x, int y);
extern void mouse_handler_absolute(int x, int y);

void event_poll()
{
	int n;
	struct timeval tv;
	gii_event event;

	if (!initialised)
		event_init();
	if (inputs==NULL)
		return;
//		Error("GII error: no inputs (perhaps you need to set GII_INPUT env var)\n");

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	giiEventPoll(inputs, emAll, &tv);
	
	n = giiEventsQueued(inputs, emAll);
	
	while (n-- > 0) 
	{
		giiEventRead(inputs, &event, emAll);
		switch (event.any.type)
		{
			case evKeyPress:
				keyboard_handler(event.key.label, 1);
				break;
			case evKeyRelease:
				keyboard_handler(event.key.label, 0);
				break;
			case evPtrAbsolute:
				mouse_handler_absolute(event.pmove.x, event.pmove.y);
				break;
			case evPtrRelative:
                	        mouse_handler_relative(event.pmove.x, event.pmove.y);
				break;
			case evPtrButtonPress:
				mouse_handler_button(event.pbutton.button, 1);
				break;
		        case evPtrButtonRelease:
				mouse_handler_button(event.pbutton.button, 0);
				break;
		}
	}
}

void event_close()
{
	if (inputs)
		giiClose(inputs);
	giiExit();
}

#ifdef GII_XWIN
int gii_xwin_initialized=0;
#include <ggi/input/xwin.h>
//void lock_nothing(void){return;}
void init_gii_xwin(Display *disp,Window win){
	printf("gii xwin %i %i\n",initialised,gii_xwin_initialized);
	if (!initialised)
		event_init();
	if (gii_xwin_initialized){
		gii_event ev;
		gii_xwin_cmddata_setparam *giiargs=(gii_xwin_cmddata_setparam *) &ev.cmd.data;
		memset(&ev,0,sizeof(gii_cmd_nodata_event)+sizeof(gii_xwin_cmddata_setparam));
		ev.cmd.code=GII_CMDCODE_XWINSETPARAM;
		ev.any.type = evCommand;
		ev.any.size=sizeof(gii_cmd_nodata_event)+sizeof(gii_xwin_cmddata_setparam);
		giiargs->win=win;
		giiargs->ptralwaysrel=1;
		giiEventSend(inputs,&ev);
	}else{
		gii_input_t inputs2;
		gii_inputxwin_arg giiargs;
		memset(&giiargs,0,sizeof(giiargs));
		giiargs.disp=disp;
		giiargs.win=win;
		giiargs.ptralwaysrel=1;
		//giiargs.gglock=lock_nothing;
		inputs2=giiOpen("xwin",&giiargs,NULL);
		if (inputs2){
			gii_xwin_initialized=1;
			if (inputs)
				inputs=giiJoinInputs(inputs,inputs2);
			else
				inputs=inputs2;
		}
	}
}
#endif

int event_init()
{
	if (!initialised){
		giiInit();
#ifdef GGI_VIDEO
		inputs = ggiJoinInputs(screenvis, NULL);
#else
		inputs=giiOpen(NULL);
#endif
		initialised = 1;
		atexit(event_close);
	}
	return 0;
}
