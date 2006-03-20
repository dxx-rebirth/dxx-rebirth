#include <stdio.h>
#include "vxd.h"
#include "Debug.h"

int _Debug_Vxd = 0;

void VxdGetEntry (int *Entry, int ID)
{
  int dummy1;

  asm ("pushl   %%es            \n\
        movw    %%di, %%es      \n\
        int     $0x2f           \n\
        movl    $0, %%ecx       \n\
        movw    %%es, %%cx      \n\
        popl    %%es"

        : "=c" (Entry [1]), "=D" (Entry [0]), "=a" (dummy1)
        : "2" (0x1684), "b" (ID), "D" (0)
        : "%edx");

 if (_Debug || _Debug_Vxd) fprintf (stderr, "Vxd: Entry for device %x at %x:%x.\r\n", ID, Entry [1], Entry [0]);
}
