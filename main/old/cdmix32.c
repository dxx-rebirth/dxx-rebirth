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

char cdmix32_rcsid[] = "$Id: cdmix32.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>

#include "inferno.h"
#include "sos.h"
#include "mono.h"
#include "error.h"

typedef struct 
 {
  int id;
  char *name;
 } DigiDevices;

#define _AWE32_8_ST				0xe208

#define NUM_OF_CARDS 4

#ifndef WINDOWS
DigiDevices SBCards[NUM_OF_CARDS]={
  {_SOUND_BLASTER_8_MONO      , "Sound Blaster" },
  {_SOUND_BLASTER_8_ST        , "Sound Blaster Pro" },
  {_SB16_8_ST                 , "Sound Blaster 16/AWE32" },
  {_AWE32_8_ST				 		,  "Sound Blaster AWE32"} 
 };
#endif

int CD_blast_mixer ()
 {
  FILE *InFile;
  char line[100],nosbcard=1;
  char *ptr,*token,*value;
  int i;
  int digiport=0,digiboard=0;
  short int DataPort=0,AddressPort=0;
  char redvol=0;


#ifndef WINDOWS
  InFile = fopen("descent.cfg", "rt");
  if (InFile == NULL) 
	 return (NULL);

  while (!feof(InFile)) {
		memset(line, 0, 80);
		fgets(line, 80, InFile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
	 		if (value[strlen(value)-1] == '\n')
   			value[strlen(value)-1] = 0;
			if (!strcmp(token, "DigiDeviceID8"))
				digiboard = strtol(value, NULL, 16);
			else if (!strncmp(token, "DigiPort",8))
			 {
				digiport = strtol(value, NULL, 16);
			 }
			else if (!strcmp(token, "RedbookVolume"))
				redvol = strtol(value, NULL, 10);
		}
	}

  mprintf ((0,"Digiport=0x%x\n",digiport));
  mprintf ((0,"Digiboard=0x%x\n",digiboard));
  mprintf ((0,"Redbook volume=%d\n",redvol));
  for (nosbcard=1,i=0;i<NUM_OF_CARDS;i++)
   {
    if (SBCards[i].id==digiboard)
		{
       mprintf ((0,"Sound board=%s\n",SBCards[i].name));	
		 digiboard=i;
		 nosbcard=0;
		 break;
		}
	}
 	  
  fclose (InFile);
 
  if (nosbcard)
	{
	 mprintf ((0,"No Soundblaster type card was found!"));
    return (0);
	}

  AddressPort=digiport+4;
  DataPort=digiport+5;

  if (digiboard==0)	// Plain SB
	SetRedSB(AddressPort,DataPort,redvol);
  else if (digiboard==1)	// SB Pro
	SetRedSBPro(AddressPort,DataPort,redvol);
  else if (digiboard==2 || digiboard==3) // Sound blaster 16/AWE 32
   SetRedSB16 (AddressPort,DataPort,redvol);
  else
	Int3(); // What? Get Jason, this shouldn't have happened

#endif

  return (1);
 }

void SetRedSB16 (short aport,short dport,char vol)
 {
  char val;
  
  if (vol>15)
   vol=15;
    
  outp (aport,0x36);
  val=inp (dport);

  if ((val>>3)==0)
   {
	  val|=(25<<3);
	  outp (dport,val);
	}

  outp (aport,0x37);
  val=inp (dport);
  if ((val>>3)==0)
   {
	  val|=(25<<3);
	  outp (dport,val);
	}
 }
void SetRedSB (short aport,short dport,char vol)
 {
  char val;
  
  if (vol>15)
   vol=15;

  vol=11;
 
  outp (aport,0x08);
  val=inp (dport);
  if ((val & 7)==0)
	{
	 val|=0x05;
	 outp (dport,val);
	}
 }
void SetRedSBPro (short aport,short dport,char vol)
 {
  char val;
  
  if (vol>15)
   vol=15;

  vol=11;
 
  outp (aport,0x28);
  val=inp (dport);
  if ((val & 7)==0)
	{
	  val|=0x55;
	  outp (dport,val);
	}
 }





  

  
  
  
