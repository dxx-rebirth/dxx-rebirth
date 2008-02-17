/* hmp 2 midi file conversion. The conversion algorithm is based on the
   conversion algorithms found in d1x, d2x-xl and jjffe. The code is mine.
   
   Copyright 2006 Hans de Goede
   
   This file and the acompanying hmp2mid.h file are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif
#include "hmp2mid.h"
#include "u_mem.h"

/* Some convience macros to keep the code below more readable */

#define HMP_READ(buf, count) \
  {\
    if (PHYSFS_read(hmp_in, buf, 1, count) != (count)) \
    { \
	  if (mid_track_buf) \
       d_free(mid_track_buf); \
      return hmp_read_error; \
    } \
    /* notice that when we are not actually in the track reading code this \
       is bogus, but does no harm. And in the track code it helps :) */ \
    track_length -= count; \
  }

#define HMP_READ_DWORD(dest) \
  { \
    unsigned char _buf[4]; \
    HMP_READ(_buf, 4); \
    *(dest) = (_buf[3] << 24) | (_buf[2] << 16) | (_buf[1] << 8) | _buf[0]; \
  }

#define MID_WRITE(buf, count) \
  if (PHYSFS_write(mid_out, buf, 1, count) != (count)) \
  { \
	if (mid_track_buf) \
	  d_free(mid_track_buf); \
	strncpy(hmp2mid_error, mid_write_error_templ, sizeof(hmp2mid_error)); \
	strncat(hmp2mid_error, PHYSFS_getLastError(), \
      sizeof(hmp2mid_error) - sizeof(mid_write_error_templ) + 1); \
    return hmp2mid_error; \
  }

#define MID_TRACK_BUF(buf, count) \
  { \
    if ((mid_track_buf_used + count) > mid_track_buf_size) \
    { \
      void *tmp = mid_track_buf; \
      mid_track_buf = d_realloc(mid_track_buf, mid_track_buf_size + 65536); \
      if (!mid_track_buf) \
      { \
        d_free(tmp); \
        return "Error could not allocate midi track memory"; \
      } \
    } \
    memcpy(mid_track_buf + mid_track_buf_used, buf, count); \
    mid_track_buf_used += count; \
  }
    
  
static const char *hmp_read_error = "Error reading from hmp file";
static const char mid_write_error_templ[] = "Error writing to mid file: ";
static char hmp2mid_error[512];
/* The beginning of the midi header */
static const char midi_header1[10] = { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1 };
/* The last 2 bytes of the midi header and track0 */
static const char midi_header2[21] = { 0, 0xC0, 'M', 'T', 'r', 'k', 0, 0, 0,
  0x0B, 0, 0xFF, 0x51, 0x03, 0x18, 0x80, 0, 0, 0xFF, 0x2F, 0 };
  
const char *hmp2mid(PHYSFS_file *hmp_in, PHYSFS_file *mid_out)
{
  unsigned char last_com = 0, buf[0x300];
  unsigned int num_tracks, track_length = 0, i, t;
  unsigned char *mid_track_buf = NULL;
  int n1, n2, mid_track_buf_used, mid_track_buf_size = 0;
  
  /* Read the header and verify that this is a hmp file */
  HMP_READ(buf, 8);
  buf[8] = 0;
  if (strcmp((char *)buf, "HMIMIDIP"))
    return "Error not a hmp file";
  
  /* Discard the next 0x28 bytes, taking us to location 0x30 and read the
     number of tracks there */
  HMP_READ(buf, 0x30 - 8);
  HMP_READ_DWORD(&num_tracks);
  
  /* Discard bytes taking us to location 0x300 around which track 1 starts */
  HMP_READ(buf, 0x300 - 0x34);
  
  /* Now search for the end of track marker of track 0 */
  HMP_READ(buf, 3);
  while ((buf[0] != 0xFF) || (buf[1] != 0x2F) || (buf[2] != 0x00))
  {
    buf[0] = buf[1];
    buf[1] = buf[2];
    HMP_READ(buf + 2, 1);
  }
  
  /* Sofar so good, write out the midi header */
  MID_WRITE(midi_header1, sizeof(midi_header1));
  /* number of tracks is a 16 bit MSB word, better not have more then 65535
     tracks :) */
  buf[0] = (num_tracks >> 8) & 0xFF;
  buf[1] = num_tracks & 0xFF;
  MID_WRITE(buf, 2);
  /* Write the last word of the header and track0 */
  MID_WRITE(midi_header2, sizeof(midi_header2));
  
  /* Read and convert all the tracks */
  for (i = 1; i < num_tracks; i++)
  {
    /* Read and verify hmp track number */
    HMP_READ_DWORD(&t);
    if (t != i)
    {
      d_free(mid_track_buf);
      return "Error invalid hmp track number";
    }
    /* Read number of bytes in this track */
    HMP_READ_DWORD(&track_length);
    /* Above we've already read the first 8 bytes of this track, adjust
       track_lenght accordingly. */
    track_length -= 8;
    /* Read and skip the next 4 unidentified header bytes */
    HMP_READ(buf, 4);
    
    /* Sofar so good write midi track header */
    MID_WRITE("MTrk", 4);

    /* Now we need to write the track length, but we don't know that yet, so
       the track gets written to a dynamicly growing buffer (which is hidden
       in the MID_TRACK_BUF macro) */
    mid_track_buf_used = 0;

    /* And finally start the real conversion */
    while (track_length > 0)
    {
      n1 = -1; /* -1 to "counter" the intital n1++ */
      do
      {
        n1++;
        HMP_READ(buf + n1, 1);
      } while (!(buf[n1] & 0x80));
      
      if (n1 >= 4)
      {
        d_free(mid_track_buf);
        return "Error parsing hmp track";
      }

      for (n2 = 0; n2 <= n1; n2++)
      {
        unsigned char u = buf[n1-n2] & 0x7F;
        
        if (n2 != n1)
          u |= 0x80;
        MID_TRACK_BUF(&u, 1);
      }

      HMP_READ(buf, 1);

      if (buf[0] == 0xFF) /* meta? */
      {
        HMP_READ(buf + 1, 2);
        HMP_READ(buf + 3, buf[2]);
        MID_TRACK_BUF(buf, 3 + buf[2]);
        if (buf[1] == 0x2F) /* end of track? */
        {
          if (buf[2] != 0x00)
          {
            d_free(mid_track_buf);
            return "Error hmp meta end of track with non zero size";
          }
          break;
        }
        else
          continue;
      }

      switch (buf[0] & 0xF0)
      {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
          t = 2;
          break;
        case 0xC0:
        case 0xD0:
          t = 1;
          break;
        default:
          d_free(mid_track_buf);
          return "Error invalid hmp command";
      }
      if (buf[0] != last_com)
        MID_TRACK_BUF(buf, 1);
      HMP_READ(buf + 1, t);
      MID_TRACK_BUF(buf + 1, t);
      last_com = buf[0];
    }
    if (track_length != 0)
    {
      d_free(mid_track_buf);
      return "Error invalid track length";
    }
    /* write the midi track length */
    buf [0] = mid_track_buf_used >> 24;
    buf [1] = (mid_track_buf_used >> 16) & 0xFF;
    buf [2] = (mid_track_buf_used >> 8) & 0xFF;
    buf [3] = mid_track_buf_used & 0xFF;
    MID_WRITE(buf, 4);
    /* and the track itself */
    MID_WRITE(mid_track_buf, mid_track_buf_used);
  }
  d_free (mid_track_buf);
  return NULL;
}
