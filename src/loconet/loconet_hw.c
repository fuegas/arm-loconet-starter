/**
 * @file loconet_hw.c
 * @brief Loconet hardware functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */
#include "loconet_hw.h"

//-----------------------------------------------------------------------------
// Peripherals to use for communication
Sercom *loconet_sercom;
Tc *loconet_flank_timer;
PortGroup *loconet_tx_port;
uint32_t loconet_tx_pin;

//-----------------------------------------------------------------------------
// Initialize USART for loconet
void loconet_init_usart(Sercom *sercom, uint32_t pm_mask, uint32_t gclock_id, uint8_t rx_pad, uint32_t nvic_irqn)
{
  // Save sercom
  loconet_sercom = sercom;

  // Enable clock for peripheral, without prescaler
  PM->APBCMASK.reg |= pm_mask;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(gclock_id)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  /* CRTLA register:
   *   DORD:      0x01  LSB first
   *   CPOL:      0x00  Tx: rising, Rx: falling
   *   CMODE:     0x00  Async
   *   FORM:      0x00  USART Frame (without parity)
   *   RXPO:      0x03  Rx on PA15
   *   TXPO:      0x00  Tx on PA14
   *   IBON:      0x00  Ignored
   *   RUNSTDBY:  0x00  Ignored
   *   MODE:      0x01  USART with internal clock
   *   ENABLE:    0x01  Enabled (set at the end of the init)
   */
  loconet_sercom->USART.CTRLA.reg =
    SERCOM_USART_CTRLA_DORD
    | SERCOM_USART_CTRLA_MODE_USART_INT_CLK
    | SERCOM_USART_CTRLA_RXPO(rx_pad)
    | SERCOM_USART_CTRLA_TXPO_PAD0;

  /* CTRLB register:
   *   RXEN:      0x01  Enable Rx
   *   TXEN:      0x00  Only enable Tx when sending
   *   PMODE:     0x00  Ignored, parity is not used
   *   SFDE:      0x00  Ignored, chip does not go into standby
   *   SBMODE:    0x00  One stop bit
   *   CHSIZE:    0x00  Char size: 8 bits
   */
  loconet_sercom->USART.CTRLB.reg =
    SERCOM_USART_CTRLB_RXEN
    | SERCOM_USART_CTRLB_TXEN
    | SERCOM_USART_CTRLB_CHSIZE(0);

  uint64_t br = (uint64_t)65536 * (F_CPU - 16 * 16666) / F_CPU;
  loconet_sercom->USART.BAUD.reg = (uint16_t)br;

  /* INTERRUPTS register
   *   RXS:       0x00  No interrupt on Rx start
   *   RXC:       0x01  Interrupt on Rx complete
   *   TXC:       0x01  Interrupt on Tx complete
   *   DRE:       0x00  No interrupt on data registry empty
   */
  loconet_sercom->USART.INTENSET.reg =
    SERCOM_USART_INTENSET_RXC
    | SERCOM_USART_INTENSET_TXC;
  NVIC_EnableIRQ(nvic_irqn);

  // Enable USART
  loconet_sercom->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
// Initialize flank detection
void loconet_init_flank_detection(uint8_t fl_int)
{
  // Enable clock for external interrupts, without prescaler
  PM->APBAMASK.reg |= PM_APBAMASK_EIC;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_EIC)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  // Enable interrupt for external pin
  EIC->INTENSET.reg |= EIC_EVCTRL_EXTINTEO(0x01ul << fl_int);
  EIC->CONFIG[fl_int / 8].reg = EIC_CONFIG_SENSE0_BOTH << 4 * (fl_int % 8);
  NVIC_EnableIRQ(EIC_IRQn);

  // Enable external interrupts
  EIC->CTRL.reg |= EIC_CTRL_ENABLE;
}

//-----------------------------------------------------------------------------
// Initialize flank timer
void loconet_init_flank_timer(Tc *timer, uint32_t pm_tmr_mask, uint32_t gclock_tmr_id, uint32_t nvic_irqn)
{
  // Save timer
  loconet_flank_timer = timer;

  // Enable clock for flank timer, without prescaler
  PM->APBCMASK.reg |= pm_tmr_mask;
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_ID(gclock_tmr_id)
    | GCLK_CLKCTRL_CLKEN
    | GCLK_CLKCTRL_GEN(0);

  /* CTRLA register:
   *   PRESCSYNC: 0x02  RESYNC
   *   RUNSTDBY:        Ignored
   *   PRESCALER: 0x03  DIV8, each tick will be 1us
   *   WAVEGEN:   0x01  MFRQ, zero counter on match
   *   MODE:      0x00  16 bits timer
   */
  loconet_flank_timer->COUNT16.CTRLA.reg =
    TC_CTRLA_PRESCSYNC_RESYNC
    | TC_CTRLA_PRESCALER_DIV8
    | TC_CTRLA_WAVEGEN_MFRQ
    | TC_CTRLA_MODE_COUNT16;

  /* INTERRUPTS:
   *   Interrupt on match
   */
  loconet_flank_timer->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);
  NVIC_EnableIRQ(nvic_irqn);

  // Start the flank rise at least once
  loconet_irq_flank_rise();
}

//-----------------------------------------------------------------------------
// Save which pin is connected to TX
void loconet_save_tx_pin(PortGroup *group, uint32_t pin)
{
  loconet_tx_port = group;
  loconet_tx_pin = (0x01ul << pin);
}

//-----------------------------------------------------------------------------
// Handle sercom (usart) interrupt
void loconet_irq_sercom(void)
{
  // Rx complete
  if (loconet_sercom->USART.INTFLAG.bit.RXC) {
    if (loconet_status.bit.COLLISION_DETECTED) {
      // Ignore byte
      loconet_sercom->USART.DATA.reg;
      // Make sure Framing error status is cleared
      loconet_sercom->USART.STATUS.reg |= SERCOM_USART_STATUS_FERR;
    } else if (loconet_sercom->USART.STATUS.bit.FERR) {
      // Reset flag
      loconet_sercom->USART.STATUS.reg |= SERCOM_USART_STATUS_FERR;
      // Framing error -> Collision detected
      loconet_irq_collision();
    } else if (loconet_status.bit.TRANSMIT) {
      // Read own bytes to see if we have a collision
      if (loconet_sercom->USART.DATA.reg ^ loconet_tx_next_rx_byte()) {
        loconet_irq_collision();
      }
    } else {
      // Turn activity led on
      loconet_activity_led_on();
      // Get data from USART and place it in the ringbuffer
      loconet_rx_buffer_push(loconet_sercom->USART.DATA.reg);
      // Turn activity led off
      loconet_activity_led_off();
    }
  }

  // Tx complete
  if (loconet_sercom->USART.INTFLAG.bit.TXC) {
    // Clear TXC flag
    loconet_sercom->USART.INTFLAG.reg |= SERCOM_USART_INTFLAG_TXC;
    // Clear transmit state and free memory
    loconet_tx_stop();
    // Turn off activity led
    loconet_activity_led_off();
  }

  // Data register empty (TX)
  if (loconet_sercom->USART.INTFLAG.bit.DRE) {
    // Is a collision detected? Or is our message gone AWOL (due to a collision)?
    // If so: do not attempt to buffer bytes to send
    if (loconet_status.bit.COLLISION_DETECTED) {
      // Disable TRANSMIT
      loconet_status.bit.TRANSMIT = 0;
      // Disable Data Register Empty interrupt
      loconet_sercom->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
    } else if (loconet_status.bit.TRANSMIT) {
      // Do we have a message and do we have another byte to send?
      if (loconet_tx_finished()) {
        // Disable TRANSMIT
        loconet_status.bit.TRANSMIT = 0;
        // Disable Data Register Empty interrupt
        loconet_sercom->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
      } else {
        loconet_sercom->USART.DATA.reg = loconet_tx_next_tx_byte();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void loconet_flank_timer_delay(uint16_t delay_us) {
  // Set timer counter to 0
  loconet_flank_timer->COUNT16.COUNT.reg = 0;
  // Set timer match, 1200us
  loconet_flank_timer->COUNT16.CC[0].reg = delay_us;
  // Enable timer
  loconet_flank_timer->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
}

//-----------------------------------------------------------------------------
// Enable RX/TX
void loconet_hw_enable_rx_tx(void)
{
  // Release TX pin
  loconet_tx_port->OUTCLR.reg |= loconet_tx_pin;
  // Enable receiving and sending
  loconet_sercom->USART.CTRLB.reg |= SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
}

//-----------------------------------------------------------------------------
// Disable RX/TX
void loconet_hw_disable_rx_tx(void)
{
  loconet_sercom->USART.CTRLB.bit.RXEN = 0;
  loconet_sercom->USART.CTRLB.bit.TXEN = 0;
}

//-----------------------------------------------------------------------------
// Set Tx pin high
void loconet_hw_force_tx_high(void)
{
  loconet_tx_port->OUTSET.reg |= loconet_tx_pin;
}

//-----------------------------------------------------------------------------
// Enable data register empty interrupt so we can send data
void loconet_hw_enable_transmit(void)
{
  loconet_sercom->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
  // Turn on activity led
  loconet_activity_led_on();
}
