                         __________
__________/ D1X-Rebirth /


http://www.dxx-rebirth.com


0. Introduction

This version of D1X is based on livna’s release of d1x-1.43.

I spend much time to improve the Sourcecode, tried to fix bugs in there and added some improvements. It is the goal of DXX-Rebirth to keep these games alive and the result is a very stable version of the Descent I port - called D1X-Rebrith.

I hope you enjoy the game as you did when you played it the first time.

If you have something to say about my release, feel free to contact me at zicodxx [at] yahoo [dot] de

- zico 20070407


1. Features

D1X-Rebirth has every little feature you already may know from the DOS Version 1.4a of Descent and much more.

For example:

    *
      High resoution Fonts and briefing screens
    *
      High resolutions with full Cockpit support
    *
      Widescreen options
    *
      Joystick and Mouse support
    *
      Possibility to run AddOn levels
    *
      Network support
    *
      Record and play demos
    *
      OpenGL functions and Eyecandy like Trilinear filtering, Transparency effects etc.
    *
      MP3/OGG/AIF/WAV Jukebox Support
    *
      everything else you know from DESCENT
    *
      ... and much more!


2. Installation

See INSTALL.txt.


3. Multiplayer

DXX-Rebirth supports Multiplayer over (obsoleted) IPX and UDP/IP.
Using UDP/IP works over LAN and Internet. Since the Networking code of the Descent Engine is Peer-to-Peer, it is necessary for
all players (Host and Clients) to open port UDP 31017.
Clients can put an offset to this port by using '-ip_baseport OFFSET'.
Hosts can also use option '-ip_relay' to route players with closed ports. Use this with caution. It will increase Lag and Ping drastically.
Also game summary will not refresh correctly for relay-players until Host has escaped the level as well.
UDP/IP also supports IPv6 by compiling the game with the designated flag. Please note IPv4- and IPv6-builds cannot play together.


4. Legal stuff

See COPYING.txt


5. Contact

http://www.dxx-rebirth.com/
zicodxx [at] yahoo [dot] de
