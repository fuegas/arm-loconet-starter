/**
 * @file delay.h
 * @brief Adds functionality to use delays
 *
 * Adds methods to let the program wait for a certain amount of time. The time
 * to wait can be specified in seconds, milliseconds or microseconds. Waiting a
 * certain amount of microseconds is as precise as I would like but it comes
 * very close. The deviation for milliseconds and seconds are less than 1% but
 * are at least(!) the specified time.
 *
 * Note: The current implementation only works for a chip running on default
 * speed (8MHz).
 *
 * Note: Interrupts can cause a delay to take longer than specified as this
 * implementation uses busy waiting instead of a timer.
 *
 * \copyright Copyright 2017 /Dev. All rights reserved.
 * \license This project is released under MIT license.
 *
 * @author Ferdi van der Werf <ferdi@slashdev.nl>
 */

#ifndef UTILS_DELAY_H
#define UTILS_DELAY_H

#include "compiler.h"

OPTIMIZE_HIGH
RAMFUNC
extern void _cycle_delay(unsigned long n);

#if F_CPU == 8000000UL
#  define DELAY_MULT 1144
#else
#  error F_CPU not defined or unknown for DELAY_MULT
#endif

/**
 * @def delay_s
 * @brief Delay in seconds.
 * @param delay Delay in seconds
 */
#define delay_s(delay) \
  ((delay) ? _cycle_delay(delay * DELAY_MULT * 1000) : _cycle_delay(1))

/**
 * @def delay_ms
 * @brief Delay in milliseconds.
 * @param delay Delay in milliseconds
 */
#define delay_ms(delay) \
  ((delay) ? _cycle_delay(delay * DELAY_MULT) : _cycle_delay(1))

/**
 * @def delay_us
 * @brief Delay in microseconds.
 * @param delay Delay in microseconds
 */
#define delay_us(delay) \
  ((delay) ? _cycle_delay(delay * DELAY_MULT / 1000) : _cycle_delay(1))

#endif /* UTILS_DELAY_H */
