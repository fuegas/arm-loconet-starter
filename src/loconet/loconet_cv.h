/**
 * @file loconet_cv.h
 * @brief Process Loconet CV messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

// ----------------------------------------------------------------------------
// Defines the total number of CV values possible in the project, if not
// already defined. The CV number range is defined by
// 0 <= LNCV < LOCONET_CV_NUMBERS.
#ifndef LOCONET_CV_NUMBERS
  #define LOCONET_CV_NUMBERS          0x1E  // 30
#endif

#ifndef _LOCONET_LOCONET_CV_H_
#define _LOCONET_LOCONET_CV_H_

#include <stdint.h>
#include <stdbool.h>
#include "loconet.h"
#include "loconet_rx.h"
#include "loconet_tx.h"
#include "loconet_tx_messages.h"
#include "loconet_messages.h"
#include "utils/eeprom.h"
#include "utils/status_codes.h"

#define LOCONET_CV_PER_PAGE         0x1E  // 30
#define LOCONET_CV_PAGE_SIZE        (EEPROM_PAGE_SIZE / 2)
// /Dev device class: 12100 (/D)
#define LOCONET_CV_DEVICE_CLASS     0x4BA // We listen to 1210
#define LOCONET_CV_INITIAL_ADDRESS  0x03  // Initial address we listen to
#define LOCONET_CV_INITIAL_PRIORITY 0x05  // Initial priority for sending

#define LOCONET_CV_SRC_MASTER       0x00
#define LOCONET_CV_SRC_KPU          0x01 // KPU is, e.g., an IntelliBox
#define LOCONET_CV_SRC_UNDEFINED    0x02 // Unknown source
#define LOCONET_CV_SRC_TWINBOX_FRED 0x03
#define LOCONET_CV_SRC_IBSWITCH     0x04
#define LOCONET_CV_SRC_MODULE       0x05

#define LOCONET_CV_DST_BROADCAST    0x0000
#define LOCONET_CV_DST_UB_SPU       0x4249
#define LOCONET_CV_DST_UB_KPU       0x4B49
#define LOCONET_CV_DST_MODULE       0x0005

#define LOCONET_CV_REQ_CFGREAD      0x1F
#define LOCONET_CV_REQ_CFGWRITE     0x20
#define LOCONET_CV_REQ_CFGREQUEST   0x21

#define LOCONET_CV_FLG_PROG_ON      0x80
#define LOCONET_CV_FLG_PROG_OFF     0x40
#define LOCONET_CV_FLG_READ_ONLY    0x01

#define LOCONET_CV_ACK_ERROR_GENERIC       0x00 // Error-codes for write-requests
#define LOCONET_CV_ACK_ERROR_OUTOFRANGE    0x01 // Value out of range
#define LOCONET_CV_ACK_ERROR_READONLY      0x02 // CV is read only
#define LOCONET_CV_ACK_ERROR_INVALID_VALUE 0x03 // Unsupported/non-existing CV
#define LOCONET_CV_ACK_OK                  0x7F // Everything OK

typedef struct {
  uint8_t source;
  __attribute__((packed)) uint16_t destination;
  uint8_t request_id;
  uint8_t most_significant_bits;
  __attribute__((packed)) uint16_t device_class;
  __attribute__((packed)) uint16_t lncv_number;
  __attribute__((packed)) uint16_t lncv_value;
  uint8_t flags;
} LOCONET_CV_MSG_Type;

//-----------------------------------------------------------------------------
extern void loconet_cv_process(LOCONET_CV_MSG_Type*, uint8_t);

//-----------------------------------------------------------------------------
extern uint16_t loconet_cv_get(uint16_t);

//-----------------------------------------------------------------------------
extern uint8_t loconet_cv_set(uint16_t, uint16_t);

//-----------------------------------------------------------------------------
extern enum status_code loconet_cv_init(void);

//-----------------------------------------------------------------------------
// Callback functions for listening on the RX messages to be programmed.
extern void loconet_cv_peer_xfer( uint8_t*, uint8_t);
extern void loconet_cv_imm_packet(uint8_t*, uint8_t);
extern void loconet_cv_wr_sl_data(uint8_t*, uint8_t);
extern void loconet_cv_rd_sl_data(uint8_t*, uint8_t);

#endif // _LOCONET_LOCONET_CV_H_
