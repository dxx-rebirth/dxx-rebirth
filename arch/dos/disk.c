#include <dos.h>
#include <limits.h>
#include "types.h"
#include "error.h"

unsigned long getdiskfree() {
	struct diskfree_t dfree;
        unsigned drive;

        _dos_getdrive(&drive);
        if (!_dos_getdiskfree(drive, &dfree))
		return dfree.avail_clusters * dfree.sectors_per_cluster * dfree.bytes_per_sector;
        else {
                Int3();         // get MARK A!!!!!
		return ULONG_MAX;					// make be biggest it can be
        }
}
