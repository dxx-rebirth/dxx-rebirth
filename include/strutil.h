//Created on 6/15/99 by Owen Evans to finally remove all those "implicit
//declaration" warnings for the string functions in Linux

#ifdef __LINUX__
extern int stricmp(const char *s1, const char *s2);
extern int strnicmp(const char *s1, const char *s2, int n);
extern void strupr(char *s1);
extern void strlwr(char *s1);
#endif
