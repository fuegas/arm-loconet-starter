/**
 * @file loconet_rx.h
 * @brief Process received Loconet messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * This file contains the processing of received loconet messages.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_RX_H_
#define _LOCONET_LOCONET_RX_H_

#include <stdint.h>
#include "loconet_cv.h"

extern void loconet_rx_init(void);
extern uint8_t loconet_rx_process(void);
extern void loconet_rx_buffer_push(uint8_t);

//-----------------------------------------------------------------------------
// Observer pattern to register for messages with a specified opcode
// Each time a message is received, all registered callbacks for that opcode
// are notified.
//
// Each callback function has three parameters:
//   - opcode: the opcode of the message
//   - *arr  : byte array containing the message itself
//   - size  : the length of the message (i.e., number of bytes)

// Registers a callback function to an opcode
extern void loconet_rx_register_callback(uint8_t opcode, void (*callback)(uint8_t, uint8_t*, uint8_t));

// Removes a callback function from an opcode
extern void loconet_rx_unregister_callback(uint8_t opcode, void (*callback)(uint8_t, uint8_t*, uint8_t));

#endif // _LOCONET_LOCONET_RX_H_
