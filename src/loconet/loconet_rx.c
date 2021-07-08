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
// Internal function used to notify all listeners to some opcode
void loconet_rx_notify(uint8_t opcode, uint8_t*, uint8_t);


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

  // Call notifier, but first check whether the OPCODE is of variable length
  if (opcode.bits.OPCODE == 0x07) {
    loconet_rx_notify(opcode.bits.NUMBER, &data[2], message_size - 3);
  } else {
    loconet_rx_notify(opcode.bits.NUMBER, &data[1], message_size - 2);
  }

  // Advance reader
  loconet_rx_ringbuffer.reader = (reader + message_size) % LOCONET_RX_RINGBUFFER_Size;

  // Return that we have processed a message
  return 1;
}

//-----------------------------------------------------------------------------
// Observer pattern for reacting on incoming messages, based on their OPCODE.

// Implemented using a linked list
typedef struct LOCONET_RX_OBSERVERITEM {
  // Opcode to register for
  uint8_t opcode;
  // Callback function
  void (*callback)(uint8_t*, uint8_t);
  // Next list item
  struct LOCONET_RX_OBSERVERITEM *next;
} LOCONET_RX_OBSERVERITEM_Type;

// The first element in the linked list of observers
LOCONET_RX_OBSERVERITEM_Type *loconet_rx_observer_list_first = 0;
// The last element of the list
LOCONET_RX_OBSERVERITEM_Type *loconet_rx_observer_list_last = 0;

// Size of the linked list
uint8_t loconet_rx_observer_listsize = 0;

void loconet_rx_register_callback(uint8_t opcode, void (*callback)(uint8_t*, uint8_t))
{
  LOCONET_RX_OBSERVERITEM_Type *item = malloc(sizeof(LOCONET_RX_OBSERVERITEM_Type));
  memset(item, 0, sizeof(LOCONET_RX_OBSERVERITEM_Type));

  item->opcode = opcode;
  item->callback = callback;

  if (loconet_rx_observer_listsize == 0) {
    loconet_rx_observer_list_first = item;
  } else {
    loconet_rx_observer_list_last->next = item;
  }
  loconet_rx_observer_list_last = item;

  loconet_rx_observer_listsize++;
}

void loconet_rx_unregister_callback(uint8_t opcode, void (*callback)(uint8_t*, uint8_t))
{
  LOCONET_RX_OBSERVERITEM_Type *cur = loconet_rx_observer_list_first;
  LOCONET_RX_OBSERVERITEM_Type *prev = 0;

  // Iterate the linked list, and remove all elements that are similar
  while(cur) {
    if (cur->opcode == opcode &&  cur->callback == callback) {
      // Remove item
      LOCONET_RX_OBSERVERITEM_Type *del = cur;
      if (prev) {
        prev->next = cur->next;
      } else {
        loconet_rx_observer_list_first = cur->next;
      }
      free(del);
      cur = prev;
      loconet_rx_observer_listsize--;
    } // End remove item
    prev = cur;
    cur = cur->next;
  }
}

void loconet_rx_notify(uint8_t opcode, uint8_t* arr, uint8_t size)
{
  if (loconet_rx_observer_listsize == 0) {
    return;
  }

  LOCONET_RX_OBSERVERITEM_Type *cur = loconet_rx_observer_list_first;

  while(cur) {
    if (cur->opcode == opcode) {
      cur->callback(arr, size);
    }
    cur = cur->next;
  }
}
