     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 3.0


		A game programming library

	       By Shawn Hargreaves, 1994/97



#include <std_disclaimer.h>

   "I do not accept responsibility for any effects, adverse or otherwise, 
    that this code may have on you, your computer, your sanity, your dog, 
    and anything else that you can think of. Use it at your own risk."



======================================
============ Introduction ============
======================================

   Allegro is a library of functions for use in computer games, written for 
   the djgpp compiler in a mixture of C and assembly language. This is 
   version 3.0: see the NEWS file for a list of differences from the 
   previous release.

   According to the Oxford Companion to Music, Allegro is the Italian for 
   "quick, lively, bright". Once upon a time it was also an acronym for 
   "Atari Low Level Game Routines", but it is a long time since I did any 
   programming on the Atari, and the name is pretty much the only thing left 
   of the original Atari code.



==================================
============ Features ============
==================================

   Supports VGA mode 13h, mode-X (twenty two tweaked VGA resolutions plus 
   unchained 640x400 Xtended mode), and SVGA modes with 8, 15, 16, 24, and 
   32 bit color depths. Uses VESA 2.0 linear framebuffers if they are 
   available, and also contains register level drivers for ATI, Cirrus, 
   Paradise, S3, Trident, Tseng, and Video-7 cards.

   Drawing functions including putpixel, getpixel, lines, rectangles, flat 
   shaded, gouraud shaded, and texture mapped polygons, circles, floodfill, 
   bezier splines, patterned fills, masked, run length encoded, and compiled 
   sprites, blitting, bitmap scaling and rotation, translucency/lighting, 
   and text output with proportional fonts. Supports clipping, and can draw 
   directly to the screen or to memory bitmaps of any size.

   Hardware scrolling, mode-X split screens, and palette manipulation.

   FLI/FLC animation player.

   Plays background MIDI music and up to 32 simultaneous sound effects. 
   Samples can be looped (forwards, backwards, or bidirectionally), and the 
   volume, pan, pitch, etc, can be adjusted while they are playing. The MIDI 
   player responds to note on, note off, main volume, pan, pitch bend, and 
   program change messages, using the General MIDI patch set and drum 
   mappings. Currently supports Adlib, SB, SB Pro, SB16, AWE32, MPU-401, and 
   software wavetable MIDI.

   Easy access to the mouse, keyboard, joystick, and high resolution timer 
   interrupts, including a vertical retrace interrupt simulator.

   Routines for reading and writing LZSS compressed files.

   Multi-object data files and a grabber utility.

   Math functions including fixed point arithmetic, lookup table trig, and 
   3d vector/matrix manipulation.

   GUI dialog manager and file selector.



===================================
============ Copyright ============
===================================

   Allegro is swap-ware. You may use, modify, redistribute, and generally 
   hack it about in any way you like, but if you do you must send me 
   something in exchange. This could be a complimentary copy of a game, an 
   addition or improvement to Allegro, a bug report, some money (this is 
   particularly encouraged if you use Allegro in a commercial product), or 
   just a copy of your autoexec.bat if you don't have anything better. If 
   you redistribute parts of Allegro or make a game using it, it would be 
   nice if you mentioned me somewhere in the credits, but if you just want 
   to pinch a few routines that is OK too. I'll trust you not to rip me off.



============================================
============ Supported hardware ============
============================================

   The bare minimum you need to use Allegro is a 386 with a VGA graphics 
   card, but a 486 is strongly recommended. To get into SVGA modes you will 
   need a compatible SVGA card, which means either one of the cards that is 
   supported directly, or a card with a working VESA driver. If you have a 
   VESA 2.0 implementation such as UniVBE (which you can get from the 
   Display Doctor link on http://www.scitechsoft.com/), you are fine just 
   using that. If you don't, beware. For one thing, everything will be much 
   slower if Allegro can't use the sexy VBE 2.0 features. For another, I 
   could go on all day telling horror stories about the buggy and generally 
   just pathetic VESA implementations that I've come across. If you are 
   having trouble with the SVGA modes, try getting a copy of UniVBE and see 
   if that clears things up (it probably will: SciTech usually get these 
   things right).

   On the sound front, Allegro supports sample playback on the SB (mono), 
   the SB Pro (stereo), and the SB16. It has MIDI drivers for the OPL2 FM 
   synth (Adlib and SB cards), the OPL3 (Adlib Gold, SB Pro-II and above), 
   the pair of OPL2 chips found in the SB Pro-I, the AWE32 EMU8000 chip, the 
   raw SB MIDI output, and the MPU-401 interface, plus it can emulate a 
   wavetable MIDI synth in software, running on top of any of the supported 
   digital soundcards. If you feel like coming up with drivers for any other 
   hardware, they would be much appreciated.

   You may notice that this release contains some code for building a Linux 
   version, but don't bother trying this: it won't work! A _lot_ more work 
   is needed before Allegro will be usable under Linux.

   There is also the first part of a VBE/AF driver in this package. VBE/AF 
   is an extension to the VBE 2.0 API which provides a standard way of 
   accessing hardware accelerator features. My driver implements all the 
   basic mode set and bank switch functions, so it works as well as the 
   regular VESA drivers, but it doesn't yet support any accelerated drawing 
   operations. This is because SciTech have, so far, only implemented VBE/AF 
   on the ATI Mach64 chipset, and I don't have access to a Mach64. I'll 
   finish the driver as soon as they implement VBE/AF on the Matrox Mystique.



============================================
============ Installing Allegro ============
============================================

   To conserve space I decided to make this a source-only distribution, so 
   you will have to compile Allegro before you can use it. To do this you 
   should:

   - Go to wherever you want to put your copy of Allegro (your main djgpp 
     directory would be fine, but you can put it somewhere else if you 
     prefer), and unzip everything. Allegro contains several subdirectories, 
     so you must specify the -d flag to pkunzip.

   - If you are using PGCC, uncomment the definition of PGCC at the top of 
     the makefile, or set the environment variable "PGCC=1".

   - Type "cd allegro", followed by "make". Then go do something 
     interesting while everything compiles. If all goes according to plan 
     you will end up with a bunch of test programs, some tools like the 
     grabber, and the library itself, liballeg.a.

     If you have any trouble with the build, look at faq.txt for the 
     solutions to some of the more common problems.

   - If you want to use the sound routines or a non-US keyboard layout, it 
     is a good idea to set up an allegro.cfg file: see below.

   - If you want to read the Allegro documentation with the Info viewer 
     or the Rhide online help system, edit the file djgpp\info\dir, and in 
     the menu section add the lines:

	 * Allegro: (allegro.inf).
		 The Allegro game programming library

   - If you want to create the HTML documentation as one large allegro.html 
     file rather than splitting it into sections, edit docs\allegro._tx, 
     remove the @multiplefiles statement from line 8, and run make again.

   To use Allegro in your programs you should:

   - Put the following line at the beginning of all C or C++ files that use 
     Allegro:

	 #include <allegro.h>

   - If you compile from the command line or with a makefile, add '-lalleg' 
     to the end of the gcc command, eg:

	 gcc foo.c -o foo.exe -lalleg

   - If you are using Rhide, go to the Options/Libraries menu, type 'alleg' 
     into the first empty space, and make sure the box next to it is checked.

   See allegro.txt for details of how to use the Allegro functions, and how 
   to build a debug version of the library.



=======================================
============ Configuration ============
=======================================

When Allegro initialises the keyboard and sound routines it reads 
information about your hardware from a file called allegro.cfg or sound.cfg. 
If this file doesn't exist it will autodetect (ie. guess :-) You can write 
your config file by hand with a text editor, or you can use the setup 
utility program.

Normally setup.exe and allegro.cfg will go in the same directory as the 
Allegro program they are controlling. This is fine for the end user, but it 
can be a pain for a programmer using Allegro because you may have several 
programs in different directories and want to use a single allegro.cfg for 
all of them. If this is the case you can set the environment variable 
ALLEGRO to the directory containing your allegro.cfg, and Allegro will look 
there if there is no allegro.cfg in the current directory.

The mapping tables used to store different keyboard layouts are stored in a 
file called keyboard.dat. This must either be located in the same directory 
as your Allegro program, or in the directory pointed to by the ALLEGRO 
environment variable. If you want to support different international 
keyboard layouts, you must distribute a copy of keyboard.dat along with your 
program.

See allegro.txt for details of the config file format.



================================================
============ Notes for the musician ============
================================================

The OPL2 synth chip can provide either nine voice polyphony or six voices 
plus five drum channels. How to make music sound good on the OPL2 is left as 
an exercise for the reader :-) On an SB Pro or above you will have eighteen 
voices, or fifteen plus drums. Allegro decides whether to use drum mode 
individually for each MIDI file you play, based on whether it contains any 
drum sounds or not. If you have an orchestral piece with just the odd cymbal 
crash, you might be better removing the drums altogether as that will let 
Allegro use the non-drum mode and give you an extra three notes polyphony.

When Allegro is playing a MIDI file in looped mode, it jumps back to the 
start of the file when it reaches the end of the piece. To control the exact 
loop point, you may need to insert a dummy marker event such as a controller 
message on an unused channel.

All the OPL chips have very limited stereo capabilities. On an OPL2, 
everything is of course played in mono. On the SB Pro-I, sounds can only be 
panned hard left or right. With the OPL3 chip in the SB Pro-II and above, 
they can be panned left, right, or centre. I could use two voices per note 
to provide more flexible panning, but that would reduce the available 
polyphony and I don't want to do that. So don't try to move sounds around 
the stereo image with streams of pan controller messages, because they will 
jerk horribly. It is also worth thinking out the panning of each channel so 
that the music will sound ok on both SB Pro-I and OPL3 cards. If you want a 
sound panned left or right, use a pan value less than 48 or greater than 80. 
If you want it centred, use a pan value between 48 and 80, but put it 
slightly to one side of the exactly central 64 to control which speaker will 
be used if the central panning isn't possible.

The DIGMID wavetable driver uses standard GUS format .pat files, and you 
will need a collection of such instruments before you can use it. This can 
either be in the standard GUS format (a set of .pat files and a default.cfg 
index), or a patches.dat file as produced by the pat2dat utility. You can 
also use pat2dat to convert AWE32 SoundFont banks into the patches.dat 
format, and if you list some MIDI files on the command line it will filter 
the sample set to only include the instruments that are actually used by 
those tunes, so it can be useful for getting rid of unused instruments when 
you are preparing to distribute a game. See the Allegro website for some 
links to suitable sample sets.

The DIGMID driver normally only loads the patches needed for each song when 
the tune is first played. This reduces the memory usage, but can result in a 
longish delay the first time you play each MIDI file. If you prefer to load 
the entire patch set in one go, call the load_midi_patches() function.

The CPU sample mixing code can support between 1 and 32 voices, going up in 
powers of two (ie. either 1, 2, 4, 8, 16, or 32 channels). By default it 
provides 8 digital voices, or 8 digital plus 24 MIDI voices (a total of 32) 
if the DIGMID driver is in use. But the more voices, the lower the output 
volume and quality, so you may wish to change this by calling the 
reserve_voices() function or setting the digi_voices and midi_voices 
parameters in allegro.cfg.



======================================
============ Contact info ============
======================================

   WWW:                 http://www.talula.demon.co.uk/allegro/

   Mailing list:        allegro@canvaslink.com. To add or remove yourself, 
			write to listserv@canvaslink.com with the text 
			"subscribe allegro yourname" or "unsubscribe 
			allegro" in the body of your message.

   Usenet:              Try the djgpp newsgroup, comp.os.msdos.djgpp

   IRC:                 #allegro channel on EFnet

   My email:            shawn@talula.demon.co.uk

   Snail mail:          Shawn Hargreaves,
			1 Salisbury Road,
			Market Drayton,
			Shropshire,
			England, TF9 1AJ.

   Telephone:           UK 01630 654346

   On Foot:             Coming down Shrewsbury Road from the town centre, 
			turn off down Salisbury Road and it is the first 
			house on the left.

   If all else fails:   52 deg 54' N
			2 deg 29' W

   The latest version of Allegro can always be found on the Allegro 
   homepage, http://www.talula.demon.co.uk/allegro/.

