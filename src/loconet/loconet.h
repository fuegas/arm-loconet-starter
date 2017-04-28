/**
 * @file loconet.h
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * The loconet receiver can work with any of the SERCOM interfaces.
 * To build `loconet_init` you call `LOCONET_BUILD` with the
 * parameters how you want use the SERCOM interface. The
 * `LOCONET_BUILD` is structured as follows:
 * LOCONET_BUILD(pmux, sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad)
 * - pmux:    the PMUX channel you'd like to use (e.g. C)
 * - sercom:  the SERCOM interface number you'd like to use (e.g. 3)
 * - tx_port: the PORT of the TX output (e.g. A)
 * - tx_pin:  the PIN of the TX output (e.g. 14)
 * - rx_port: the PORT of the RX input (e.g. A)
 * - rx_pin:  the PIN of the RX input (e.g. 15)
 * - rx_pad:  the PAD to use for RX input on the SERCOM interface
 *     (e.g. 1, see datasheet)
 * - fl_port: the PORT of the FLANK detection (e.g. A)
 * - fl_pin:  the PIN of the FLANK detection (e.g. 13)
 * - fl_int:  the external interrupt associated to fl_pin
 *     (e.g. 1, see datasheet)
 * - fl_tmr:  the TIMER used for Carrier and Break detection
 * - led_port: the PORT of the activity LED
 * - led_pin:  the PIN of the activity LED
 *
 * Loconet uses CSMA/CD techniques to arbitrage and control network
 * access. The bit times are 60 uSecs or 16.66 KBaud +/- 1.5%. This
 * means that a Baudrate of 16.457 can be used as well. The data is
 * sent asynchonous using 1 start bit, 8 data bits and 1 stop bit.
 * The data is sent LSB first. Bytes can be sent back-to-back.
 *
 * Flank detection is used to start the delays for carrier detect
 * and break detect. The current code assumes the chip to run with
 * F_CPU 8 MHz.
 *
 * The LEDs for Tx and Rx will light up if a message is being sent or received.
 *
 * Before you can use the logger functions, initialize the logger
 * using `logger_init(baudrate);`.
 *
 * To process incoming messages from loconet, place `loconet_loop`
 * in your main loop, e.g.:
 *
 *   while (1) {
 *     loconet_loop();
 *   }
 *
 * To process the flank detection we need to use the external
 * interrups. But we don't want to claim all external interrupt
 * handling for loconet, so you'll have to catch the interrupt
 * and call our handler. If our handler returns 1, it has handled
 * the interrupt and you could skip the rest of your handling, e.g.:
 *
 *   void irq_handler_eic(void);
 *   void irq_handler_eic(void) {
 *     if (loconet_handle_eic()) {
 *       return;
 *     }
 *   }
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_H_
#define _LOCONET_LOCONET_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "samd20.h"
#include "hal_gpio.h"
#include "loconet_rx.h"
#include "loconet_tx.h"

//-----------------------------------------------------------------------------
// Give a warning if F_CPU is not 8MHz
#if F_CPU != 8000000
#warn "F_CPU is not 8000000, CD and BREAK timer wont work as expected!"
#endif

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
// Initializations
extern void loconet_init(void);
extern void loconet_init_usart(Sercom*, uint32_t, uint32_t, uint8_t, uint32_t);
extern void loconet_init_flank_detection(uint8_t);
extern void loconet_init_flank_timer(Tc*, uint32_t, uint32_t, uint32_t);
extern void loconet_save_tx_pin(PortGroup*, uint32_t);

//-----------------------------------------------------------------------------
// IRQs for flank rise / fall
extern void loconet_irq_flank_rise(void);
extern void loconet_irq_flank_fall(void);
// IRQ for timeout of timer
extern void loconet_irq_timer(void);
// IRQ for sercom
extern void loconet_irq_sercom(void);

extern uint8_t loconet_handle_eic(void);

//-----------------------------------------------------------------------------
// Loconet activity LED control
extern void loconet_activity_led_on(void);
extern void loconet_activity_led_off(void);

//-----------------------------------------------------------------------------
// Loconet loop to be used in the main loop
// Handles processing and sending of messages
extern void loconet_loop(void);

extern void loconet_sercom_enable_dre_irq(void);

//-----------------------------------------------------------------------------
// Calculate checksum of a message
extern uint8_t loconet_calc_checksum(uint8_t *data, uint8_t length);

// Macro for loconet_init and irq_handler_sercom<nr>
#define LOCONET_BUILD(pmux, sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad, fl_port, fl_pin, fl_int, fl_tmr, led_port, led_pin) \
  HAL_GPIO_PIN(LOCONET_TX, tx_port, tx_pin);                                  \
  HAL_GPIO_PIN(LOCONET_RX, rx_port, rx_pin);                                  \
  HAL_GPIO_PIN(LOCONET_FL, fl_port, fl_pin);                                  \
  HAL_GPIO_PIN(LOCONET_LED, led_port, led_pin);                               \
                                                                              \
  void loconet_init()                                                         \
  {                                                                           \
    /* Mark loconet as busy */                                                \
    loconet_status.reg |= LOCONET_STATUS_BUSY;                                \
    /* Set Tx pin as output */                                                \
    HAL_GPIO_LOCONET_TX_out();                                                \
    HAL_GPIO_LOCONET_TX_pmuxen(PORT_PMUX_PMUXE_##pmux##_Val);                 \
    HAL_GPIO_LOCONET_TX_clr();                                                \
    /* Set Rx pin as input */                                                 \
    HAL_GPIO_LOCONET_RX_in();                                                 \
    HAL_GPIO_LOCONET_RX_pmuxen(PORT_PMUX_PMUXE_##pmux##_Val);                 \
    /* Set Fl pin as input */                                                 \
    HAL_GPIO_LOCONET_FL_in();                                                 \
    HAL_GPIO_LOCONET_FL_pullup();                                             \
    HAL_GPIO_LOCONET_FL_pmuxen(PORT_PMUX_PMUXE_A_Val);                        \
    /* Set Tx and Rx LED as output */                                         \
    HAL_GPIO_LOCONET_LED_out();                                               \
    HAL_GPIO_LOCONET_LED_clr();                                               \
    /* Initialize usart */                                                    \
    loconet_init_usart(                                                       \
      SERCOM##sercom,                                                         \
      PM_APBCMASK_SERCOM##sercom,                                             \
      SERCOM##sercom##_GCLK_ID_CORE,                                          \
      rx_pad,                                                                 \
      SERCOM##sercom##_IRQn                                                   \
    );                                                                        \
    /* Initialize flank detection */                                          \
    loconet_init_flank_detection(                                             \
      fl_int                                                                  \
    );                                                                        \
    /* Initialize flank timer */                                              \
    loconet_init_flank_timer(                                                 \
      TC##fl_tmr,                                                             \
      PM_APBCMASK_TC##fl_tmr,                                                 \
      TC##fl_tmr##_GCLK_ID,                                                   \
      TC##fl_tmr##_IRQn                                                       \
    );                                                                        \
    /* Save tx pin */                                                         \
    loconet_save_tx_pin(                                                      \
      &PORT->Group[HAL_GPIO_PORT##tx_port],                                   \
      tx_pin                                                                  \
    );                                                                        \
  }                                                                           \
  uint8_t loconet_handle_eic(void) {                                          \
    /* Return if it's not our external pin to watch */                        \
    if (!EIC->INTFLAG.bit.EXTINT##fl_int) {                                   \
      return 0;                                                               \
    }                                                                         \
    /* Reset flag */                                                          \
    EIC->INTFLAG.reg |= EIC_INTFLAG_EXTINT##fl_int;                           \
    /* Determine RISE / FALL */                                               \
    if (HAL_GPIO_LOCONET_FL_read()) {                                         \
      loconet_irq_flank_rise();                                               \
    } else {                                                                  \
      loconet_irq_flank_fall();                                               \
    }                                                                         \
    return 1;                                                                 \
  }                                                                           \
  /* Handle timer interrupt */                                                \
  void irq_handler_tc##fl_tmr(void);                                          \
  void irq_handler_tc##fl_tmr(void) {                                         \
    /* Disable timer */                                                       \
    TC##fl_tmr->COUNT16.CTRLA.bit.ENABLE = 0;                                 \
    /* Reset clock interrupt flag */                                          \
    TC##fl_tmr->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);                       \
    /* Handle loconet timer */                                                \
    loconet_irq_timer();                                                      \
  }                                                                           \
  /* Handle received bytes */                                                 \
  void irq_handler_sercom##sercom(void);                                      \
  void irq_handler_sercom##sercom(void)                                       \
  {                                                                           \
    loconet_irq_sercom();                                                     \
  }                                                                           \
  void loconet_activity_led_on(void)                                          \
  {                                                                           \
    HAL_GPIO_LOCONET_LED_set();                                               \
  }                                                                           \
  void loconet_activity_led_off(void)                                         \
  {                                                                           \
    HAL_GPIO_LOCONET_LED_clr();                                               \
  }                                                                           \

#endif // _LOCONET_LOCONET_H_
