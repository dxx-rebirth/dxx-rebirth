/* 3/15/99 by Victor Rachels to add new vulcan firing system for less lag

 *   basically works by sending ON/OFF packet to toggle vulcan.  Vulcan fires
 *   on local system every X ms.  ON packet is resent every Y ms. if no ON
 *   packet is received in Y+Z ms, vulcan is automatically shut off */
#ifndef _VULCAN_H
#define _VULCAN_H

typedef struct VulcanPlayerStruct
{
  int vulcanon;
  fix lastvulcanfired;
  fix lastvulcaninfo;
} VulcanPlayerStruct;

void vulcan_init();
void vulcanframe();
void got_vulcan_info(int vulcan_on, int pl);
void do_vulcan_fire(int vulcan_on);

extern int use_alt_vulcanfire;
#endif
