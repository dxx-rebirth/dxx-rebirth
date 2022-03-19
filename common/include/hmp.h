/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef __HMP_H
#define __HMP_H

#include <memory>
#include <tuple>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include "physfsx.h"

#ifdef __cplusplus
namespace dcx {

#define HMP_TRACKS 32
#ifdef _WIN32
#define MIDI_VOLUME_SCALE 128
#define HMP_BUFFERS 4
#define HMP_BUFSIZE 1024
#define HMP_INVALID_FILE -1
#define HMP_OUT_OF_MEM -2
#define HMP_MM_ERR -3
#define HMP_EOF 1

#define MIDI_CONTROL_CHANGE        0xB
#define MIDI_PROGRAM_CHANGE        0xC

#define MIDI_BANK_SELECT_MSB       0x00
#define MIDI_BANK_SELECT_LSB       0x20
#define MIDI_VOLUME                0x07
#define MIDI_PANPOT                0x0A
#define MIDI_REVERB                0x5B
#define MIDI_CHORUS                0x5D
#define MIDI_ALL_SOUNDS_OFF        0x78
#define MIDI_RESET_ALL_CONTROLLERS 0x79
#define MIDI_ALL_NOTES_OFF         0x7B

#define HMP_LOOP_START             0x6E
#define HMP_LOOP_END               0x6F
#endif

#ifdef _WIN32
struct event
{
	unsigned int delta;
	unsigned char msg[3];
	unsigned char *data;
	unsigned int datalen;
};
#endif

struct hmp_track
{
	std::unique_ptr<uint8_t[]> data;
	unsigned char *loop;
	unsigned int len;
	unsigned char *cur;
	unsigned int left;
	unsigned int cur_time;
	unsigned int loop_start;
	int loop_set;
};

struct hmp_file
{
	~hmp_file();
	PHYSFS_sint64 filesize;
	int num_trks;
	std::array<hmp_track, HMP_TRACKS> trks;
	unsigned int cur_time;
	unsigned int loop_start;
	unsigned int loop_end;
	int looping;
	int tempo;
#ifdef _WIN32
	MIDIHDR *evbuf;
	HMIDISTRM hmidi;
	UINT devid;
#endif
	unsigned char *pending;
	unsigned int pending_size;
	unsigned int pending_event;
	int stop;
	int bufs_in_mm;
	int bLoop;
	unsigned int midi_division;
};

enum class hmp_open_error : uint8_t
{
	None,
	/* All PhysFS errors must be before any non-PhysFS errors.  The error
	 * printing code relies on this to determine which error numbers are
	 * physfs_* and which are not.
	 */
	physfs_open,
	physfs_read_header,
	physfs_seek0,
	physfs_read_tracks,
	physfs_seek1,
	physfs_read_tempo,
	physfs_seek2,
	physfs_read_track_length,
	physfs_read_track_data,
	/* Non-PhysFS reasons start here */
	content_header,
	track_count,
};

/* PHYSFS_ErrorCode member is only meaningful if hmp_open_error is a
 * hmp_open_error::physfs_* code.  Otherwise, it is formally undefined.
 * Informally, it is 0. */
using hmp_open_result = std::tuple<std::unique_ptr<hmp_file>, hmp_open_error, PHYSFS_ErrorCode>;
using hmpmid_result = std::tuple<std::vector<uint8_t>, hmp_open_error>;

hmp_open_result hmp_open(const char *filename);
hmpmid_result hmp2mid(const char *hmp_name);
#ifdef _WIN32
void hmp_setvolume(hmp_file *hmp, int volume);
int hmp_play(hmp_file *hmp, int bLoop);
void hmp_pause(hmp_file *hmp);
void hmp_resume(hmp_file *hmp);
void hmp_reset();
#endif

}
#endif

#endif
