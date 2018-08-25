/**
 * \file
 *
 * \brief Commonly used includes, types and macros.
 *
 * Copyright (c) 2010-2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef UTILS_COMPILER_H
#define UTILS_COMPILER_H

/**
 * \defgroup group_sam_utils Compiler abstraction layer and code utilities
 *
 * Compiler abstraction layer and code utilities for AT91SAM.
 * This module provides various abstraction layers and utilities to make code compatible between different compilers.
 *
 * \{
 */
#include <stddef.h>

#if (defined __ICCARM__)
#  include <intrinsics.h>
#endif

//_____ D E C L A R A T I O N S ____________________________________________

#ifndef __ASSEMBLY__ // Not defined for assembling.

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __ICCARM__
/*! \name Compiler Keywords
 *
 * Port of some keywords from GCC to IAR Embedded Workbench.
 */
//! @{
#define __asm__             asm
#define __inline__          inline
#define __volatile__
//! @}

#endif

#define FUNC_PTR                            void *
/**
 * \def UNUSED
 * \brief Marking \a v as a unused parameter or value.
 */
#define UNUSED(v)          (void)(v)

/**
 * \def unused
 * \brief Marking \a v as a unused parameter or value.
 */
#define unused(v)          do { (void)(v); } while(0)

/**
 * \def barrier
 * \brief Memory barrier
 */
#define barrier()          __DMB()

/**
 * \brief Emit the compiler pragma \a arg.
 *
 * \param arg The pragma directive as it would appear after \e \#pragma
 * (i.e. not stringified).
 */
#define COMPILER_PRAGMA(arg)            _Pragma(#arg)

/**
 * \def COMPILER_PACK_SET(alignment)
 * \brief Set maximum alignment for subsequent struct and union
 * definitions to \a alignment.
 */
#define COMPILER_PACK_SET(alignment)   COMPILER_PRAGMA(pack(alignment))

/**
 * \def COMPILER_PACK_RESET()
 * \brief Set default alignment for subsequent struct and union
 * definitions.
 */
#define COMPILER_PACK_RESET()          COMPILER_PRAGMA(pack())


/**
 * \brief Set aligned boundary.
 */
#if (defined __GNUC__) || (defined __CC_ARM)
#   define COMPILER_ALIGNED(a)    __attribute__((__aligned__(a)))
#elif (defined __ICCARM__)
#   define COMPILER_ALIGNED(a)    COMPILER_PRAGMA(data_alignment = a)
#endif

/**
 * \brief Set word-aligned boundary.
 */
#if (defined __GNUC__) || defined(__CC_ARM)
#define COMPILER_WORD_ALIGNED    __attribute__((__aligned__(4)))
#elif (defined __ICCARM__)
#define COMPILER_WORD_ALIGNED    COMPILER_PRAGMA(data_alignment = 4)
#endif

/**
 * \def __always_inline
 * \brief The function should always be inlined.
 *
 * This annotation instructs the compiler to ignore its inlining
 * heuristics and inline the function no matter how big it thinks it
 * becomes.
 */
#if defined(__CC_ARM)
#   define __always_inline   __forceinline
#elif (defined __GNUC__)
#ifdef __always_inline
# undef __always_inline
#endif
# define __always_inline   inline __attribute__((__always_inline__))
#elif (defined __ICCARM__)
# define __always_inline   _Pragma("inline=forced")
#endif

/**
 * \def __no_inline
 * \brief The function should not be inlined.
 *
 * This annotation instructs the compiler to ignore its inlining
 * heuristics and not inline the function.
 */
#if defined(__CC_ARM)
#   define __no_inline   __attribute__((noinline))
#elif (defined __GNUC__)
# define __no_inline   __attribute__((__noinline__))
#elif (defined __ICCARM__)
# define __no_inline   _Pragma("inline=never")
#endif

/* Define WEAK attribute */
#if defined   ( __CC_ARM   ) /* Keil µVision 4 */
#   define WEAK __attribute__ ((weak))
#elif defined ( __ICCARM__ ) /* IAR Ewarm 5.41+ */
#   define WEAK __weak
#elif defined (  __GNUC__  ) /* GCC CS3 2009q3-68 */
#   define WEAK __attribute__ ((weak))
#endif

/* Define NO_INIT attribute */
#if defined   ( __CC_ARM   )
#   define NO_INIT __attribute__((zero_init))
#elif defined ( __ICCARM__ )
#   define NO_INIT __no_init
#elif defined (  __GNUC__  )
#   define NO_INIT __attribute__((section(".no_init")))
#endif

/* Define RAMFUNC attribute */
#if defined   ( __CC_ARM   ) /* Keil µVision 4 */
#   define RAMFUNC __attribute__ ((section(".ramfunc")))
#elif defined ( __ICCARM__ ) /* IAR Ewarm 5.41+ */
#   define RAMFUNC __ramfunc
#elif defined (  __GNUC__  ) /* GCC CS3 2009q3-68 */
#   define RAMFUNC __attribute__ ((section(".ramfunc")))
#endif

/* Define OPTIMIZE_HIGH attribute */
#if defined   ( __CC_ARM   ) /* Keil µVision 4 */
#   define OPTIMIZE_HIGH _Pragma("O3")
#elif defined ( __ICCARM__ ) /* IAR Ewarm 5.41+ */
#   define OPTIMIZE_HIGH _Pragma("optimize=high")
#elif defined (  __GNUC__  ) /* GCC CS3 2009q3-68 */
#   define OPTIMIZE_HIGH __attribute__((optimize("s")))
#endif

#endif /* __ASSEMBLY__ */

#endif /* UTILS_COMPILER_H */
