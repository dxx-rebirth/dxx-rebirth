#ifndef _LINUX_JOYSTICK_H
#define _LINUX_JOYSTICK_H

/*
 *  /usr/include/linux/joystick.h  Version 1.2
 *
 *  Copyright (C) 1996-1998 Vojtech Pavlik
 */

#include <asm/types.h>
#include <linux/module.h>

/*
 * Version
 */

#define JS_VERSION		0x01020a

/*
 * Types and constants for reading from /dev/js
 */

#define JS_EVENT_BUTTON		0x01	/* button pressed/released */
#define JS_EVENT_AXIS		0x02	/* joystick moved */
#define JS_EVENT_INIT		0x80	/* initial state of device */

struct js_event {
	__u32 time;	/* event timestamp in miliseconds */
	__s16 value;	/* value */
	__u8 type;	/* event type */
	__u8 number;	/* axis/button number */
};

/*
 * IOCTL commands for joystick driver
 */

#define JSIOCGVERSION		_IOR('j', 0x01, __u32)			/* get driver version */

#define JSIOCGAXES		_IOR('j', 0x11, __u8)			/* get number of axes */
#define JSIOCGBUTTONS		_IOR('j', 0x12, __u8)			/* get number of buttons */
#define JSIOCGNAME(len)		_IOC(_IOC_READ, 'j', 0x13, len)         /* get identifier string */

#define JSIOCSCORR		_IOW('j', 0x21, struct js_corr)		/* set correction values */
#define JSIOCGCORR		_IOR('j', 0x22, struct js_corr)		/* get correction values */

/*
 * Types and constants for get/set correction
 */

#define JS_CORR_NONE		0x00	/* returns raw values */
#define JS_CORR_BROKEN		0x01	/* broken line */

struct js_corr {
	__s32 coef[8];
	__s16 prec;
	__u16 type;
};

/*
 * v0.x compatibility definitions
 */

#define JS_RETURN		sizeof(struct JS_DATA_TYPE)
#define JS_TRUE			1
#define JS_FALSE		0
#define JS_X_0			0x01
#define JS_Y_0			0x02
#define JS_X_1			0x04
#define JS_Y_1			0x08
#define JS_MAX			2

#define JS_DEF_TIMEOUT		0x1300
#define JS_DEF_CORR		0
#define JS_DEF_TIMELIMIT	10L

#define JS_SET_CAL		1
#define JS_GET_CAL		2
#define JS_SET_TIMEOUT		3
#define JS_GET_TIMEOUT		4
#define JS_SET_TIMELIMIT	5
#define JS_GET_TIMELIMIT	6
#define JS_GET_ALL		7
#define JS_SET_ALL		8

struct JS_DATA_TYPE {
	int buttons;
	int x;
	int y;
};

struct JS_DATA_SAVE_TYPE {
	int JS_TIMEOUT;
	int BUSY;
	long JS_EXPIRETIME;
	long JS_TIMELIMIT;
	struct JS_DATA_TYPE JS_SAVE;
	struct JS_DATA_TYPE JS_CORR;
};

/*
 * Internal definitions
 */

#ifdef __KERNEL__

#define JS_BUFF_SIZE		64		/* output buffer size */

#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef LINUX_VERSION_CODE
#error "You need to use at least 2.0 Linux kernel."
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,0,0)
#error "You need to use at least 2.0 Linux kernel."
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,0)
#define JS_HAS_RDTSC	(current_cpu_data.x86_capability & 0x10)
#include <linux/init.h>
#else
#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,0,35)
#define JS_HAS_RDTSC	(x86_capability & 0x10)
#else
#define JS_HAS_RDTSC	0
#endif
#else
#define JS_HAS_RDTSC	(x86_capability & 0x10)
#endif
#define __initdata
#define __init
#define MODULE_AUTHOR(x)
#define MODULE_PARM(x,y)
#define MODULE_SUPPORTED_DEVICE(x)
#define signal_pending(x) (((x)->signal) & ~((x)->blocked))
#endif

/*
 * Parport stuff
 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,0)
#define USE_PARPORT
#endif

#ifdef USE_PARPORT
#include <linux/parport.h>
#define JS_PAR_STATUS(y)	parport_read_status(y->port)
#define JS_PAR_DATA_IN(y)	parport_read_data(y->port)
#define JS_PAR_DATA_OUT(x,y)	parport_write_data(y->port, x)
#define JS_PAR_CTRL_OUT(x,y)	parport_write_control(y->port, x)
#else
#define JS_PAR_STATUS(y)	inb(y+1)
#define JS_PAR_DATA_IN(y)	inb(y)
#define JS_PAR_DATA_OUT(x,y)	outb(x,y)
#define JS_PAR_CTRL_OUT(x,y)	outb(x,y+2)
#endif

#define JS_PAR_STATUS_INVERT	(0x80)

/*
 * Internal types
 */

struct js_dev;

typedef int (*js_read_func)(void *info, int **axes, int **buttons);
typedef unsigned int (*js_time_func)(void);
typedef int (*js_delta_func)(unsigned int x, unsigned int y);
typedef int (*js_ops_func)(struct js_dev *dev);

struct js_data {
	int *axes;
	int *buttons;
};

struct js_dev {
	struct js_dev *next;
	struct js_list *list;
	struct js_port *port;
	struct wait_queue *wait;
	struct js_data cur;
	struct js_data new;
	struct js_corr *corr;
	struct js_event buff[JS_BUFF_SIZE];
	js_ops_func open;
	js_ops_func close;
	int ahead;
	int bhead;
	int tail;
	int num_axes;
	int num_buttons;
	char *name;
};

struct js_list {
	struct js_list *next;
	struct js_dev *dev;
	int tail;
	int startup;
};

struct js_port {
	struct js_port *next;
	struct js_port *prev;
	js_read_func read;
	struct js_dev **devs;
	int **axes;
	int **buttons;
	struct js_corr **corr;
	void *info;
	int ndevs;
};

/*
 * Sub-module interface
 */

extern unsigned int js_time_speed;
extern js_time_func js_get_time;
extern js_delta_func js_delta;

extern unsigned int js_time_speed_a;
extern js_time_func js_get_time_a;
extern js_delta_func js_delta_a;

extern struct js_port *js_register_port(struct js_port *port, void *info,
	int devs, int infos, js_read_func read);
extern struct js_port *js_unregister_port(struct js_port *port);

extern int js_register_device(struct js_port *port, int number, int axes,
	int buttons, char *name, js_ops_func open, js_ops_func close);
extern void js_unregister_device(struct js_dev *dev);

/*
 * Kernel interface
 */

extern int js_init(void);
extern int js_am_init(void);
extern int js_an_init(void);
extern int js_as_init(void);
extern int js_console_init(void);
extern int js_db9_init(void);
extern int js_gr_init(void);
extern int js_l4_init(void);
extern int js_lt_init(void);
extern int js_sw_init(void);
extern int js_tm_init(void);

extern void js_am_setup(char *str, int *ints);
extern void js_an_setup(char *str, int *ints);
extern void js_as_setup(char *str, int *ints);
extern void js_console_setup(char *str, int *ints);
extern void js_db9_setup(char *str, int *ints);
extern void js_l4_setup(char *str, int *ints);

#endif /* __KERNEL__ */

#endif /* _LINUX_JOYSTICK_H */
