/**
 * @file delay.c
 * @brief Adds functionality to use delays
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 */

#include "delay.h"

// Delay loop is put to SRAM so that FWS will not affect delay time
OPTIMIZE_HIGH
RAMFUNC
void _cycle_delay(unsigned long n)
{
  UNUSED(n);

  __asm (
    "loop: DMB      \n"
    "SUB r0, r0, #1 \n"
    "CMP r0, #0     \n"
    "BNE loop         "
  );
}
