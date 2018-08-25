/**
 * @file loconet_tx.c
 * @brief Process sending Loconet messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include "loconet_tx.h"

//-----------------------------------------------------------------------------
// Loconet message/linked list definition
typedef struct MESSAGE {
  // Control fields
  uint8_t priority;
  struct MESSAGE *next;
  // Message fields
  uint8_t *data;
  uint8_t data_length;
  // Current index we're sending
  uint8_t tx_index;
  uint8_t rx_index;
} LOCONET_MESSAGE_Type;

static LOCONET_MESSAGE_Type *loconet_tx_queue = 0;
static LOCONET_MESSAGE_Type *loconet_tx_current = 0;

//-----------------------------------------------------------------------------
// Stop transmission and free memory of the message
void loconet_tx_stop(void)
{
  loconet_status.bit.TRANSMIT = 0;
  // We might not have a message due to collision detection
  if (loconet_tx_current) {
    free(loconet_tx_current->data);
    free(loconet_tx_current);
  }
}

//-----------------------------------------------------------------------------
void loconet_tx_reset_current_message_to_queue(void)
{
  // Reset transmit and receive index
  loconet_tx_current->tx_index = 0;
  loconet_tx_current->rx_index = 0;
  // Place message back at front of queue
  loconet_tx_current->next = loconet_tx_queue;
  loconet_tx_queue = loconet_tx_current;
  loconet_tx_current = 0;
}

//-----------------------------------------------------------------------------
uint8_t loconet_tx_next_rx_byte(void)
{
  if (!loconet_tx_current) {
    return 0xFF;
  }
  return loconet_tx_current->data[loconet_tx_current->rx_index++];
}

//-----------------------------------------------------------------------------
uint8_t loconet_tx_next_tx_byte(void)
{
  if (!loconet_tx_current) {
    return 0;
  }
  return loconet_tx_current->data[loconet_tx_current->tx_index++];
}

//-----------------------------------------------------------------------------
uint8_t loconet_tx_finished(void)
{
  // We're done if are the end of sending data
  if (loconet_tx_current && loconet_tx_current->tx_index < loconet_tx_current->data_length) {
    return 0;
  }
  // We're done
  return 1;
}

//-----------------------------------------------------------------------------
void loconet_tx_process(void)
{
  // Can we start transmission?
  if (!loconet_tx_queue) {
    // No message is in the queue
    return;
  } else if (loconet_status.reg & (LOCONET_STATUS_BUSY | LOCONET_STATUS_TRANSMIT | LOCONET_STATUS_COLLISION_DETECT)) {
    // We're not allowed to transmit, don't try to
    // This can be because:
    // - the line is busy
    // - we are already sending
    // - a collision is detection
    return;
  }

  // We have a queue, loconet is idle, so we can start sending
  loconet_status.reg |= LOCONET_STATUS_TRANSMIT;

  // Set which bytes need to be send
  loconet_tx_current = loconet_tx_queue;
  loconet_tx_queue = loconet_tx_current->next;
  loconet_tx_current->next = 0;

  // Start sending
  loconet_enable_transmit();

  return;
}

//-----------------------------------------------------------------------------
static void loconet_tx_enqueue(LOCONET_MESSAGE_Type *message)
{
  // If queue is empty, push it
  if (!loconet_tx_queue) {
    loconet_tx_queue = message;
    return;
  }

  // Pointers to previous and current node
  LOCONET_MESSAGE_Type *prev = loconet_tx_queue;
  LOCONET_MESSAGE_Type *curr = loconet_tx_queue->next;

  // Loop through message which are more important (lower priority)
  for (; curr && curr->priority < message->priority + 1; prev = curr, curr = curr->next);

  // Loop through messages which have the same priority.
  // All priority should be lowered by 1, and place the message at the end.
  for (; curr && curr->priority < message->priority + 2; curr->priority--, prev = curr, curr = curr->next);
  prev->next = message;
  message->next = curr;

  // All next messages should have their priorities decreased.
  // This prevents starvation of messages at the end of the queue
  for (; curr; curr->priority--, curr = curr->next);
}

//-----------------------------------------------------------------------------
// Build an empty message with the correct length
static LOCONET_MESSAGE_Type *loconet_build_message(uint8_t length)
{
  // Allocate space for linked list node
  LOCONET_MESSAGE_Type *message = malloc(sizeof(LOCONET_MESSAGE_Type));
  memset(message, 0, sizeof(LOCONET_MESSAGE_Type));
  // Allocate space for message bytes
  message->data = malloc(sizeof(uint8_t) * length);
  message->data_length = length;
  // Return the new message
  return message;
}

//-----------------------------------------------------------------------------
uint16_t loconet_tx_queue_size(void)
{
  uint16_t length = 0;
  LOCONET_MESSAGE_Type *curr = loconet_tx_queue;
  for (; curr; length++, curr = curr->next);
  return length;
}

//-----------------------------------------------------------------------------
void loconet_tx_queue_2(uint8_t opcode, uint8_t priority)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(2);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = loconet_calc_checksum(message->data, 1);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_4(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(4);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = a;
  message->data[2] = b;
  message->data[3] = loconet_calc_checksum(message->data, 3);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_6(uint8_t opcode, uint8_t priority, uint8_t  a, uint8_t b, uint8_t c, uint8_t d)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(6);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  message->data[1] = a;
  message->data[2] = b;
  message->data[3] = c;
  message->data[4] = d;
  message->data[5] = loconet_calc_checksum(message->data, 5);
  // Enqueue message
  loconet_tx_enqueue(message);
}

void loconet_tx_queue_n(uint8_t opcode, uint8_t priority, uint8_t *data, uint8_t length)
{
  LOCONET_MESSAGE_Type *message = loconet_build_message(length + 2);
  // Set priority
  message->priority = priority;
  // Fill message
  message->data[0] = opcode;
  for(uint8_t idx = 0; idx < length; message->data[idx+1] = data[idx], idx++);
  message->data[length+1] = loconet_calc_checksum(message->data, length + 1);
  // Enqueue message
  loconet_tx_enqueue(message);
}
