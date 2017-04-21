/*
 * Copyright (c) 2017, Alex Taradov <alex@taradov.com>,
 * Ferdi van der Werf <ferdi@slashdev.nl> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "samd20.h"
#include "hal_gpio.h"
#include "components/fast_clock.h"
#include "loconet/loconet.h"
#include "loconet/loconet_cv.h"
#include "utils/eeprom.h"
#include "utils/logger.h"

//-----------------------------------------------------------------------------
// Loconet: Tx: PA14, Rx: PA15, Flank: PA13, Flank timer:
LOCONET_BUILD(
  D,          /* pmux */
  0,          /* sercom */
  A, 4,       /* tx: port, pin */
  A, 5, 1,    /* rx: port, pin, pad */
  A, 6, 6, 0  /* flank: port, pin, interrupt, timer */
);

//-----------------------------------------------------------------------------
FAST_CLOCK_BUILD(1);

//-----------------------------------------------------------------------------
// Logger Tx: PB22, Rx: PB23
LOGGER_BUILD(
  D,        /* pmux */
  5,        /* sercom */
  B, 22,    /* Tx: port, pin */
  B, 23, 3  /* Rx: port, pin, pad */
);


//-----------------------------------------------------------------------------
void irq_handler_eic(void);
void irq_handler_eic(void) {
  if (loconet_handle_eic()) {
    return;
  }
}

//-----------------------------------------------------------------------------
static void sys_init(void)
{
  // Switch to 8MHz clock (disable prescaler)
  SYSCTRL->OSC8M.bit.PRESC = 0;

  // Enable interrupts
  asm volatile ("cpsie i");
}

//-----------------------------------------------------------------------------
static void hard_reset(void)
{
  __DSB();
  asm volatile ("cpsid i");
  WDT->CONFIG.reg = 0;
  WDT->CTRL.reg |= WDT_CTRL_ENABLE;
  while(1);
}

//-----------------------------------------------------------------------------
static void eeprom_init(void)
{
  enum status_code error_code = eeprom_emulator_init();

  // Fusebits for memory are not set, or too low.
  // We need at least 3 pages, so set to 1024
  if (error_code == STATUS_ERR_NO_MEMORY) {
    struct nvm_fusebits fusebits;
    nvm_get_fuses(&fusebits);
    fusebits.eeprom_size = NVM_EEPROM_EMULATOR_SIZE_1024;
    nvm_set_fuses(&fusebits);
    hard_reset();
  } else if (error_code != STATUS_OK) {
    // Erase eeprom, assume unformated or corrupt
    eeprom_emulator_erase_memory();
    hard_reset();
  }
}

static inline void initialize(void)
{
  // System
  sys_init();
  eeprom_init();
  logger_init(LOGGER_BAUDRATE);

  // Core
  loconet_cv_init();
  loconet_init();

  // Components
  fast_clock_init();
}

//-----------------------------------------------------------------------------
int main(void)
{
  // Initialize
  initialize();

  while (1) {
    loconet_loop();
    fast_clock_loop();
  }
  return 0;
}

