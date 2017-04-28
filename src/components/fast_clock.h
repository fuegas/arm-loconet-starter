/**
 * @file loconet.h
 * @brief Loconet fast clock functionality
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * This is a basic implementation of the clock system for Loconet.
 * It reacts on the fast clock messages of loconet to sync the clock.
 * Internally, it updates the clock, until a new fast clock message
 * arrives, after which it resets the clock to the received message,
 * and starts ticking again, using the appropriate clock rate.
 *
 * Additionally, the system can run as a clock master, i.e., it sends
 * the fast clock messages itself. For this, set the appropriate values
 * using:
 *
 *    fast_clock_set_master(id1, id2, intermessage_delay)
 *
 * id1 and id2 are used to identify the clock, the intermessage_delay
 * states the seconds between every two clock messages.
 *
 * To return to slave mode, use
 *
 *     fast_clock_set_slave();
 *
 * To use the clock system, one should initialize a clock using
 *
 *    FAST_CLOCK_BUILD(timer)
 *
 * Where
 * - timer: the TIMER used for fast clock
 *
 * Make sure that the main loop calls the function
 *
 *     fast_clock_loop();
 *
 * as this function is the main function that handles the time update.
 *
 * To react on clock changes, you should use the following function
 *
 *     fast_clock_handle_update(FAST_CLOCK_TIME_Type time)
 *
 * It is triggered after every update of the minute counter.
 *
 * @author Jan Martijn van der Werf <janmartijn@slashdev.nl>
 */

// ------------------------------------------------------------------

#ifndef _LOCONET_FAST_CLOCK_H_
#define _LOCONET_FAST_CLOCK_H_

// Do we want this component?
#ifdef COMPONENTS_FAST_CLOCK

#include <stdbool.h>
#include <stdint.h>
#include "loconet/loconet_tx_messages.h"
#include "utils/logger.h"

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
} FAST_CLOCK_TIME_Type;


// ------------------------------------------------------------------
// Sets the fast_clock as master. It uses id1 and id2 for
// identifying the master in clock messages.
// The intermessage_delay is the delay in seconds between two
// messages
extern void fast_clock_set_master(uint8_t id1, uint8_t id2, uint8_t intermessage_delay);

// ------------------------------------------------------------------
// React on the clock messages as a slave, i.e. it does not send its
// own time messages
extern void fast_clock_set_slave(void);

// ------------------------------------------------------------------
// Sets the rate for the clock message. Only useful to use in case
// the system acts as a clock master.
extern void fast_clock_set_rate(uint8_t);

// ------------------------------------------------------------------
// Sets the time
extern void fast_clock_set_time(FAST_CLOCK_TIME_Type time);

// ------------------------------------------------------------------
// Returns the current time as FAST_CLOCK_TIME_Type
extern FAST_CLOCK_TIME_Type fast_clock_get_time(void);

// ------------------------------------------------------------------
// Returns the time as hour * 100 + minutes
extern uint16_t fast_clock_get_time_as_int(void);

// ------------------------------------------------------------------
// This is the IRQ function that is called after every clock cycle.
extern void fast_clock_irq(void);

// ------------------------------------------------------------------
// This is the function that should be added to the main loop, as it
// handles the actual time update.
extern void fast_clock_process(void);


// ------------------------------------------------------------------
extern void fast_clock_init(void);
extern void fast_clock_init_timer(Tc*, uint32_t, uint32_t, uint32_t);

#define FAST_CLOCK_BUILD(timer)                                               \
  void fast_clock_init(void)                                                  \
  {                                                                           \
    fast_clock_init_timer(                                                    \
      TC##timer,                                                              \
      PM_APBCMASK_TC##timer,                                                  \
      TC##timer##_GCLK_ID,                                                    \
      TC##timer##_IRQn                                                        \
    );                                                                        \
  }                                                                           \
  /* Handle timer interrupt */                                                \
  void irq_handler_tc##timer(void);                                           \
  void irq_handler_tc##timer(void)                                            \
  {                                                                           \
    /* Reset clock interrupt flag */                                          \
    TC##timer->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);                        \
    fast_clock_irq();                                                        \
  }                                                                           \

// ------------------------------------------------------------------
// Reacts on the fast clock messages to update the internal clock.
extern void loconet_rx_fast_clock(uint8_t *data, uint8_t length);

#else // COMPONENTS_FAST_CLOCK

#define fast_clock_set_master(...) do {} while(0)
#define fast_clock_set_slave(...) do {} while(0)
#define fast_clock_set_rate(...) do {} while(0)
#define fast_clock_set_time(...) do {} while(0)
#define fast_clock_get_time(...) do {} while(0)
#define fast_clock_get_time_as_int(...) do {} while(0)
#define fast_clock_irq(...) do {} while(0)
#define fast_clock_process(...) do {} while(0)
#define fast_clock_init(...) do {} while(0)
#define fast_clock_init_timer(...) do {} while(0)
#define FAST_CLOCK_BUILD(...)
#define loconet_rx_fast_clock(...) do {} while(0)

#endif // COMPONENTS_FAST_CLOCK

#endif // _LOCONET_FAST_CLOCK_H_
