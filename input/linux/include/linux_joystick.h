typedef struct joystick_device {
	int		device_number;
	int		version;
	int		buffer;
	char		num_buttons;
	char		num_axes;
} joystick_device;

typedef struct joystick_axis {
	int		value;
	int		min_val;
	int		center_val;
	int		max_val;
	int		joydev;
} joystick_axis;

typedef struct joystick_button {
	ubyte		state;
	ubyte		last_state;
//changed 6/24/1999 to finally squish the timedown bug - Owen Evans 
	fix		timedown;
//end changed - OE
	ubyte		downcount;
	int		num;
	int		joydev;
} joystick_button;

extern int j_num_axes;
extern int j_num_buttons;

extern joystick_device j_joystick[4];
extern joystick_axis j_axis[MAX_AXES];
extern joystick_button j_button[MAX_BUTTONS];

extern int j_Update_state ();
extern int j_Get_joydev_axis_number (int all_axis_number);
extern int j_Get_joydev_button_number (int all_button_number);

extern void joy_set_min (int axis_number, int value);
extern void joy_set_center (int axis_number, int value);
extern void joy_set_max (int axis_number, int value);
