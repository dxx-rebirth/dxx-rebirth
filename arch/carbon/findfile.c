/*
 *  findfile.c
 *  D2X (Descent2)
 *
 *  Created by Chris Taylor on Tue Jun 22 2004.
 *
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include "pstypes.h"
#include "findfile.h"
#include "u_mem.h"
#include "strutil.h"

#ifdef macintosh
#include <Files.h>
extern void macify_posix_path(char *posix_path, char *mac_path);
#else
#include <CoreServices/CoreServices.h>
#endif

// Gets the names of matching files and the number of matching files.
int FindFiles(char *search_str, int max_matches, char **names)
{
    FSRef 			parent;
    char			path[_MAX_PATH];
    int				path_len;
    FSRef			*refs;
    ItemCount		num_found = 0;
    FSIterator		iterator;
    int				i;
    OSStatus		myErr;
    
    for (path_len = 0; *search_str && *search_str != '*'; path_len ++)
        path[path_len] = *search_str++;
    path[path_len] = 0;
    if (*search_str == '*')
        search_str++;

#ifdef macintosh
    // Convert the search directory to an FSRef in a way that OS 9 can handle
    {
        FSSpec		parentSpec;
        Str255		pascalPath;

        macify_posix_path(path, path);
        CopyCStringToPascal(path, pascalPath);
        if (FSMakeFSSpec(0, 0, pascalPath, &parentSpec) != noErr)
            return 0;
        if (FSpMakeFSRef(&parentSpec, &parent) != noErr)
            return 0;
    }
#else
// "This function, though available through Carbon on Mac OS 8 and 9, is only implemented on Mac OS X."
    if ((myErr = FSPathMakeRef((unsigned char const *) (path_len? path : "."), &parent, NULL)) != noErr)
        return 0;
#endif
    if (FSRefMakePath(&parent, (unsigned char *) path, _MAX_PATH) != noErr)	// Get the full path, to get the length
        return 0;
    path_len = strlen(path)
#ifndef macintosh
        + 1 // For the '/'
#endif
        ;
    
    if (FSOpenIterator(&parent, kFSIterateFlat, &iterator) != noErr)
        return 0;
    
    MALLOC(refs, FSRef, max_matches);
    
    do {
        ItemCount num_new_files;
        
        myErr = FSGetCatalogInfoBulk(iterator, max_matches, &num_new_files, NULL, kFSCatInfoNone, NULL, refs, NULL, NULL);
        
        for (i = 0; i < num_new_files; i++) {
            char *p = path + path_len;
            char *filename;
            
            FSRefMakePath (refs + i, (unsigned char *) path, 255);
            for (filename = p; *filename && strnicmp(filename, search_str, strlen(search_str)); filename++) {}
            
            if (*filename) {
                strncpy(*names, p, 12);
                num_found++;
                names++;
            }
        }
    } while (myErr == noErr && num_found < max_matches);
    
    d_free(refs);
    if (FSCloseIterator(iterator) != noErr)
        return 0;
    
    return num_found;
}
