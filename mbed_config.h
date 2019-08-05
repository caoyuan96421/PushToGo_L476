/*
 * mbed SDK
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Automatically generated configuration file.
// DO NOT EDIT, content will be overwritten.

#ifndef __MBED_CONFIG_DATA__
#define __MBED_CONFIG_DATA__

// Configuration parameters
#define CLOCK_SOURCE                                         USE_PLL_HSE_XTAL                        // set by application[NUCLEO_L476RG]
#define LPTICKER_DELAY_TICKS                                 0                                       // set by target:NUCLEO_L476RG
#define MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE             256                                     // set by library:drivers
#define MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE             256                                     // set by library:drivers
#define MBED_CONF_EVENTS_PRESENT                             1                                       // set by library:events
#define MBED_CONF_EVENTS_SHARED_DISPATCH_FROM_APPLICATION    0                                       // set by library:events
#define MBED_CONF_EVENTS_SHARED_EVENTSIZE                    768                                     // set by library:events
#define MBED_CONF_EVENTS_SHARED_HIGHPRIO_EVENTSIZE           256                                     // set by library:events
#define MBED_CONF_EVENTS_SHARED_HIGHPRIO_STACKSIZE           1024                                    // set by library:events
#define MBED_CONF_EVENTS_SHARED_STACKSIZE                    2048                                    // set by library:events
#define MBED_CONF_EVENTS_USE_LOWPOWER_TIMER_TICKER           0                                       // set by library:events
#define MBED_CONF_FILESYSTEM_PRESENT                         1                                       // set by library:filesystem
#define MBED_CONF_PLATFORM_CRASH_CAPTURE_ENABLED             1                                       // set by library:platform[NUCLEO_L476RG]
#define MBED_CONF_PLATFORM_CTHUNK_COUNT_MAX                  8                                       // set by library:platform
#define MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE          1024000                                 // set by application[NUCLEO_L476RG]
#define MBED_CONF_PLATFORM_ERROR_ALL_THREADS_INFO            0                                       // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_FILENAME_CAPTURE_ENABLED    0                                       // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_HIST_ENABLED                0                                       // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_HIST_SIZE                   4                                       // set by library:platform
#define MBED_CONF_PLATFORM_ERROR_REBOOT_MAX                  1                                       // set by library:platform
#define MBED_CONF_PLATFORM_FATAL_ERROR_AUTO_REBOOT_ENABLED   1                                       // set by library:platform[NUCLEO_L476RG]
#define MBED_CONF_PLATFORM_FORCE_NON_COPYABLE_ERROR          0                                       // set by library:platform
#define MBED_CONF_PLATFORM_MAX_ERROR_FILENAME_LEN            16                                      // set by library:platform
#define MBED_CONF_PLATFORM_POLL_USE_LOWPOWER_TIMER           0                                       // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_BAUD_RATE                   1024000                                 // set by application[NUCLEO_L476RG]
#define MBED_CONF_PLATFORM_STDIO_BUFFERED_SERIAL             0                                       // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_CONVERT_NEWLINES            0                                       // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_CONVERT_TTY_NEWLINES        0                                       // set by library:platform
#define MBED_CONF_PLATFORM_STDIO_FLUSH_AT_EXIT               1                                       // set by library:platform
#define MBED_CONF_PLATFORM_USE_MPU                           1                                       // set by library:platform
#define MBED_CONF_PUSHTOGO_ACCELERATION_STEP_TIME            5                                       // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_CORRECTION_SPEED_SIDEREAL         32.0                                    // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_CORRECTION_TOLERANCE              0.03                                    // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_DEFAULT_GUIDE_SPEED_SIDEREAL      0.5                                     // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_DEFAULT_SLEW_SPEED                4.0                                     // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_DEFAULT_TRACK_SPEED_SIDEREAL      1.0                                     // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_MAX_CORRECTION_ANGLE              5                                       // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_MAX_GUIDE_TIME                    5000                                    // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_MIN_CORRECTION_TIME               5                                       // set by library:pushtogo
#define MBED_CONF_PUSHTOGO_MIN_SLEW_ANGLE                    0.3                                     // set by library:pushtogo
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE                512                                     // set by library:rtos
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE_DEBUG_EXTRA    128                                     // set by library:rtos[STM]
#define MBED_CONF_RTOS_IDLE_THREAD_STACK_SIZE_TICKLESS_EXTRA 256                                     // set by library:rtos
#define MBED_CONF_RTOS_MAIN_THREAD_STACK_SIZE                4096                                    // set by library:rtos
#define MBED_CONF_RTOS_PRESENT                               1                                       // set by library:rtos
#define MBED_CONF_RTOS_THREAD_STACK_SIZE                     4096                                    // set by library:rtos
#define MBED_CONF_RTOS_TIMER_THREAD_STACK_SIZE               768                                     // set by library:rtos
#define MBED_CONF_TARGET_BOOT_STACK_SIZE                     0x400                                   // set by library:rtos[*]
#define MBED_CONF_TARGET_CONSOLE_UART                        1                                       // set by target:Target
#define MBED_CONF_TARGET_DEEP_SLEEP_LATENCY                  3                                       // set by target:FAMILY_STM32
#define MBED_CONF_TARGET_INIT_US_TICKER_AT_BOOT              1                                       // set by target:FAMILY_STM32
#define MBED_CONF_TARGET_LPTICKER_LPTIM                      0                                       // set by application[NUCLEO_L476RG]
#define MBED_CONF_TARGET_LPTICKER_LPTIM_CLOCK                1                                       // set by target:FAMILY_STM32
#define MBED_CONF_TARGET_LPUART_CLOCK_SOURCE                 USE_LPUART_CLK_LSE|USE_LPUART_CLK_PCLK1 // set by target:FAMILY_STM32
#define MBED_CONF_TARGET_LSE_AVAILABLE                       1                                       // set by target:FAMILY_STM32
#define MBED_CONF_TARGET_MPU_ROM_END                         0x0fffffff                              // set by target:Target
#define MBED_CONF_TARGET_TICKLESS_FROM_US_TICKER             1                                       // set by application[NUCLEO_L476RG]
#define MBED_LFS_BLOCK_SIZE                                  512                                     // set by library:littlefs
#define MBED_LFS_ENABLE_INFO                                 0                                       // set by library:littlefs
#define MBED_LFS_INTRINSICS                                  1                                       // set by library:littlefs
#define MBED_LFS_LOOKAHEAD                                   512                                     // set by library:littlefs
#define MBED_LFS_PROG_SIZE                                   64                                      // set by library:littlefs
#define MBED_LFS_READ_SIZE                                   64                                      // set by library:littlefs
#define NVSTORE_ENABLED                                      1                                       // set by library:nvstore
#define NVSTORE_MAX_KEYS                                     32                                      // set by application[*]
// Macros
#define DEVICE_SPI_COUNT                                     3                                       // defined by application
#define MBEDTLS_PSA_HAS_ITS_IO                                                                       // defined by library:mbed-crypto
#define OS_ISR_FIFO_QUEUE                                    64                                      // defined by application
#define _RTE_                                                                                        // defined by library:rtos

#endif
