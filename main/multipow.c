#include "powerup.h"
#include "multipow.h"
#include "text.h"

int multi_allow_powerup_mask[MAX_POWERUP_TYPES] =
{ NETFLAG_DOINVUL, 0, 0, NETFLAG_DOLASER, 0, 0, 0, 0, 0, 0, 0, 0, NETFLAG_DOQUAD,
  NETFLAG_DOVULCAN, NETFLAG_DOSPREAD, NETFLAG_DOPLASMA, NETFLAG_DOFUSION,
  NETFLAG_DOPROXIM, NETFLAG_DOHOMING, NETFLAG_DOHOMING, NETFLAG_DOSMART,
  NETFLAG_DOMEGA, NETFLAG_DOVULCAN, NETFLAG_DOCLOAK, 0, NETFLAG_DOINVUL, 0, 0, 0 };

#if 0
char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX] =
{ "Laser upgrade", TXT_QUAD_LASERS, TXT_W_VULCAN, TXT_W_SPREADFIRE, TXT_W_PLASMA,
  TXT_W_FUSION, TXT_W_H_MISSILE, TXT_W_S_MISSILE, TXT_W_M_MISSILE, TXT_W_P_BOMB,
  "Cloaking", TXT_INVULNERABILITY };
#endif
char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX] =
{ "Laser upgrade", "Quad lasers", "Vulcan cannon", "Spreadfire cannon", "Plasma cannon",
  "Fusion cannon", "Homing missiles", "Smart missiles", "Mega missiles", "Proximity bombs",
  "Cloaking", "Invulnerability" };
