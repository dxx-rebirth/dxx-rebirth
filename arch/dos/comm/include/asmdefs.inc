/* How many stacks to allocate for the irq wrappers. This can't be in the
 * main headers, because it is used by both C and asm code. You could 
 * probably get away with fewer of these, if you want to save memory and
 * you are feeling brave...
 */
#define IRQ_STACKS      8

#include "asmdef.inc"
