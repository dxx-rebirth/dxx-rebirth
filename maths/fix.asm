;THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
;SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
;END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
;ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
;IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
;SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
;FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
;CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
;AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
;COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.

[BITS 32]

%ifdef __LINUX__
%define _fixdivquadlong fixdivquadlong
%define _fixmul fixmul
%define _fixdiv fixdiv
%define _fixmulaccum fixmulaccum
%define _fixmuldiv fixmuldiv
%define _fixquadadjust fixquadadjust
%define _fixquadnegate fixquadnegate
%define _quad_sqrt quad_sqrt
%define _long_sqrt long_sqrt
%define _fix_sqrt fix_sqrt
%define _fix_asin fix_asin
%define _fix_acos fix_acos
%define _fix_atan2 fix_atan2
%define _fix_fastsincos fix_fastsincos
%define _fix_sincos fix_sincos
%endif

global _fixdivquadlong
global _fixmul
global _fixdiv
global _fixmulaccum
global _fixmuldiv
global _fixquadadjust
global _fixquadnegate
global _long_sqrt
global _quad_sqrt
global _fix_sqrt
global _fix_asin
global _fix_acos
global _fix_atan2
global _fix_fastsincos
global _fix_sincos
global quad_sqrt_asm	; for assembler vecmat
global fix_sincos_asm	; for assembler vecmat
global fix_acos_asm	; for assembler vecmat
global long_sqrt_asm	     ; for assembler vecmat
;global fix_asin_asm
global fix_fastsincos_asm

[SECTION .data]
sin_table  dw	   0
	dw	402
	dw	804
	dw	1205
	dw	1606
	dw	2006
	dw	2404
	dw	2801
	dw	3196
	dw	3590
	dw	3981
	dw	4370
	dw	4756
	dw	5139
	dw	5520
	dw	5897
	dw	6270
	dw	6639
	dw	7005
	dw	7366
	dw	7723
	dw	8076
	dw	8423
	dw	8765
	dw	9102
	dw	9434
	dw	9760
	dw	10080
	dw	10394
	dw	10702
	dw	11003
	dw	11297
	dw	11585
	dw	11866
	dw	12140
	dw	12406
	dw	12665
	dw	12916
	dw	13160
	dw	13395
	dw	13623
	dw	13842
	dw	14053
	dw	14256
	dw	14449
	dw	14635
	dw	14811
	dw	14978
	dw	15137
	dw	15286
	dw	15426
	dw	15557
	dw	15679
	dw	15791
	dw	15893
	dw	15986
	dw	16069
	dw	16143
	dw	16207
	dw	16261
	dw	16305
	dw	16340
	dw	16364
	dw	16379
cos_table	dw	16384
	dw	16379
	dw	16364
	dw	16340
	dw	16305
	dw	16261
	dw	16207
	dw	16143
	dw	16069
	dw	15986
	dw	15893
	dw	15791
	dw	15679
	dw	15557
	dw	15426
	dw	15286
	dw	15137
	dw	14978
	dw	14811
	dw	14635
	dw	14449
	dw	14256
	dw	14053
	dw	13842
	dw	13623
	dw	13395
	dw	13160
	dw	12916
	dw	12665
	dw	12406
	dw	12140
	dw	11866
	dw	11585
	dw	11297
	dw	11003
	dw	10702
	dw	10394
	dw	10080
	dw	9760
	dw	9434
	dw	9102
	dw	8765
	dw	8423
	dw	8076
	dw	7723
	dw	7366
	dw	7005
	dw	6639
	dw	6270
	dw	5897
	dw	5520
	dw	5139
	dw	4756
	dw	4370
	dw	3981
	dw	3590
	dw	3196
	dw	2801
	dw	2404
	dw	2006
	dw	1606
	dw	1205
	dw	804
	dw	402
	dw	0
	dw	-402
	dw	-804
	dw	-1205
	dw	-1606
	dw	-2006
	dw	-2404
	dw	-2801
	dw	-3196
	dw	-3590
	dw	-3981
	dw	-4370
	dw	-4756
	dw	-5139
	dw	-5520
	dw	-5897
	dw	-6270
	dw	-6639
	dw	-7005
	dw	-7366
	dw	-7723
	dw	-8076
	dw	-8423
	dw	-8765
	dw	-9102
	dw	-9434
	dw	-9760
	dw	-10080
	dw	-10394
	dw	-10702
	dw	-11003
	dw	-11297
	dw	-11585
	dw	-11866
	dw	-12140
	dw	-12406
	dw	-12665
	dw	-12916
	dw	-13160
	dw	-13395
	dw	-13623
	dw	-13842
	dw	-14053
	dw	-14256
	dw	-14449
	dw	-14635
	dw	-14811
	dw	-14978
	dw	-15137
	dw	-15286
	dw	-15426
	dw	-15557
	dw	-15679
	dw	-15791
	dw	-15893
	dw	-15986
	dw	-16069
	dw	-16143
	dw	-16207
	dw	-16261
	dw	-16305
	dw	-16340
	dw	-16364
	dw	-16379
	dw	-16384
	dw	-16379
	dw	-16364
	dw	-16340
	dw	-16305
	dw	-16261
	dw	-16207
	dw	-16143
	dw	-16069
	dw	-15986
	dw	-15893
	dw	-15791
	dw	-15679
	dw	-15557
	dw	-15426
	dw	-15286
	dw	-15137
	dw	-14978
	dw	-14811
	dw	-14635
	dw	-14449
	dw	-14256
	dw	-14053
	dw	-13842
	dw	-13623
	dw	-13395
	dw	-13160
	dw	-12916
	dw	-12665
	dw	-12406
	dw	-12140
	dw	-11866
	dw	-11585
	dw	-11297
	dw	-11003
	dw	-10702
	dw	-10394
	dw	-10080
	dw	-9760
	dw	-9434
	dw	-9102
	dw	-8765
	dw	-8423
	dw	-8076
	dw	-7723
	dw	-7366
	dw	-7005
	dw	-6639
	dw	-6270
	dw	-5897
	dw	-5520
	dw	-5139
	dw	-4756
	dw	-4370
	dw	-3981
	dw	-3590
	dw	-3196
	dw	-2801
	dw	-2404
	dw	-2006
	dw	-1606
	dw	-1205
	dw	-804
	dw	-402
	dw	0
	dw	402
	dw	804
	dw	1205
	dw	1606
	dw	2006
	dw	2404
	dw	2801
	dw	3196
	dw	3590
	dw	3981
	dw	4370
	dw	4756
	dw	5139
	dw	5520
	dw	5897
	dw	6270
	dw	6639
	dw	7005
	dw	7366
	dw	7723
	dw	8076
	dw	8423
	dw	8765
	dw	9102
	dw	9434
	dw	9760
	dw	10080
	dw	10394
	dw	10702
	dw	11003
	dw	11297
	dw	11585
	dw	11866
	dw	12140
	dw	12406
	dw	12665
	dw	12916
	dw	13160
	dw	13395
	dw	13623
	dw	13842
	dw	14053
	dw	14256
	dw	14449
	dw	14635
	dw	14811
	dw	14978
	dw	15137
	dw	15286
	dw	15426
	dw	15557
	dw	15679
	dw	15791
	dw	15893
	dw	15986
	dw	16069
	dw	16143
	dw	16207
	dw	16261
	dw	16305
	dw	16340
	dw	16364
	dw	16379
	dw	16384

asin_table	dw	0
	dw	41
	dw	81
	dw	122
	dw	163
	dw	204
	dw	244
	dw	285
	dw	326
	dw	367
	dw	408
	dw	448
	dw	489
	dw	530
	dw	571
	dw	612
	dw	652
	dw	693
	dw	734
	dw	775
	dw	816
	dw	857
	dw	897
	dw	938
	dw	979
	dw	1020
	dw	1061
	dw	1102
	dw	1143
	dw	1184
	dw	1225
	dw	1266
	dw	1307
	dw	1348
	dw	1389
	dw	1431
	dw	1472
	dw	1513
	dw	1554
	dw	1595
	dw	1636
	dw	1678
	dw	1719
	dw	1760
	dw	1802
	dw	1843
	dw	1884
	dw	1926
	dw	1967
	dw	2009
	dw	2050
	dw	2092
	dw	2134
	dw	2175
	dw	2217
	dw	2259
	dw	2300
	dw	2342
	dw	2384
	dw	2426
	dw	2468
	dw	2510
	dw	2551
	dw	2593
	dw	2636
	dw	2678
	dw	2720
	dw	2762
	dw	2804
	dw	2847
	dw	2889
	dw	2931
	dw	2974
	dw	3016
	dw	3059
	dw	3101
	dw	3144
	dw	3187
	dw	3229
	dw	3272
	dw	3315
	dw	3358
	dw	3401
	dw	3444
	dw	3487
	dw	3530
	dw	3573
	dw	3617
	dw	3660
	dw	3704
	dw	3747
	dw	3791
	dw	3834
	dw	3878
	dw	3922
	dw	3965
	dw	4009
	dw	4053
	dw	4097
	dw	4142
	dw	4186
	dw	4230
	dw	4275
	dw	4319
	dw	4364
	dw	4408
	dw	4453
	dw	4498
	dw	4543
	dw	4588
	dw	4633
	dw	4678
	dw	4723
	dw	4768
	dw	4814
	dw	4859
	dw	4905
	dw	4951
	dw	4997
	dw	5043
	dw	5089
	dw	5135
	dw	5181
	dw	5228
	dw	5274
	dw	5321
	dw	5367
	dw	5414
	dw	5461
	dw	5508
	dw	5556
	dw	5603
	dw	5651
	dw	5698
	dw	5746
	dw	5794
	dw	5842
	dw	5890
	dw	5938
	dw	5987
	dw	6035
	dw	6084
	dw	6133
	dw	6182
	dw	6231
	dw	6281
	dw	6330
	dw	6380
	dw	6430
	dw	6480
	dw	6530
	dw	6580
	dw	6631
	dw	6681
	dw	6732
	dw	6783
	dw	6835
	dw	6886
	dw	6938
	dw	6990
	dw	7042
	dw	7094
	dw	7147
	dw	7199
	dw	7252
	dw	7306
	dw	7359
	dw	7413
	dw	7466
	dw	7521
	dw	7575
	dw	7630
	dw	7684
	dw	7740
	dw	7795
	dw	7851
	dw	7907
	dw	7963
	dw	8019
	dw	8076
	dw	8133
	dw	8191
	dw	8249
	dw	8307
	dw	8365
	dw	8424
	dw	8483
	dw	8543
	dw	8602
	dw	8663
	dw	8723
	dw	8784
	dw	8846
	dw	8907
	dw	8970
	dw	9032
	dw	9095
	dw	9159
	dw	9223
	dw	9288
	dw	9353
	dw	9418
	dw	9484
	dw	9551
	dw	9618
	dw	9686
	dw	9754
	dw	9823
	dw	9892
	dw	9963
	dw	10034
	dw	10105
	dw	10177
	dw	10251
	dw	10324
	dw	10399
	dw	10475
	dw	10551
	dw	10628
	dw	10706
	dw	10785
	dw	10866
	dw	10947
	dw	11029
	dw	11113
	dw	11198
	dw	11284
	dw	11371
	dw	11460
	dw	11550
	dw	11642
	dw	11736
	dw	11831
	dw	11929
	dw	12028
	dw	12130
	dw	12234
	dw	12340
	dw	12449
	dw	12561
	dw	12677
	dw	12796
	dw	12919
	dw	13046
	dw	13178
	dw	13315
	dw	13459
	dw	13610
	dw	13770
	dw	13939
	dw	14121
	dw	14319
	dw	14538
	dw	14786
	dw	15079
	dw	15462
	dw	16384
	dw	16384	;extra for when exacty 1


acos_table	dw	16384
	dw	16343
	dw	16303
	dw	16262
	dw	16221
	dw	16180
	dw	16140
	dw	16099
	dw	16058
	dw	16017
	dw	15976
	dw	15936
	dw	15895
	dw	15854
	dw	15813
	dw	15772
	dw	15732
	dw	15691
	dw	15650
	dw	15609
	dw	15568
	dw	15527
	dw	15487
	dw	15446
	dw	15405
	dw	15364
	dw	15323
	dw	15282
	dw	15241
	dw	15200
	dw	15159
	dw	15118
	dw	15077
	dw	15036
	dw	14995
	dw	14953
	dw	14912
	dw	14871
	dw	14830
	dw	14789
	dw	14748
	dw	14706
	dw	14665
	dw	14624
	dw	14582
	dw	14541
	dw	14500
	dw	14458
	dw	14417
	dw	14375
	dw	14334
	dw	14292
	dw	14250
	dw	14209
	dw	14167
	dw	14125
	dw	14084
	dw	14042
	dw	14000
	dw	13958
	dw	13916
	dw	13874
	dw	13833
	dw	13791
	dw	13748
	dw	13706
	dw	13664
	dw	13622
	dw	13580
	dw	13537
	dw	13495
	dw	13453
	dw	13410
	dw	13368
	dw	13325
	dw	13283
	dw	13240
	dw	13197
	dw	13155
	dw	13112
	dw	13069
	dw	13026
	dw	12983
	dw	12940
	dw	12897
	dw	12854
	dw	12811
	dw	12767
	dw	12724
	dw	12680
	dw	12637
	dw	12593
	dw	12550
	dw	12506
	dw	12462
	dw	12419
	dw	12375
	dw	12331
	dw	12287
	dw	12242
	dw	12198
	dw	12154
	dw	12109
	dw	12065
	dw	12020
	dw	11976
	dw	11931
	dw	11886
	dw	11841
	dw	11796
	dw	11751
	dw	11706
	dw	11661
	dw	11616
	dw	11570
	dw	11525
	dw	11479
	dw	11433
	dw	11387
	dw	11341
	dw	11295
	dw	11249
	dw	11203
	dw	11156
	dw	11110
	dw	11063
	dw	11017
	dw	10970
	dw	10923
	dw	10876
	dw	10828
	dw	10781
	dw	10733
	dw	10686
	dw	10638
	dw	10590
	dw	10542
	dw	10494
	dw	10446
	dw	10397
	dw	10349
	dw	10300
	dw	10251
	dw	10202
	dw	10153
	dw	10103
	dw	10054
	dw	10004
	dw	9954
	dw	9904
	dw	9854
	dw	9804
	dw	9753
	dw	9703
	dw	9652
	dw	9601
	dw	9549
	dw	9498
	dw	9446
	dw	9394
	dw	9342
	dw	9290
	dw	9237
	dw	9185
	dw	9132
	dw	9078
	dw	9025
	dw	8971
	dw	8918
	dw	8863
	dw	8809
	dw	8754
	dw	8700
	dw	8644
	dw	8589
	dw	8533
	dw	8477
	dw	8421
	dw	8365
	dw	8308
	dw	8251
	dw	8193
	dw	8135
	dw	8077
	dw	8019
	dw	7960
	dw	7901
	dw	7841
	dw	7782
	dw	7721
	dw	7661
	dw	7600
	dw	7538
	dw	7477
	dw	7414
	dw	7352
	dw	7289
	dw	7225
	dw	7161
	dw	7096
	dw	7031
	dw	6966
	dw	6900
	dw	6833
	dw	6766
	dw	6698
	dw	6630
	dw	6561
	dw	6492
	dw	6421
	dw	6350
	dw	6279
	dw	6207
	dw	6133
	dw	6060
	dw	5985
	dw	5909
	dw	5833
	dw	5756
	dw	5678
	dw	5599
	dw	5518
	dw	5437
	dw	5355
	dw	5271
	dw	5186
	dw	5100
	dw	5013
	dw	4924
	dw	4834
	dw	4742
	dw	4648
	dw	4553
	dw	4455
	dw	4356
	dw	4254
	dw	4150
	dw	4044
	dw	3935
	dw	3823
	dw	3707
	dw	3588
	dw	3465
	dw	3338
	dw	3206
	dw	3069
	dw	2925
	dw	2774
	dw	2614
	dw	2445
	dw	2263
	dw	2065
	dw	1846
	dw	1598
	dw	1305
	dw	922
	dw	0
        dw      0       ;extra for when exacty 1


guess_table:
	times 1  db 1
	times 3  db 1
	times 5  db 2
	times 7  db 3
	times 9  db 4
	times 11 db 5
	times 13 db 6
	times 15 db 7
	times 17 db 8
	times 19 db 9
	times 21 db 10
	times 23 db 11
	times 25 db 12
	times 27 db 13
	times 29 db 14
	times 31 db 15

[SECTION .text]

%macro abs_eax 0
	cdq
        xor     eax,edx
        sub     eax,edx
%endmacro

%macro m_fixdiv 1
	mov	edx,eax
	sar	edx,16
	shl	eax,16
	idiv	%1
%endmacro

_fixdivquadlong:
mov eax,[esp+4]
mov edx,[esp+8]
idiv dword [esp+12]
ret

_fixmul:
mov eax,[esp+4]
imul dword [esp+8]
shrd eax,edx,16
ret

_fixdiv:
mov eax,[esp+4]
mov edx,eax
sar edx,16
shl eax,16
idiv dword [esp+8]
ret

_fixmulaccum:
mov ecx,[esp+4]
mov eax,[esp+8]
imul dword [esp+12]
add [ecx],eax
adc [ecx+4],edx
ret

_fixmuldiv:
mov eax,[esp+4]
imul dword [esp+8]
idiv dword [esp+12]
ret

_fixquadadjust:
mov ecx,[esp+4]
mov eax,[ecx]
mov edx,[ecx+4]
shrd eax,edx,16
ret

_fixquadnegate:
mov eax,[esp+4]
neg dword [eax]
not dword [eax+4]
sbb dword [eax+4],-1
ret

;standard Newtonian-iteration square root routine.  takes eax, returns ax
;trashes eax,ebx,ecx,edx,esi,edi
_long_sqrt:
	mov eax,[esp+4]
long_sqrt_asm:
	or	eax,eax ;check sign
	jle	near error   ;zero or negative

	push ebx
	push esi
	push edi

	mov	edx,eax
	and	eax,0ffffh
	shr	edx,16	;split eax -> dx:ax

;get a good first quess by checking which byte most significant bit is in
	xor	ebx,ebx	;clear high bytes for index

	or	dh,dh	;highest byte
	jz	not_dh
	mov	bl,dh	;get value for lookup
	mov	cl,12
	jmp	got_guess
not_dh:	or	dl,dl
	jz	not_dl
	mov	bl,dl	;get value for lookup
	mov	cl,8
	jmp	got_guess
not_dl:	or	ah,ah
	jz	not_ah
	mov	bl,ah	;get value for lookup
	mov	cl,4
	jmp	got_guess
not_ah:	mov	bl,al	;get value for lookup
	mov	cl,0
got_guess:
	movzx	ebx,byte [guess_table+ebx] ;get byte guess
	sal	ebx,cl	;get in right place

	mov	ecx,eax
	mov	esi,edx	;save dx:ax

;the loop nearly always executes 3 times, so we'll unroll it 2 times and
;not do any checking until after the third time.  By my calcutations, the
;loop is executed 2 times in 99.97% of cases, 3 times in 93.65% of cases, 
;four times in 16.18% of cases, and five times in 0.44% of cases.  It never
;executes more than five times.  By timing, I determined that is is faster
;to always execute three times and not check for termination the first two
;times through.  This means that in 93.65% of cases, we save 6 cmp/jcc pairs,
;and in 6.35% of cases we do an extra divide.  In real life, these numbers
;might not be the same.

;newt_loop:
%rep 2
	mov	eax,ecx
	mov	edx,esi	;restore dx:ax
	div	bx	;dx:ax / bx
;	 mov	 edi,ebx ;save for compare
	add	ebx,eax
	rcr	ebx,1	 ;next guess = (d + q)/2
%endrep

newt_loop:	mov	eax,ecx
	mov	edx,esi   ;restore dx:ax
	div	bx	;dx:ax / bx
	cmp	eax,ebx ;correct?
	je	got_it	;..yep
	mov	edi,ebx   ;save for compare
	add	ebx,eax
	rcr	ebx,1	 ;next guess = (d + q)/2
	cmp	ebx,eax
	je	almost_got_it
	cmp	ebx,edi
	jne	newt_loop

almost_got_it:	mov	eax,ebx
	or	dx,dx	;check remainder
	jz	got_it
	inc	eax
got_it: and eax,0ffffh
	pop edi
	pop esi
	pop ebx
	ret

;sqrt called with zero or negative input. return zero
error:	xor	eax,eax
        ret

;standard Newtonian-iteration square root routine.  takes edx:eax, returns eax
_quad_sqrt:
	mov	eax,[esp+4]
	mov	edx,[esp+8]
quad_sqrt_asm:
	or	edx,edx ;check sign
	js	error	;can't do negative number!
	jnz	must_do_quad	;we really must do 64/32 div
	or	eax,eax	;check high bit of low longword
	jns	near long_sqrt_asm   ;we can use longword version
must_do_quad:

	push ebx
	push esi
	push edi

;get a good first quess by checking which byte most significant bit is in
	xor	ebx,ebx	;clear high bytes for index

	ror	edx,16	;get high 2 bytes

	or	dh,dh
	jz	q_not_dh
	mov	bl,dh	;get value for lookup
	mov	cl,12+16
	ror	edx,16	;restore edx
	jmp	q_got_guess
q_not_dh:	or	dl,dl
	jz	q_not_dl
	mov	bl,dl	;get value for lookup
	mov	cl,8+16
	ror	edx,16	;restore edx
	jmp	q_got_guess
q_not_dl:	ror	edx,16	;restore edx
	or	dh,dh
	jz	q_not_ah
	mov	bl,dh	;get value for lookup
	mov	cl,4+16
	jmp	q_got_guess
q_not_ah:	mov	bl,dl	;get value for lookup
	mov	cl,0+16
q_got_guess:
	movzx	ebx,byte [guess_table+ebx] ;get byte guess
	sal	ebx,cl	;get in right place

q_really_got_guess:
	mov	ecx,eax
	mov	esi,edx	;save edx:eax

;quad loop usually executes 4 times

;q_newt_loop:
%rep 3
	mov	eax,ecx
	mov	edx,esi	;restore dx:ax
	div	ebx	;dx:ax / bx
	mov	edi,ebx	;save for compare
	add	ebx,eax
	rcr	ebx,1	;next guess = (d + q)/2
%endrep

q_newt_loop:	mov	eax,ecx
	mov	edx,esi	;restore dx:ax
	div	ebx	;dx:ax / bx
	cmp	eax,ebx	;correct?
	je	q_got_it	;..yep
	mov	edi,ebx	;save for compare
	add	ebx,eax
	rcr	ebx,1	;next guess = (d + q)/2
	cmp	ebx,eax
	je	q_almost_got_it
	cmp	ebx,edi
	jne	q_newt_loop

q_almost_got_it:	mov	eax,ebx
	or	edx,edx	;check remainder
	jz	q_got_it
	inc	eax
q_got_it:
	pop edi
	pop esi
	pop ebx
	ret


;fixed-point square root
_fix_sqrt:
	mov	eax,[esp+4]
	call	long_sqrt_asm
;	 movzx	 eax,ax ; now in long_sqrt
	sal	eax,8
        ret

;the sincos functions have two varients: the C version is passed pointer
;to variables for sin & cos, and the assembly version returns the values
;in two registers

;takes ax=angle, returns eax=sin, ebx=cos.
fix_fastsincos_asm:
	movzx	eax,ah	;get high byte 
	movsx	ebx,word [cos_table+eax*2]
	sal	ebx,2	;make a fix
	movsx	eax,word [sin_table+eax*2]
	sal	eax,2	;make a fix
	ret

;takes ax=angle, returns eax=sin, ebx=cos.
fix_sincos_asm:
	push	ecx
	push	edx
	xor	edx, edx
	xor	ecx, ecx
	mov	dl, ah	;get high byte
	mov	cl, al	;save low byte
	shl	edx, 1

	movsx	eax,word [sin_table+edx]
	movsx	ebx,word [sin_table+edx+2]
	sub	ebx,eax
	imul	ebx,ecx	;mul by fraction
	sar	ebx,8
	add	eax,ebx	;add in frac part
	sal	eax,2	;make a fix

	movsx	ebx,word [cos_table+edx]
	movsx	edx,word [cos_table+edx+2]
	sub	edx,ebx
	imul	edx,ecx	;mul by fraction
	sar	edx,8
	add	ebx,edx	;add in frac part
	sal	ebx,2	;make a fix
	pop	edx
	pop	ecx
	ret

	align	16

_fix_acos:
	mov	eax,[esp+4]
;takes eax=cos angle, returns ax=angle
fix_acos_asm:
	push	ebx
	push	ecx
	push	edx

	abs_eax 	;get abs eax
	push	edx	;save sign

	cmp	eax,10000h
	jle	no_acos_oflow
	mov	eax,10000h
no_acos_oflow:
	movzx	ecx,al	;save low byte (fraction)

	mov	edx,eax

	sar	edx,8	;get high byte (+1 bit)
	movsx	eax,word [acos_table+edx*2]
	movsx	ebx,word [acos_table+edx*2+2]
	sub	ebx,eax
	imul	ebx,ecx	;mul by fraction
	sar	ebx,8
	add	eax,ebx	;add in frac part

	pop	edx	;get sign back
	xor	eax,edx
	sub	eax,edx	;make correct sign
	and	edx,8000h	;zero or 1/2
	add	eax,edx

	pop	edx
	pop	ecx
	pop	ebx

	ret

;takes eax=sin angle, returns ax=angle
_fix_asin:
	mov	eax,[esp+4]
fix_asin_asm:
	push	ebx
	push	ecx
	push	edx

	abs_eax 	;get abs value
	push	edx	;save sign

	cmp	eax,10000h
	jle	no_asin_oflow
	mov	eax,10000h
no_asin_oflow:
	movzx	ecx,al	;save low byte (fraction)

	mov	edx,eax

	sar	edx,8	;get high byte (+1 bit)
	movsx	eax,word [asin_table+edx*2]
	movsx	ebx,word [asin_table+edx*2+2]
	sub	ebx,eax
	imul	ebx,ecx	;mul by fraction
	sar	ebx,8
	add	eax,ebx	;add in frac part

	pop	edx	;get sign back
	xor	eax,edx	;make sign correct
	sub	eax,edx

	pop	edx
	pop	ecx
	pop	ebx

	ret

;given cos & sin of an angle, return that angle. takes eax=cos,ebx=sin. 
;returns ax. parms need not be normalized, that is, the ratio eax/ebx must
;equal the ratio cos/sin, but the parms need not be the actual cos & sin.  
;NOTE: this is different from the standard C atan2, since it is left-handed.
;uses either asin or acos, to get better precision

_fix_atan2:
	push	ebx
	push	ecx
	push	edx
	mov	eax,[esp+16]
	mov	ebx,[esp+20]

	%ifdef NOT_DEF
	%ifndef  NDEBUG
	mov	edx,eax
	or	edx,ebx
	break_if	z,'Both parms to atan2 are zero!'
	%endif
	%endif

	push	ebx
	push	eax

;find smaller of two
	push	eax ;save
	push	ebx 
	abs_eax 	;get abs value
        xchg    eax,ebx
	abs_eax 	;get abs value
	xor	eax,edx
        sub     eax,edx
        cmp     ebx,eax ;compare x to y
	pop	ebx
	pop	eax
	jl	use_cos

;sin is smaller, use arcsin

	imul	eax
	xchg	eax,ebx
	mov	ecx,edx
	imul	eax
	add	eax,ebx
	adc	edx,ecx
	call	quad_sqrt_asm
	mov	ecx,eax	;ecx = mag

	pop	ebx	;get cos, save in ebx
	pop	eax	;get sin
	jecxz	sign_ok	;abort!
	m_fixdiv	ecx	;normalize it
	call	fix_asin_asm	;get angle
	or	ebx,ebx	;check sign of cos
	jns	sign_ok
	sub	eax,8000h	;adjust
	neg	eax
sign_ok:
	pop	edx
	pop	ecx
	pop	ebx
	ret


;cos is smaller, use arccos

use_cos:	imul	eax
	xchg	eax,ebx
	mov	ecx,edx
	imul	eax
	add	eax,ebx
	adc	edx,ecx
	call	quad_sqrt_asm
	mov	ecx,eax

	pop	eax	;get cos
	m_fixdiv	ecx	;normalize it
	call	fix_acos_asm ; get angle
	mov	ebx,eax	;save in ebx
	pop	eax	;get sin
	cdq		;get sign of sin
	mov	eax,ebx	;get cos back
	xor	eax,edx
	sub	eax,edx	;make sign correct

	pop	edx
	pop	ecx
	pop	ebx
	ret


; C version - takes angle,*sin,*cos. fills in sin&cos.
;either (or both) pointers can be null
;trashes eax,ecx,edx
_fix_fastsincos:
	push	ebx
	mov	eax,[esp+8]
	call	fix_fastsincos_asm
	mov	ecx,[esp+12]
	mov	edx,[esp+16]
	or	ecx,ecx
	jz	no_sin
	mov	[ecx],eax
no_sin: or	edx,edx
	jz	no_cos
	mov	[edx],ebx
no_cos: pop	ebx
	ret

;C version - takes angle,*sin,*cos. fills in sin&cos.
;trashes eax,ecx,edx
;either (or both) pointers can be null
_fix_sincos:
	push	ebx
	mov	eax,[esp+8]
	call	fix_sincos_asm
	mov	ecx,[esp+12]
	mov	edx,[esp+16]
	or	ecx,ecx
	jz	no_sin
	mov	[ecx],eax
	or	edx,edx
	jz	no_cos
	mov	[edx],ebx
	pop	ebx
        ret
