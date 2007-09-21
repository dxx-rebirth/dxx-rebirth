/* Routines to import/export various descent data types.
 * These routines convert various data types to the canonical
 * i386 little endian format, with fixed point numbers; and back.
 *
 * dph is responsible for this, so feel free to flame me.
 *
 * These are architecture specific. They will change on different platforms.
 */

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "maths.h"
#include "vecmat.h"
#include "d_conv.h" // This must be the last include file.

void d_import_vms_vector(vms_vector *dest, char *src)
{
 dest->x = d_import_fix(src);
 src += SIZEOF_FIX;
 dest->y = d_import_fix(src);
 src += SIZEOF_FIX;
 dest->z = d_import_fix(src);
}

void d_export_vms_vector(char *dest, vms_vector *src)
{
 d_export_fix(dest,src->x);
 dest += SIZEOF_FIX;
 d_export_fix(dest,src->y);
 dest += SIZEOF_FIX;
 d_export_fix(dest,src->z);
}

void d_import_vms_matrix(vms_matrix *dest, char *src)
{
 d_import_vms_vector(&dest->rvec,src);
 src += SIZEOF_VMS_VECTOR;
 d_import_vms_vector(&dest->uvec,src);
 src += SIZEOF_VMS_VECTOR;
 d_import_vms_vector(&dest->fvec,src);
}

void d_export_vms_matrix(char *dest, vms_matrix *src)
{
 d_export_vms_vector(dest,&src->rvec);
 dest += SIZEOF_VMS_VECTOR;
 d_export_vms_vector(dest,&src->uvec);
 dest += SIZEOF_VMS_VECTOR;
 d_export_vms_vector(dest,&src->fvec);
}

void d_import_vms_angvec(vms_angvec *dest, char *src)
{
 dest->p = d_import_fixang(src);
 src += SIZEOF_FIXANG;
 dest->b = d_import_fixang(src);
 src += SIZEOF_FIXANG;
 dest->h = d_import_fixang(src);
}

void d_export_vms_angvec(char *dest, vms_angvec *src)
{
 d_export_fixang(dest,src->p);
 dest += SIZEOF_FIXANG;
 d_export_fixang(dest,src->b);
 dest += SIZEOF_FIXANG;
 d_export_fixang(dest,src->h);
}

