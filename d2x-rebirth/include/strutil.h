#ifndef _STRUTILS_H
#define _STRUTILS_H

#if defined(macintosh)
extern void snprintf(char *out_string, int size, char * format, ... );
#endif
extern int d_stricmp( const char *s1, const char *s2 );
extern int d_strnicmp( const char *s1, const char *s2, int n );
extern void d_strlwr( char *s1 );
extern void d_strupr( char *s1 );
extern void d_strrev( char *s1 );
extern char *d_strdup(char *str);

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

//give a filename a new extension, doesn't work with paths with no extension already there
extern void change_filename_extension( char *dest, const char *src, char *new_ext );

extern void d_splitpath(char *name, char *drive, char *path, char *base, char *ext);

// create a growing 2D array with a single growing buffer for the text
// this system is likely to cause less memory fragmentation than having one malloc'd buffer per string
int string_array_new(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf);

// add a string to a growing 2D array
int string_array_add(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, const char *str);

// sort function passed to qsort - also useful for 2d string arrays with individual string pointers 
int string_array_sort_func(char **e0, char **e1);

// reallocate pointers to save memory, sort list alphabetically and remove duplicates according to 'comp'
void string_array_tidy(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, int offset, int (*comp)( const char *, const char * ));

#endif /* _STRUTILS_H */
