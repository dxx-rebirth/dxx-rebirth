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
static char rcsid[] = "$Id: coindev.c,v 1.1.1.1 2001-01-19 03:30:05 bradleyb Exp $";
#pragma on (unreferenced)

#include <stdio.h>
#include <dos.h>
#include <conio.h>

#include "coindev.h"

int CoinMechPort[3]  =  { COINMECH1_PORT,
                          COINMECH2_PORT,
                          COINMECH3_PORT };

int CoinMechCtrl[3]  =  { COINMECH1_CTRLMASK,
                          COINMECH2_CTRLMASK,
                          COINMECH3_CTRLMASK };

unsigned int CoinMechLastCnt[3];


int coindev_init(CoinMechNumber)
{
    int x;

    int CoinMechAdj[3]   =  { COINMECH1_ADJMASK,
                              COINMECH2_ADJMASK,
                              COINMECH3_ADJMASK };

    /* set up IO port for our special use */
    outp(COINMECH_CMDPORT, CoinMechCtrl[CoinMechNumber]);
    outp(CoinMechPort[CoinMechNumber], 0);
    outp(CoinMechPort[CoinMechNumber], 0);

    /* write to the IO board so that the counter eventually gets cleared */
    for( x = 10; x > 0; x-- )
    {
       outp(COINMECH_ADJPORT, CoinMechAdj[CoinMechNumber]);
       outp(COINMECH_ADJPORT, 0);
       if( coindev_read(CoinMechNumber) == 0 )
       {
          break;
       }
    }

    CoinMechLastCnt[CoinMechNumber] = 0;

    return(x);  /* TRUE == CoinMech is cleared;  FALSE == CoinMech error! */
}


/* Do a raw read of the specified CoinMech */
unsigned int coindev_read(CoinMechNumber)
{
    unsigned int x;

    /* read lower byte, then upper byte */
    x  = (inp(CoinMechPort[CoinMechNumber]));
    x += (inp(CoinMechPort[CoinMechNumber]) * 256);

    /* conver to usable number */
    if( x )
    {
        x = (65536L - x);
    }

    return(x);
}


/* read the specified CoinMech and return the */
/* ...number of clicks since the last read    */
unsigned int coindev_count(CoinMechNumber)
{
    unsigned int x, y;

    x = coindev_read(CoinMechNumber);
    y = x - CoinMechLastCnt[CoinMechNumber];

    CoinMechLastCnt[CoinMechNumber] = x;

    return(y);
}


