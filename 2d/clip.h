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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Macros used for clipping
 *
 */


/*#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
*/
#define IABS(x)         (((x) < 0) ?  -(x) : (x))

#define EXCHG(a,b) do {                                                 \
	int __temp__ = (a);                                                 \
	(a) = (b);                                                          \
	(b) = __temp__;                                                     \
} while(0)

// sort two values
#define SORT2(a,b) do {                                                 \
	if((a) > (b)) EXCHG(a,b);                                           \
} while(0)

# define SCALE(var,arg,num,den)                                         \
	((var) = ((arg) * (num)) / (den))

# define USCALE(var,arg,num,den)                                        \
	((var) = ((unsigned)(arg) * (unsigned)(num)) / (unsigned)(den))

# define FIXSCALE(var,arg,num,den) ((var) = fixmuldiv((arg),(num),(den)))

#define CLIPDOT(xx,yy,WHEN_OUTSIDE,WHEN_CLIPPED) {                      \
	if((xx > XMAX) || (xx < XMIN)) { WHEN_OUTSIDE; }                    \
	if((yy > YMAX) || (yy < YMIN)) { WHEN_OUTSIDE; }                    \
}

#define CLIPHLINE(x1,x2,yy,WHEN_OUTSIDE,WHEN_CLIPPED) {                 \
	if(x1 > x2) EXCHG(x1,x2);                                           \
	if((x1 > XMAX) || (x2 < XMIN)) { WHEN_OUTSIDE; }                    \
	if((yy > YMAX) || (yy < YMIN)) { WHEN_OUTSIDE; }                    \
	if(x1 < XMIN) { x1 = XMIN; WHEN_CLIPPED; }                          \
	if(x2 > XMAX) { x2 = XMAX; WHEN_CLIPPED; }                          \
}

#define CLIPVLINE(xx,y1,y2,WHEN_OUTSIDE,WHEN_CLIPPED) {                 \
	if(y1 > y2) EXCHG(y1,y2);                                           \
	if((xx > XMAX) || (xx < XMIN)) { WHEN_OUTSIDE; }                    \
	if((y1 > YMAX) || (y2 < YMIN)) { WHEN_OUTSIDE; }                    \
	if(y1 < YMIN) { y1 = YMIN; WHEN_CLIPPED; }                          \
	if(y2 > YMAX) { y2 = YMAX; WHEN_CLIPPED; }                          \
}

#define CLIPBOX(x1,y1,x2,y2,WHEN_OUTSIDE,WHEN_CLIPPED) {                \
	if(x1 > x2) EXCHG(x1,x2);                                           \
	if(y1 > y2) EXCHG(y1,y2);                                           \
	if((x1 > XMAX) || (x2 < 0)) { WHEN_OUTSIDE; }                       \
	if((y1 > YMAX) || (y2 < 0)) { WHEN_OUTSIDE; }                       \
	if(x1 < 0) { x1 = 0; WHEN_CLIPPED; }                                \
	if(y1 < 0) { y1 = 0; WHEN_CLIPPED; }                                \
	if(x2 > XMAX) { x2 = XMAX; WHEN_CLIPPED; }                          \
	if(y2 > YMAX) { y2 = YMAX; WHEN_CLIPPED; }                          \
}

#define CLIPLINE(x1,y1,x2,y2,XMIN,YMIN,XMAX,YMAX,WHEN_OUTSIDE,WHEN_CLIPPED,MY_SCALE) do {       \
	register int temp;                                                  \
	if(y1 > y2)                                                         \
		{ EXCHG(y1,y2); EXCHG(x1,x2); }                                 \
	if((y2 < YMIN) || (y1 > YMAX))                                      \
		{ WHEN_OUTSIDE; }                                               \
	if(x1 < x2) {                                                       \
		if((x2 < XMIN) || (x1 > XMAX)) {                                \
			WHEN_OUTSIDE;                                               \
		}                                                               \
		if(x1 < XMIN) {                                                 \
			MY_SCALE(temp,(y2 - y1),(XMIN - x1),(x2 - x1)); \
			if((y1 += temp) > YMAX) { WHEN_OUTSIDE; }                   \
			x1 = XMIN;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(x2 > XMAX) {                                                 \
			MY_SCALE(temp,(y2 - y1),(x2 - XMAX),(x2 - x1)); \
			if((y2 -= temp) < YMIN) { WHEN_OUTSIDE; }                   \
			x2 = XMAX;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(y1 < YMIN) {                                                 \
			MY_SCALE(temp,(x2 - x1),(YMIN - y1),(y2 - y1)); \
			x1 += temp;                                                 \
			y1 = YMIN;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(y2 > YMAX) {                                                 \
			MY_SCALE(temp,(x2 - x1),(y2 - YMAX),(y2 - y1)); \
			x2 -= temp;                                                 \
			y2 = YMAX;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
	}                                                                   \
	else {                                                              \
		if((x1 < XMIN) || (x2 > XMAX)) {                                \
			WHEN_OUTSIDE;                                               \
		}                                                               \
		if(x1 > XMAX) {                                                 \
			MY_SCALE(temp,(y2 - y1),(x1 - XMAX),(x1 - x2)); \
			if((y1 += temp) > YMAX) { WHEN_OUTSIDE; }                   \
			x1 = XMAX;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(x2 < XMIN) {                                                 \
			MY_SCALE(temp,(y2 - y1),(XMIN - x2),(x1 - x2)); \
			if((y2 -= temp) < YMIN) { WHEN_OUTSIDE; }                   \
			x2 = XMIN;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(y1 < YMIN) {                                                 \
			MY_SCALE(temp,(x1 - x2),(YMIN - y1),(y2 - y1)); \
			x1 -= temp;                                                 \
			y1 = YMIN;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
		if(y2 > YMAX) {                                                 \
			MY_SCALE(temp,(x1 - x2),(y2 - YMAX),(y2 - y1)); \
			x2 += temp;                                                 \
			y2 = YMAX;                                                  \
			WHEN_CLIPPED;                                               \
		}                                                               \
	}                                                                   \
} while(0)
