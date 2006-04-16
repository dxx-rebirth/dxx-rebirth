    - - - - ~ ] --------- [ ~ - - - -
  - - - - ~ ]               [ ~ - - - -
- - - - ~ ]     DESCENT I     [ ~ - - - -
- - - - ~ ]  The D1X Rebirth  [ ~ - - - -
  - - - - ~ ]               [ ~ - - - -
    - - - - ~ ] --------- [ ~ - - - -
________// Codename:      __________v0.42
               'Zaphod' //


http://www.dxx-rebirth.de


0. Introduction
^^^^^^^^^^^^^^^

This version of D1X is based on livna's release of d1x-1.43.

I spend much time to improve the sourcecode, tried to fix any bug in there and
added some improvements.
It is the goal of DXX-Rebirth to keep these games alive and the result is a
very stable version of the Descent I port - called
D1X-Rebrith.

I hope you enjoy the game as you did when you played it the first time.

If you have something to say about my release, feel free to contact me at
zicothehentaifreak@gmx.de

- zico 20051229


1. Features
^^^^^^^^^^^

This Version of D1X has every little feature you already may know from the DOS
Version 1.4a of Descent and much more.
For example:
* High resoution Fonts and briefing screens
* Full cockpits on all resolutions
* high resolutions
* widescreen options
* joystick and mouse support
* possibility to run AddOn levels
* network support
* record and play demos
* OpenGL functions like trilinear filtering etc.
* everything else you know from DESCENT
* ... and much more!


2. Installation
^^^^^^^^^^^^^^^

After you extracted the archive you're almost ready for Descent. But there is
still a little bit work to do.
You need to copy the original Descent data - namely "descent.hog" and
"descent.pig" to the current directory.
NOTE: These files need to be patched to version 1.4a.
      You will find the DOS- and a self made linux patch on
      http://www.dxx-rebirth.de.

If you wish to add some extra levels like "Levels of the world" copy them
to this directory, too.
[NOTE: if you compiled the source, copy the files to your data directory you 
selected]

If you want to create Shortcurts to D1X on your Desktop and/or WM
there is a wonderful icon stored in this directory.
Novacron aka Troy Anderson of http://www.planetdescent.com has created it and
allowed me to implement it to this release. Thanks again, Novacron!
[NOTE: for the source you need to download this icons from 
http://www.dxx-rebirth.de]

This package contains a set of HiRes fonts and briefing pictures D1X-Rebirth
will use. If you don't want them, delete them, move them to another directory,
or whatever you want.
[NOTE: if you compiled the source you need to download these HiRes sets from
Http://www.dxx-rebirth.de and store them into your data path]

NOTE for Linux:
D1X-Rebirth will probably need a special version of libstdc++.
This file is located in the subdirectory "lib". Place this file to "/usr/lib".
But do NOT overwrite any of your libs. If D1X-Rebirth refuses to work, please
use the source to create your very own binary.

WARNING:
IF YOU UPDATE FROM v0.31 OR OLDER RESET YOUR RESOLUTION AND CONTROLS IN THE
MENUS. OTHERWISE THE GAME MAY CRASH!


3. Running the game
^^^^^^^^^^^^^^^^^^^

windows: Just start the d1x-rebirth.exe :)
linux: go to the directory where you extracted the archive and type
       './d1x-rebirth'.

There are also a lot of optional command-line options you can set for
D1X-Rebirth. Just open the file d1x.ini for a complete list of options and
select whatever you want to use.
You can also apply these options in a shortcut, a small shellscript [linux]
or by starting it in a terminal [linux] or command line [windows].

Be aware that all your configuration data, savegames and recorded demos will be
stored (and loaded) in the directory from where you start the game. So you
should make sure you have write permissions in this directory [linux].


4. Known Issues
^^^^^^^^^^^^^^^

Problem:  [LINUX] The game crashes while loading a personal level (RDL file).
Reason:   No Bug in D1X. Linux reads input case sensitive. RDL file description
          in MSN file has the wrong letter case.
Solution: If you look in the MSN file of your level you will find the name of
          the RDL file. This string should exactly named as the RDL file
          itself.
Example:  * In MYLEVEL.MSN - 'MyLevel.RDL'. RDL file is named 'mylevel.RDL'.
            This won't work.
          * In MYLEVEL.MSN - 'MYLEVEL.RDL'. RDL file is named 'MYLEVEL.RDL'.
            This will work.
          (NOTE: there is also a small shell script for download, that may help
                 you correcting your addon levels)

Problem:  The game looks like the old DOS version? Where are the OpenGL FX?
Reason:   You just don't have activated them.
Solution: Use the option '-gl_mipmap' or '-gl_trilinear' to activate
          linear/trilinear filtering.

Problem:  [LINUX] MIDI music doesn't work.
Reason:   The game is binaries are compiled for AWE support and should run with
          it very well.
Solution: If you use an MPU401 card, just make the changes in defines.mak
          and compile it again. If it doesn't work, make shure you have
          MIDI configures correctly.
          Hint for AWE cards: asfxload -V100 8mbgmsfx.sf2

Problem:  The mouse movement is too slow. Is there no way to control the ship
          like in other first person shooters?
Reason:   The Pyro is no human, it can't turn that fast. ;)
          But there is a way indeed...
Solution: To enable a mouselook styl control tye as you know it from other
          first person shooters. Just start the game with the
          command-line option '-mouselook' to enable this. But to be fair to
          oter players which do not use mouselook it will not work in a
          multiplayer game.

Problem:  D1X runs a little bit choppy. No matter if bilinear (gl_mipmap) or 
          trilinear (gl_trilinear).
Reason:   D1X with activated open-gl runs on most systems at its best, but not
          at all PCs. There could be a problem with thee refresh rate.
Solution: To check that out try the following two options in the d1x.ini and 
          that values:
          -maxfps 80
          -gl_refresh 60
          If you see no difference or D1X runs less fluently change the d1x.ini
          back to its old settings because your System is in best condition
          without that settings.
          If D1X now runs more fluently: CONGRATULATIONS, it was a pleasure to
          help you.
          If your D1X RUNS BETTER with that settings, please send a short info-e-
          mail to zicothehentaifreak@gmx.de. I would like to know on how many
          systems this behaviour occure.
          [Thanks for this hint go to SNIPER]

I'll try to find better solutions for these problems listed above if possible.
If you find a new bug or a better workaround for any existing problem, please
submit it to zicothehentaifreak@gmx.de.


5. LEGAL STUFF
^^^^^^^^^^^^^^

From D1X:
=========
D1x License 

Preamble
--------

This License is designed to allow the Descent programming community to
continue to have the Descent source open and available to anyone.

Original Parallax License
-------------------------

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

We make no warranties as to the usability or correctness of this code.
-------------------------

The D1x project is the combination of the original Parallax code and the 
modifications and additions made to source code.  While the original code is 
only under the Original Parallax License the D1x project contains original 
code that was not made by Parallax.  This ADDED and/or CHANGED code has the 
following added restrictions:

1) By using this source you are agreeing to these terms.  

2) D1x and derived works may only be modified if ONE of the following is done:
	
	a) The modified version is placed under this license.  The source 
	code used to create the modified version is freely and publicly 
	available under this license.

	b) The modified version is only used by the developer.

3) D1X IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

From me:
========
I don't give any warranty for the functionality of this program named d1x.
Further I am not responsible for any issues, damages and/or irritations the
installer and/or the program d1x itself could cause on your computer hardware,
software and/or yourself.
But if something doesn't work just contact me and i will try to fix it.


6. Thanks
^^^^^^^^^

First I want to thank Interplay and Parallax for this great piece of software
called DESCENT.
Next i want to thank Victor Rachels and his team for making D1X. Without you
we all won't be able to play it on Linux.
More thanks:
* My girldfriend - for being very patient :)
* KyroMaster for great patches and good program code
* The guys at http://www.unixboard.de for technical assistance
* Maystorm for technical assistance and endless hours of BETA-testing
* Sniper of http://www.descentforum.de for windows BETA-testing
* Everyone who helps to let this project live on
* Novacron for allowing me t use his icons and everyone at
  http://www.planetdescent.com
* Linus Torvalds for Linux
* KMFDM for good music
* Avaurus at http://www.unixboard.de for the webspace and domain.
* everyone i forgot