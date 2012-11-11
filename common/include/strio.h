/* fileio.c in /misc for d1x file reading */
#ifndef _STRIO_H
#define _STRIO_H

#ifdef __cplusplus
extern "C" {
#endif

char* fgets_unlimited(PHYSFS_file *f);
char *splitword(char *s, char splitchar);

#ifdef __cplusplus
}
#endif

#endif
