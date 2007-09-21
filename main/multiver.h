//multiver.h added 4/18/99 - Matt Mueller

#ifndef _MULTIVER_H
#define _MULTIVER_H

void multi_do_d1x_ver_set(int src,int shp, int pps);
void multi_d1x_ver_queue_init(int host,int mode);
void multi_do_d1x_ver(char * buf);
void multi_d1x_ver_queue_send(int dest, int mode);
void multi_d1x_ver_send(int dest, int mode);
void multi_d1x_ver_queue_remove(int dest);
void multi_d1x_ver_frame(void);
void network_send_config_messages(int dest, int mode);

#endif
