/**
 * @file loconet.h
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_H_
#define _LOCONET_LOCONET_H_

#include <stdint.h>
#include <stdlib.h>
#include "loconet_hw.h"
#include "loconet_tx.h"

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint16_t ADDRESS:10;
    uint8_t  MASTER:1;
    uint8_t  PRIORITY:4;
    uint8_t  :1;
  } bit;
  uint16_t reg;
} LOCONET_CONFIG_Type;

#define LOCONET_CONFIG_ADDRESS_Pos 0
#define LOCONET_CONFIG_ADDRESS_Mask (0x3FFul << LOCONET_CONFIG_ADDRESS_Pos)
#define LOCONET_CONFIG_ADDRESS(value) (LOCONET_CONFIG_ADDRESS_Mask & ((value) << LOCONET_CONFIG_ADDRESS_Pos))
#define LOCONET_CONFIG_MASTER_Pos 10
#define LOCONET_CONFIG_MASTER (0x01ul << LOCONET_CONFIG_MASTER_Pos)
#define LOCONET_CONFIG_PRIORITY_Pos 11
#define LOCONET_CONFIG_PRIORITY_Mask (0x3FFul << LOCONET_CONFIG_PRIORITY_Pos)
#define LOCONET_CONFIG_PRIORITY(value) (LOCONET_CONFIG_ADDRESS_Mask & ((value) << LOCONET_CONFIG_PRIORITY_Pos))

extern LOCONET_CONFIG_Type loconet_config;

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t BUSY:1;
    uint8_t TRANSMIT:1;
    uint8_t COLLISION_DETECTED:1;
    uint8_t :5;
  } bit;
  uint8_t reg;
} LOCONET_STATUS_Type;

#define LOCONET_STATUS_BUSY_Pos 0
#define LOCONET_STATUS_BUSY (0x01ul << LOCONET_STATUS_BUSY_Pos)
#define LOCONET_STATUS_TRANSMIT_Pos 1
#define LOCONET_STATUS_TRANSMIT (0x01ul << LOCONET_STATUS_TRANSMIT_Pos)
#define LOCONET_STATUS_COLLISION_DETECT_Pos 2
#define LOCONET_STATUS_COLLISION_DETECT (0x01ul << LOCONET_STATUS_COLLISION_DETECT_Pos)

extern LOCONET_STATUS_Type loconet_status;

//-----------------------------------------------------------------------------
// IRQs for flank rise / fall
extern void loconet_irq_flank_rise(void);
extern void loconet_irq_flank_fall(void);
// IRQ for timeout of timer
extern void loconet_irq_timer(void);
// IRQ for collision detected
extern void loconet_irq_collision(void);
// IRQ for flank detection
extern uint8_t loconet_handle_eic(void);

//-----------------------------------------------------------------------------
// Calculate checksum of a message
extern uint8_t loconet_calc_checksum(uint8_t *data, uint8_t length);

//-----------------------------------------------------------------------------
// Enable transmitting
extern void loconet_enable_transmit(void);

#endif // _LOCONET_LOCONET_H_
