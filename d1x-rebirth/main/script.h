#ifndef _SCRIPT_H
#define _SCRIPT_H

void script_notify(int nt_type, ...);
#define NT_ROBOT_DIED	1

void script_init();
void script_reset(); // clear all functions and variabeles
int script_load(char *filename);
//int script_exec(char *function, char *args, ...);
void script_done();

#endif
