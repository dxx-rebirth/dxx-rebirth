/// tmap_scanline_per - Pentium-Pro-optimized assembly version
/// written by Brian Raiter, Mar 1998.
/// lighting roundoff error fixed by Matt Mueller, July 1999

/// The gist of the algorithm is as follows (note that this is
/// pseudocode, not actual C):
///
/// int  u = fx_u;
/// int  v = fx_v;
/// int  z = fx_z;
/// int  l = fx_l;
/// int  x, ubyz, vbyz;
/// byte texmap[64][64] = pixptr;
/// byte framebuffer[][bytes_per_row] = write_buffer;
/// byte lightingtable[][256] = gr_fade_table;
/// byte c;
///
/// for (x = fx_xleft ; x <= fx_xright ; ++x) {
///	ubyz = (u / z) & 63;
///	vbyz = (v / z) & 63;
///	c = texmap[ubyz][vbyz];
///	if (c != TRANSPARENT_COLOR)
///	    framebuffer[fx_y][x] = lightingtable[l / 65536][c];
///	u += fx_du_dx;
///	v += fx_dv_dx;
///	z += fx_dz_dx;
///	l += fx_dl_dx;
/// }
///
/// The global variable Transparency_on is zero when it is known that
/// there are no transparencies involved, so in that case we use a
/// different loop that skips the transparency test.
///
/// The actual algorithm used here only does the division calculations
/// every fourth pixel, and linearly interpolates the other three.
/// Something along the lines of:
///
/// /* Initial values as before */
/// int ubyz0, ubyz0, ubyz4, ubyz4, du1, dv1, i;
///
/// ubyz0 = u / z;
/// vbyz0 = v / z;
/// for (x = fx_xleft ; x <= fx_xright - 3 ; x += 4) {
///	u += fx_du_dx * 4;
///	v += fx_dv_dx * 4;
///	z += fx_dz_dx * 4;
///	ubyz4 = u / z;
///	vbyz4 = v / z;
///	du1 = (ubyz4 - ubyz0) / 4;
///	dv1 = (vbyz4 - vbyz0) / 4;
///	ubyz = ubyz0;
///	vbyz = vbyz0;
///	for (i = 0 ; i < 4 ; ++i) {
///	    c = texmap[ubyz & 63][vbyz & 63];
///	    if (c != TRANSPARENT_COLOR)
///		framebuffer[fx_y][x + i] = lightingtable[l / 65536][c];
///	    ubyz += du1;
///	    vbyz += dv1;
///	    l += fx_dl_dx;
///	}
///	ubyz0 = ubyz4;
///	vbyz0 = vbyz4;
/// }
/// for ( ; x <= fx_xright ; ++x) {
///	/* Finish off remaining 0-3 pixels */
/// }
///
/// So much for the basic overview.
///
/// In this version, the PPro's floating-point unit is pressed into
/// service to do the actual divisions, so that 1/z can be calculated
/// first, and the resulting reciprocal multiplied with u and v. These
/// two products are then stored back out as integers. This keeps us
/// down to doing only one division every four pixels, during which
/// other integer instructions can be overlapped. 
///
/// The algorithm actually divides 64 by z, so that the rounded-off
/// products will effectively be stored with six fraction bits. This
/// allows the algorithm to correct for minor floating-point roundoff
/// errors. Two fraction bits are kept during the interpolation of the
/// three middle pixels, which hopefully increases the accuracy of the
/// approximations.
///
/// We only need the lowest six (integral) bits of u/z and v/z for
/// each pixptr offset, so we only need eight bits of each fourth
/// pair of values to figure the interpolation. Add with the two
/// fractional bits we keep for extra precision flavor, this makes ten
/// bits for each value, or twenty to store the full pair. To simplify
/// the interpolation, the pair is packed into a single 32-bit
/// register like so:
///
///		3      2       1
///		1      4       6       8       0
///		vvVVVVVVvv____________uuUUUUUUuu
///		  \v&63/                \u&63/
///
/// The unused bits between the u and v values permit the packed
/// values to be added/subtracted without the u values spilling over
/// into the v values. Then, the instructions "bswap %eax ; roll $6,
/// %eax ; andl $0x0FFF, %eax" will right-justify the desired values
/// into a pixptr offset.
///
/// The FP stack is loaded up with the values of u, v, and z,
/// converted to floats. %ebp is used to hold the value of l, %esi is
/// is set to pixptr, and %edi points to our current position in
/// write_buffer.

#include <conf.h>

// This is used to abbreviate an annoying external variable name.

.equ    fadetbl, _gr_fade_table


// The following macro encapsulates the floating-point instructions
// that put the results of a prior division to use and prepare for the
// next division. At the beginning of the macro, the FP stack contains
// (from top to bottom): 64/z, z, u, v. The macro computes (64*u)/z,
// which is stored in ubyz4, and (64*v)/z, which is stored in vybz4.
// Simultaneous with this, the macro adds dudx to u, dvdx to v, and
// dzdx to z, and finally puts 64 back onto the stack. At the end of
// the macro, the stack contains: 64, z, u, v.

.macro DoFPCalcs 0		// The FP stack after each instruction:
				//		 64/z  z    u    v
	fst	%st(4)		//		 64/z  z    u    v  64/z
	fxch	%st(2)		//		   u   z  64/z   v  64/z
	fmul	%st, %st(4)	// (64 * u) / z	   u   z  64/z   v   u/z
	fadds	(dudx)		// u += dudx	   u'  z  64/z   v   u/z
	fxch	%st(3)		//		   v   z  64/z   u'  u/z
	fmul	%st, %st(2)	// (64 * v) / z     v   z   v/z   u'  u/z
	fadds	(dvdx)		// v += dvdx	   v'  z   v/z   u'  u/z
	fxch	%st(1)		//		   z   v'  v/z   u'  u/z
	fadds	(dzdx)		// z += dzdx	   z'  v'  v/z   u'  u/z
	fxch	%st(2)		//		  v/z  v'   z'   u'  u/z
	flds	(flt64)		//		   64 v/z   v'   z'   u'   u/z
	fxch	%st(5)		//		  u/z v/z   v'   z'   u'    64
	fistpl	(ubyz4)		//		  v/z  v'   z'   u'   64
	fistpl	(vbyz4)		//		   v'  z'   u'   64
	fxch	%st(3)		//		   64  z'   u'   v'
				// (ready to start the next division)
.endm


#ifdef __ENV_LINUX__
.equ _pixptr, pixptr
.equ _gr_fade_table, gr_fade_table
.equ _write_buffer, write_buffer
.equ _bytes_per_row, bytes_per_row
.equ _fx_xleft, fx_xleft
.equ _fx_xright, fx_xright
.equ _fx_y, fx_y
.equ _fx_u, fx_u
.equ _fx_v, fx_v
.equ _fx_z, fx_z
.equ _fx_l, fx_l
.equ _fx_du_dx, fx_du_dx
.equ _fx_dv_dx, fx_dv_dx
.equ _fx_dz_dx, fx_dz_dx
.equ _fx_dl_dx, fx_dl_dx
.equ _Transparency_on, Transparency_on

.globl asm_tmap_scanline_per
#else
.globl _asm_tmap_scanline_per
#endif

.extern _pixptr, _gr_fade_table, _write_buffer
.extern _bytes_per_row, _fx_xleft, _fx_xright, _fx_y
.extern _fx_u, _fx_v, _fx_z, _fx_l
.extern _fx_du_dx, _fx_dv_dx, _fx_dz_dx, _fx_dl_dx
.extern _Transparency_on

//.local  dudx, dvdx, dzdx, dldx, l
//.local  ubyz, vbyz, uvzero
//.local  lastquartet, lastpixel, ctwl
//.local  flt64

.data

.balign 4

dudx:		.long	0		// u's rate of change as a float
dvdx:		.long	0		// v's rate of change as a float
dzdx:		.long	0		// z's rate of change as a float
dldx:		.long	0		// l's rate of change as an integer
l:		.long	0		// the current l value
ubyz4:		.long	0		// u/z for the next iteration
vbyz4:		.long	0		// v/z for the next iteration
uvzero:		.long	0		// packed u/z and v/z values
lastquartet:	.long	0		// where to stop the 4-pixels loop
lastpixel:	.long	0		// where to stop drawing entirely
flt64:		.long	0x42800000	// 64.0 (what we divide z into)
ctlwd:		.long	0		// the pre-tweaked FPU control word


.text

.balign 4

//
// void c_tmap_scanline_per(void)
//

#ifdef __ENV_LINUX__
asm_tmap_scanline_per:
#else
_asm_tmap_scanline_per:
#endif

// Save registers the compiler might be using.

		pushl	%ebp
		pushl	%edi
		pushl	%esi

// Kick the FPU into the lowest precision (still enough for our needs)
// so as to speed up fdiv.

		fnstcw	(ctlwd)
		movw	(ctlwd), %ax
		movl	%eax, %ebx
		andb	$0xFC, %bh
		movw	%bx, (ctlwd)
		fldcw	(ctlwd)
		movw	%ax, (ctlwd)

// Multiply dudx, dvdx, and dzdx by four, and store locally, converted
// into floating point.

                movl    (_fx_du_dx), %eax
		sall	$2, %eax
		movl	%eax, (dudx)
                movl    (_fx_dv_dx), %eax
		sall	$2, %eax
		movl	%eax, (dvdx)
                movl    (_fx_dz_dx), %eax
		sall	$2, %eax
		movl	%eax, (dzdx)
		fildl	(dudx)
		fildl	(dvdx)
		fildl	(dzdx)
		fxch	%st(2)
		fstps	(dudx)
		fstps	(dvdx)
		fstps	(dzdx)

// bytes_per_row * fx_y is the offset for the current scanline. (We do
// this now before we start the first FP division.)

                movl    (_bytes_per_row), %eax
		xorl	%edx, %edx
                mull    (_fx_y)

// Push v, u, z, and 64.0 onto the FPU stack, and then start
// calculating the first 64 / z.

                fildl   (_fx_v)
                fildl   (_fx_u)
                fildl   (_fx_z)
		flds	(flt64)
		fdiv	%st(1)

// Meanwhile, get l and dldx (again, the latter multiplied by four).
// l will be stored in %ebp for the duration. The original values are
// divided by 256 so that the byte needed for the fade table offset
// will be aligned.

//Dividing by 256 is bad.. rounding errors and crap.  We'll now do that
//right before we need to access the table instead.  -MM

                movl    (_fx_l), %edx
//		sarl	$8, %edx
		movl	%edx, (l)
                movl    (_fx_dl_dx), %edx
//		sarl	$6, %edx
		sall	$2, %edx
		movl	%edx, (dldx)

// Store pixptr, the pointer to our 64x64 texture map, in %esi. Store
// write_buffer, the pointer to our frame buffer, in %edi. Then offset
// %edi so that it points to pixel [fx_y][fx_xleft]. Calculate a
// pointer to [fx_y][fx_xright + 1] so we know when to stop drawing.
// Also calculate a pointer to [fx_y][(fx_xright + 1) & ~3] so we know
// when to stop drawing four pixels at a time.

                movl    (_pixptr), %esi
                movl    (_write_buffer), %edi
                movl    (_fx_xright), %ecx
		addl	%eax, %edi
		incl	%ecx
		addl	%edi, %ecx
		movl	%ecx, (lastpixel)
                addl    (_fx_xleft), %edi
		movl	%ecx, %eax
		subl	%edi, %eax
		jle	LeaveNow
		andl	$3, %eax
		subl	%eax, %ecx
		movl	%ecx, (lastquartet)

// Calculate round(64 * u / z) and round(64 * v / z), store, and
// increment u, v, and z. Then start calculating the second 64 / z.

		DoFPCalcs
		fdiv	%st(1)

// Get our u/z and v/z values, lop off the bits we don't care
// about, pack, and store in uvzero.

		movl	(ubyz4), %eax
		incl	%eax
		andl	$0x3FF0, %eax
		shrl	$4, %eax
		movl	(vbyz4), %ebx
		incl	%ebx
		andl	$0x3FF0, %ebx
		shll	$18, %ebx
		orl	%eax, %ebx
		movl	%ebx, (uvzero)

// Are there at least four pixels to draw? If not, skip to the epilog
// code.

		cmpl	%ecx, %edi
		je	LastBits

// Do we need to test for transparencies?

                testl   $(~0), (_Transparency_on)
		jnz	LoopTransOn

// If not, then use the simpler loop here.

.balign 4

LoopTransOff:

// While the FPU is busy dividing, the latest u/z and v/z values are
// retrieved, packed, and stored in uvzero (to be used again in the
// next iteration). The old uvzero value, which contains the uv values
// for pixel 0, gets subtracted from the new uvzero value to
// determined the total change in u/z and v/z across the four pixels,
// and this is divided by 4 to get the average. This average is then
// used to estimate the values for pixels 1, $2, and 3. The old uvzero
// value is used immediately to calculate pixel 0, while %eax, %ebx, and
// %ecx are entrusted with the uv values for pixels 1, $2, and 3
// respectively, while %edx is our "cleansed" register for using byte
// values as memory pointer offsets. %ebp is loaded with the high byte
// of l, forming half of the offset for the fade table lookup. (The
// pixel from the texture-map bitmap supplies the other half.) Each
// value is used to set its pixel as follows (assuming %eax holds our
// packed uv value):
//
//	a:	bswapl	%eax				/ move u and v to the
//	b:	roll	$6, %eax			/   far right
//	c:	andl	$0x0FFF, %eax			/ mask off extra bits
//	d:	movb	(%esi,%eax), %dl		/ get texture-map pixel
//	e:	movb	fadetbl(%edx,%ebp), %dl		/ correct for lighting
//	f:	movb	%dl, (%edi)			/ write to frame buffer
//
// The above is done four times, once for each pixel. Some of the
// calculations may appear to be interleaved haphazardly, but the PPro
// seems to like it this way.

		DoFPCalcs
		fdiv	%st(1)

		xorl	%edx, %edx
                movl    (uvzero), %eax                  // %eax = uv for pixel 0
                bswapl  %eax                            // 0 a
                roll    $6, %eax                        // 0 b
                andl    $0x0FFF, %eax                   // 0 c
                movb    (%esi,%eax), %dl                // 0 d
		movl	(l), %ebp
		movl	(dldx), %ecx
		addl	%ebp, %ecx
		movl	%ecx, (l)
		sarl	$8, %ebp
		andl	$0x7F00, %ebp
        movb    fadetbl(%edx,%ebp), %dl         // 0 e

		movl	(vbyz4), %ebx
		incl	%ebx
		andl	$0x3FF0, %ebx
		movl	(ubyz4), %ecx
		shll	$18, %ebx
		incl	%ecx
		andl	$0x3FF0, %ecx
		shrl	$4, %ecx
		movl	(uvzero), %eax
		orl	%ebx, %ecx
		movl	%ecx, (uvzero)
		orl	$0x1000, %ecx
		subl	%eax, %ecx
		shrl	$2, %ecx

                movb    %dl, (%edi)                     // 0 f
                lea     (%eax,%ecx,2), %ebx             // %ebx = uv for pixel 2
                addl    %ecx, %eax                      // %eax = uv for pixel 1
                bswapl  %eax                            // 1 a
                roll    $6, %eax                        // 1 b
                addl    %ebx, %ecx                      // %ecx = uv for pixel 3
                bswapl  %ebx                            // 2 a
                roll    $6, %ebx                        // 2 b
                bswapl  %ecx                            // 3 a
                andl    $0x0FFF, %eax                   // 1 c
                andl    $0x0FFF, %ebx                   // 2 c
                roll    $6, %ecx                        // 3 b

                movb    (%esi,%eax), %dl                // 1 d
                movb    fadetbl(%edx,%ebp), %al         // 1 e
                movb    (%esi,%ebx), %dl                // 2 d
                movb    fadetbl(%edx,%ebp), %bl         // 2 e
                movb    %al, 1(%edi)                    // 1 f
                andl    $0x0FFF, %ecx                   // 3 c
                movb    %bl, 2(%edi)                    // 2 f
                movb    (%esi,%ecx), %dl                // 3 d
                movb    fadetbl(%edx,%ebp), %cl         // 3 e
                movb    %cl, 3(%edi)                    // 3 f

		addl	$4, %edi
		cmpl	(lastquartet), %edi
		jl	LoopTransOff

// Are there any pixels left at all?

		cmpl	(lastpixel), %edi
		jne	LastBits
		jmp	LeaveNow


.balign 4

LoopTransOn:

// This is similar to the LoopTransOff loop, the big change being that
// each value retrieved from the texture map is tested against 255,
// the transparent "color". A value of 255 in the texture map means to
// let the existing value for that pixel in write_buffer go by
// unchanged. Thus the code for each pixel looks something like this
// instead:
//
//	a:	bswapl	%eax				/ move u and v to the
//	b:	roll	$6, %eax			/   far right
//	c:	andl	$0x0FFF, %eax			/ mask off extra bits
//	d:	movb	(%esi,%eax), %dl		/ get texture-map pixel
//	e:	cmpb	$255, %dl			/ is pixel transparent?
//	f:	sbbb	%ah, %ah			/ yes:%ah=00, no:%ah=FF
//	g:	movb	fadetbl(%edx,%ebp), %dl		/ correct for lighting
//	h:	movb	(%edi), %al			/ get current pixel
//	i:	xorb	%al, %dl			/ combine the two
//	j:	andb	%dl, %ah			/ use %ah as a mask to
//	k:	xorb	%ah, %al			/   select which pixel
//	l:	movb	%al, (%edi)			/ write to frame buffer
// 
// When the texture-map value is 255, the code simply writes the
// original frame-buffer value back out again; otherwise the new pixel
// is written instead. The ands and xors used to accomplish this bulk
// up the code, but on the whole it is better than having four
// unpredictable jumps in the loop.

		DoFPCalcs
		fdiv	%st(1)

                movl    (uvzero), %eax                  // %eax = uv for pixel 0
                bswapl  %eax                            // 0 a
		movl	(dldx), %ecx
		movl	(l), %ebp
                addl    %ebp, %ecx                      
                roll    $6, %eax                        // 0 b
                andl    $0x0FFF, %eax                   // 0 c
		xorl	%edx, %edx
                movb    (%esi,%eax), %dl                // 0 d
                cmpb    $255, %dl                       // 0 e
                sbbb    %ah, %ah                        // 0 f
		movl	%ecx, (l)
		sarl	$8, %ebp
		andl	$0x7F00, %ebp

                movb    fadetbl(%edx,%ebp), %dl         // 0 g
                movb    (%edi), %al                     // 0 h
                xorb    %al, %dl                        // 0 i
                andb    %dl, %ah                        // 0 j
                xorb    %ah, %al                        // 0 k
                movb    %al, (%edi)                     // 0 l

		movl	(vbyz4), %ebx
		movl	(ubyz4), %ecx
		incl	%ebx
		andl	$0x3FF0, %ebx
		incl	%ecx
		andl	$0x3FF0, %ecx
		shll	$18, %ebx
		shrl	$4, %ecx
		orl	%ebx, %ecx
		movl	(uvzero), %eax
		movl	%ecx, (uvzero)
		orl	$0x1000, %ecx
		subl	%eax, %ecx
		shrl	$2, %ecx

                lea     (%eax,%ecx,2), %ebx             // %ebx = uv for pixel 2
                addl    %ecx, %eax                      // %eax = uv for pixel 1
                bswapl  %eax                            // 1 a
                roll    $6, %eax                        // 1 b
                addl    %ebx, %ecx                      // %ecx = uv for pixel 3
                bswapl  %ebx                            // 2 a
                roll    $6, %ebx                        // 2 b
                andl    $0x0FFF, %eax                   // 1 c
                movb    (%esi,%eax), %dl                // 1 d
                cmpb    $255, %dl                       // 1 e
                sbbb    %ah, %ah                        // 1 f
                bswapl  %ecx                            // 3 a
                movb    1(%edi), %al                    // 1 h
                movb    fadetbl(%edx,%ebp), %dl         // 1 g

                roll    $6, %ecx                        // 3 b
                andl    $0x0FFF, %ebx                   // 2 c
                xorb    %al, %dl                        // 1 i
                andb    %dl, %ah                        // 1 j
                movb    (%esi,%ebx), %dl                // 2 d
                cmpb    $255, %dl                       // 2 e
                sbbb    %bh, %bh                        // 2 f
                movb    fadetbl(%edx,%ebp), %dl         // 2 g
                andl    $0x0FFF, %ecx                   // 3 c
                movb    2(%edi), %bl                    // 2 h
                xorb    %bl, %dl                        // 2 i
                andb    %dl, %bh                        // 2 j
                                                        
                movb    (%esi,%ecx), %dl                // 3 d
                cmpb    $255, %dl                       // 3 e
                sbbb    %ch, %ch                        // 3 f
                movb    3(%edi), %cl                    // 3 h
                movb    fadetbl(%edx,%ebp), %dl         // 3 g
                xorb    %cl, %dl                        // 3 i
                andb    %dl, %ch                        // 3 j

                xorb    %ah, %al                        // 1 k
                movb    %al, 1(%edi)                    // 1 l
                xorb    %bh, %bl                        // 2 k
                movb    %bl, 2(%edi)                    // 2 l
                xorb    %ch, %cl                        // 3 k
                movb    %cl, 3(%edi)                    // 3 l

		addl	$4, %edi
		cmpl	(lastquartet), %edi
		jl	LoopTransOn

// Quit if there are none at all left.

		cmpl	(lastpixel), %edi
		je	LeaveNow


LastBits:

// Here we finish off the last one-to-three pixels assigned to us.
// Rather than calculating values for all four pixels, we just divide
// the difference by four and keep adding this average into the value
// as needed. (This code is not particularly optimized, by the way,
// since it represents such a miniscule amount of the running time.)

		DoFPCalcs
		movl	(l), %ebp
		sarl	$8, %ebp
		andl	$0x7F00, %ebp
		movl	(ubyz4), %eax
		incl	%eax
		andl	$0x3FF0, %eax
		shrl	$4, %eax
		movl	(vbyz4), %ecx
		incl	%ecx
		andl	$0x3FF0, %ecx
		shll	$18, %ecx
		orl	%eax, %ecx
		movl	(uvzero), %ebx
		orl	$0x1000, %ecx
		subl	%ebx, %ecx
		shrl	$2, %ecx
		xorl	%edx, %edx

LoopLastBits:	movl	%ebx, %eax
		bswapl	%eax
		roll	$6, %eax
		andl	$0x0FFF, %eax
		movb	(%esi,%eax), %dl
		cmpb	$255, %dl
		je	LetPixelBy
		movb	fadetbl(%edx,%ebp), %dl
		movb	%dl, (%edi)
LetPixelBy:	incl	%edi
		addl	%ecx, %ebx
		cmpl	(lastpixel), %edi
		jl	LoopLastBits


LeaveNow:

// We're done! Clear the stacks, reset the FPU control word, and we
// are so out of here.

		popl	%esi
		popl	%edi
		popl	%ebp
		fcompp
		fcompp
		fldcw	(ctlwd)
		ret
