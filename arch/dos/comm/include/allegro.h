#ifndef _ALLEGRO_H
#define _ALLEGRO_H

// nothing
#include <pc.h>
#include <go32.h>
#define allegro_init()
#define install_keyboard()
#define keypressed() kbhit()
#define readkey() getkey()

#ifdef DJGPP

/* for djgpp */
#define END_OF_FUNCTION(x)    void x##_end() { }
#define LOCK_VARIABLE(x)      _go32_dpmi_lock_data((void *)&x, sizeof(x))
#define LOCK_FUNCTION(x)      _go32_dpmi_lock_code(x, (long)x##_end - (long)x)

#else 

/* for linux */
#define END_OF_FUNCTION(x)
#define LOCK_VARIABLE(x)
#define LOCK_FUNCTION(x)

#endif

#define DISABLE() __asm__ __volatile__("cli");
#define ENABLE() __asm__ __volatile__("sti");


int _install_irq(int num, int (*handler)());
void _remove_irq(int num);
#define FALSE 0
#define TRUE (!FALSE)

#include <dpmi.h>
typedef struct _IRQ_HANDLER
{
   int (*handler)();             /* our C handler */
   int number;                   /* irq number */
   __dpmi_paddr old_vector;      /* original protected mode vector */
} _IRQ_HANDLER;

#endif
