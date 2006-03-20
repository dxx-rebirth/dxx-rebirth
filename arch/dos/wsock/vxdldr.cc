#include <dpmi.h>
#include <string.h>
#include <stdio.h>

#include "farptrx.h"
#include "debug.h"

#include "vxdldr.h"
#include "vxd.h"

#define VxdLdr_DeviceID 0x0027
#define VxdLdr_LoadDevice 1
#define VxdLdr_UnLoadDevice 2

int _Debug_VxdLdr = 0;

int VxdLdrEntry [2] = {0, 0};

void VxdLdrInit (void)
{
  VxdGetEntry (VxdLdrEntry, VxdLdr_DeviceID);
}

int VxdLdrLoadDevice (char Device [])
{
  if (VxdLdrEntry [1] == 0) VxdLdrInit ();
  if (VxdLdrEntry [1] == 0) return -1;

  int Error;
  int ExitCode;

  __dpmi_meminfo _dev;
  int dev;

  _dev.handle = 0;
  _dev.size = strlen (Device) + 1;
  _dev.address = 0;

  __dpmi_allocate_memory (&_dev);
  dev = __dpmi_allocate_ldt_descriptors (1);
  __dpmi_set_segment_base_address (dev, _dev.address);
  __dpmi_set_segment_limit (dev, _dev.size);

  _farpokex (dev, 0, Device, _dev.size);

  asm ("pushl   %%ds            \n\
        movw    %%cx, %%ds      \n\
        lcall   %%cs:_VxdLdrEntry \n\
        movzwl  %%ax, %%ebx
        setc    %%al            \n\
        movzbl  %%al, %%eax     \n\
        popl    %%ds"
        : "=a" (Error), "=b" (ExitCode)
        : "a" (VxdLdr_LoadDevice), "c" (dev), "d" (0));

  __dpmi_free_ldt_descriptor (dev);
  __dpmi_free_memory (_dev.handle);

  if (_Debug || _Debug_VxdLdr)
  {
    if (Error) fprintf (stderr, "VxdLdr: \"%s\" loaded unsucessfully. (%x)\r\n", Device, ExitCode);
      else  fprintf (stderr, "VxdLdr: \"%s\" loaded sucessfully. (%x)\r\n", Device, ExitCode);
  }

  return Error;
}

int VxdLdrUnLoadDevice (char Device [])
{
  if (VxdLdrEntry [1] == 0) VxdLdrInit ();
  if (VxdLdrEntry [1] == 0) return -1;

  int Error;
  int ExitCode;

  __dpmi_meminfo _dev;
  int dev;

  _dev.handle = 0;
  _dev.size = strlen (Device) + 1;
  _dev.address = 0;

  __dpmi_allocate_memory (&_dev);
  dev = __dpmi_allocate_ldt_descriptors (1);
  __dpmi_set_segment_base_address (dev, _dev.address);
  __dpmi_set_segment_limit (dev, _dev.size);

  asm ("pushl   %%ds            \n\
        movw    %%cx, %%ds      \n\
        lcall   %%cs:_VxdLdrEntry \n\
        movzwl  %%ax, %%ebx
        setc    %%al            \n\
        movzbl  %%al, %%eax     \n\
        popl    %%ds"
        : "=a" (Error), "=b" (ExitCode)
        : "a" (VxdLdr_UnLoadDevice), "c" (dev), "d" (0));

  __dpmi_free_ldt_descriptor (dev);
  __dpmi_free_memory (_dev.handle);

  if (_Debug || _Debug_VxdLdr)
  {
    if (Error) fprintf (stderr, "VxdLdr: \"%s\" loaded unsucessfully. (%x)\r\n", Device, ExitCode);
      else  fprintf (stderr, "VxdLdr: \"%s\" loaded sucessfully. (%x)\r\n", Device, ExitCode);
  }

  return Error;
}
