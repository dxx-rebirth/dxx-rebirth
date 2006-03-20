#ifndef PINGSTAT_H
#define PINGSTAT_H

#ifdef NETWORK
extern int ping_stats_on;

void ping_stats_frame();
void ping_stats_received(int pl, int pingtime);
void ping_stats_init();
void ping_stats_sentping();
int ping_stats_getping(int pl);
int ping_stats_getsent(int pl);
int ping_stats_getgot(int pl);
#endif
#endif
