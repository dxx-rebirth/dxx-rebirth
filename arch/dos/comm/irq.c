/*
 * This file is part of DZComm.
 * Version : 0.6
 * Minor changes by Dim Zegebart (zager@post.comstar.ru) to run with Palantir
 */

/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      Hardware interrupt wrapper functions. Unlike the _go32_dpmi_*
 *      functions, these can deal with reentrant interrupts.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>

#include "allegro.h"
#include "internal.h"
#include "asmdefs.inc"


#define MAX_IRQS     16
#define STACK_SIZE   8*1024      /* 8k stack should be plenty */


static int irq_virgin = TRUE;

_IRQ_HANDLER _irq_handler[MAX_IRQS];

unsigned char *_irq_stack[IRQ_STACKS];

extern void _irq_wrapper_0(), _irq_wrapper_1(),
            _irq_wrapper_2(), _irq_wrapper_3(),
            _irq_wrapper_4(), _irq_wrapper_5(),
            _irq_wrapper_6(), _irq_wrapper_7(),
            _irq_wrapper_8(), _irq_wrapper_9(),
            _irq_wrapper_10(), _irq_wrapper_11(),
            _irq_wrapper_12(), _irq_wrapper_13(),
            _irq_wrapper_14(), _irq_wrapper_15(),
            _irq_wrapper_0_end();



/* _install_irq:
 *  Installs a hardware interrupt handler for the specified irq, allocating
 *  an asm wrapper function which will save registers and handle the stack
 *  switching. The C function should return zero to exit the interrupt with
 *  an iret instruction, and non-zero to chain to the old handler.
 */
int _install_irq(int num, int (*handler)())
{
   int c;
   __dpmi_paddr addr;

   if (irq_virgin) {                /* first time we've been called? */
     LOCK_VARIABLE(_irq_handler);
      LOCK_VARIABLE(_irq_stack);
      LOCK_FUNCTION(_irq_wrapper_0);

      for (c=0; c<MAX_IRQS; c++) {
         _irq_handler[c].handler = NULL;
         _irq_handler[c].number = 0;
      }

      for (c=0; c<IRQ_STACKS; c++) {
         _irq_stack[c] = malloc(STACK_SIZE);
         if (_irq_stack[c]) {
            _go32_dpmi_lock_data(_irq_stack[c], STACK_SIZE);
            _irq_stack[c] += STACK_SIZE - 32;   /* stacks grow downwards */
         }
      }

      irq_virgin = FALSE;
   }

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].handler == NULL) {

         addr.selector = _my_cs();

         switch (c) {
            case 0: addr.offset32 = (long)_irq_wrapper_0; break;
            case 1: addr.offset32 = (long)_irq_wrapper_1; break;
            case 2: addr.offset32 = (long)_irq_wrapper_2; break;
            case 3: addr.offset32 = (long)_irq_wrapper_3; break;
            case 4: addr.offset32 = (long)_irq_wrapper_4; break;
            case 5: addr.offset32 = (long)_irq_wrapper_5; break;
            case 6: addr.offset32 = (long)_irq_wrapper_6; break;
            case 7: addr.offset32 = (long)_irq_wrapper_7; break;
            case 8: addr.offset32 = (long)_irq_wrapper_8; break;
            case 9: addr.offset32 = (long)_irq_wrapper_9; break;
            case 10: addr.offset32 = (long)_irq_wrapper_10; break;
            case 11: addr.offset32 = (long)_irq_wrapper_11; break;
            case 12: addr.offset32 = (long)_irq_wrapper_12; break;
            case 13: addr.offset32 = (long)_irq_wrapper_13; break;
            case 14: addr.offset32 = (long)_irq_wrapper_14; break;
            case 15: addr.offset32 = (long)_irq_wrapper_15; break;

            default: return -1;
         }

         _irq_handler[c].handler = handler;
         _irq_handler[c].number = num;

         __dpmi_get_protected_mode_interrupt_vector(num,
                                                &_irq_handler[c].old_vector);

         __dpmi_set_protected_mode_interrupt_vector(num, &addr);

         return 0;
      }
   }

   return -1;
}



/* _remove_irq:
 *  Removes a hardware interrupt handler, restoring the old vector.
 */
void _remove_irq(int num)
{
   int c;

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].number == num) {
         __dpmi_set_protected_mode_interrupt_vector(num,
                                                &_irq_handler[c].old_vector);
         _irq_handler[c].number = 0;
         _irq_handler[c].handler = NULL;

         break;
      }
   }
}
