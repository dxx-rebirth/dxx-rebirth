/// $Id: tmappent.S,v 1.4 2003-02-18 20:15:48 btb Exp $
/// tmap_scanline_per - Pentium-optimized assembly version
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
/// In this version, the Pentium's floating-point unit is pressed into
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
/// each pixptr offset, so we only need eight bits of each fourth pair
/// of values to figure the interpolation. Add with the two fractional
/// bits we keep for extra precision flavor, this makes ten bits for
/// each value, or twenty to store the full pair. To simplify the
/// interpolation, the pair is packed into a single 32-bit register
/// like so:
///
///		3      2       1
///		1      4       6       8       0
///		________vvVVVVVVvv____uuUUUUUUuu
///		          \v&63/        \u&63/
///
/// The unused bits between the u and v values permit the packed
/// values to be added/subtracted without the u values spilling over
/// into the v values. Then, after anding out the carry/borrow bits,
/// the instructions "movb %al, %ah ; shrl $10, %eax" nicely
/// right-justify the desired values into a pixptr offset.
/// 
/// The FP stack is loaded up with the values of u, v, and z,
/// converted to floats. %ebp is used to hold the value of l, %esi is
/// set to pixptr, and %edi points to our current position in
/// write_buffer.



// This is used to abbreviate an annoying external variable name.

.equ    fadetbl, _gr_fade_table


// The following macro encapsulates the floating-point instructions
// that put the results of a prior division to use and prepare for the
// next division. At the beginning of the macro, the FP stack contains
// (from top to bottom): z, u, v, 64/z. The macro computes (64*u)/z,
// which is stored in ubyz4, and (64*v)/z, which is stored in vbyz4.
// The number (2^51 + 2^52) is added to each number before they are
// stored as qwords. Since qwords only have 52 bits of precision, this
// magic number causes the fractional part to be shifted off the end,
// leaving the integral part right-shifted. Thus, reading the low
// dword gives the original number rounded off to the nearest integer
// - in two's complement, no less. (This technique allows for more
// pipelining than using the more straightforward fist/p
// instruction.) Simultaneous with this, the macro adds dudx to u,
// dvdx to v, and dzdx to z, and finally puts 64 back onto the stack.
// At the end of the macro, the stack contains: z, u, v, 64.

.macro DoFPCalcs		// The FP stack after each instruction:
				//                 z    u    v  64/z
	fadds	(dzdx)		// z += dzdx	  z'   u    v  64/z
	fxch	%st(1)		//		  u    z'   v  64/z
	fst	%st(4)		//		  u    z'   v  64/z   u
	fmul	%st(3)		// (64 / z) * u	 u/z   z'   v  64/z   u
	fxch	%st(4)		//		  u    z'   v  64/z  u/z
	fadds	(dudx)		// u += dudx	  u'   z'   v  64/z   u
	fxch	%st(2)		//		  v    z'   u' 64/z  u/z
	fmul	%st, %st(3)	// (64 / z) * v	  v    z'   u'  v/z  u/z
	fxch	%st(4)		//		 u/z   z'   u'  v/z   v
	fadds	(magic)		//		 U/Z   z'   u'  v/z   v
	fxch	%st(4)		//		  v    z'   u'  v/z  U/Z
	fadds	(dvdx)		// v += dvdx	  v'   z'   u'  v/z  U/Z
	fxch	%st(3)		//		 v/z   z'   u'   v'  U/Z
	fadds	(magic)		//		 V/Z   z'   u'   v'  U/Z
	flds	(flt64)		//		  64  V/Z   z'   u'   v'  U/Z
	fxch	%st(5)		//		 U/Z  V/Z   z'   u'   v'   64
	fstpl	(ubyz4)		//		 V/Z   z'   u'   v'   64
	fstpl	(vbyz4)		//		  z'   u'   v'   64
				// (ready to start the next division)
.endm

#ifdef __linux__
.equ _pixptr, pixptr
.equ _gr_fade_table, gr_fade_table
.equ _write_buffer, write_buffer
.equ _bytes_per_row,bytes_per_row
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

.globl asm_pent_tmap_scanline_per
#else
.globl _asm_pent_tmap_scanline_per
#endif

.extern _pixptr, _gr_fade_table, _write_buffer
.extern _bytes_per_row, _fx_xleft, _fx_xright, _fx_y
.extern _fx_u, _fx_v, _fx_z, _fx_l
.extern _fx_du_dx, _fx_dv_dx, _fx_dz_dx, _fx_dl_dx
.extern _Transparency_on




//.local  dudx, dvdx, dzdx, dldx
//.local  ubyz4, vbyz4, uvzero
//.local  lastquartet, lastpixel, ctlwd
//.local  flt64, magic


.data

.balign 8

lastquartet:	.long	0		// where to stop the 4-pixels loop
lastpixel:	.long	0		// where to stop drawing entirely
flt64:		.long	0x42800000	// 64.0 (what we divide z into)
magic:		.long	0x59C00000	// 2^51 + 2^52 (to get ints from floats)
ubyz4:		.double	0.0		// u/z for the next iteration
vbyz4:		.double	0.0		// v/z for the next iteration
dudx:		.long	0		// u's rate of change as a float
dvdx:		.long	0		// v's rate of change as a float
dzdx:		.long	0		// z's rate of change as a float
dldx:		.long	0		// l's rate of change as an integer
uvzero:		.long	0		// packed u/z and v/z values
ctlwd:		.word	0		// the pre-tweaked FPU control word


.text

.balign 4

//
// void c_tmap_scanline_per(void)
//

#ifdef __linux__
asm_pent_tmap_scanline_per:
#else
_asm_pent_tmap_scanline_per:
#endif

// Save registers the compiler might be using.

		pushl	%ebp
		pushl	%edi
		pushl	%esi

// Tell the FPU to use 64-bit numbers (still plenty precise enough for
// our needs) so as to speed up fdiv.

		fnstcw	(ctlwd)
		movw	(ctlwd), %ax
		movl	%eax, %ebx
		andb	$0xFC, %bh
		orb	$0x02, %bh
		movw	%bx, (ctlwd)
		fldcw	(ctlwd)
		movw	%ax, (ctlwd)

// Multiply dudx, dvdx, and dzdx by four, and store locally, converted
// into floating point.

                movl    (_fx_du_dx), %ebx
                movl    (_fx_dv_dx), %ecx
		sall	$2, %ebx
                movl    (_fx_dz_dx), %edx
		sall	$2, %ecx
		movl	%ebx, (dudx)
		sall	$2, %edx
		movl	%ecx, (dvdx)
		movl	%edx, (dzdx)
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

// Push 64.0, v, u, and z onto the FPU stack, and then start
// calculating the first 64 / z.

		flds	(flt64)
                fildl   (_fx_v)
                fildl   (_fx_u)
                fildl   (_fx_z)
		fdivr	%st, %st(3)

// Meanwhile, get l and dldx (again, the latter multiplied by four)
// into %edx and %ebp, where they will be stored for the duration. The
// original values are divided by 256 so that the byte needed for the
// fade table offset is squarely in %dh.

//Dividing by 256 is bad.. rounding errors and crap.  We'll now do that
//right before we need to access the table instead.  -MM

                movl    (_fx_l), %ebp
//		sarl	$8, %ebp
                movl    (_fx_dl_dx), %edx
//		sarl	$6, %edx
		sall	$2, %edx
		movl	%edx, (dldx)

// Store pixptr, the pointer to our 64x64 texture map, in %esi. Store
// write_buffer, the pointer to our frame buffer, in %edi. Then offset
// %edi so that it points to pixel (fx_y)(fx_xleft). Calculate a
// pointer to (fx_y)[fx_xright + 1] so we know when to stop drawing.
// Also calculate a pointer to (fx_y)[(fx_xright + 1) & ~3] so we know
// when to stop drawing four pixels at a time.

                movl    (_pixptr), %esi
                movl    (_write_buffer), %edi
                movl    (_fx_xright), %ecx
		addl	%eax, %edi
		incl	%ecx
		addl	%edi, %ecx
                addl    (_fx_xleft), %edi
		movl	%ecx, %eax
		subl	%edi, %eax
		jle	LeaveNow
		andl	$3, %eax
		movl	%ecx, (lastpixel)
		subl	%eax, %ecx
		movl	%ecx, (lastquartet)

// Calculate round(64 * u / z) and round(64 * v / z), store, and
// increment u, v, and z. Then start calculating the second 64 / z.

		DoFPCalcs
		fdivr	%st, %st(3)

// Get our u/z and v/z values, lop off the bits we don't care about,
// pack, and store in uvzero.

		movl	(ubyz4), %eax
		movl	(vbyz4), %ebx
		incl	%eax
		incl	%ebx
		andl	$0x3FF0, %eax
		andl	$0x3FF0, %ebx
		shrl	$4, %eax
		shll	$10, %ebx
		orl	%eax, %ebx
		movl	%ebx, (uvzero)

// While we're waiting for the last division to finish, we might as
// well get the frame buffer into the cache.

		cmpb	(%edi), %al

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
// used to estimate the values for pixels 1, 2, and 3. The old uvzero
// value is used immediately to calculate pixel 0, while %eax, %ebx, and
// %ecx are entrusted with the uv values for pixels 1, 2, and 3
// respectively. %edx is set to the current value of l, such that %dh is
// already set as half of the offset into fadetbl. Each uv value is
// used to set its pixel as follows (assuming our packed uv value is
// in %ebx):
//
//	a:	andl	$0x003F00FC, %ebx	/ mask off extraneous bits
//	b:	movb	%bl, %bh		/ make u flush with v
//	c:	shrl	$10, %ebx		/ right-justify u and v
//	d:	movb	(%esi,%ebx), %dl	/ get texture-map pixel
//	e:	movb	fadetbl(%edx), %bl	/ correct for lighting level
//	f:	movb	%bl, (%edi)		/ write pixel to frame buffer
// 
// The above is done four times, once for each pixel. All of the
// calculcations are interleaved in order to avoid AGI stalls and
// missed pairing opportunities.

		DoFPCalcs
		fdivr	%st, %st(3)
		movl	(ubyz4), %ebx
		movl	(vbyz4), %edx
		incl	%ebx
		incl	%edx
		shrl	$4, %ebx
		andl	$0x3FF0, %edx
		shll	$10, %edx
		andl	$0x03FF, %ebx
                movl    (uvzero), %ecx          // %ecx = uv value for pixel 0
		orl	%edx, %ebx
		movl	%ecx, %eax
		movl	%ebx, (uvzero)
                andl    $0x003F00FC, %ecx       // 0 a
		orl	$0x1000, %ebx
                movb    %cl, %ch                // 0 b
		subl	%eax, %ebx
                shrl    $10, %ecx               // 0 c
		movl	$0x7F0000, %edx
		shrl	$2, %ebx
		andl	%ebp, %edx
		sarl	$8, %edx
                movb    (%esi,%ecx), %dl        // 0 d
		addl	$4, %edi
                lea     (%eax,%ebx,2), %ecx     // %ecx = uv value for pixel 2
                addl    %ebx, %eax              // %eax = uv value for pixel 1
                addl    %ecx, %ebx              // %ebx = uv value for pixel 3
                andl    $0x003F00FC, %ecx       // 2 a
                movb    %cl, %ch                // 2 b
                movb    fadetbl(%edx), %dl      // 0 e
                shrl    $10, %ecx               // 2 c
                andl    $0x003F00FC, %eax       // 1 a
                movb    %dl, -4(%edi)           // 0 f
                movb    %al, %ah                // 1 b
                movb    (%esi,%ecx), %dl        // 2 d
                andl    $0x003F00FC, %ebx       // 3 a
                shrl    $10, %eax               // 1 c
                movb    %bl, %bh                // 3 b
                movb    fadetbl(%edx), %cl      // 2 e
                movb    (%esi,%eax), %dl        // 1 d
                shrl    $10, %ebx               // 3 c
                movb    %cl, -2(%edi)           // 2 f
		movl	(dldx), %ecx
                movb    fadetbl(%edx), %al      // 1 e
                movb    (%esi,%ebx), %dl        // 3 d
                movb    %al, -3(%edi)           // 1 f
		addl	%ecx, %ebp
                movb    fadetbl(%edx), %bl      // 3 e
		movl	(lastquartet), %ecx
                movb    %bl, -1(%edi)           // 3 f
		cmpl	%ecx, %edi
		jl	LoopTransOff

// Are there any pixels left at all?

		cmpl	(lastpixel), %edi
		jnz	LastBits
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
//	a:	andl	$0x003F00FC, %ebx	/ mask off extraneous bits
//	b:	movb	%bl, %bh		/ make u flush with v
//	c:	shrl	$10, %ebx		/ right-justify u and v
//	d:	movb	(%esi,%ebx), %dl	/ get texture-map pixel
//	e:	cmpb	$255, %dl		/ is pixel transparent?
//	f:	sbbb	%bh, %bh		/ yes, %bh=00; no, %bh=FF
//	g:	movb	fadetbl(%edx), %dl	/ get lighting-corrected pixel
//	h:	movb	(%edi), %bl		/ get pixel in frame buffer now
//	i:	xorb	%bl, %dl		/ combine the two
//	j:	andb	%dl, %bh		/ use %bh as a mask to select
//	k:	xorb	%bl, %bh		/     which pixel to keep
//	l:	movb	%bh, (%edi)		/ write pixel to frame buffer
// 
// When the texture-map value is 255, the code simply writes the
// original frame-buffer value back out again; otherwise the new pixel
// is written instead. The ands and xors used to accomplish this bulk
// up the code, but on the whole it is better than having four
// unpredictable jumps in the loop. The four repeats of the above code
// are even more intertwined than the other loop, due to the extra
// register usage. Also note that the last two pixels combine steps i,
// j, and k with each other.

		DoFPCalcs
		fdivr	%st, %st(3)
		movl	(ubyz4), %ebx
		movl	(vbyz4), %edx
		incl	%ebx
		incl	%edx
                movl    (uvzero), %ecx          // %ecx = uv for pixel 0
		andl	$0x3FF0, %ebx
		shrl	$4, %ebx
		andl	$0x3FF0, %edx
		shll	$10, %edx
		movl	%ecx, %eax
                andl    $0x003F00FC, %ecx       // 0 a
		orl	%edx, %ebx
                movb    %cl, %ch                // 0 b
		addl	$4, %edi
                shrl    $10, %ecx               // 0 c
		movl	$0x7F0000, %edx
		movl	%ebx, (uvzero)
		andl	%ebp, %edx
		sarl	$8, %edx
                movb    (%esi,%ecx), %dl        // 0 d
		orl	$0x1000, %ebx
		subl	%eax, %ebx
                movb    -4(%edi), %ch           // 0 h
                movb    fadetbl(%edx), %cl      // 0 g
                cmpb    $255, %dl               // 0 e
                sbbb    %dl, %dl                // 0 f
                xorb    %ch, %cl                // 0 i
		shrl	$2, %ebx
                andb    %cl, %dl                // 0 j
                xorb    %ch, %dl                // 0 k
/               nop                             // (V-pipe idle)
                lea     (%eax,%ebx,2), %ecx     // %ecx = uv for pixel 2
                addl    %ebx, %eax              // %eax = uv for pixel 1
                andl    $0x003F00FC, %eax       // 1 a
                addl    %ecx, %ebx              // %ebx = uv for pixel 3
                movb    %al, %ah                // 1 b
                andl    $0x003F00FC, %ecx       // 2 a
                shrl    $10, %eax               // 1 c
                andl    $0x003F00FC, %ebx       // 3 a
                movb    %cl, %ch                // 2 b
                movb    %bl, %bh                // 3 b
                movb    %dl, -4(%edi)           // 0 l
                movb    (%esi,%eax), %dl        // 1 d
                movb    -3(%edi), %al           // 1 h
                cmpb    $255, %dl               // 1 e
                sbbb    %ah, %ah                // 1 f
                movb    fadetbl(%edx), %dl      // 1 g
                shrl    $10, %ecx               // 2 c
                xorb    %al, %dl                // 1 i
                shrl    $10, %ebx               // 3 c
                andb    %dl, %ah                // 1 j
                xorb    %al, %ah                // 1 k
                movb    (%esi,%ecx), %dl        // 2 d
                movb    %ah, -3(%edi)           // 1 l
                cmpb    $255, %dl               // 2 e
                sbbb    %ah, %ah                // 2 f
                movb    fadetbl(%edx), %ch      // 2 g
                movb    (%esi,%ebx), %dl        // 3 d
                movb    -2(%edi), %bh           // 2 h
                cmpb    $255, %dl               // 3 e
                movb    -1(%edi), %bl           // 3 h
                sbbb    %al, %al                // 3 f
                movb    fadetbl(%edx), %cl      // 2 g
		movl	(dldx), %edx
                xorl    %ebx, %ecx              // 2 i and 3 i
		addl	%edx, %ebp
                andl    %ecx, %eax              // 2 j and 3 j
		movl	(lastquartet), %ecx
                xorl    %ebx, %eax              // 2 k and 3 k
                movb    %ah, -2(%edi)           // 2 l
		cmpl	%ecx, %edi
                movb    %al, -1(%edi)           // 3 l
		jl	LoopTransOn

// Quit if there are none at all left.

		cmpl	(lastpixel), %edi
		jz	LeaveNow


LastBits:

// Here we finish off the last one-to-three pixels assigned to us.
// Rather than calculating values for all four pixels, we just divide
// the difference by four and keep adding this average into the value
// as needed. (This code is not particularly optimized, by the way,
// since it represents such a miniscule amount of the running time.)

		DoFPCalcs
		movl	(ubyz4), %ecx
		movl	(vbyz4), %edx
		incl	%ecx
		incl	%edx
		shrl	$4, %ecx
		andl	$0x3FF0, %edx
		shll	$10, %edx
		andl	$0x03FF, %ecx
		movl	(uvzero), %ebx
		orl	%edx, %ecx
		orl	$0x1000, %ecx
		subl	%ebx, %ecx
		shrl	$2, %ecx
		andl	$0x003FC0FF, %ecx
		movl	%ebp, %edx
		movl	(lastpixel), %ebp
		andl	$0x7F0000, %edx
		sarl	$8, %edx

LoopLastBits:	movl	%ebx, %eax
		movb	%al, %ah
		shrl	$10, %eax
		andb	$0x0F, %ah
		movb	(%esi,%eax), %dl
		cmpb	$255, %dl
		jz	LetPixelBy
		movb	fadetbl(%edx), %al
		movb	%al, (%edi)
LetPixelBy:	addl	%ecx, %ebx
		incl	%edi
		cmpl	%ebp, %edi
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
