/**
 * @file loconet_hw.h
 * @brief Loconet base functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

#ifndef _LOCONET_LOCONET_HW_H_
#define _LOCONET_LOCONET_HW_H_

#include <stdint.h>
#include <stdlib.h>
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
// Initializations
extern void loconet_init(void);
extern void loconet_init_usart(Sercom*, uint32_t, uint32_t, uint8_t, uint32_t);
extern void loconet_init_flank_detection(uint8_t);
extern void loconet_init_flank_timer(Tc*, uint32_t, uint32_t, uint32_t);
extern void loconet_save_tx_pin(PortGroup*, uint32_t);

//-----------------------------------------------------------------------------
// IRQ for sercom
extern void loconet_irq_sercom(void);

//-----------------------------------------------------------------------------
// Hardware actions
extern void loconet_flank_timer_delay(uint16_t);
extern void loconet_hw_enable_rx_tx(void);
extern void loconet_hw_disable_rx_tx(void);
extern void loconet_hw_force_tx_high(void);
extern void loconet_hw_enable_transmit(void);

//-----------------------------------------------------------------------------
// Loconet activity LED control
extern void loconet_activity_led_on(void);
extern void loconet_activity_led_off(void);

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

#endif // _LOCONET_LOCONET_HW_H_
