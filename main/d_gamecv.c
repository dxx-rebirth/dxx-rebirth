// Game conversion functions
// On i386 most of these could be replaced with a memcpy.
// But we'll keep them like this as it's much more portable and they're
// still pretty fast.

#include <string.h>

#include "maths.h"
#include "vecmat.h"
#include "aistruct.h"
#include "d_gamecv.h"

// AI related structs.

void d_import_ai_local(ai_local *dest, char *src)
{
 int i;
 dest->player_awareness_type = *src++;
 dest->retry_count = *src++;
 dest->consecutive_retries = *src++;
 dest->mode = *src++;
 dest->previous_visibility = *src++;
 dest->rapidfire_count = *src++;
 dest->goal_segment = d_import_short(src);        src +=SIZEOF_SHORT;
 dest->last_see_time = d_import_fix(src);         src +=SIZEOF_FIX;
 dest->last_attack_time = d_import_fix(src);      src +=SIZEOF_FIX;
 dest->wait_time = d_import_fix(src);             src +=SIZEOF_FIX;
 dest->next_fire = d_import_fix(src);             src +=SIZEOF_FIX;
 dest->player_awareness_time = d_import_fix(src); src +=SIZEOF_FIX;
 dest->time_player_seen = d_import_fix(src);      src +=SIZEOF_FIX;
 dest->time_player_sound_attacked = d_import_fix(src); src +=SIZEOF_FIX;
 dest->next_misc_sound_time = d_import_fix(src);  src +=SIZEOF_FIX;
 dest->time_since_processed = d_import_fix(src);  src +=SIZEOF_FIX;
 for (i=0; i<MAX_SUBMODELS; i++) {
   d_import_vms_angvec(&dest->goal_angles[i],src);
   src += SIZEOF_VMS_ANGVEC;
 }
 for (i=0; i<MAX_SUBMODELS; i++) {
   d_import_vms_angvec(&dest->delta_angles[i],src);
   src += SIZEOF_VMS_ANGVEC;
 }
 for (i=0; i<MAX_SUBMODELS; i++)
   dest->goal_state[i] = *src++;
 for (i=0; i<MAX_SUBMODELS; i++)
   dest->achieved_state[i] = *src++;
}

void d_export_ai_local(char *dest, ai_local *src)
{
 int i;
 *dest++ = src->player_awareness_type;
 *dest++ = src->retry_count;
 *dest++ = src->consecutive_retries;
 *dest++ = src->mode;
 *dest++ = src->previous_visibility;
 *dest++ = src->rapidfire_count;
 d_export_short(dest,src->goal_segment);        dest += SIZEOF_SHORT;
 d_export_fix(dest,src->last_see_time);         dest += SIZEOF_FIX;
 d_export_fix(dest,src->last_attack_time);      dest += SIZEOF_FIX;
 d_export_fix(dest,src->wait_time);             dest += SIZEOF_FIX;
 d_export_fix(dest,src->next_fire);             dest += SIZEOF_FIX;
 d_export_fix(dest,src->player_awareness_time); dest += SIZEOF_FIX;
 d_export_fix(dest,src->time_player_seen);      dest += SIZEOF_FIX;
 d_export_fix(dest,src->time_player_sound_attacked); dest += SIZEOF_FIX;
 d_export_fix(dest,src->next_misc_sound_time);  dest += SIZEOF_FIX;
 d_export_fix(dest,src->time_since_processed);  dest += SIZEOF_FIX;
 for (i=0; i<MAX_SUBMODELS; i++) {
   d_export_vms_angvec(dest, &src->goal_angles[i]);
   dest += SIZEOF_VMS_ANGVEC;
 }
 for (i=0; i<MAX_SUBMODELS; i++) {
   d_export_vms_angvec(dest, &src->delta_angles[i]);
   dest += SIZEOF_VMS_ANGVEC;
 }
 for (i=0; i<MAX_SUBMODELS; i++)
   *dest++=src->goal_state[i];
 for (i=0; i<MAX_SUBMODELS; i++)
   *dest++=src->achieved_state[i];
}

void d_import_point_seg(point_seg *dest, char *src)
{
 dest->segnum = d_import_int(src);
 d_import_vms_vector(&dest->point, src + SIZEOF_INT);
}

void d_export_point_seg(char *dest, point_seg *src)
{
 d_export_int(dest,src->segnum);
 d_export_vms_vector(dest + SIZEOF_INT, &src->point);
}

