// Empty file
typedef struct FILEFINDSTRUCT {
	        unsigned long size;
		        char name[256];
} FILEFINDSTRUCT;
int FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct);
int FileFindNext(FILEFINDSTRUCT *ffstruct);
int FileFindClose(void);

typedef struct FILETIMESTRUCT {
	        unsigned short date,time;
} FILETIMESTRUCT;

//the both return 0 if no error
//int GetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct);
//int SetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct);
//
