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

//	Global Variables

FSIterator		iterator;
int				path_len;
FSRef			*refs;
int				i;
char			type[5];
ItemCount 		num_new_files;


//	Functions

int FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct)
{
    FSRef 			parent;
    char			path[_MAX_PATH];
    OSStatus		myErr;
    
    for (path_len = 0; *search_str && *search_str != '*'; path_len ++)
        path[path_len] = *search_str++;
    path[path_len] = 0;
    if (*search_str == '*')
        search_str++;
    strcpy(type, search_str);

#ifdef macintosh
    // Convert the search directory to an FSRef in a way that OS 9 can handle
    {
        FSSpec		parentSpec;
        Str255		pascalPath;

        macify_posix_path(path, path);
        CopyCStringToPascal(path, pascalPath);
        if (FSMakeFSSpec(0, 0, pascalPath, &parentSpec) != noErr)
            return 1;
        if (FSpMakeFSRef(&parentSpec, &parent) != noErr)
            return 1;
    }
#else
// "This function, though available through Carbon on Mac OS 8 and 9, is only implemented on Mac OS X."
    if ((myErr = FSPathMakeRef((unsigned char const *) (path_len? path : "."), &parent, NULL)) != noErr)
        return 1;
#endif
    if (FSRefMakePath(&parent, (unsigned char *) path, _MAX_PATH) != noErr)	// Get the full path, to get the length
        return 1;
    path_len = strlen(path)
#ifndef macintosh
        + 1 // For the '/'
#endif
        ;
    
    if (FSOpenIterator(&parent, kFSIterateFlat, &iterator) != noErr)
        return 1;
    
    MALLOC(refs, FSRef, 256);
    
    myErr = FSGetCatalogInfoBulk(iterator, 256, &num_new_files, NULL, kFSCatInfoNone, NULL, refs, NULL, NULL);
    
    i = 0;
    do {
        char *p = path + path_len;

        if (i >= num_new_files) {
            if (!myErr) {
                myErr = FSGetCatalogInfoBulk(iterator, 256, &num_new_files, NULL, kFSCatInfoNone, NULL, refs, NULL, NULL);
                i = 0;
            } else
                return 1;	// The last file
        }
        
        FSRefMakePath (refs + i, (unsigned char *) path, 255);

        i++;
        if (!stricmp(p + strlen(p) - strlen(search_str), search_str)) {
            strncpy(ffstruct->name, p, 256);
            return 0;	// Found one
        }
    } while (1);
}

int	FileFindNext(FILEFINDSTRUCT *ffstruct)
{
    OSErr myErr;
    
    do {
        char path[_MAX_PATH];
        char *p = path + path_len;
    
        if (i >= num_new_files) {
            if (!myErr) {
                myErr = FSGetCatalogInfoBulk(iterator, 256, &num_new_files, NULL, kFSCatInfoNone, NULL, refs, NULL, NULL);
                i = 0;
            } else
                return 1;	// The last file
        }
    
        FSRefMakePath (refs + i, (unsigned char *) path, 255);
    
        i++;
        if (!stricmp(p + strlen(p) - strlen(type), type)) {
            strncpy(ffstruct->name, p, 256);
            return 0;	// Found one
        }
    } while (1);
}

int	FileFindClose(void)
{
    d_free(refs);
    if (FSCloseIterator(iterator) != noErr)
        return 1;
    
    return 0;
}
