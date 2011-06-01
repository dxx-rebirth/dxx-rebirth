/* $Id: player.c,v 1.1.1.1 2006/03/17 19:56:48 zicodxx Exp $ */

/*
 *
 * Player Stuff
 *
 */

#include "player.h"
#include "byteswap.h"

#ifdef RCS
static char rcsid[] = "$Id: player.c,v 1.1.1.1 2006/03/17 19:56:48 zicodxx Exp $";
#endif

void player_rw_swap(player_rw *p, int swap)
{
	int i;

	if (!swap)
		return;

	p->objnum = SWAPINT(p->objnum);
	p->n_packets_got = SWAPINT(p->n_packets_got);
	p->n_packets_sent = SWAPINT(p->n_packets_sent);
	p->flags = SWAPINT(p->flags);
	p->energy = SWAPINT(p->energy);
	p->shields = SWAPINT(p->shields);
	p->killer_objnum = SWAPSHORT(p->killer_objnum);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		p->primary_ammo[i] = SWAPSHORT(p->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		p->secondary_ammo[i] = SWAPSHORT(p->secondary_ammo[i]);
	p->last_score = SWAPINT(p->last_score);
	p->score = SWAPINT(p->score);
	p->time_level = SWAPINT(p->time_level);
	p->time_total = SWAPINT(p->time_total);
	p->cloak_time = SWAPINT(p->cloak_time);
	p->invulnerable_time = SWAPINT(p->invulnerable_time);
	p->net_killed_total = SWAPSHORT(p->net_killed_total);
	p->net_kills_total = SWAPSHORT(p->net_kills_total);
	p->num_kills_level = SWAPSHORT(p->num_kills_level);
	p->num_kills_total = SWAPSHORT(p->num_kills_total);
	p->num_robots_level = SWAPSHORT(p->num_robots_level);
	p->num_robots_total = SWAPSHORT(p->num_robots_total);
	p->hostages_rescued_total = SWAPSHORT(p->hostages_rescued_total);
	p->hostages_total = SWAPSHORT(p->hostages_total);
	p->homing_object_dist = SWAPINT(p->homing_object_dist);
}
