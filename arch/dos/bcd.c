/* bcd.c -- Brennan's CD-ROM Audio Playing Library
   by Brennan Underwood, http://brennan.home.ml.org/ */
//edited on 9/15/98 by Victor Rachels to stick it in d1x without extra crap

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>

#include "bcd.h"

typedef struct {
  int is_audio;
  int start, end, len;
} Track;

static int mscdex_version;
static int first_drive;
static int num_tracks;
static int lowest_track, highest_track;
static int audio_length;

static Track *tracks;

static int dos_mem_segment, dos_mem_selector = -1;

int _status, _error, _error_code;
char *_bcd_error = NULL;

#define RESET_ERROR (_error = _error_code = 0)
#define ERROR_BIT (1 << 15)
#define BUSY_BIT (1 << 9)
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#pragma pack(1)

/* I know 'typedef struct {} bleh' is a bad habit, but... */
typedef struct {
  unsigned char len		__attribute__((packed));
  unsigned char unit		__attribute__((packed));
  unsigned char command		__attribute__((packed));
  unsigned short status		__attribute__((packed));
  unsigned char reserved[8]	__attribute__((packed));
} RequestHeader;

typedef struct {
  RequestHeader request_header	__attribute__((packed));
  unsigned char descriptor	__attribute__((packed));
  unsigned long address		__attribute__((packed));
  unsigned short len		__attribute__((packed));
  unsigned short secnum		__attribute__((packed));
  unsigned long ptr		__attribute__((packed));
} IOCTLI;

typedef struct {
  unsigned char control		__attribute__((packed));
  unsigned char lowest		__attribute__((packed));
  unsigned char highest		__attribute__((packed));
  unsigned char total[4]	__attribute__((packed));
} DiskInfo;

typedef struct {
  unsigned char control		__attribute__((packed));
  unsigned char track_number	__attribute__((packed));
  unsigned char start[4]	__attribute__((packed));
  unsigned char info		__attribute__((packed));
} TrackInfo;

typedef struct {
  RequestHeader request	__attribute__((packed));
  unsigned char mode	__attribute__((packed));
  unsigned long start	__attribute__((packed));
  unsigned long len	__attribute__((packed));
} PlayRequest;

typedef struct {
  RequestHeader request	__attribute__((packed));
} StopRequest;

typedef struct {
  RequestHeader request	__attribute__((packed));
} ResumeRequest;

typedef struct {
  unsigned char control	__attribute__((packed));
  unsigned char input0	__attribute__((packed));
  unsigned char volume0	__attribute__((packed));
  unsigned char input1	__attribute__((packed));
  unsigned char volume1	__attribute__((packed));
  unsigned char input2	__attribute__((packed));
  unsigned char volume2	__attribute__((packed));
  unsigned char input3	__attribute__((packed));
  unsigned char volume3	__attribute__((packed));
} VolumeRequest;

typedef struct {
  unsigned char control	__attribute__((packed));
  unsigned char fn	__attribute__((packed));
} LockRequest;

typedef struct {
  unsigned char control	__attribute__((packed));
  unsigned char mbyte	__attribute__((packed));
} MediaChangedRequest;

typedef struct {
  unsigned char control	__attribute__((packed));
  unsigned long status	__attribute__((packed));
} StatusRequest;

typedef struct {
  unsigned char control	__attribute__((packed));
  unsigned char mode	__attribute__((packed));
  unsigned long loc	__attribute__((packed));
} PositionRequest;

#pragma pack()

char *bcd_error(void)
{
  static char retstr[132];
  char *errorcodes[] = { "Write-protect violation",
                         "Unknown unit",
                         "Drive not ready",
                         "Unknown command",
                         "CRC error",
                         "Bad drive request structure length",
                         "Seek error",
                         "Unknown media",
                         "Sector not found",
                         "Printer out of paper: world coming to an end",/* I mean really, on a CD? */
                         "Write fault",
                         "Read fault",
                         "General failure",
                         "Reserved",
                         "Reserved",
                         "Invalid disk change" };
  *retstr = 0;

    if (_error != 0)
     {
       strcat(retstr, "Device error: ");
        if (_error_code < 0 || _error_code > 0xf)
         strcat(retstr, "Invalid error");
        else
         strcat(retstr, errorcodes[_error_code]);
       strcat(retstr, "  ");
     }
    if (_bcd_error != NULL)
     {
        if (*retstr) strcat(retstr, ", ");
         strcat(retstr, "BCD error: ");
       strcat(retstr, _bcd_error);
     }
   return retstr;
}

/* DOS IOCTL w/ command block */
static void bcd_ioctl(IOCTLI *ioctli, void *command, int len) {
  int ioctli_len = sizeof(IOCTLI);
  unsigned long command_address = dos_mem_segment << 4;
  __dpmi_regs regs;

  memset(&regs, 0, sizeof regs);
  regs.x.es = (__tb >> 4) & 0xffff;
  regs.x.ax = 0x1510;
  regs.x.bx = __tb & 0xf;
  regs.x.cx = first_drive;
  ioctli->address = dos_mem_segment << 16;
  ioctli->len = len;
  dosmemput(ioctli, ioctli_len, __tb);		/* put ioctl into dos area */
  dosmemput(command, len, command_address);	/* and command too */
  if (__dpmi_int(0x2f, &regs) == -1) {
    _bcd_error = "__dpmi_int() failed";
    return;
  }
  dosmemget(__tb, ioctli_len, ioctli);		/* retrieve results */
  dosmemget(command_address, len, command);
  _status = ioctli->request_header.status;
  if (_status & ERROR_BIT) {
    _error = TRUE;
    _error_code = _status & 0xff;
  } else {
    _error = FALSE;
    _error_code = 0;
  }
}

/* no command block */
static void bcd_ioctl2(void *cmd, int len) {
  __dpmi_regs regs;
  memset(&regs, 0, sizeof regs);
  regs.x.es = (__tb >> 4) & 0xffff;
  regs.x.ax = 0x1510;
  regs.x.bx = __tb & 0xf;
  regs.x.cx = first_drive;
  dosmemput(cmd, len, __tb); /* put ioctl block in dos arena */
  if (__dpmi_int(0x2f, &regs) == -1) {
    _bcd_error = "__dpmi_int() failed";
    return;
  }
  /* I hate to have no error capability for ioctl2 but the command block
     doesn't necessarily have a status field */
  RESET_ERROR;
}

static int red2hsg(char *r) {
  return r[0] + r[1]*75 + r[2]*4500 - 150;
}

int bcd_now_playing(void) {
  int i, loc = bcd_audio_position();
  _bcd_error = NULL;
  if (!bcd_audio_busy()) {
    _bcd_error = "Audio not playing";
    return 0;
  }
  if (tracks == NULL && !bcd_get_audio_info())
    return 0;
  for (i = lowest_track; i <= highest_track; i++) {
    if (loc >= tracks[i].start && loc <= tracks[i].end) return i;
  }
  /* some bizarre location? */
  _bcd_error = "Head outside of bounds";
  return 0;
}

/* handles the setup for CD-ROM audio interface */
int bcd_open(void) {
  __dpmi_regs regs;
  _bcd_error = NULL;

  /* disk I/O wouldn't work anyway if you set sizeof tb this low, but... */
  if (_go32_info_block.size_of_transfer_buffer < 4096) {
    _bcd_error = "Transfer buffer too small";
    return 0;
  }

  memset(&regs, 0, sizeof regs);
  regs.x.ax = 0x1500;
  regs.x.bx = 0x0;
  __dpmi_int(0x2f, &regs);
  if (regs.x.bx == 0) {	/* abba no longer lives */
    _bcd_error = "MSCDEX not found";
    return 0;
  }

  first_drive = regs.x.cx; /* use the first drive */

  /* check for mscdex at least 2.0 */
  memset(&regs, 0, sizeof regs);
  regs.x.ax = 0x150C;
  __dpmi_int(0x2f, &regs);
  if (regs.x.bx == 0) {
    _bcd_error = "MSCDEX version < 2.0";
    return 0;
  }
  mscdex_version = regs.x.bx;

  /* allocate 256 bytes of dos memory for the command blocks */
  if ((dos_mem_segment = __dpmi_allocate_dos_memory(16, &dos_mem_selector))<0) {
    _bcd_error = "Could not allocate 256 bytes of DOS memory";
    return 0;
  }

  atexit( (void(*)()) bcd_close );
  return mscdex_version;
}

/* Shuts down CD-ROM audio interface */
int bcd_close(void) {
  _bcd_error = NULL;
  if (dos_mem_selector != -1) {
    __dpmi_free_dos_memory(dos_mem_selector);
    dos_mem_selector = -1;
  }
#ifndef STATIC_TRACKS
  if (tracks) free(tracks);
  tracks = NULL;
#endif
  RESET_ERROR;
  return 1;
}

int bcd_open_door(void) {
  IOCTLI ioctli;
  char eject = 0;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 12;
  ioctli.len = 1;
  bcd_ioctl(&ioctli, &eject, sizeof eject);
  if (_error) return 0;
  return 1;
}

int bcd_close_door(void) {
  IOCTLI ioctli;
  char closeit = 5;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 12;
  ioctli.len = 1;
  bcd_ioctl(&ioctli, &closeit, sizeof closeit);
  if (_error) return 0;
  return 1;
}

int bcd_lock(int fn) {
  IOCTLI ioctli;
  LockRequest req;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  memset(&req, 0, sizeof req);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 12;
  ioctli.len = sizeof req;
  req.control = 1;
  req.fn = fn ? 1 : 0;
  bcd_ioctl(&ioctli, &req, sizeof req);
  if (_error) return 0;
  return 1;
}


int bcd_disc_changed(void) {
  IOCTLI ioctli;
  MediaChangedRequest req;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  memset(&req, 0, sizeof req);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 3;
  ioctli.len = sizeof req;
  req.control = 9;
  bcd_ioctl(&ioctli, &req, sizeof req);
  return req.mbyte;
}

int bcd_reset(void) {
  IOCTLI ioctli;
  char reset = 2;
  _bcd_error = NULL;

  memset(&ioctli, 0, sizeof ioctli);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 12;
  ioctli.len = 1;
  bcd_ioctl(&ioctli, &reset, sizeof reset);
  if (_error) return 0;
  return 1;
}

int bcd_device_status(void) {
  IOCTLI ioctli;
  StatusRequest req;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  memset(&req, 0, sizeof req);
  ioctli.request_header.len = sizeof ioctli; // ok
  ioctli.request_header.command = 3;
  ioctli.len = sizeof req;
  req.control = 6;
  bcd_ioctl(&ioctli, &req, sizeof req);
  return req.status;
}

static int bcd_get_status_word(void) {
  IOCTLI ioctli;
  DiskInfo disk_info;

  /* get cd info as an excuse to get a look at the status word */
  memset(&disk_info, 0, sizeof disk_info);
  memset(&ioctli, 0, sizeof ioctli);

  ioctli.request_header.len = 26;
  ioctli.request_header.command = 3;
  ioctli.len = 7;
  disk_info.control = 10;
  bcd_ioctl(&ioctli, &disk_info, sizeof disk_info);
  return _status;
}

int bcd_audio_busy(void) {
  _bcd_error = NULL;
  /* If the door is open, then the head is busy, and so the busy bit is
     on. It is not, however, playing audio. */
  if (bcd_device_status() & BCD_DOOR_OPEN)
    return 0;

  bcd_get_status_word();
  if (_error) return -1;
  return (_status & BUSY_BIT) ? 1 : 0;
}

int bcd_audio_position(void) {
  IOCTLI ioctli;
  PositionRequest req;
  _bcd_error = NULL;
  memset(&ioctli, 0, sizeof ioctli);
  memset(&req, 0, sizeof req);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 3;
  ioctli.len = sizeof req;
  req.control = 1;
  bcd_ioctl(&ioctli, &req, sizeof req);
  return req.loc;
}

/* Internal function to get track info */
static void bcd_get_track_info(int n, Track *t) {
  IOCTLI ioctli;
  TrackInfo info;

  memset(&ioctli, 0, sizeof ioctli);
  memset(&info, 0, sizeof info);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 3;
  info.control = 11;
  info.track_number = n;
  bcd_ioctl(&ioctli, &info, sizeof info);
  t->start = red2hsg(info.start);
  if (info.info & 64)
    t->is_audio = 0;
  else
    t->is_audio = 1;
}

int bcd_get_audio_info(void) {
  IOCTLI ioctli;
  DiskInfo disk_info;
  int i;


  _bcd_error = NULL;

  if (tracks) free(tracks);
  tracks = NULL;

  memset(&disk_info, 0, sizeof disk_info);
  memset(&ioctli, 0, sizeof ioctli);

  ioctli.request_header.len = 26;
  ioctli.request_header.command = 3;
  ioctli.len = 7;
  disk_info.control = 10;
  bcd_ioctl(&ioctli, &disk_info, sizeof disk_info);
  if (_error) return 0;

  lowest_track = disk_info.lowest;
  highest_track = disk_info.highest;
   if(lowest_track > highest_track)
    lowest_track = 1;
  num_tracks = highest_track - lowest_track + 1;

  /* alloc max space in order to attempt to avoid possible overrun bug */
  tracks = calloc(highest_track+1, sizeof(Track));
  if (tracks == NULL) {
    _bcd_error = "Out of memory allocating tracks\n";
    return 0;
  }

  /* get track starts */
  for (i = lowest_track; i <= highest_track; i++)
    bcd_get_track_info(i, tracks+i);
 
  /* figure out track ends */
  for (i = lowest_track; i < highest_track; i++)
    tracks[i].end = tracks[i+1].start-1;
  audio_length = red2hsg(disk_info.total);
  tracks[i].end = audio_length;
  for (i = lowest_track; i <= highest_track; i++)
    tracks[i].len = tracks[i].end - tracks[i].start;

  return num_tracks;
}

int bcd_get_track_address(int trackno, int *start, int *len) {
  _bcd_error = NULL;
  //if (trackno >= num_tracks+1 || trackno <= 0) {
  if (trackno < lowest_track || trackno > highest_track) {
    _bcd_error  = "Track out of range";
    *start = *len = 0;
    return 0;
  }
  *start = tracks[trackno].start;
  *len = tracks[trackno].len;
  return 1;
}

int bcd_track_is_audio(int trackno) {
  //if (trackno >= num_tracks+1 || trackno <= 0) {
  if (trackno < lowest_track || trackno > highest_track) {
    _bcd_error = "Track out of range";
    return 0;
  }
  return tracks[trackno].is_audio;
}

int bcd_set_volume(int volume) {
  IOCTLI ioctli;
  VolumeRequest v;

  _bcd_error = NULL;
  if (volume > 255) volume = 255;
  else if (volume < 0) volume = 0;
  memset(&ioctli, 0, sizeof ioctli);
  ioctli.request_header.len = sizeof ioctli;
  ioctli.request_header.command = 12;
  ioctli.len = sizeof v;
  v.control = 3;
  v.volume0 = volume;
  v.input0 = 0;
  v.volume1 = volume;
  v.input1 = 1;
  v.volume2 = volume;
  v.input2 = 2;
  v.volume3 = volume;
  v.input3 = 3;
  
  bcd_ioctl(&ioctli, &v, sizeof v);
  if (_error) return 0;
  return 1;
}

int bcd_play(int location, int frames) {
  PlayRequest cmd;
  memset(&cmd, 0, sizeof cmd);

  _bcd_error = NULL;
  /* the following should be in user code, but it'll fail otherwise */
  if (bcd_audio_busy())
    bcd_stop();

  cmd.request.len = sizeof cmd;
  cmd.request.command = 132;
  cmd.start = location;
  cmd.len = frames;
  bcd_ioctl2(&cmd, sizeof cmd);
  if (_error) return 0;
  return 1;
}

int bcd_play_track(int *trackno) {
  _bcd_error = NULL;
  if (!bcd_get_audio_info()) return 0;

  if (*trackno < lowest_track)
   *trackno = highest_track;
  else if (*trackno > highest_track)
   *trackno = lowest_track;

  if (! tracks[*trackno].is_audio)
   {
    _bcd_error = "Not an audio track";
    return 0;
   }

  return bcd_play(tracks[*trackno].start, tracks[*trackno].len);
}

int bcd_stop(void) {
  StopRequest cmd;
  _bcd_error = NULL;
  memset(&cmd, 0, sizeof cmd);
  cmd.request.len = sizeof cmd;
  cmd.request.command = 133;
  bcd_ioctl2(&cmd, sizeof cmd);
  if (_error) return 0;
  return 1;
}

int bcd_resume(void) {
  ResumeRequest cmd;
  _bcd_error = NULL;
  memset(&cmd, 0, sizeof cmd);
  cmd.request.len = sizeof cmd;
  cmd.request.command = 136;
  bcd_ioctl2(&cmd, sizeof cmd);
  if (_error) return 0;
  return 1;
}

#ifdef __cplusplus
}
#endif

#ifdef STANDALONE
static char *card(int c) {
  return c == 1 ? "" : "s";
}

static void print_hsg(int hsg) {
  int hours, minutes, seconds;
  seconds = hsg / 75;
  minutes = seconds / 60;
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  printf("%2d:%02d:%02d", hours, minutes, seconds);
}

static void print_binary(int v, int len) {
  for (;len;len--)
    printf("%d", (v & (1 << len)) ? 1 : 0);
}

int main(int argc, char *argv[]) {
  int i, n1, n2, t;

  if (!bcd_open()) {
    fprintf(stderr, "Couldn't open CD-ROM drive. %s\n", bcd_error());
    exit(0);
  }
  
  for (i = 1; i < argc; i++) {
    if (*argv[i] == '-') strcpy(argv[i], argv[i]+1);
    if (!strcmp(argv[i], "open") || !strcmp(argv[i], "eject")) {
      bcd_open_door();
    } else if (!strcmp(argv[i], "close")) {
      bcd_close_door();
    } else if (!strcmp(argv[i], "sleep")) {
      if (++i >= argc) break;
      sleep(atoi(argv[i]));
    } else if (!strcmp(argv[i], "list")) {
      int nd = 0, na = 0, audio_time = 0;
      if (!bcd_get_audio_info()) {
        printf("Error getting audio info\n");
      } else if (lowest_track == 0) {
        printf("No audio tracks\n");
      } else {
        for (t = lowest_track; t <= highest_track; t++) {
          printf("Track %2d: ", t);
          print_hsg(tracks[t].start);
          printf(" -> ");
          print_hsg(tracks[t].end);
          printf(" (");
          print_hsg(tracks[t].len);
          if (tracks[t].is_audio) {
            na++;
            printf(") audio");
            audio_time += tracks[t].len;
          } else {
            nd++;
            printf(") data ");
          }
          printf(" (HSG: %06d->%06d)\n", tracks[t].start, tracks[t].end);
        }
        printf("%d audio track%s, %d data track%s\n", na, card(na), nd, card(nd));
        if (audio_time) {
          printf("Audio time: ");
          print_hsg(audio_time);
          printf("\n");
        }
      }
    } else if (!strcmp(argv[i], "lock")) {
      bcd_lock(1);
    } else if (!strcmp(argv[i], "pladdr")) {
      if (++i >= argc) break;
      n1 = atoi(argv[i]);
      if (++i >= argc) break;
      n2 = atoi(argv[i]);
      printf("Playing frame %d to frame %d\n", n1, n2);
      bcd_play(n1, n2-n1);
    } else if (!strcmp(argv[i], "play")) {
      if (++i >= argc) break;
      if (bcd_audio_busy()) {
        bcd_stop();
        delay(1000);
      }
      n1 = atoi(argv[i]);
      printf("Playing track %d\n", n1);
      bcd_play_track(n1);
    } else if (!strcmp(argv[i], "reset")) {
      bcd_reset();
    } else if (!strcmp(argv[i], "resume")) {
      bcd_resume();
    } else if (!strcmp(argv[i], "status")) {
      int s;
      s = bcd_device_status();
      printf("MSCDEX version %d.%d\n", mscdex_version >> 8,
             mscdex_version & 0xff);
      printf("Device status word '");
      print_binary(s, 16);
      printf("'\nDoor is %sopen\n", s & BCD_DOOR_OPEN ? "" : "not ");
      printf("Door is %slocked\n", s & BCD_DOOR_UNLOCKED ? "not " : "");
      printf("Audio is %sbusy\n", bcd_audio_busy() ? "" : "not ");
      s = bcd_disc_changed();
      if (s == BCD_DISC_UNKNOWN) printf("Media change status unknown\n");
      else printf("Media has %schanged\n",
           (s == BCD_DISC_CHANGED) ? "" : "not ");
    } else if (!strcmp(argv[i], "stop")) {
      bcd_stop();
    } else if (!strcmp(argv[i], "unlock")) {
      bcd_lock(0);
    } else if (!strcmp(argv[i], "volume")) {
      bcd_set_volume(atoi(argv[++i]));
    } else if (!strcmp(argv[i], "wait")) {
      while (bcd_audio_busy()) {
        int n = bcd_now_playing();
        if (n == 0) break;
        printf("%2d: ", n);
        print_hsg(bcd_audio_position() - tracks[n].start);
        printf("\r");
        fflush(stdout);
        delay(100);
        if (kbhit() && getch() == 27) break;
      }
      printf("\n");
    } else if (!strcmp(argv[i], "help") || !strcmp(argv[i], "usage") ||
               !strcmp(argv[i], "?")    || !strcmp(argv[i], "/?")      ) {
      printf("BCD version %x.%x\n" \
             "Usage: BCD {commands}\n" \
             "Valid commands:\n" \
             "\tclose		- close door/tray\n" \
             "\tdelay {n}	- delay {n} seconds\n" \
             "\tlist		- list track info\n" \
             "\tlock		- lock door/tray\n" \
             "\topen		- open door/tray\n" \
             "\tpladdr {n1} {n2}- play frame {n1} to {n2}\n" \
             "\tplay {n}	- play track {n}\n" \
             "\treset		- reset the drive\n" \
             "\tresume		- resume from last stop\n" \
             "\tstatus		- show drive status\n" \
             "\tstop		- stop audio playback\n" \
             "\tunlock		- unlock door/tray\n" \
             "\tvolume {n}	- set volume to {n} where 0 <= {n} <= 255\n",
             BCD_VERSION >> 8, BCD_VERSION & 0xff);
    } else
      printf("Unknown command '%s'\n", argv[i]);
    if (_error || _bcd_error) printf("%s\n", bcd_error());
  }
  bcd_close();
  exit(0);
}
#endif
