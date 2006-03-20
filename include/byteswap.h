//byteswap.h - added 03/04/99 Matt Mueller

#ifdef BIGENDIAN
//untested, borrowed from ggi-descent
#define swapshort(x) ( (x << 8) | (((ushort)x) >> 8) )
#define swapint(x)  ( (x << 24) | (((/*ulong*/uint)x) >> 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) )
#else
#define swapint(x) x
#define swapshort(x) x
#endif
