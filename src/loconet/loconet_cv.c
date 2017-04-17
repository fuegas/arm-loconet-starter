/**
 * @file loconet_cv.c
 * @brief Process Loconet CV messages
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#include "loconet_cv.h"

bool loconet_cv_programming;

//-----------------------------------------------------------------------------
void loconet_cv_prog_off_event_dummy(void);
void loconet_cv_prog_off_event_dummy(void)
{
}

__attribute__ ((weak, alias ("loconet_cv_prog_off_event_dummy"))) \
  void loconet_cv_prog_off_event(void);

//-----------------------------------------------------------------------------
uint8_t loconet_cv_write_allowed_dummy(uint16_t, uint16_t);
uint8_t loconet_cv_write_allowed_dummy(uint16_t lncv_number, uint16_t value)
{
  (void)lncv_number;
  (void)value;
  return LOCONET_CV_ACK_OK;
}

__attribute__ ((weak, alias ("loconet_cv_write_allowed_dummy"))) \
  uint8_t loconet_cv_write_allowed(uint16_t, uint16_t);

//-----------------------------------------------------------------------------
void loconet_cv_written_event_dummy(uint16_t lncv_number, uint16_t value);
void loconet_cv_written_event_dummy(uint16_t lncv_number, uint16_t value)
{
  (void)lncv_number;
  (void)value;
}

__attribute__ ((weak, alias ("loconet_cv_written_event_dummy"))) \
  void loconet_cv_written_event(uint16_t, uint16_t);

//-----------------------------------------------------------------------------
static void loconet_cv_response(LOCONET_CV_MSG_Type *msg)
{
  uint8_t resp_data[13];
  resp_data[0] = 15; // Length

  LOCONET_CV_MSG_Type *resp = (LOCONET_CV_MSG_Type*)&resp_data[1];
  resp->source = LOCONET_CV_SRC_MODULE;
  switch (msg->source) {
    case LOCONET_CV_SRC_KPU:
      resp->destination = LOCONET_CV_DST_UB_KPU;
      break;
    default:
      resp->destination = msg->source;
  }
  resp->request_id = LOCONET_CV_REQ_CFGREAD;
  resp->device_class = msg->device_class;
  resp->lncv_number = msg->lncv_number;
  resp->lncv_value = loconet_cv_get(msg->lncv_number);
  resp->flags = 0; // Always 0 for responses

  // Calculate Most Significant Bits
  resp->most_significant_bits = 0;
  for (uint8_t index = 0; index < 7; index++) {
    if (resp_data[6+index] & 0x80) {
      resp->most_significant_bits |= 0x01 << index;
      resp_data[6+index] &= 0x7F;
    }
  }

  // Send message
  loconet_tx_queue_n(0xE5, 1, resp_data, 13);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_on(LOCONET_CV_MSG_Type *msg)
{
  // lncv_number should be 0, and lncv_value should be 0xFFFF
  // or the address of the device.
  if (msg->lncv_number != 0 || (msg->lncv_value != 0xFFFF && msg->lncv_value != loconet_config.bit.ADDRESS)) {
    return;
  }

  // Start programming
  loconet_cv_programming = true;
  loconet_cv_response(msg);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_off(LOCONET_CV_MSG_Type *msg)
{
  (void)msg;
  loconet_cv_programming = false;
  loconet_cv_prog_off_event();
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_read(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  if (msg->lncv_number >= LOCONET_CV_NUMBERS) {
    loconet_tx_long_ack(opcode, LOCONET_CV_ACK_ERROR_OUTOFRANGE);
    return;
  }

  loconet_cv_response(msg);
}

//-----------------------------------------------------------------------------
static void loconet_cv_prog_write(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  // Write is not allowed if we're not in programming mode
  if (!loconet_cv_programming) {
    return;
  }

  loconet_tx_long_ack(opcode, loconet_cv_set(msg->lncv_number, msg->lncv_value));
}

//-----------------------------------------------------------------------------
void loconet_cv_process(LOCONET_CV_MSG_Type *msg, uint8_t opcode)
{
  if (msg->device_class != LOCONET_CV_DEVICE_CLASS) {
    return; // We only listen to our own device class
  }
  if (msg->flags == LOCONET_CV_FLG_PROG_ON) {
    loconet_cv_prog_on(msg);
  } else if (msg->flags == LOCONET_CV_FLG_PROG_OFF) {
    loconet_cv_prog_off(msg);
  } else if (msg->request_id == LOCONET_CV_REQ_CFGWRITE) {
    loconet_cv_prog_write(msg, opcode);
  } else {
    loconet_cv_prog_read(msg, opcode);
  }
}

//-----------------------------------------------------------------------------
uint16_t loconet_cv_get(uint16_t lncv_number)
{
  if (lncv_number >= LOCONET_CV_NUMBERS) {
    return 0xFFFF;
  }

  // Read the page from Eeprom
  uint16_t page_data[LOCONET_CV_PAGE_SIZE];
  eeprom_emulator_read_page(lncv_number / LOCONET_CV_PER_PAGE, (uint8_t *)page_data);

  // If lncv 1 does not contain the magic value (device class) then we assume the module has not
  // been configured by the user. Thus we use the initial address as address to listen to.
  if (lncv_number == 0 && page_data[1] != LOCONET_CV_DEVICE_CLASS) {
    return LOCONET_CV_INITIAL_ADDRESS;
  } else if (lncv_number == 2 && page_data[1] != LOCONET_CV_DEVICE_CLASS) {
    return LOCONET_CV_INITIAL_PRIORITY;
  } else {
    return page_data[lncv_number % LOCONET_CV_PER_PAGE];
  }
}

//-----------------------------------------------------------------------------
uint8_t loconet_cv_set(uint16_t lncv_number, uint16_t lncv_value)
{
  // Do not allow to write to number 1
  if (lncv_number == 1) {
    return LOCONET_CV_ACK_ERROR_READONLY;
  // Do not allow to write out of bounds
  } else if (lncv_number >= LOCONET_CV_NUMBERS) {
    return LOCONET_CV_ACK_ERROR_OUTOFRANGE;
  }

  // Is this write allowed?
  uint8_t ack = loconet_cv_write_allowed(lncv_number, lncv_value);

  uint8_t page = lncv_number / LOCONET_CV_PER_PAGE;
  uint8_t index = lncv_number % LOCONET_CV_PER_PAGE;

  uint16_t page_data[LOCONET_CV_PAGE_SIZE];
  eeprom_emulator_read_page(page, (uint8_t *)page_data);

  // Write value if it's allowed and different than the current stored value
  if (ack == LOCONET_CV_ACK_OK && lncv_value != page_data[index]) {
    page_data[index] = lncv_value;
    if (lncv_number == 0) {
      // Set magic value to detect we have configured the address.
      page_data[1] = LOCONET_CV_DEVICE_CLASS;
      // Change lncv_address
      loconet_config.bit.ADDRESS = lncv_value;
    } else if (lncv_number == 2) {
      loconet_config.bit.PRIORITY = lncv_value;
    }
    eeprom_emulator_write_page(page, (uint8_t *)page_data);
    eeprom_emulator_commit_page_buffer();
    loconet_cv_written_event(lncv_number, lncv_value);
  }

  return ack;
}

//-----------------------------------------------------------------------------
enum status_code loconet_cv_init(void)
{
  // Check if Eeprom is initialized
  struct eeprom_emulator_parameters eeprom_parameters;
  if (eeprom_emulator_get_parameters(&eeprom_parameters) == STATUS_ERR_NOT_INITIALIZED) {
    return STATUS_ERR_NOT_INITIALIZED;
  }

  // Get address and priority from Eeprom
  loconet_config.bit.ADDRESS = loconet_cv_get(0);
  loconet_config.bit.PRIORITY = loconet_cv_get(2);

  // Disable programming on init
  loconet_cv_programming = false;

  return STATUS_OK;
}
