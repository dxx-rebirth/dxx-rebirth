/*
 * Allegro Sound library support routines
 * Arne de Bruijn
 */

#include <go32.h>
#include <dpmi.h>
#include <stdlib.h>
#include <string.h>
#include "cfile.h"
#include "internal.h"
#include <sys/stat.h>
#include "error.h"
#include "timer.h"
#include "d_io.h"

extern int digi_timer_rate;
int retrace_count;
char allegro_error[80]="";

#define TIMERS_PER_SECOND     1193181L
#define MSEC_TO_TIMER(x)      ((long)(x) * (TIMERS_PER_SECOND / 1000))
#define MAX_TIMERS      16
static struct {                           /* list of active callbacks */
   void ((*proc)());
   long speed;
   long counter;
} my_int_queue[MAX_TIMERS];
static int find_timer_slot(void (*proc)())
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if (my_int_queue[x].proc == proc)
	 return x;

   return -1;
}
int install_int_ex(void (*proc)(), long speed)
{
   int x;
#if 0
   if (!timer_installed) {
      /* we are not alive yet: flag this callback to be started later */
      if (waiting_list_size >= MAX_TIMERS)
	 return -1;

      waiting_list[waiting_list_size].proc = proc;
      waiting_list[waiting_list_size].speed = speed;
      waiting_list_size++;
      return 0;
   }
#endif
   x = find_timer_slot(proc);          /* find the handler position */

   if (x < 0)                          /* if not there, find free slot */
      x = find_timer_slot(NULL);

   if (x < 0)                          /* are there any free slots? */
      return -1;

   if (proc != my_int_queue[x].proc) { /* add new entry */
      my_int_queue[x].counter = speed;
      my_int_queue[x].proc = proc; 
   }
   else {                              /* alter speed of existing entry */
      my_int_queue[x].counter -= my_int_queue[x].speed;
      my_int_queue[x].counter += speed;
   }

   my_int_queue[x].speed = speed;

   return 0;
}
int install_int(void (*proc)(), long speed)
{
   return install_int_ex(proc, MSEC_TO_TIMER(speed));
}
void remove_int(void (*proc)())
{
   int x = find_timer_slot(proc);

   if (x >= 0) {
      my_int_queue[x].proc = NULL;
      my_int_queue[x].speed = 0;
      my_int_queue[x].counter = 0;
   }
}

void allg_snd_timer() {
	int x;
	for (x=0; x<MAX_TIMERS; x++) {
		my_int_queue[x].counter -= digi_timer_rate;
		while ((my_int_queue[x].proc) && (my_int_queue[x].counter <= 0)) {
			   my_int_queue[x].counter += my_int_queue[x].speed;
			   my_int_queue[x].proc();
		}
	}
	retrace_count++;
}
END_OF_FUNCTION(allg_snd_timer)

void _add_exit_func(void (*func)()) {
	atexit(func);
}
void _remove_exit_func(void (*func)()) {
}
void _unlock_dpmi_data(void *addr, int size)
{
   unsigned long baseaddr;
   __dpmi_meminfo mem;

   __dpmi_get_segment_base_address(_go32_my_ds(), &baseaddr);

   mem.address = baseaddr + (unsigned long)addr;
   mem.size = size;

   __dpmi_unlock_linear_region(&mem );
}

struct oldirq {
	int intnum;
	_go32_dpmi_seginfo old;
	_go32_dpmi_seginfo new;
};

//added/killed on 10/27/98 by adb.  moved to comm/irq.c
//-killed- struct oldirq saveirq={-1};

//-killed- int _install_irq(int intnum, int (*func)()) {
//-killed-     if (saveirq.intnum != -1)
//-killed-         return 1;
//-killed-     saveirq.intnum=intnum;
//-killed-     saveirq.new.pm_offset = (int)func;
//-killed-     saveirq.new.pm_selector = _my_cs();
//-killed-     _go32_dpmi_get_protected_mode_interrupt_vector(intnum, &saveirq.old);
//-killed-     _go32_dpmi_allocate_iret_wrapper(&saveirq.new);
//-killed-     _go32_dpmi_set_protected_mode_interrupt_vector(intnum, &saveirq.new);
//-killed-     return 0;
//-killed- }
//-killed- 
//-killed- void _remove_irq(int intnum) {
//-killed-            _go32_dpmi_set_protected_mode_interrupt_vector(intnum, &saveirq.old);
//-killed-            _go32_dpmi_free_iret_wrapper(&saveirq.new);
//-killed-            saveirq.intnum = -1;
//-killed- }
//end this section kill - adb

static unsigned char *conv_delta(unsigned char *data, unsigned char *dataend) {
    unsigned char *p;
    unsigned long delta = 0;
    int shift = 0;

    p = data;
    while ((p < dataend) && !(*p & 0x80)) {
	delta += *(p++) << shift;
	shift += 7;
    }
    if (p == dataend)
	return NULL;
    delta += (*(p++) & 0x7f) << shift;

    data = p;
    shift = 0;
    while ((delta > 0) || (shift == 0)) {
	*(--p) = (delta & 127) | shift;
	delta >>= 7;
	shift = 128;
    }
    return data;
}

/*
 * read a MIDI type variabele length number
 */
static unsigned char *read_var(unsigned char *data, unsigned char *dataend,
 unsigned long *value) {
    unsigned long v = 0;

    while ((data < dataend) && (*data & 0x80))
	v = (v << 7) + (*(data++) & 0x7f);
    if (data == dataend)
	return NULL;
    v = (v << 7) + *(data++);
    if (value) *value = v;
    return data;
}

static int trans_data(unsigned char *data, int size) {
    static int cmdlen[7]={3,3,3,3,2,2,3};
    unsigned char *dataend = data + size;
    unsigned long v;

    while (data < dataend) {
	if (!(data = conv_delta(data, dataend)))
	    return 1;

	if (data == dataend)
	    return 1; /* need something after delta */

	if (*data < 0x80)
	    return 1; /* invalid command */
	if (*data < 0xf0) {
	    data += cmdlen[((*data) >> 4) - 8];
	} else if (*data == 0xff) {
	    data += 2;
	    if (data >= dataend)
		return 1;
	    if (!(data = read_var(data, dataend, &v)))
		return 1;
	    data += v;
	} else /* sysex -> error */
	    return 1;
    }
    return (data != dataend); /* processed as many as received? */
}

/*
 * load HMP file into Allegro MIDI structure by translating the deltas
 * and adding tempo information
 */
MIDI *load_hmp(char *filename) {
    static unsigned char hmp_tempo[7] =
     {0x00, 0xff, 0x51, 0x03, 0x0f, 0x42, 0x40};
    int c;
    char buf[256];
    long data;
    CFILE *fp;
    MIDI *midi;
    int num_tracks;
    unsigned char *p;

    // try .mid first
    removeext(filename, buf);
    strcat(buf, ".mid");
    if ((midi = load_midi(buf)))
	return midi;
    if (!(fp = cfopen(filename, "rb")))
            return NULL;

    midi = malloc(sizeof(MIDI));	      /* get some memory */
    if (!midi) {
	cfclose(fp);
	return NULL;
    }

    for (c=0; c<MIDI_TRACKS; c++) {
	midi->track[c].data = NULL;
	midi->track[c].len = 0;
    }

    if ((cfread(buf, 1, 8, fp) != 8) || (memcmp(buf, "HMIMIDIP", 8)))
	goto err;

    if (cfseek(fp, 0x30, SEEK_SET))
	goto err;

    if (cfread(&num_tracks, 4, 1, fp) != 1)
	goto err;

    if ((num_tracks < 1) || (num_tracks > MIDI_TRACKS))
	goto err;

    midi->divisions = 120;

    if (cfseek(fp, 0x308, SEEK_SET))
	goto err;

    for (c=0; c<num_tracks; c++) {	      /* read each track */
	if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(&data, 4, 1, fp) != 1))
	    goto err;

	data -= 12;

	if (c == 0)  /* track 0: reserve length for tempo */
	    data += sizeof(hmp_tempo);

	midi->track[c].len = data;

	if (!(p = midi->track[c].data = malloc(data)))	/* allocate memory */
	    goto err;

	if (c == 0) { /* track 0: add tempo */
	    memcpy(p, hmp_tempo, sizeof(hmp_tempo));
	    p += sizeof(hmp_tempo);
	    data -= sizeof(hmp_tempo);
	}
					     /* finally, read track data */
	if ((cfseek(fp, 4, SEEK_CUR)) || (cfread(p, data, 1, fp) != 1))
            goto err;

	if (trans_data(p, data)) /* translate deltas hmp -> midi */
	    goto err;
   }

   cfclose(fp);
   lock_midi(midi);
   return midi;

err:
   cfclose(fp);
   destroy_midi(midi);
   return NULL;
}

char *pack_fgets(char *p, int max, PACKFILE *f) {
   int c;

   if (pack_feof(f)) {
      p[0] = 0;
      return NULL;
   }

   for (c=0; c<max-1; c++) {
      p[c] = pack_getc(f);
      if (p[c] == '\r')
         c--;
      else if (p[c] == '\n')
         break;
   }

   p[c] = 0;

   if (ferror(f))
      return NULL;
   else
      return p;
}

char *get_extension(const char *s) {
    char *p;
    if ((p = strrchr(s, '.')))
        return p+1;
    return strchr(s, 0);
}
char *get_filename(const char *s) {
    char *p;
    if ((p = strrchr(s, '\\')))
        return p+1;
    if ((p = strrchr(s, '/')))
        return p+1;
    return (char *)s;
}
long file_size(const char *n) {
        struct stat st;
        if (stat(n, &st))
                return -1;
        return st.st_size;
}
void put_backslash(char *n) {
        char *p;
        if(((p=strchr(n,0))>=p) && (*(p-1)!='/') && (*(p-1)!='\\')
         && (*(p-1)!=':')) { *(p++)='/'; *p=0;}
}

void allg_snd_init() {
    if (LOCK_VARIABLE(my_int_queue) ||
	LOCK_VARIABLE(digi_timer_rate) ||
	LOCK_VARIABLE(retrace_count) ||
	LOCK_FUNCTION(allg_snd_timer))
	Error("Error locking sound timer");
    timer_set_function(allg_snd_timer);
}

// return true if using the digmid driver
int allegro_using_digmid() {
    return get_config_int("sound", "midi_card", MIDI_AUTODETECT) == MIDI_DIGMID;
}
