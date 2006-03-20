/* $Id: escort.h,v 1.1.1.1 2006/03/17 19:55:00 zicodxx Exp $ */

/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

extern int Buddy_dude_cheat;


#define GUIDEBOT_NAME_LEN 9
extern char guidebot_name[];
extern char real_guidebot_name[];

extern void change_guidebot_name(void);


extern void do_escort_menu(void);
extern void detect_escort_goal_accomplished(int index);
extern void set_escort_special_goal(int key);

#endif // _ESCORT_H
