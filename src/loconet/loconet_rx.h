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

extern uint8_t loconet_rx_process(void);
extern void loconet_rx_buffer_push(uint8_t);

#endif // _LOCONET_LOCONET_RX_H_
