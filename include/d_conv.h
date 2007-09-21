/* Routines to import/export various descent data types.
 * These routines convert various data types to the canonical
 * i386 little endian format, with fixed point numbers.
 *
 * #INCLUDE THIS FILE AFTER YOUR OTHER INCLUDE FILES!
 *
 * dph is responsible for this, so feel free to flame me.
 *
 * These are architecture specific. They will change on different platforms.
 */

#ifndef _D_CONV_H
#define _D_CONV_H

#define SIZEOF_SHORT            2
#define SIZEOF_INT              4
#define SIZEOF_FIX              4
#define SIZEOF_FIXANG           2
#define SIZEOF_VMS_VECTOR       12
#define SIZEOF_VMS_MATRIX       36
#define SIZEOF_VMS_ANGVEC       6

static __inline int d_import_int(char *src)
{
 return *((int *)src);
}

static __inline void d_export_int(char *dest, int src)
{
 *((int *)dest) = src;
}

static __inline short d_import_short(char *src)
{
 return *((short *)src);
}

static __inline void d_export_short(char *dest, short src)
{
 *((short *)dest) = src;
}

#ifdef _MATHS_H // If the maths header has been included
static __inline fix d_import_fix(char *src)
{
 return *((fix *)src);
}

static __inline void d_export_fix(char *dest, fix src)
{
 *((fix *)dest) = src;
}

static __inline fixang d_import_fixang(char *src)
{
 return *((fixang *)src);
}

static __inline void d_export_fixang(char *dest, fixang src)
{
 *((fixang *)dest) = src;
}

#endif // (_MATHS_H)

#ifdef _VECMAT_H

extern void d_import_vms_vector(vms_vector *dest, char *src);
extern void d_export_vms_vector(char *dest, vms_vector *src);
extern void d_import_vms_matrix(vms_matrix *dest, char *src);
extern void d_export_vms_matrix(char *dest, vms_matrix *src);
extern void d_import_vms_angvec(vms_angvec *dest, char *src);
extern void d_export_vms_angvec(char *dest, vms_angvec *src);

#endif // (_VECMAT_H)

#endif // !(_D_CONV_H)
