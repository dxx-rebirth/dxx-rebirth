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
static char rcsid[] = "$Id: medfunc.c,v 1.1 2004-12-19 13:08:49 btb Exp $";
#pragma on (unreferenced)

typedef struct {
	char *  name;
	int     nparams;
	double  (*cfunction)(void);
} FUNCTION;

FUNCTION funtable[] = {

// In khelp.c
{   "med-help",                         0,      DoHelp },

// In kcurve.c
{   "med-curve-init",                   0,      InitCurve },
{   "med-curve-generate"                0,      GenerateCurve },
{   "med-curve-decrease-r4",            0,      DecreaseR4 },
{   "med-curve-increase-r4",            0,      IncreaseR4 },
{   "med-curve-decrease-r1",            0,      DecreaseR1 },
{   "med-curve-increase-r1",            0,      IncreaseR1 },
{   "med-curve-delete",                 0,      DeleteCurve },
{   "med-curve-set",                    0,      SetCurve },

// In kmine.c
{   "med-mine-save",                    0,      SaveMine },
{   "med-mine-load",                    0,      LoadMine },
{   "med-mine-menu",                    0,      MineMenu },
{   "med-mine-create-new",              0,      CreateNewMine },


// In kview.c
{   "med-view-zoom-out",                0,      ZoomOut },
{   "med-view-zoom-in",                 0,      ZoomIn },
{   "med-view-move-away",               0,      MoveAway },
{   "med-view-move-closer",             0,      MoveCloser },
{   "med-view-toggle-chase",            0,      ToggleChaseMode },

// In kbuild.c
{   "med-build-bridge",                 0,      CreateBridge },
{   "med-build-joint",                  0,      FormJoint },
{   "med-build-adj-joint",              0,      CreateAdjacentJoint },
{   "med-build-adj-joints-segment",     0,      CreateAdjacentJointsSegment },
{   "med-build-adj-joints-all",         0,      CreateAdjacentJointsAll },

// In ksegmove.c
{   "med-segmove-decrease-heading",     0,      DecreaseHeading },
{   "med-segmove-increase-heading",     0,      IncreaseHeading },
{   "med-segmove-decrease-pitch",       0,      DecreasePitch },
{   "med-segmove-increase-pitch",       0,      IncreasePitch },
{   "med-segmove-decrease-bank",        0,      DecreaseBank },
{   "med-segmove-increase-bank",        0,      IncreaseBank },

// In ksegsel.c
{   "med-segsel-next-segment",          0,      SelectCurrentSegForward },
{   "med-segsel-prev-segment",          0,      SelectCurrentSegBackward },
{   "med-segsel-next-side",             0,      SelectNextSide },
{   "med-segsel-prev-side",             0,      SelectPrevSide },
{   "med-segsel-set-marked",            0,      CopySegToMarked },
{   "med-segsel-bottom",                0,      SelectBottom },
{   "med-segsel-front",                 0,      SelectFront },
{   "med-segsel-top",                   0,      SelectTop },
{   "med-segsel-back",                  0,      SelectBack },
{   "med-segsel-left",                  0,      SelectLeft },
{   "med-segsel-right",                 0,      SelectRight },

// In ksegsize.c
{   "med-segsize-increase-length",      0,      IncreaseSegLength },
{   "med-segsize-decrease-length",      0,      DecreaseSegLength },
{   "med-segsize-decrease-width",       0,      DecreaseSegWidth },
{   "med-segsize-increase-width",       0,      IncreaseSegWidth },
{   "med-segsize-increase-height",      0,      IncreaseSegHeight },
{   "med-segsize-decrease-height",      0,      DecreaseSegHeight },

{   "med-segsize-increase-length-big",  0,      IncreaseSegLengthBig },
{   "med-segsize-decrease-length-big",  0,      DecreaseSegLengthBig },
{   "med-segsize-decrease-width-big",   0,      DecreaseSegWidthBig },
{   "med-segsize-increase-width-big",   0,      IncreaseSegWidthBig },
{   "med-segsize-increase-height-big",  0,      IncreaseSegHeightBig },
{   "med-segsize-decrease-height-big",  0,      DecreaseSegHeightBig },

//	In ktmap.c
{   "med-tmap-assign",                  0,      AssignTexture },
{   "med-tmap-propogate",               0,      PropagateTextures },
{   "med-tmap-propogate-selected",      0,      PropagateTexturesSelected },

// In macro.c
{   "med-macro-menu",                   0,      MacroMenu },
{   "med-macro-play-fast",              0,      MacroPlayFast },
{   "med-macro-play-normal",            0,      MacroPlayNormal },
{   "med-macro-record-all",             0,      MacroRecordAll },
{   "med-macro-record-keys",            0,      MacroRecordKeys },
{   "med-macro-save",                   0,      MacroSave },
{   "med-macro-load",                   0,      MacroLoad },


// The terminating marker
{   NULL, 0, NULL } };


