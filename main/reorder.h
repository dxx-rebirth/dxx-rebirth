// User interface+data for weapon autoselect order selection
// Arne de Bruijn, 1998
#ifndef _REORDER_H
#define _REORDER_H

#define NEWPRIMS 8
#define NEWSECS 0

extern int default_primary_order[MAX_PRIMARY_WEAPONS+NEWPRIMS];
extern int default_secondary_order[MAX_SECONDARY_WEAPONS+NEWSECS];

extern int primary_order[MAX_PRIMARY_WEAPONS+NEWPRIMS];
extern int secondary_order[MAX_SECONDARY_WEAPONS+NEWSECS];

extern int highest_primary;
extern int highest_secondary;

extern int LaserPowSelected;

void reorder_primary();
void reorder_secondary();

#endif
