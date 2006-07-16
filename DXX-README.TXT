    - - - - ~ ] --------- [ ~ - - - -
  - - - - ~ ]               [ ~ - - - -
- - - - ~ ]    DESCENT  ][    [ ~ - - - -
- - - - ~ ]  The D2X Rebirth  [ ~ - - - -
  - - - - ~ ]               [ ~ - - - -
    - - - - ~ ] --------- [ ~ - - - -
________// Codename:      __________v0.42
               'Zaphod' //


0. Introduction
^^^^^^^^^^^^^^^

This version of D2X is based on the latest CVS Source of November 29th 2005
coded and released by Bradley Bell and his team.

I spend much time to improve the sourcecode, tried to fix any bug in there and
added some improvements.
It is the goal of DXX-Rebirth to keep these games alive and the result is a
very stable version of the Descent ][ port - called
D2X-Rebrith.

To improve the original D2X, I still submit my patches to the original D2X
team. As D2X-Rebirth would never be possible without this great work i try to
help them in any way possible.

I hope you enjoy the game as you did when you played it the first time.

If you have something to say about my release, feel free to contact me at
zicothehentaifreak@gmx.de

- zico 20051229


1. Features
^^^^^^^^^^^

This Version of D2X has every little feature you already may know from the
DOS/W32 Version of Descent ][ and much more.
For example:
* High resoution Fonts and briefing screens
* Full cockpits on all resolutions
* high resolutions
* widescreen options
* joystick and mouse support
* Vertigo Series support
* possibility to run AddOn levels
* network support
* record and play demos
* OpenGL functions like trilinear filtering etc.
* everything else you know from DESCENT
* ... and much more!


2. Installation
^^^^^^^^^^^^^^^

The original game:
==================
After you extracted the archive you're almost ready for Descent.
But there is still a little bit work to do.
You need to copy the original Descent ][ data to the current directory.

To do so you don't have to install Descent ][ on your Windows.
Just use the program 'unarj' to extract the file 'descent.sow' located on your
Descent ][ CD.

Place the following files to the current directory:
descent2.ham
descent2.hog
descent2.s11
descent2.s22
alien1.pig
alien2.pig
fire.pig
groupa.pig
ice.pig
water.pig
intro-h.mvl and/or intro-l.mvl
other-h.mvl and/or other-l.mvl
robots-h.mvl and/or robots-l.mvl

[NOTE: if you compiled the source, copy the files to your data directory you
selected]
In linux, all filenames must be in _lower case_. It's important.

That's it :)

Vertigo Series:
===============

To play the Vertigo missions, copy the following files to the current
directory [NOTE: for source - the data directory, you know]:

arcan01b.pcx
arcan02b.pcx
arcan03b.pcx
arcan04b.pcx
arcan05b.pcx
d2x-h.mvl and/or d2x-l.mvl
hoard.ham (if you want to use HOARD)

and now the vertigo missions itself to the subdirectory "missions":

d2x.hog
d2x.mn2

Now you are able to play the "Vertigo Series"

AddOn Levels:
=============

To play other AddOn Levels, just place them (MN2, HOG, RL2 files) to
the subdirectory "missions".
D2X will recognize them automaticly.

Icons:
======
If you want to create Shortcurts to D2X on your Desktop and/or WM
there is a wonderful icon stored in this directoy.
Novacron aka Troy Anderson of http://www.planetdescent.com has created it and
allowed me to implement it to this release. Thanks again, Novacron!
[NOTE: for the source you need to download this icons from 
http://www.dxx-rebirth.de]

NOTE for Linux:
D2X-Rebirth will probably need a special version of libstdc++ and
PhysicsFS-1.0.1.
These files are located in the subdirectory "lib". Place these file to
"/usr/lib". But do NOT overwrite any of your libs. 
If D2X-Rebirth refuses to work, please use the source to create your very own
binary.


3. Running the game
^^^^^^^^^^^^^^^^^^^

windows: Just start the d2x-rebirth.exe :)
linux: go to the directory where you extracted the archive and type
       './d2x-rebirth'.

There are also a lot of optional command-line options you can set for
D2X-Rebirth. Just open the file d2x.ini for a complete list of options and
select whatever you want to use.
You can also apply these options in a shortcut, a small shellscript [linux]
or by starting it in a terminal [linux] or command line [windows].

Be aware that all your configuration data, savegames and recorded demos will be
stored (and loaded) in the directory from where you start the game. So you
should make sure you have write permissions in this directory [linux].


4. Known Issues
^^^^^^^^^^^^^^^

Problem:  Redbook/CD-Audio doesn't work.
Reason:   The D2X Redbook only works with an analog signal.
Solution: Make sure you have connected a audio cable from your CD drive to your
          soundcard.

Problem:  The game crashes while loading a personal level (RL2 file).
Reason:   No Bug in D2X. Linux reads input case sensitive. RL2 file description
          in MN2 file has the wrong letter case.
Solution: If you look in the MN2 file of your level you will find the name of
          the RL2 file. This string should exactly named as the RL2 file 
          itself.
Example:  * In MYLEVEL.MN2 - 'MyLevel.RL2'. RL2 file is named 'mylevel.RL2'.
            This won't work.
          * In MYLEVEL.MN2 - 'MYLEVEL.RL2'. RL2 file is named 'MYLEVEL.RL2'.
            This will work.
          (NOTE: there is also a small shell script for download, that may help
                 you correcting your addon levels)

Problem:  D2X doesn't look better than the DOS version. Where are the GL FX?
Reason:   You just don't have activated them.
Solution: Use the option '-gl_mipmap' or '-gl_trilinear' to activate
          linear/trilinear filtering.

Problem:  [LINUX] MIDI music doesn't work.
Reason:   The game should accept AWE and MPU401 compatible devices.
Solution: Make sure you configures MIDI correctly.
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

Problem:  Using Open-GL my gameplay is not really fluid and/or my screen looks
          sometimes like cut in an upper and lower half while turning.
          (V-Sync-Problem)
Reason:   For some configurations D2X is not able to syncronize the
          screen-refresh of the operating system and the fps-rate of D2X.
Solution: Set the refresh-rate of the display in the menu of your operating 
          System to 75 or 60 Hz and set the same fps-value in the D2X-Menu or
          the d2x.ini. (-maxfps <x>)
          [Thanks for this hint go to SNIPER]

I'll try to find better solutions for these problems listed above if possible.
If you find a new bug or a better workaround for any existing problem, please
submit it to zicothehentaifreak@gmx.de.


5. LEGAL STUFF
^^^^^^^^^^^^^^

From D2X:
=========
Legal Stuff:

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

We make no warranties as to the usability or correctness of this code.

From me:
========
I don't give any warranty for the functionality of this program named d2x.
Further I am not responsible for any issues, damages and/or irritations the
installer and/or the program d2x itself could cause on your computer hardware,
software and/or yourself.
But if something doesn't work just contact me and i will try to fix it.


6. Thanks
^^^^^^^^^

First I want to thank Interplay and Parallax for this great piece of software
called DESCENT.
Next I want to thank Bradley Bell and his team for making D2X. Without you
we all won't be able to play it on Linux.
Special thanks go to Christopher Taylor of the D2X team. He answered all my
probably bothering mails and helped me to solve some bugs in D2X.
More thanks:
* My girlfriend - for being very patient :)
* KyroMaster for great patches and good program code
* The guys at http://www.unixboard.de for technical assistance
* Maystorm for technical assistance and endless hours of BETA-testing
* Sniper of http://www.descentforum.de for windows BETA-testing
* Everyone who helps to let this project live on
* Novacron for allowing me to use his icons and everyone at
  http://www.planetdescent.com
* Linus Torvalds for Linux
* KMFDM for good music
* everyone i forgot