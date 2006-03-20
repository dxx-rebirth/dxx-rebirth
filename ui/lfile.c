/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "key.h"

#include "ui.h"
#include "mono.h"

//#include "mem.h"
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM


char filename_list[300][13];
char directory_list[100][13];

static int error_mode = 0;

void InstallErrorHandler()
{
       //Above line modified by KRB, added (void *) cast
       //for the compiler.
}

void file_sort( int n, char list[][13] )
{
       int i, j, incr;
       char t[14];

       incr = n / 2;
       while( incr > 0 )
       {
               for (i=incr; i<n; i++ )
               {
                       j = i - incr;
                       while (j>=0 )
                       {
                               if (strncmp(list[j], list[j+incr], 12) > 0)
                               {
                                       memcpy( t, list[j], 13 );
                                       memcpy( list[j], list[j+incr], 13 );
                                       memcpy( list[j+incr], t, 13 );
                                       j -= incr;
                               }
                               else
                                       break;
                       }
               }
               incr = incr / 2;
       }
}


int SingleDrive()
{
       int FloppyPresent, FloppyNumber;
       unsigned char b;

       b = *((unsigned char *)0x410);

       FloppyPresent = b & 1;
       FloppyNumber = ((b&0xC0)>>6)+1;
       if (FloppyPresent && (FloppyNumber==1) )
               return 1;
       else
               return 0;
}

void SetFloppy(int d)
{
       if (SingleDrive())
       {
       if (d==1)
               *((unsigned char *)0x504) = 0;
       else if (d==2)
               *((unsigned char *)0x504) = 1;
       }
}


void file_capitalize( char * s )
{
       while( (*s++ = toupper(*s)) );
}



// Changes to directory in dir.  Even drive is changed.
// Returns 1 if failed.
//  0 = Changed ok.
//  1 = Invalid disk drive.
//  2 = Invalid directory.

int file_chdir( char * dir )
{
       int e;
       char OriginalDirectory[100];
       char * Path;
       char NoDir[] = "/.";

       getcwd( OriginalDirectory, 100 );

       {
               Path = dir;

       }

       if (!(*Path))
       {
               Path = NoDir;
       }

       // This chdir might get a critical error!
       e = chdir( Path );
       if (e)
       {
               return 2;
       }
       return 0;

}


int file_getdirlist( int MaxNum, char list[][13] )
{
       struct stat fileinfo;
       DIR *dir;
       struct dirent *dirent;
       int NumDirs = 0;
       char cwd[PATH_MAX];

       getcwd(cwd, 128 );

       if (strlen(cwd) >= 2)
       {
               sprintf( list[NumDirs++], ".." );
       }

       if ((dir = opendir("."))) {
               while ((dirent = readdir(dir))) {
                       if (!stat(dirent->d_name, &fileinfo) && S_ISDIR(fileinfo.st_mode) &&
                               strcmp( "..", dirent->d_name) && strcmp( ".", dirent->d_name)) {
                               if (NumDirs==100) {
                                       MessageBox( -2,-2, 1, "Only the first 100 directories will be displayed.", "Ok" );
                                       break;
                               } else {
                                       list[NumDirs][12] = 0;
                                       strncpy(list[NumDirs++], dirent->d_name, 12 );
                               }
                       }
               }
       }

       file_sort( NumDirs, list );

       return NumDirs;
}

int file_getfilelist( int MaxNum, char list[][13], char * filespec )
{
       struct stat fileinfo;
       DIR *dir;
       struct dirent *dirent;
       int NumFiles = 0;

       if ((dir = opendir("."))) {
               while ((dirent = readdir(dir))) {
                       if (!stat(dirent->d_name, &fileinfo) && S_ISREG(fileinfo.st_mode)) {
                               if (NumFiles==300)
                               {
                                       MessageBox( -2,-2, 1, "Only the first 300 files will be displayed.", "Ok" );
                                       break;
                               } else {
                                       list[NumFiles][12] = 0;
                                       strncpy(list[NumFiles++], dirent->d_name, 12 );
                               }
                       }
               }
       }

       file_sort( NumFiles, list );

       return NumFiles;
}


void split_dir_name(const char *path, char *dir, char *name)
{
       char *p = strrchr(path, '/');
       if (p) {
               if (name) strcpy(name, p + 1);
               if (dir) {
                       strncpy(dir, path, p - path);
                       dir[p - path] = 0;
               }
       } else {
               if (name) strcpy(name, path);
               if (dir) *dir = 0;
       }
}

static int FirstTime = 1;
static char CurDir[128];

int ui_get_filename( char * filename, char * Filespec, char * message  )
{
       FILE * TempFile;
       int NumFiles, NumDirs,i;
       char InputText[100];
       char Spaces[35];
       char ErrorMessage[100];
       UI_WINDOW * wnd;
       UI_GADGET_BUTTON * Button1, * Button2, * HelpButton;
       UI_GADGET_LISTBOX * ListBox1;
       UI_GADGET_LISTBOX * ListBox2;
       UI_GADGET_INPUTBOX * UserFile;
       int new_listboxes;

       char fulldir[PATH_MAX];
       char fullfname[PATH_MAX];


       char OrgDir[128];

       getcwd( OrgDir, 128 );

       if (FirstTime)
               getcwd( CurDir, 128 );
       FirstTime=0;

       file_chdir( CurDir );
 
       //MessageBox( -2,-2, 1,"DEBUG:0", "Ok" );
       for (i=0; i<35; i++)
               Spaces[i] = ' ';
       Spaces[34] = 0;

       NumFiles = file_getfilelist( 300, filename_list, Filespec );

       NumDirs = file_getdirlist( 100, directory_list );

       wnd = ui_open_window( 200, 100, 400, 370, WIN_DIALOG );

       ui_wprintf_at( wnd, 10, 5, message );

       split_dir_name(filename, NULL, InputText);
       {
               char *p = strrchr(filename, '/');
               if (p)
                       strcpy(InputText, p + 1);
               else
                       strcpy(InputText, filename);
       }

       ui_wprintf_at( wnd, 20, 32,"N&ame" );
       UserFile  = ui_add_gadget_inputbox( wnd, 60, 30, 40, 40, InputText );

       ui_wprintf_at( wnd, 20, 86,"&Files" );
       ui_wprintf_at( wnd, 210, 86,"&Dirs" );

       ListBox1 = ui_add_gadget_listbox( wnd,  20, 110, 125, 200, NumFiles, filename_list[0], 13 );
       ListBox2 = ui_add_gadget_listbox( wnd, 210, 110, 100, 200, NumDirs, directory_list[0], 13 );

       Button1 = ui_add_gadget_button( wnd,     20, 330, 60, 25, "Ok", NULL );
       Button2 = ui_add_gadget_button( wnd,    100, 330, 60, 25, "Cancel", NULL );
       HelpButton = ui_add_gadget_button( wnd, 180, 330, 60, 25, "Help", NULL );

       wnd->keyboard_focus_gadget = (UI_GADGET *)UserFile;

       Button1->hotkey = KEY_CTRLED + KEY_ENTER;
       Button2->hotkey = KEY_ESC;
       HelpButton->hotkey = KEY_F1;
       ListBox1->hotkey = KEY_ALTED + KEY_F;
       ListBox2->hotkey = KEY_ALTED + KEY_D;
       UserFile->hotkey = KEY_ALTED + KEY_A;

       ui_gadget_calc_keys(wnd);

       ui_wprintf_at( wnd, 20, 60, "%s", Spaces );
       ui_wprintf_at( wnd, 20, 60, "%s", CurDir );

       new_listboxes = 0;

       while( 1 )
       {
               ui_mega_process();
               ui_window_do_gadgets(wnd);

               if ( Button2->pressed )
               {
                       file_chdir( OrgDir );
                       ui_close_window(wnd);
                       return 0;
               }

               if ( HelpButton->pressed )
                       MessageBox( -1, -1, 1, "Sorry, no help is available!", "Ok" );

               if (ListBox1->moved || new_listboxes)
               {
                       if (ListBox1->current_item >= 0 )
                       {
                               strcpy(UserFile->text, filename_list[ListBox1->current_item] );
                               UserFile->position = strlen(UserFile->text);
                               UserFile->oldposition = UserFile->position;
                               UserFile->status=1;
                               UserFile->first_time = 1;
                       }
               }

               if (ListBox2->moved || new_listboxes)
               {
                       if (ListBox2->current_item >= 0 )
                        {
                               if (strrchr( directory_list[ListBox2->current_item], ':' ))
                                       sprintf( UserFile->text, "%s%s", directory_list[ListBox2->current_item], Filespec );
                               else
                                       sprintf( UserFile->text, "%s\\%s", directory_list[ListBox2->current_item], Filespec );
                               UserFile->position = strlen(UserFile->text);
                               UserFile->oldposition = UserFile->position;
                               UserFile->status=1;
                               UserFile->first_time = 1;
                        }
               }
               new_listboxes = 0;

               if (Button1->pressed || UserFile->pressed || (ListBox1->selected_item > -1 ) || (ListBox2->selected_item > -1 ))
                {
                       ui_mouse_hide();

                       if (ListBox2->selected_item > -1 )      {
                               if (strrchr( directory_list[ListBox2->selected_item], ':' ))
                                       sprintf( UserFile->text, "%s%s", directory_list[ListBox2->selected_item], Filespec );
                               else
                                       sprintf( UserFile->text, "%s\\%s", directory_list[ListBox2->selected_item], Filespec );
                       }

                       error_mode = 1; // Critical error handler automatically fails.

                       TempFile = fopen( UserFile->text, "r" );
                      if (TempFile)
                       {
                               // Looks like a valid filename that already exists!
                               fclose( TempFile );
                               break;
                       }

                       // File doesn't exist, but can we create it?
                       TempFile = fopen( UserFile->text, "w" );
                       if (TempFile)
                       {
                               // Looks like a valid filename!
                               fclose( TempFile );
                               remove( UserFile->text );
                               break;
                       }

                       split_dir_name(UserFile->text, fulldir, fullfname);
                
                       //mprintf( 0, "----------------------------\n" );
                       //mprintf( 0, "Full text: '%s'\n", UserFile->text );
                       //mprintf( 0, "Drive: '%s'\n", drive );
                       //mprintf( 0, "Dir: '%s'\n", dir );
                       //mprintf( 0, "Filename: '%s'\n", fname );
                       //mprintf( 0, "Extension: '%s'\n", ext );
                       //mprintf( 0, "Full dir: '%s'\n", fulldir );
                       //mprintf( 0, "Full fname: '%s'\n", fname );

                       if (strrchr( fullfname, '?' ) || strrchr( fullfname, '*' ) ) 
                       {
                               strcat(fulldir, ".");
                       } else {
                               strcpy( fullfname, Filespec );
                               strcpy( fulldir, UserFile->text );
                       }

                       //mprintf( 0, "----------------------------\n" );
                       //mprintf( 0, "Full dir: '%s'\n", fulldir );
                       //mprintf( 0, "Full fname: '%s'\n", fullfname );

                       if (file_chdir( fulldir )==0)
                       {
                               NumFiles = file_getfilelist( 300, filename_list, fullfname );

                               strcpy(UserFile->text, fullfname );
                               UserFile->position = strlen(UserFile->text);
                               UserFile->oldposition = UserFile->position;
                               UserFile->status=1;
                               UserFile->first_time = 1;

                               NumDirs = file_getdirlist( 100, directory_list );

                               ui_listbox_change( wnd, ListBox1, NumFiles, filename_list[0], 13 );
                               ui_listbox_change( wnd, ListBox2, NumDirs, directory_list[0], 13 );
                               new_listboxes = 0;

                               getcwd( CurDir, 35 );
                               ui_wprintf_at( wnd, 20, 60, "%s", Spaces );
                               ui_wprintf_at( wnd, 20, 60, "%s", CurDir );

/*                             i = TICKER;
                               while ( TICKER < i+2 );
*/                      

                       }else {
                               sprintf(ErrorMessage, "Error changing to directory '%s'", fulldir );
                               MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
                               UserFile->first_time = 1;

                       }

                       error_mode = 0;

                       ui_mouse_show();

               }
       }

       //key_flush();

       split_dir_name( UserFile->text, fulldir, fullfname);
 
       if ( strlen(fulldir) > 1 )
               file_chdir( fulldir );
 
       getcwd( CurDir, 35 );
        
       if ( strlen(CurDir) > 0 )
       {
                       if ( CurDir[strlen(CurDir)-1] == '\\' )
                               CurDir[strlen(CurDir)-1] = 0;
       }
        
       sprintf( filename, "%s/%s", CurDir, fullfname );
       //MessageBox( -2, -2, 1, filename, "Ok" );
 
       file_chdir( OrgDir );
        
       ui_close_window(wnd);

       return 1;
}



int ui_get_file( char * filename, char * Filespec  )
{
       int x, i, NumFiles;
       char * text[200];

       NumFiles = file_getfilelist( 200, filename_list, Filespec );

       for (i=0; i< NumFiles; i++ )
       {
               //MALLOC( text[i], char, 15 );//Another compile hack -KRB
               text[i]=(char *)malloc(15*sizeof(char));
               strcpy(text[i], filename_list[i] );
       }

       x = MenuX( -1, -1, NumFiles, text );

       if ( x > 0 )
               strcpy(filename, filename_list[x-1] );

       for (i=0; i< NumFiles; i++ )
       {
               free( text[i] );
       }

       if ( x>0 )
               return 1;
       else
               return 0;

}

