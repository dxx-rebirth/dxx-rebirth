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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#pragma off (unreferenced)
static char rcsid[] = "$Id: winmono.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#include <windows.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


static int				WinMonoInitialized = 0;
static HANDLE			hConOutput = 0;



//	----------------------------------------------------------------------------
//	Functions
//	----------------------------------------------------------------------------

int minit(void)
{

	if (WinMonoInitialized) return 1;

	if (!AllocConsole()) return 0;
	hConOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConOutput == INVALID_HANDLE_VALUE) return 0;

	WinMonoInitialized = 1;
	
	return 1;

}

int mkill(void)
{
	FreeConsole();

	return 1;
}


void mopen(short n, short row, short col, short width, short height, char *title)
{
	if (!WinMonoInitialized) return;
}
		

void mclose(int n)
{
	if (!WinMonoInitialized) return;
}	


void msetcursor(short row, short col)
{
	COORD coord;

	coord.X =  col;
	coord.Y =  row;

	SetConsoleCursorPosition(hConOutput, coord);
}


void mputc(short n, char c)
{
	char buf[2];
	DWORD chwritten;

	if (!WinMonoInitialized) return;

	buf[0] = c; buf[1] = 0;

	WriteConsole(hConOutput, buf, 1, &chwritten,  NULL); 
}


void mputc_at(short n, short row, short col, char c)
{
	if (!WinMonoInitialized) return;
	
	msetcursor(row,col);

	mputc(n, c);
}


void scroll(short n)
{
}		


static char temp_m_buffer[1000];

void mprintf(short n, char *format, ...)
{
	char *ptr = temp_m_buffer;
	va_list args;
	DWORD chwritten;
	
	if (!WinMonoInitialized) return;

	va_start(args, format);
	vsprintf(temp_m_buffer, format, args);
	WriteConsole(hConOutput, ptr, strlen(ptr), &chwritten, NULL); 
}


void mprintf_at(short n, short row, short col, char *format, ...)
{
	char *ptr = temp_m_buffer;
	va_list args;
	DWORD chwritten;
	
	if (!WinMonoInitialized) return;

	va_start(args, format);
	vsprintf(temp_m_buffer, format, args);
	msetcursor(row, col);
	WriteConsole(hConOutput, ptr, strlen(ptr), &chwritten, NULL); 
}
	

void drawbox(short n)
{
//	Obsolete in the New Order
}


void mrefresh(short n)
{
//	Huh??? :)
}

