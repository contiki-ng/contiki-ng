/***************************************************************************//**
 * @file
 * @brief Application assert configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef APP_ASSERT_CONFIG_H
#define APP_ASSERT_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <e APP_ASSERT_ENABLE> Assert component
// <i> Enables Assert.
#define APP_ASSERT_ENABLE      1

// <q APP_ASSERT_SCHEDULE_LOCK> Enable schedule lock
// <i> Enables schedule locking under OS
// <i> When both schedule lock and breakpoint are present,
// <i> breakpoint will be used
#define APP_ASSERT_SCHEDULE_LOCK      0

// <q APP_ASSERT_BREAKPOINT> Enable breakpoint insertion
// <i> Inserts breakpoint to assert locations, which can halt the application
// <i> upon failure. While debugging this feature stops the execution
// <i> and jumps to the location of the assertion in case it fails.
// <i> When both schedule lock and breakpoint are present,
// <i> breakpoint will be used
#define APP_ASSERT_BREAKPOINT      1

// <e APP_ASSERT_LOG_ENABLE> Logging
// <i> Enables logging for assert.
#define APP_ASSERT_LOG_ENABLE      1

// <q APP_ASSERT_TRACE_ENABLE> Enable trace
// <i> Enables trace for assert.
#define APP_ASSERT_TRACE_ENABLE      1

// </e>

// </e>

// <<< end of configuration section >>>

#endif // APP_ASSERT_CONFIG_H
