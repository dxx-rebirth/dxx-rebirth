#!/bin/bash
#
# Enable some 3Dfx OpenGL hints
#
export MESA_GLX_FX="f"
export MESA_CONFIG="quake2"
#
# Disable the 3Dfx GLIDE logo
#
export FX_GLIDE_NO_SPLASH="1"
#
# Change the swap buffers in Voodoo FIFO from 2 to 4
#
export MESA_FX_SWAP_PENDING="4"
#
# Print Mesa statistics at end of game
#
export MESA_FX_INFO="1"
#
# Enable monitor's vertical refresh rate syncing on _most_ NVidia cards.
#   (see /usr/share/doc/NVIDIA_GLX-1.0/README)
export __GL_SYNC_TO_VBLANK="1"
#
# Enable Full Scene Anti-Aliasing on NVidia cards who supports it.
#   (see /usr/share/doc/NVIDIA_GLX-1.0/README)
export __GL_FSAA_MODE="3"
#
# Note: this is *mandatory* with single-threaded applications
# like d2x-gl on systems equipped with NVidia cards and an old
# version of ld.so.
#   (see /usr/share/doc/NVIDIA_GLX-1.0/README)
#
export __GL_SINGLE_THREADED="1"
#
# Finally, run the real game.
# All switches can be overwritten by command-line arguments.
#
nice /usr/games/d2x-gl.real -tmap fp -nocdrom -nomovies \
	-gl_mipmap -gl_alttexmerge -gl_reticle 2 \
	-nofades \
	$1 $2 $3 $4 $5

