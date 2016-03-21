/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#pragma once

#ifdef __cplusplus
#include "dsx-ns.h"

// In khelp.c
int DoHelp();

// In kcurve.c
int InitCurve();
int GenerateCurve();
int DecreaseR4();
int IncreaseR4();
int DecreaseR1();
int IncreaseR1();
int DeleteCurve();
int SetCurve();

// In kmine.c
int SaveMine();
int LoadMine();
int MineMenu();
int CreateNewMine();
int LoadOldMine();

int SaveSituation();
int LoadSituation();

// In kgame.c
int SetPlayerPosition(void);
int SaveGameData();
int LoadGameData();
int LoadMineOnly();
void ResetFilename();

// In group.c
int LoadGroup();
int SaveGroup();
int PrevGroup();
int NextGroup();
int CreateGroup();
int SubtractFromGroup();
int DeleteGroup();
int MarkGroupSegment();
int MoveGroup(void);
int CopyGroup(void);
int AttachSegmentNew();
int UngroupSegment();
int GroupSegment();
int Degroup();
int RotateGroup();

// In segment.c
int ToggleBottom();
void make_curside_bottom_side();
#ifdef DXX_BUILD_DESCENT_II
int select_segment_by_number();
int select_segment_with_powerup();
#endif

// In editor.c
int UndoCommand();

// In kview.c
int ZoomOut();
int ZoomIn();
int MoveAway();
int MoveCloser();
int ToggleChaseMode();

// In kbuild.c
int CreateBridge();
int FormJoint();
int CreateAdjacentJoint();
int CreateAdjacentJointsSegment();
int CreateAdjacentJointsAll();
int CreateSloppyAdjacentJoint();
int CreateSloppyAdjacentJointsGroup();

// In ksegmove.c
int DecreaseHeading();
int IncreaseHeading();
int DecreasePitch();
int IncreasePitch();
int DecreaseBank();
int IncreaseBank();

// In ksegsel.c
int SelectCurrentSegForward();
int SelectCurrentSegBackward();
int SelectNextSide();
int SelectPrevSide();
int CopySegToMarked();
int SelectBottom();
int SelectFront();
int SelectTop();
int SelectBack();
int SelectLeft();
int SelectRight();

// In ksegsize.c
#ifdef dsx
namespace dsx {
int IncreaseSegLength();
int DecreaseSegLength();
int DecreaseSegWidth();
int IncreaseSegWidth();
int IncreaseSegHeight();
int DecreaseSegHeight();
int ToggleSegSizeMode();
int PerturbCurside();
int PerturbCursideBig();

int IncreaseSegLengthBig();
int DecreaseSegLengthBig();
int DecreaseSegWidthBig();
int IncreaseSegWidthBig();
int IncreaseSegHeightBig();
int DecreaseSegHeightBig();

int IncreaseSegLengthDefault();
int DecreaseSegLengthDefault();
int IncreaseSegWidthDefault();
int DecreaseSegWidthDefault();
int IncreaseSegHeightDefault();
int DecreaseSegHeightDefault();
}
#endif

//	In ktmap.c
int AssignTexture();
int AssignTexture2();
int ClearTexture2();
int PropagateTextures();
int PropagateTexturesMove();
int PropagateTexturesMoveUVs();
int PropagateTexturesUVs();
int PropagateTexturesSelected();

//--//// In macro.c
//--//int MacroMenu();
//--//int MacroPlayFast();
//--//int MacroPlayNormal();
//--//int MacroRecordAll();
//--//int MacroRecordKeys();
//--//int MacroSave();
//--//int MacroLoad();

// In editor.c
int AttachSegment();
int DeleteSegment();
int CallLisp();
int ExitEditor();
int ShowAbout();
int ExchangeMarkandCurseg();
#ifdef DXX_BUILD_DESCENT_II
int CopySegtoMarked();
#endif
int med_keypad_goto_prev();
int med_keypad_goto_next();
int med_keypad_goto();
int med_increase_tilings();
int med_decrease_tilings();
int ToggleAutosave();
#ifndef NDEBUG
int MarkStart();
int MarkEnd();
#endif

//	Texture.c
int	TexFlipX();
int	TexFlipY();
int	TexSlideUp();
int	TexSlideLeft();
int	TexSetDefault();
#ifdef DXX_BUILD_DESCENT_II
int	TexSetDefaultSelected();
#endif
int	TexSlideRight();
int	TexRotateLeft();
int	TexSlideDown();
int	TexRotateRight();
int	TexSelectActiveEdge();
int	TexRotate90Degrees();
int	TexIncreaseTiling();
int	TexDecreaseTiling();
int	TexSlideUpBig();
int	TexSlideLeftBig();
int	TexSlideRightBig();
int	TexRotateLeftBig();
int	TexSlideDownBig();
int	TexRotateRightBig();
int	TexStretchDown();
int	TexStretchUp();
#ifdef DXX_BUILD_DESCENT_II
int	TexChangeAll();
int	TexChangeAll2();
#endif

//	object.c
#ifdef dsx
namespace dsx {
int	ObjectPlaceObject();
int	ObjectMakeCoop();
int	ObjectPlaceObjectTmap();
int	ObjectDelete();
int	ObjectMoveForward();
int	ObjectMoveLeft();
int	ObjectSetDefault();
int	ObjectMoveRight();
int	ObjectMoveBack();
int	ObjectMoveDown();
int	ObjectMoveUp();
int	ObjectMoveNearer();
int	ObjectMoveFurther();
int	ObjectSelectNextinSegment();
int	ObjectSelectNextType();

template <int p, int b, int h>
int ObjectChangeRotation();

#define ROTATION_UNIT (4096/4)

#define ObjectDecreaseBank ObjectChangeRotation<0, -ROTATION_UNIT, 0>
#define ObjectIncreaseBank ObjectChangeRotation<0, ROTATION_UNIT, 0>
#define ObjectDecreasePitch ObjectChangeRotation<-ROTATION_UNIT, 0, 0>
#define ObjectIncreasePitch ObjectChangeRotation<ROTATION_UNIT, 0, 0>
#define ObjectDecreaseHeading ObjectChangeRotation<0, 0, -ROTATION_UNIT>
#define ObjectIncreaseHeading ObjectChangeRotation<0, 0, ROTATION_UNIT>

#define ObjectDecreaseBankBig ObjectChangeRotation<0, -(ROTATION_UNIT*4), 0>
#define ObjectIncreaseBankBig ObjectChangeRotation<0, (ROTATION_UNIT*4), 0>
#define ObjectDecreasePitchBig ObjectChangeRotation<-(ROTATION_UNIT*4), 0, 0>
#define ObjectIncreasePitchBig ObjectChangeRotation<(ROTATION_UNIT*4), 0, 0>
#define ObjectDecreaseHeadingBig ObjectChangeRotation<0, 0, -(ROTATION_UNIT*4)>
#define ObjectIncreaseHeadingBig ObjectChangeRotation<0, 0, (ROTATION_UNIT*4)>

int  	ObjectResetObject();
}
#endif

//	elight.c
int	LightSelectNextVertex();
int	LightSelectNextEdge();
int	LightCopyIntensity();
int	LightCopyIntensitySegment();
int	LightDecreaseLightVertex();
int	LightIncreaseLightVertex();
int	LightDecreaseLightSide();
int	LightIncreaseLightSide();
int	LightDecreaseLightSegment();
int	LightIncreaseLightSegment();
int	LightSetMaximum();
int	LightSetDefault();
int	LightSetDefaultAll();
int	LightAmbientLighting();

// seguvs.c
#ifdef dsx
namespace dsx {
int fix_bogus_uvs_on_side();
int fix_bogus_uvs_all();
}
#endif
int set_average_light_on_curside(void);
int set_average_light_on_all(void);
int set_average_light_on_all_quick(void);

// Miscellaneous, please put in correct file if you have time
int GotoMainMenu();
int GotoGameScreen();
int DropIntoDebugger();
int CreateDefaultNewSegment();
int CreateDefaultNewSegmentandAttach();
int ClearSelectedList();
int ClearFoundList();
int SetPlayerFromCurseg();
int SetPlayerFromCursegAndRotate();
int SetPlayerFromCursegMinusOne();
int FindConcaveSegs();
int do_reset_orient();
int GameZoomOut();
int GameZoomIn();

// John's temp page stuff
int medtmp_set_page();

// In objpage.c
int objpage_goto_next_object();

// In medsel.c
extern int SortSelectedList(void);
extern int SelectNextFoundSeg(void);
extern int SelectPreviousFoundSeg(void);

// In wall.c
extern int wall_add_blastable(void);
extern int wall_add_closed_wall(void);
extern int wall_add_external_wall(void);
extern int wall_lock_door(void);
extern int wall_automate_door(void);
extern int wall_deautomate_door(void);
extern int wall_assign_door_1(void);
extern int wall_assign_door_2(void);
extern int wall_assign_door_3(void);
extern int wall_assign_door_4(void);
extern int wall_assign_door_5(void);
extern int wall_assign_door_6(void);
extern int wall_assign_door_7(void);
extern int wall_assign_door_8(void);
extern int do_wall_dialog(void);
extern int do_trigger_dialog(void);
extern int check_walls(void);
extern int delete_all_walls(void);
#ifdef DXX_BUILD_DESCENT_II
extern int delete_all_controlcen_triggers(void);
#endif

// In switch.c
//extern int trigger_add_damage(void);
//extern int trigger_add_blank(void);
//extern int trigger_add_exit(void);
//extern int trigger_add_repair(void);
//extern int trigger_control(void);
//extern int trigger_remove(void);
//extern int trigger_add_if_control_center_dead(void);
extern int bind_wall_to_control_trigger(void);

// In med.c
extern int fuelcen_create_from_curseg();
extern int repaircen_create_from_curseg();
extern int controlcen_create_from_curseg();
extern int robotmaker_create_from_curseg();
extern int fuelcen_reset_all();
extern int RestoreGameState();
extern int fuelcen_delete_from_curseg();
#ifdef DXX_BUILD_DESCENT_II
extern int goal_blue_create_from_curseg();
extern int goal_red_create_from_curseg();
#endif

// In editor\robot.c
extern int do_robot_dialog();
extern int do_object_dialog();

#endif

