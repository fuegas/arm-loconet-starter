/**
 * @file loconet.c
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */
#include "loconet.h"

//-----------------------------------------------------------------------------
// Global variables
LOCONET_CONFIG_Type loconet_config = { 0 };
LOCONET_STATUS_Type loconet_status = { 0 };

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t CARRIER_DETECT:1;
    uint8_t MASTER_DELAY:1;
    uint8_t LINE_BREAK:1;
    uint8_t PRIORITY_DELAY:1;
    uint8_t :4;
  } bit;
  uint8_t reg;
} LOCONET_TIMER_STATUS_Type;

#define LOCONET_TIMER_STATUS_CARRIER_DETECT_Pos 0
#define LOCONET_TIMER_STATUS_CARRIER_DETECT (0x01ul << LOCONET_TIMER_STATUS_CARRIER_DETECT_Pos)
#define LOCONET_TIMER_STATUS_MASTER_DELAY_Pos 1
#define LOCONET_TIMER_STATUS_MASTER_DELAY (0x01ul << LOCONET_TIMER_STATUS_MASTER_DELAY_Pos)
#define LOCONET_TIMER_STATUS_LINE_BREAK_Pos 2
#define LOCONET_TIMER_STATUS_LINE_BREAK (0x01ul << LOCONET_TIMER_STATUS_LINE_BREAK_Pos)
#define LOCONET_TIMER_STATUS_PRIORITY_DELAY_Pos 3
#define LOCONET_TIMER_STATUS_PRIORITY_DELAY (0x01ul << LOCONET_TIMER_STATUS_PRIORITY_DELAY_Pos)

#define LOCONET_DELAY_CARRIER_DETECT 1200 /* 20x bit time (60ux) */
#define LOCONET_DELAY_MASTER_DELAY    360 /*  6x bit time (60us) */
#define LOCONET_DELAY_LINE_BREAK      900 /* 15x bit time (60us) */
#define LOCONET_DELAY_PRIORITY_DELAY   60 /*  1x bit time (60ux) */

static LOCONET_TIMER_STATUS_Type loconet_timer_status = { 0 };

//-----------------------------------------------------------------------------
void loconet_irq_flank_rise(void) {
  loconet_flank_timer_delay(LOCONET_DELAY_CARRIER_DETECT);
  loconet_timer_status.reg = LOCONET_TIMER_STATUS_CARRIER_DETECT;
  // If flank changes, loconet is busy
  loconet_status.reg |= LOCONET_STATUS_BUSY;
}

//-----------------------------------------------------------------------------
void loconet_irq_flank_fall(void) {
  loconet_flank_timer_delay(LOCONET_DELAY_LINE_BREAK);
  loconet_timer_status.reg = LOCONET_TIMER_STATUS_LINE_BREAK;
  // If flank changes, loconet is busy
  loconet_status.reg |= LOCONET_STATUS_BUSY;
}

//-----------------------------------------------------------------------------
void loconet_irq_timer(void) {
  // Carrier detect?
  if (loconet_timer_status.bit.CARRIER_DETECT) {
    if (loconet_config.bit.MASTER) {
      // Master, remove busy flag directly
      loconet_status.bit.BUSY = 0;
    } else {
      // Start master delay
      loconet_flank_timer_delay(LOCONET_DELAY_MASTER_DELAY);
      loconet_timer_status.reg = LOCONET_TIMER_STATUS_MASTER_DELAY;
    }
  } else if (loconet_timer_status.bit.MASTER_DELAY) {
    if (loconet_config.bit.PRIORITY) {
      // Start priority delay
      loconet_flank_timer_delay(loconet_config.bit.PRIORITY * LOCONET_DELAY_PRIORITY_DELAY);
      loconet_timer_status.reg = LOCONET_TIMER_STATUS_PRIORITY_DELAY;
    } else {
      loconet_status.bit.BUSY = 0;
    }
  } else if (loconet_timer_status.bit.PRIORITY_DELAY) {
    loconet_status.bit.BUSY = 0;
  } else if (loconet_timer_status.bit.LINE_BREAK) {
    // Remove collision detected flag
    loconet_status.bit.COLLISION_DETECTED = 0;
    loconet_hw_enable_rx_tx();
  }
}

//-----------------------------------------------------------------------------
void loconet_irq_collision(void)
{
  // Set collision detected flag
  loconet_status.bit.COLLISION_DETECTED = 1;
  // Stop receiving and sending
  loconet_hw_disable_rx_tx();
  // If we were transmitting, enforce line break
  if (loconet_status.bit.TRANSMIT) {
    // Disable TRANSMIT
    loconet_status.bit.TRANSMIT = 0;
    // Reset message to queue
    loconet_tx_reset_current_message_to_queue();
    // Force line break
    loconet_hw_force_tx_high();
    // Turn off activity led
    loconet_activity_led_off();
  }
}

//-----------------------------------------------------------------------------
// Calculate the checksum of a message
uint8_t loconet_calc_checksum(uint8_t *data, uint8_t length)
{
  uint8_t checksum = 0xFF;
  while (length--) {
    // Dereference data pointer, XOR it with checksum and advance pointer by 1
    checksum ^= *data++;
  }
  return checksum;
}

//-----------------------------------------------------------------------------
// Should be included in the main loop to keep loconet going.
void loconet_loop(void)
{
  // If a message is received and handled, keep processing new messages
  while(loconet_rx_process());
  // Send a message if there is one available
  loconet_tx_process();
}

//-----------------------------------------------------------------------------
void loconet_enable_transmit(void)
{
  loconet_hw_enable_transmit();
}
