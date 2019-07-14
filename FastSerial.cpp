/*
 * FastSerial.cpp
 *
 *  Created on: Jul 13, 2019
 *      Author: caoyu
 */

#include "FastSerial.h"

// Defined in serial_api.c
extern UART_HandleTypeDef uart_handlers[];

namespace mbed{

static uint8_t uart_index = 0;

#if defined(USART1_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_1>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART2_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_2>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART3_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_3>::huart = &uart_handlers[uart_index++];
//template<> FastSerial<UART_3> *FastSerial<UART_3>::instance = NULL;
#endif

#if defined(UART4_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_4>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART4_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_5>::huart = &uart_handlers[uart_index++];
#endif

#if defined(UART5_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_5>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART5_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_5>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART6_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_6>::huart = &uart_handlers[uart_index++];
#endif

#if defined(UART7_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_7>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART7_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_7>::huart = &uart_handlers[uart_index++];
#endif

#if defined(UART8_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_8>::huart = &uart_handlers[uart_index++];
#endif

#if defined(USART8_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_8>::huart = &uart_handlers[uart_index++];
#endif

#if defined(UART9_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_9>::huart = &uart_handlers[uart_index++];
#endif

#if defined(UART10_BASE)
template<> UART_HandleTypeDef *FastSerial<UART_10>::huart = &uart_handlers[uart_index++];
#endif

#if defined(LPUART1_BASE)
template<> UART_HandleTypeDef *FastSerial<LPUART_1>::huart = &uart_handlers[uart_index++];
#endif

}

