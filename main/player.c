/* $Id: player.c,v 1.3 2003-10-10 09:36:35 btb Exp $ */

/*
 *
 * Player Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "player.h"
#include "cfile.h"

#ifdef RCS
static char rcsid[] = "$Id: player.c,v 1.3 2003-10-10 09:36:35 btb Exp $";
#endif

/*
 * reads a player_ship structure from a CFILE
 */
void player_ship_read(player_ship *ps, CFILE *fp)
{
	int i;

	ps->model_num = cfile_read_int(fp);
	ps->expl_vclip_num = cfile_read_int(fp);
	ps->mass = cfile_read_fix(fp);
	ps->drag = cfile_read_fix(fp);
	ps->max_thrust = cfile_read_fix(fp);
	ps->reverse_thrust = cfile_read_fix(fp);
	ps->brakes = cfile_read_fix(fp);
	ps->wiggle = cfile_read_fix(fp);
	ps->max_rotthrust = cfile_read_fix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		cfile_read_vector(&(ps->gun_points[i]), fp);
}
