#include <stdio.h>

#include "dpmi.h"
#include "timer.h"
#include "key.h"
#include "error.h"
#include "rbaudio.h"


int main()
{
	fix fsecs;
	int oldmin=-1, oldsec=-1;

	error_init(NULL, NULL);
	dpmi_init(0);
	timer_init();
	key_init();
	RBAInit();
	RBARegisterCD();
	RBAPlayTrack(2);

	fsecs = timer_get_fixed_seconds();
	do
	{
		int min, sec, frame;
		long headloc;

		if ((timer_get_fixed_seconds() - fsecs) >= F2_0) {
			headloc = RBAGetHeadLoc(&min, &sec, &frame);
			printf("Head loc: %d (%d:%d:%d)\n", headloc, min, sec, frame);
			if (min==oldmin && sec==oldsec) {
				printf("\nRepeating track..\n");
				RBAPlayTrack(2);
			}
			oldmin = min; oldsec = sec;
			fsecs = timer_get_fixed_seconds();
		}
		if (key_inkey()) {
			RBAStop();
			printf("\nCD stopped.\n");
			break;
		}
	}
	while (1); 

	key_close();
	timer_close();

	return 0;
}
ÿ