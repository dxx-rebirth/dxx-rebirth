#include <conf.h>
#ifdef __ENV_DJGPP__

#include <stdio.h>
#include <string.h>
#ifdef __DJGPP__
#define _BORLAND_DOS_REGS 1
#endif
#include <dos.h>

#include "pstypes.h"
#include "inferno.h"
#include "text.h"
#include "console.h"
#include "args.h"
#include "error.h"

#include "joy.h"
#include "timer.h"
#include "key.h"
#include "mono.h"
#include "u_dpmi.h"
#include "mouse.h"

//added on 9/15/98 by Victor Rachels to add cd controls
//#include "bcd.h"
//end this section addition - Victor Rachels


void install_int3_handler(void);

#ifdef __WATCOMC__
int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned far * devhdr );
#endif

#ifndef NDEBUG
#ifdef __WATCOMC__
void do_heap_check()
{
        int heap_status;

        heap_status = _heapset( 0xFF );
        switch( heap_status )
        {
        case _HEAPBADBEGIN:
                mprintf((1, "ERROR - heap is damaged\n"));
                Int3();
                break;
        case _HEAPBADNODE:
                mprintf((1, "ERROR - bad node in heap\n" ));
                Int3();
                break;
        }
}
#endif
#endif



#ifdef VR_DEVICES
int is_3dbios_installed()
{
        dpmi_real_regs rregs;
        memset(&rregs,0,sizeof(dpmi_real_regs));
        rregs.eax = 0x4ed0;
        //rregs.ebx = 0x3d10;   
        dpmi_real_int386x( 0x10, &rregs );
        if ( (rregs.edx & 0xFFFF) != 0x3344 )
                return 0;
        else
                return 1;
}
#endif

// Returns 1 if ok, 0 if failed...
int init_gameport()
{
        union REGS regs;

        memset(&regs,0,sizeof(regs));
        regs.x.eax = 0x8400;
        regs.x.edx = 0xF0;
        int386( 0x15, &regs, &regs );
	if ( ( regs.x.eax & 0xFFFF ) == 0x4753 /*'SG'*/ )
                return 1;
        else
                return 0;
}

void check_dos_version()
{
        int major, minor;
        union REGS regs;

        memset(&regs,0,sizeof(regs));
        regs.x.eax = 0x3000;                                                    // Get MS-DOS Version Number
        int386( 0x21, &regs, &regs );

        major = regs.h.al;
        minor = regs.h.ah;
        
        if ( major < 5 )        {
                printf( "Using MS-DOS version %d.%d\nThis is not compatable with Descent.", major, minor);
                exit(1);
        }
        //printf( "\nUsing MS-DOS %d.%d...\n", major, minor );
}

void dos_check_file_handles(int num_required)
{
        int i, n;
        FILE * fp[16];

        if ( num_required > 16 )
                num_required = 16;

        n = 0;  
        for (i=0; i<16; i++ )
                fp[i] = NULL;
        for (i=0; i<16; i++ )   {
                fp[i] = fopen( "nul", "wb" );
                if ( !fp[i] ) break;
        }
        n = i;
        for (i=0; i<16; i++ )   {
                if (fp[i])
                        fclose(fp[i]);
        }
        if ( n < num_required ) {
                printf( "\n%s\n", TXT_NOT_ENOUGH_HANDLES );
                printf( "------------------------\n" );
                printf( "%d/%d %s\n", n, num_required, TXT_HANDLES_1 );
                printf( "%s\n", TXT_HANDLES_2);
                printf( "%s\n", TXT_HANDLES_3);
                exit(1);
        }
}

#define NEEDED_DOS_MEMORY                       ( 300*1024)             // 300 K
#define NEEDED_LINEAR_MEMORY                    (7680*1024)             // 7.5 MB
#define LOW_PHYSICAL_MEMORY_CUTOFF      (5*1024*1024)   // 5.0 MB
#define NEEDED_PHYSICAL_MEMORY          (2000*1024)             // 2000 KB

extern int piggy_low_memory;

void mem_int_to_string( int number, char *dest )
{
        int i,l,c;
        char buffer[20],*p;

        sprintf( buffer, "%d", number );

        l = strlen(buffer);
        if (l<=3) {
                // Don't bother with less than 3 digits
                sprintf( dest, "%d", number );
                return;
        }

        c = l % 3;
        p=dest;
        for (i=0; i<l; i++ ) {
                if (c==0) {
                        if (i) *p++=',';
            c = 3;
        }
        c--;
        *p++ = buffer[i];
    }
        *p++ = '\0';
}

void check_memory()
{
        char text[32];

        printf( "\n%s\n", TXT_AVAILABLE_MEMORY);
        printf( "----------------\n" );
        mem_int_to_string( dpmi_dos_memory/1024, text );
        printf( "Conventional: %7s KB\n", text );
        mem_int_to_string( dpmi_physical_memory/1024, text );
        printf( "Extended:     %7s KB\n", text );
        if ( dpmi_available_memory > dpmi_physical_memory )     {
                mem_int_to_string( (dpmi_available_memory-dpmi_physical_memory)/1024, text );
        } else {
                mem_int_to_string( 0, text );
        }
        printf( "Virtual:      %7s KB\n", text );
        printf( "\n" );

        if ( dpmi_dos_memory < NEEDED_DOS_MEMORY )      {
                printf( "%d %s\n", NEEDED_DOS_MEMORY - dpmi_dos_memory, TXT_MEMORY_CONFIG );
                exit(1);
        }

        if ( dpmi_available_memory < NEEDED_LINEAR_MEMORY )     {
                if ( dpmi_virtual_memory )      {
                        printf( "%d %s\n", NEEDED_LINEAR_MEMORY - dpmi_available_memory, TXT_RECONFIGURE_VMM );
                } else {
                        printf( "%d %s\n", NEEDED_LINEAR_MEMORY - dpmi_available_memory, TXT_MORE_MEMORY );
                        printf( "%s\n", TXT_MORE_MEMORY_2);
                }
                exit(1);
        }

        if ( dpmi_physical_memory < NEEDED_PHYSICAL_MEMORY )    {
                printf( "%d %s\n", NEEDED_PHYSICAL_MEMORY - dpmi_physical_memory, TXT_PHYSICAL_MEMORY );
                if ( dpmi_virtual_memory )      {       
                        printf( "%s\n", TXT_PHYSICAL_MEMORY_2);
                }
                exit(1);
        }

        if ( dpmi_physical_memory < LOW_PHYSICAL_MEMORY_CUTOFF )        {
                piggy_low_memory = 1;
        }
}

//NO_STACK_SIZE_CHECK uint * stack, *stack_ptr;
//NO_STACK_SIZE_CHECK int stack_size, unused_stack_space;
//NO_STACK_SIZE_CHECK int sil;
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK int main(int argc,char **argv)
//NO_STACK_SIZE_CHECK {
//NO_STACK_SIZE_CHECK   uint ret_value;
//NO_STACK_SIZE_CHECK   
//NO_STACK_SIZE_CHECK   unused_stack_space = 0;
//NO_STACK_SIZE_CHECK   stack = &ret_value;
//NO_STACK_SIZE_CHECK   stack_size = stackavail()/4;
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK   for ( sil=0; sil<stack_size; sil++ )    {
//NO_STACK_SIZE_CHECK           stack--;
//NO_STACK_SIZE_CHECK           *stack = 0xface0123;
//NO_STACK_SIZE_CHECK   }
//NO_STACK_SIZE_CHECK
//NO_STACK_SIZE_CHECK   ret_value = descent_main( argc, argv );         // Rename main to be descent_main
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK   for ( sil=0; sil<stack_size; sil++ )    {
//NO_STACK_SIZE_CHECK           if ( *stack == 0xface0123 )     
//NO_STACK_SIZE_CHECK                   unused_stack_space++;
//NO_STACK_SIZE_CHECK           stack++;
//NO_STACK_SIZE_CHECK   }
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK   mprintf(( 0, "Program used %d/%d stack space\n", (stack_size - unused_stack_space)*4, stack_size*4 ));
//NO_STACK_SIZE_CHECK   key_getch();
//NO_STACK_SIZE_CHECK 
//NO_STACK_SIZE_CHECK   return ret_value;
//NO_STACK_SIZE_CHECK }

extern int digi_timer_rate;

#ifdef __WATCOMC__
#pragma off (check_stack)
int __far descent_critical_error_handler(unsigned deverror, unsigned errcode, unsigned __far * devhdr )
{
        devhdr = devhdr;
        descent_critical_error++;
        descent_critical_deverror = deverror;
        descent_critical_errcode = errcode;
        return _HARDERR_FAIL;
}
void chandler_end (void)  // dummy functions
{
}
#pragma on (check_stack)
#endif

void arch_init_start() {
	// Initialize DPMI before anything else!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // (To check memory size and availbabitliy and allocate some low DOS memory)
        // adb: no TXT_... loaded yet
        //if (Inferno_verbose) printf( "%s... ", TXT_INITIALIZING_DPMI);
        con_printf(CON_VERBOSE, "Initializing DPMI services... ");
        dpmi_init(1);             // Before anything
        con_printf(CON_VERBOSE, "\n" );

#ifndef __GNUC__
        con_printf(CON_VERBOSE, "\n%s...", TXT_INITIALIZING_CRIT);
        if (!dpmi_lock_region((void near *)descent_critical_error_handler,(char *)chandler_end - (char near *)descent_critical_error_handler))  {
                Error( "Unable to lock critial error handler" );
        }
        if (!dpmi_lock_region(&descent_critical_error,sizeof(int)))     {
                Error( "Unable to lock critial error handler" );
        }
        if (!dpmi_lock_region(&descent_critical_deverror,sizeof(unsigned)))     {
                Error( "Unable to lock critial error handler" );
        }
        if (!dpmi_lock_region(&descent_critical_errcode,sizeof(unsigned)))      {
                Error( "Unable to lock critial error handler" );
        }
        _harderr((void *) descent_critical_error_handler );
        //Above line modified by KRB, added (void *) cast
        //for the compiler.
#endif
}

void arch_init() {
        if ( !FindArg( "-nodoscheck" ))
                check_dos_version();
        
        if ( !FindArg( "-nofilecheck" ))
                dos_check_file_handles(5);

        if ( !FindArg( "-nomemcheck" ))
                check_memory();

	#ifndef NDEBUG
                minit();
                mopen( 0, 9, 1, 78, 15, "Debug Spew");
                mopen( 1, 2, 1, 78,  5, "Errors & Serious Warnings");
        #endif

/*        if (!WVIDEO_running)
                mprintf((0,"WVIDEO_running = %d\n",WVIDEO_running));*/

        //if (!WVIDEO_running) install_int3_handler();


        con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_2);
        timer_init();
        timer_set_rate( digi_timer_rate );                      // Tell our timer how fast to go (120 Hz)
        joy_set_timer_rate( digi_timer_rate );      // Tell joystick how fast timer is going

        con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_3);
        key_init();
        if (!FindArg( "-nomouse" ))     {
                con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_4);
                if (FindArg( "-nocyberman" ))
                        mouse_init(0);
                else
                        mouse_init(1);
        } else {
                con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_5);
        }
        if (!FindArg( "-nojoystick" ))  {
                con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_6);
                joy_init();
                if ( FindArg( "-joyslow" ))     {
                        con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_7);
                        joy_set_slow_reading(JOY_SLOW_READINGS);
                }
                if ( FindArg( "-joypolled" ))   {
                        con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_8);
                        joy_set_slow_reading(JOY_POLLED_READINGS);
                }
                if ( FindArg( "-joybios" ))     {
                        con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_9);
                        joy_set_slow_reading(JOY_BIOS_READINGS);
                }
                if ( FindArg( "-joynice" ))     {
                        con_printf(CON_VERBOSE, "\n%s", "Using nice joystick poller..." );
                        joy_set_slow_reading(JOY_FRIENDLY_READINGS);
                }
                if ( FindArg( "-gameport" ))    {
                        if ( init_gameport() )  {                       
                                joy_set_slow_reading(JOY_BIOS_READINGS);
                        } else {
                                Error( "\nCouldn't initialize the Notebook Gameport.\nMake sure the NG driver is loaded.\n" );
                        }
                }
        } else {
                con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_10);
        }
        #if 0 // no divzero
        if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_11);
        div0_init(DM_ERROR);
        #endif
}

#endif // __ENV_DJGPP__
