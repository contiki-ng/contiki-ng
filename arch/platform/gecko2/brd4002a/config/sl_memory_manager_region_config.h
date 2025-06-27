/***************************************************************************//**
 * @file
 * @brief Memory Heap and stack size configuration file.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

// <<< Use Configuration Wizard in Context Menu >>>

#ifndef SL_MEMORY_MANAGER_REGION_CONFIG_H
#define SL_MEMORY_MANAGER_REGION_CONFIG_H

// <h> Memory configuration

// <o SL_STACK_SIZE> Stack size for the application.
// <i> Default: 4096
// <i> The stack size configured here will be used by the stack that the
// <i> application uses when coming out of a reset.
#ifndef SL_STACK_SIZE
#define SL_STACK_SIZE 4096
#endif
// </h>

// <<< end of configuration section >>>

#endif /* SL_MEMORY_MANAGER_REGION_CONFIG_H */
