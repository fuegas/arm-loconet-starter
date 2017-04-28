/**
 * @file loconet_rx.c
 * @brief Process received Loconet messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include "loconet_rx.h"

//-----------------------------------------------------------------------------
// Prototypes
void loconet_rx_dummy_0(void);
void loconet_rx_dummy_2(uint8_t, uint8_t);
void loconet_rx_dummy_4(uint8_t, uint8_t, uint8_t, uint8_t);
void loconet_rx_dummy_n(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
// Define LOCONET_RX_RINGBUFFER_Size if it's not defined
#ifndef LOCONET_RX_RINGBUFFER_Size
#define LOCONET_RX_RINGBUFFER_Size 64
#endif

typedef struct {
  uint8_t buffer[LOCONET_RX_RINGBUFFER_Size];
  volatile uint8_t writer;
  volatile uint8_t reader;
} LOCONET_RX_RINGBUFFER_Type;

static LOCONET_RX_RINGBUFFER_Type loconet_rx_ringbuffer = { { 0 }, 0, 0};

//-----------------------------------------------------------------------------
void loconet_rx_buffer_push(uint8_t byte)
{
  // Get index + 1 of buffer head
  uint8_t index = (loconet_rx_ringbuffer.writer + 1) % LOCONET_RX_RINGBUFFER_Size;

  // If the buffer is full, wait until the reader empties
  // a slot in the buffer to write to.
  while (index == loconet_rx_ringbuffer.reader) {
    continue;
  }

  // Write the byte
  loconet_rx_ringbuffer.buffer[loconet_rx_ringbuffer.writer] = byte;
  loconet_rx_ringbuffer.writer = index;
}

//-----------------------------------------------------------------------------
#define LOCONET_RX_DUMMY_0(name) \
  __attribute__ ((weak, alias ("loconet_rx_dummy_0"))) \
  void loconet_rx_##name(void)
#define LOCONET_RX_DUMMY_2(name) \
  __attribute__ ((weak, alias ("loconet_rx_dummy_2"))) \
  void loconet_rx_##name(uint8_t, uint8_t)
#define LOCONET_RX_DUMMY_4(name) \
  __attribute__ ((weak, alias ("loconet_rx_dummy_4"))) \
  void loconet_rx_##name(uint8_t, uint8_t, uint8_t, uint8_t)
#define LOCONET_RX_DUMMY_N(name) \
  __attribute__ ((weak, alias ("loconet_rx_dummy_n"))) \
  void loconet_rx_##name(uint8_t*, uint8_t)

//-----------------------------------------------------------------------------
LOCONET_RX_DUMMY_0(busy);
LOCONET_RX_DUMMY_0(gpoff);
LOCONET_RX_DUMMY_0(gpon);
LOCONET_RX_DUMMY_0(idle);

//-----------------------------------------------------------------------------
LOCONET_RX_DUMMY_2(loco_spd);
LOCONET_RX_DUMMY_2(loco_dirf);
LOCONET_RX_DUMMY_2(loco_snd);
LOCONET_RX_DUMMY_2(sw_req);
LOCONET_RX_DUMMY_2(sw_rep);
LOCONET_RX_DUMMY_2(input_rep);
LOCONET_RX_DUMMY_2(long_ack);
LOCONET_RX_DUMMY_2(slot_stat1);
LOCONET_RX_DUMMY_2(consist_func);
LOCONET_RX_DUMMY_2(unlink_slots);
LOCONET_RX_DUMMY_2(link_slots);
LOCONET_RX_DUMMY_2(move_slots);
LOCONET_RX_DUMMY_2(rq_sl_data);
LOCONET_RX_DUMMY_2(sw_state);
LOCONET_RX_DUMMY_2(sw_ack);
LOCONET_RX_DUMMY_2(loco_adr);

//-----------------------------------------------------------------------------
LOCONET_RX_DUMMY_N(wr_sl_data);
LOCONET_RX_DUMMY_N(rd_sl_data);
LOCONET_RX_DUMMY_N(peer_xfer);
LOCONET_RX_DUMMY_N(imm_packet);
LOCONET_RX_DUMMY_N(prog_task_start);
LOCONET_RX_DUMMY_N(prog_task_final);
LOCONET_RX_DUMMY_N(fast_clock);

//-----------------------------------------------------------------------------
// Special handlers which cannot be overriden
static void loconet_rx_wr_sl_data_(uint8_t*, uint8_t);
static void loconet_rx_rd_sl_data_(uint8_t*, uint8_t);
static void loconet_rx_peer_xfer_(uint8_t*, uint8_t);
static void loconet_rx_imm_packet_(uint8_t*, uint8_t);

//-----------------------------------------------------------------------------
void (* const ln_messages_0[32])(void) = {
  loconet_rx_dummy_0,     // 0x80
  loconet_rx_busy,        // 0x81
  loconet_rx_gpoff,       // 0x82
  loconet_rx_gpon,        // 0x83
  loconet_rx_dummy_0,     // 0x84
  loconet_rx_idle,        // 0x85
  loconet_rx_dummy_0,     // 0x86
  loconet_rx_dummy_0,     // 0x87
  loconet_rx_dummy_0,     // 0x88
  loconet_rx_dummy_0,     // 0x89
  loconet_rx_dummy_0,     // 0x8A
  loconet_rx_dummy_0,     // 0x8B
  loconet_rx_dummy_0,     // 0x8C
  loconet_rx_dummy_0,     // 0x8D
  loconet_rx_dummy_0,     // 0x8E
  loconet_rx_dummy_0,     // 0x8F
  loconet_rx_dummy_0,     // 0x90
  loconet_rx_dummy_0,     // 0x91
  loconet_rx_dummy_0,     // 0x92
  loconet_rx_dummy_0,     // 0x93
  loconet_rx_dummy_0,     // 0x94
  loconet_rx_dummy_0,     // 0x95
  loconet_rx_dummy_0,     // 0x96
  loconet_rx_dummy_0,     // 0x97
  loconet_rx_dummy_0,     // 0x98
  loconet_rx_dummy_0,     // 0x99
  loconet_rx_dummy_0,     // 0x9A
  loconet_rx_dummy_0,     // 0x9B
  loconet_rx_dummy_0,     // 0x9C
  loconet_rx_dummy_0,     // 0x9D
  loconet_rx_dummy_0,     // 0x9E
  loconet_rx_dummy_0,     // 0x9F
};

//-----------------------------------------------------------------------------
void (* const ln_messages_2[32])(uint8_t, uint8_t) = {
  loconet_rx_loco_spd,    // 0xA0
  loconet_rx_loco_dirf,   // 0xA1
  loconet_rx_loco_snd,    // 0xA2
  loconet_rx_dummy_2,     // 0xA3
  loconet_rx_dummy_2,     // 0xA4
  loconet_rx_dummy_2,     // 0xA5
  loconet_rx_dummy_2,     // 0xA6
  loconet_rx_dummy_2,     // 0xA7
  loconet_rx_dummy_2,     // 0xA8
  loconet_rx_dummy_2,     // 0xA9
  loconet_rx_dummy_2,     // 0xAA
  loconet_rx_dummy_2,     // 0xAB
  loconet_rx_dummy_2,     // 0xAC
  loconet_rx_dummy_2,     // 0xAD
  loconet_rx_dummy_2,     // 0xAE
  loconet_rx_dummy_2,     // 0xAF
  loconet_rx_sw_req,      // 0xB0
  loconet_rx_sw_rep,      // 0xB1
  loconet_rx_input_rep,   // 0xB2
  loconet_rx_dummy_2,     // 0xB3
  loconet_rx_long_ack,    // 0xB4
  loconet_rx_slot_stat1,  // 0xB5
  loconet_rx_consist_func,// 0xB6
  loconet_rx_dummy_2,     // 0xB7
  loconet_rx_unlink_slots,// 0xB8
  loconet_rx_link_slots,  // 0xB9
  loconet_rx_move_slots,  // 0xBA
  loconet_rx_rq_sl_data,  // 0xBB
  loconet_rx_sw_state,    // 0xBC
  loconet_rx_sw_ack,      // 0xBD
  loconet_rx_dummy_2,     // 0xBE
  loconet_rx_loco_adr,    // 0xBF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_4[32])(uint8_t, uint8_t, uint8_t, uint8_t) = {
  loconet_rx_dummy_4,     // 0xC0
  loconet_rx_dummy_4,     // 0xC1
  loconet_rx_dummy_4,     // 0xC2
  loconet_rx_dummy_4,     // 0xC3
  loconet_rx_dummy_4,     // 0xC4
  loconet_rx_dummy_4,     // 0xC5
  loconet_rx_dummy_4,     // 0xC6
  loconet_rx_dummy_4,     // 0xC7
  loconet_rx_dummy_4,     // 0xC8
  loconet_rx_dummy_4,     // 0xC9
  loconet_rx_dummy_4,     // 0xCA
  loconet_rx_dummy_4,     // 0xCB
  loconet_rx_dummy_4,     // 0xCC
  loconet_rx_dummy_4,     // 0xCD
  loconet_rx_dummy_4,     // 0xCE
  loconet_rx_dummy_4,     // 0xCF
  loconet_rx_dummy_4,     // 0xD0
  loconet_rx_dummy_4,     // 0xD1
  loconet_rx_dummy_4,     // 0xD2
  loconet_rx_dummy_4,     // 0xD3
  loconet_rx_dummy_4,     // 0xD4
  loconet_rx_dummy_4,     // 0xD5
  loconet_rx_dummy_4,     // 0xD6
  loconet_rx_dummy_4,     // 0xD7
  loconet_rx_dummy_4,     // 0xD8
  loconet_rx_dummy_4,     // 0xD9
  loconet_rx_dummy_4,     // 0xDA
  loconet_rx_dummy_4,     // 0xDB
  loconet_rx_dummy_4,     // 0xDC
  loconet_rx_dummy_4,     // 0xDD
  loconet_rx_dummy_4,     // 0xDE
  loconet_rx_dummy_4,     // 0xDF
};

//-----------------------------------------------------------------------------
void (* const ln_messages_n[32])(uint8_t*, uint8_t) = {
  loconet_rx_dummy_n,     // 0xE0
  loconet_rx_dummy_n,     // 0xE1
  loconet_rx_dummy_n,     // 0xE2
  loconet_rx_dummy_n,     // 0xE3
  loconet_rx_dummy_n,     // 0xE4
  loconet_rx_peer_xfer_,  // 0xE5
  loconet_rx_dummy_n,     // 0xE6
  loconet_rx_rd_sl_data_, // 0xE7
  loconet_rx_dummy_n,     // 0xE8
  loconet_rx_dummy_n,     // 0xE9
  loconet_rx_dummy_n,     // 0xEA
  loconet_rx_dummy_n,     // 0xEB
  loconet_rx_dummy_n,     // 0xEC
  loconet_rx_imm_packet_, // 0xED
  loconet_rx_dummy_n,     // 0xEE
  loconet_rx_wr_sl_data_, // 0xEF
  loconet_rx_dummy_n,     // 0xF0
  loconet_rx_dummy_n,     // 0xF1
  loconet_rx_dummy_n,     // 0xF2
  loconet_rx_dummy_n,     // 0xF3
  loconet_rx_dummy_n,     // 0xF4
  loconet_rx_dummy_n,     // 0xF5
  loconet_rx_dummy_n,     // 0xF6
  loconet_rx_dummy_n,     // 0xF7
  loconet_rx_dummy_n,     // 0xF8
  loconet_rx_dummy_n,     // 0xF9
  loconet_rx_dummy_n,     // 0xFA
  loconet_rx_dummy_n,     // 0xFB
  loconet_rx_dummy_n,     // 0xFC
  loconet_rx_dummy_n,     // 0xFD
  loconet_rx_dummy_n,     // 0xFE
  loconet_rx_dummy_n,     // 0xFF
};

//-----------------------------------------------------------------------------
// Read slot data (SL_RD_DATA)
// Handle special cases
static void loconet_rx_rd_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7C) { // Program task final
    loconet_rx_prog_task_final(&data[1], length - 1);
  } else { // Default handler
    loconet_rx_rd_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
// Write slot data (RW_SL_DATA)
// Handle special cases
static void loconet_rx_wr_sl_data_(uint8_t *data, uint8_t length) {
  if (data[0] == 0x7B) { // Fast clock
    loconet_rx_fast_clock(&data[1], length - 1);
  } else if (data[0] == 0x7C) { // Program task start
    loconet_rx_prog_task_start(&data[1], length - 1);
  } else { // Default handler
    loconet_rx_wr_sl_data(data, length);
  }
}

//-----------------------------------------------------------------------------
// Fix most significant bits for LNCV messages from an IntelliBox
static void loconet_fix_msb(uint8_t msb, uint8_t *data, uint8_t length)
{
  for (uint8_t index = 0; index < length; index++) {
    *data++ |= ((msb & (1 << index)) << (length - index));
  }
}

//-----------------------------------------------------------------------------
// Peer to peer transfer
// Handle special cases
static void loconet_rx_peer_xfer_(uint8_t *data, uint8_t length) {
  // Length 12 and source KPU, we take over the message
  if (length == 0x0C && data[0] == LOCONET_CV_SRC_KPU) {
    loconet_fix_msb(data[4], &data[5], 7);
    loconet_cv_process((LOCONET_CV_MSG_Type *)data, 0xE5);
  } else {
    // Call normal function
    loconet_rx_peer_xfer(data, length);
  }
}

//-----------------------------------------------------------------------------
// IMM Packet
// Handle special cases
static void loconet_rx_imm_packet_(uint8_t *data, uint8_t length) {
  // Length 12 and source KPU, we take over the message
  if (length == 0x0C && data[0] == LOCONET_CV_SRC_KPU) {
    loconet_fix_msb(data[4], &data[5], 7);
    loconet_cv_process((LOCONET_CV_MSG_Type *)data, 0xED);
  } else {
    // Call normal function
    loconet_rx_imm_packet(data, length);
  }
}

//-----------------------------------------------------------------------------
typedef union {
  struct {
    uint8_t NUMBER:5;
    uint8_t OPCODE:3;
  } bits;
  uint8_t byte;
} LOCONET_OPCODE_BYTE_Type;

#define LOCONET_OPCODE_FLAG_Pos 7
#define LOCONET_OPCODE_FLAG (0x01ul << LOCONET_OPCODE_FLAG_Pos)

//-----------------------------------------------------------------------------
uint8_t loconet_rx_process(void)
{
  // Get values from ringbuffer
  uint8_t *buffer = loconet_rx_ringbuffer.buffer;
  uint8_t reader = loconet_rx_ringbuffer.reader;
  uint8_t writer = loconet_rx_ringbuffer.writer;

  // Expect an opcode byte from ringbuffer
  LOCONET_OPCODE_BYTE_Type opcode;
  opcode.byte = buffer[reader];

  // If the reader is "ahead" of the writer in the ringbuffer,
  // add the size of the ringbuffer to the writer for its
  // fictive index.
  if (reader > writer) {
    writer += LOCONET_RX_RINGBUFFER_Size;
  }

  if (writer <= reader + 1) {
    // We want at least two bytes before we try to read the message
    return 0;
  }

  // If it's not an OPCODE byte, skip it
  if (!(opcode.byte & LOCONET_OPCODE_FLAG)) {
    loconet_rx_ringbuffer.reader = (reader + 1) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // New message
  uint8_t message_size = 0;
  switch (opcode.bits.OPCODE) {
    case 0x04:
      message_size = 2;
      break;
    case 0x05:
      message_size = 4;
      break;
    case 0x06:
      message_size = 6;
      break;
    case 0x07:
      message_size = buffer[(reader + 1) % LOCONET_RX_RINGBUFFER_Size];
      break;
  }

  // Check if the buffer contains no new opcodes (could happen due to colissions)
  uint8_t index_of_writer_or_eom = writer < (reader + message_size) ? writer : reader + message_size;
  for (uint8_t index = reader + 1; index < index_of_writer_or_eom; index++) {
    if (buffer[index % LOCONET_RX_RINGBUFFER_Size] & LOCONET_OPCODE_FLAG) {
      loconet_rx_ringbuffer.reader = index % LOCONET_RX_RINGBUFFER_Size;
      return 1; // Read the new message right away
    }
  }

  // Check if we have all the bytes for this message
  if (writer < reader + message_size) {
    return 0;
  }

  // Get bytes for passing (and build checksum)
  uint8_t data[message_size];
  uint8_t *p_data = data;
  uint8_t eom_index = reader + message_size;

  for (uint8_t index = reader; index < eom_index; index++) {
    *p_data++ = buffer[index % LOCONET_RX_RINGBUFFER_Size];
  }

  // Verify checksum (skip message if failed)
  if (loconet_calc_checksum(data, message_size)) {
    loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;
    return 0;
  }

  // Handle message
  switch(opcode.bits.OPCODE) {
    case 0x04: // Length 0
      (*ln_messages_0[opcode.bits.NUMBER])();
      break;
    case 0x05: // Length 2
      (*ln_messages_2[opcode.bits.NUMBER])(data[1], data[2]);
      break;
    case 0x06: // Length 4
      (*ln_messages_4[opcode.bits.NUMBER])(data[1], data[2], data[3], data[4]);
      break;
    case 0x07: // Variable length
      (*ln_messages_n[opcode.bits.NUMBER])(&data[2], message_size - 3);
      break;
  }

  // Advance reader
  loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;

  // Return that we have processed a message
  return 1;
}

//-----------------------------------------------------------------------------
// Dummy handlers
void loconet_rx_dummy_0(void)
{
}

void loconet_rx_dummy_2(uint8_t a, uint8_t b)
{
  (void)a;
  (void)b;
}

void loconet_rx_dummy_4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
}

void loconet_rx_dummy_n(uint8_t *d, uint8_t l)
{
  (void)d;
  (void)l;
}
