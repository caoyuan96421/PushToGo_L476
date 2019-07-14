/*
 * FastSerial.h
 *
 *  Created on: Jul 13, 2019
 *      Author: caoyu
 */

#ifndef FASTSERIAL_H_
#define FASTSERIAL_H_

#include "platform/FileHandle.h"
#include "SerialBase.h"
#include "hal/serial_api.h"
#include "platform/CircularBuffer.h"
#include "platform/NonCopyable.h"
#include "platform/PlatformMutex.h"
#include "rtos/ThisThread.h"

namespace mbed {

#ifndef MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE
#define MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE  256
#endif

#ifndef MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE
#define MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE  256
#endif

inline IRQn_Type serial_get_irq_n(UARTName uart_name) {
	IRQn_Type irq_n;

	switch (uart_name) {
#if defined(USART1_BASE)
	case UART_1:
		irq_n = USART1_IRQn;
		break;
#endif
#if defined(USART2_BASE)
	case UART_2:
		irq_n = USART2_IRQn;
		break;
#endif
#if defined(USART3_BASE)
	case UART_3:
		irq_n = USART3_IRQn;
		break;
#endif
#if defined(UART4_BASE)
	case UART_4:
		irq_n = UART4_IRQn;
		break;
#endif
#if defined(UART5_BASE)
	case UART_5:
		irq_n = UART5_IRQn;
		break;
#endif
#if defined(LPUART1_BASE)
	case LPUART_1:
		irq_n = LPUART1_IRQn;
		break;
#endif
	default:
		irq_n = (IRQn_Type) 0;
	}

	return irq_n;
}

inline DMA_Channel_TypeDef *serial_get_dma_tx(UARTName uart_name,
		uint32_t &request) {
	DMA_Channel_TypeDef *dma;

	switch (uart_name) {
#if defined(USART1_BASE)
	case UART_1:
		dma = DMA1_Channel4;
		request = 2;
		break;
#endif
#if defined(USART2_BASE)
	case UART_2:
		dma = DMA1_Channel7;
		request = 2;
		break;
#endif
#if defined(USART3_BASE)
	case UART_3:
		dma = DMA1_Channel2;
		request = 2;
		break;
#endif
#if defined(UART4_BASE)
	case UART_4:
		dma = DMA2_Channel3;
		request = 2;
		break;
#endif
#if defined(UART5_BASE)
	case UART_5:
		dma = DMA2_Channel1;
		request = 2;
		break;
#endif
#if defined(LPUART1_BASE)
	case LPUART_1:
		dma = DMA2_Channel6;
		request = 4;
		break;
#endif
	default:
		dma = (DMA_Channel_TypeDef *) 0;
	}

	return dma;
}

inline IRQn_Type dma_irqn(DMA_Channel_TypeDef *dma) {
	switch ((uint32_t) dma) {

	case (const uint32_t) DMA1_Channel1_BASE:
		return DMA1_Channel1_IRQn;
	case (const uint32_t) DMA1_Channel2_BASE:
		return DMA1_Channel2_IRQn;
	case (const uint32_t) DMA1_Channel3_BASE:
		return DMA1_Channel3_IRQn;
	case (const uint32_t) DMA1_Channel4_BASE:
		return DMA1_Channel4_IRQn;
	case (const uint32_t) DMA1_Channel5_BASE:
		return DMA1_Channel5_IRQn;
	case (const uint32_t) DMA1_Channel6_BASE:
		return DMA1_Channel6_IRQn;
	case (const uint32_t) DMA1_Channel7_BASE:
		return DMA1_Channel7_IRQn;
	case (const uint32_t) DMA2_Channel1_BASE:
		return DMA2_Channel1_IRQn;
	case (const uint32_t) DMA2_Channel2_BASE:
		return DMA2_Channel2_IRQn;
	case (const uint32_t) DMA2_Channel3_BASE:
		return DMA2_Channel3_IRQn;
	case (const uint32_t) DMA2_Channel4_BASE:
		return DMA2_Channel4_IRQn;
	case (const uint32_t) DMA2_Channel5_BASE:
		return DMA2_Channel5_IRQn;
	case (const uint32_t) DMA2_Channel6_BASE:
		return DMA2_Channel6_IRQn;
	case (const uint32_t) DMA2_Channel7_BASE:
		return DMA2_Channel7_IRQn;
	default:
		return (IRQn_Type) 0;
	}
}

#define FASTSERIAL_FLAG_RX 0x01
#define FASTSERIAL_FLAG_TX 0x02
#define FASTSERIAL_FLAG_TX_COMPLETE 0x04

extern uint32_t serial_isr_flag;

template<UARTName uart>
class FastSerial: private SerialBase, public FileHandle, private NonCopyable<
		FastSerial<uart> > {
public:

	/** Create a FastSerial port, connected to the specified transmit and receive pins, with a particular baud rate.
	 *  @param tx Transmit pin
	 *  @param rx Receive pin
	 *  @param baud The baud rate of the serial port (optional, defaults to MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE)
	 */
	FastSerial(PinName tx, PinName rx, int baud =
	MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE) :
			SerialBase(tx, rx, baud), _blocking(true), rx_head(rx_start), rx_tail(
					rx_start), rx_end(
					rx_start + MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE), tx_head(
					tx_start), tx_tail(tx_start), tx_end(
					tx_start + MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE) {
		//  All taken care by SerialBase constructor
		MBED_ASSERT(_serial.serial.uart == uart); // Make sure we're dealing with the same thing

		// DMA initialization
		DMA_InitTypeDef &d = tx_dma.Init;
		d.Direction = DMA_MEMORY_TO_PERIPH;
		d.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		d.MemInc = DMA_MINC_ENABLE;
		d.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		d.PeriphInc = DMA_PINC_DISABLE;
		d.Mode = DMA_NORMAL;
		d.Priority = DMA_PRIORITY_MEDIUM;

		tx_dma.Instance = serial_get_dma_tx(uart, d.Request);
		huart->hdmatx = &tx_dma;
		tx_dma.Parent = huart;

		if ((uint32_t) tx_dma.Instance < (uint32_t)(DMA2_Channel1)){
			__HAL_RCC_DMA1_CLK_ENABLE();
		}
		else{
			__HAL_RCC_DMA2_CLK_ENABLE();
		}

		HAL_DMA_Init(&tx_dma);

		//  UART IRQ
		IRQn_Type irq_n = serial_get_irq_n(uart);
		NVIC_ClearPendingIRQ(irq_n);
		NVIC_DisableIRQ(irq_n);
		NVIC_SetPriority(irq_n, 10);
		NVIC_SetVector(irq_n, (uint32_t) irq_entry);
		NVIC_EnableIRQ(irq_n);

		// DMA IRQ
		irq_n = dma_irqn(tx_dma.Instance);
		NVIC_ClearPendingIRQ(irq_n);
		NVIC_DisableIRQ(irq_n);
		NVIC_SetPriority(irq_n, 10);
		NVIC_SetVector(irq_n, (uint32_t) irq_dma_tx_entry);
		NVIC_EnableIRQ(irq_n);

		instance = this;

		// Enable RX immediately
		HAL_UART_Receive_IT(huart, (uint8_t*) rx_start,
		MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE - 1);
	}

	virtual ~FastSerial() {
	}

	/** Equivalent to POSIX poll(). Derived from FileHandle.
	 *  Provides a mechanism to multiplex input/output over a set of file handles.
	 */
	virtual short poll(short events) const {
		return 0;
	}

	/** Write the contents of a buffer to a file
	 *
	 *  Follows POSIX semantics:
	 *
	 * * if blocking, block until all data is written
	 * * if no data can be written, and non-blocking set, return -EAGAIN
	 * * if some data can be written, and non-blocking set, write partial
	 *
	 *  @param buffer   The buffer to write from
	 *  @param length   The number of bytes to write
	 *  @return         The number of bytes written, negative error on failure
	 */
	virtual ssize_t write(const void *buffer, size_t length) {
		tx_mtx.lock();
		core_util_critical_section_enter();

		if (!_blocking) {
			size_t capacity = capacity_tx();
			if (capacity == 0) {
				core_util_critical_section_exit();
				tx_mtx.unlock();
				return -EAGAIN; // Nothing can be written
			} else {
				length = (length < capacity) ? length : capacity; // Write partial
			}
		}
		unsigned char *p = (unsigned char *) buffer;

		while (length > 0) {
			size_t capacity = capacity_tx();
			size_t max_len = (length < capacity) ? length : capacity; // Maximum transfer
			for (size_t i = 0; i < max_len; i++) {
				*tx_tail++ = *p++;
				if (tx_tail == tx_end)
					tx_tail = tx_start;
			}
			length -= max_len;

			if (!(HAL_UART_GetState(huart) & 0x01)) {
				// No ongoing transmission
				size_t tx_len;
				if (tx_tail >= tx_head) {
					tx_len = tx_tail - tx_head;
				} else {
					tx_len = tx_end - tx_head; // Should not wrap, the IRQ will take care of the rest of the transmission
				}

//				HAL_UART_Transmit_IT(huart, (uint8_t*) tx_head, tx_len); // Start transmission
				HAL_UART_Transmit_DMA(huart, (uint8_t*) tx_head, tx_len);
			}

			if (_blocking) {
				// Wait for transmission to finish
				// Due to the mutex, no other data will be added further.
				// So just wait for the current transmission to finish
				while (!tx_empty()) {
					core_util_critical_section_exit();
					rtos::ThisThread::yield(); // TODO
					core_util_critical_section_enter();
				}
			}

		}
		core_util_critical_section_exit();
		tx_mtx.unlock();

		return p - (unsigned char*) buffer;
	}

	/** Read the contents of a file into a buffer
	 *
	 *  Follows POSIX semantics:
	 *
	 *  * if no data is available, and non-blocking set return -EAGAIN
	 *  * if no data is available, and blocking set, wait until data is available
	 *  * If any data is available, call returns immediately
	 *
	 *  @param buffer   The buffer to read in to
	 *  @param length   The number of bytes to read
	 *  @return         The number of bytes read, 0 at end of file, negative error on failure
	 */
	virtual ssize_t read(void *buffer, size_t length) {
		rx_mtx.lock();
		core_util_critical_section_enter();
		unsigned char *p = (unsigned char *) buffer;
		while (length > 0) {
			size_t available = available_rx();
			// First read all available data up to length
			size_t copied = available < length ? available : length;
			for (unsigned int i = 0; i < copied; i++) {
				*p++ = *rx_head++;
				if (rx_head == rx_end)
					rx_head = rx_start;
			}
			length -= copied;
			if (length == 0)
				break;

			// Still not done yet. Wait or not depend on _blocking
			if (!_blocking) {
				core_util_critical_section_exit();
				rx_mtx.unlock();
				return copied == 0 ? -EAGAIN : copied;
			} else {
				// Wait for data to be available for copy
				// Init receive if not so
				if (!(HAL_UART_GetState(huart) & 0x02)) {
					// No ongoing reception
					size_t rx_len;
					if (rx_tail < rx_head) {
						rx_len = rx_head - rx_tail - 1;
					} else {
						if (rx_head != rx_start)
							rx_len = rx_end - rx_tail;
						else
							rx_len = rx_end - rx_tail - 1; // If rx_head=0, rx_tail=N-1, the queue is full and nothing can be read
					}

					HAL_UART_Receive_IT(huart, (uint8_t*) rx_tail, rx_len); // Start receive
				}
				while (rx_empty()) {
					core_util_critical_section_exit();
					rtos::ThisThread::sleep_for(1); // TODO Improve
					core_util_critical_section_enter();
				}
			}
		}

		core_util_critical_section_exit();
		rx_mtx.unlock();

		return p - (unsigned char*) buffer;
	}

	/** Close a file
	 *
	 *  @return         0 on success, negative error code on failure
	 */
	virtual int close() {
		return 0;
	}

	/** Check if the file in an interactive terminal device
	 *
	 *  @return         True if the file is a terminal
	 *  @return         False if the file is not a terminal
	 *  @return         Negative error code on failure
	 */
	virtual int isatty() {
		return 1;
	}

	/** Move the file position to a given offset from from a given location
	 *
	 * Not valid for a device type FileHandle like ModSerial.
	 * In case of ModSerial, returns ESPIPE
	 *
	 *  @param offset   The offset from whence to move to
	 *  @param whence   The start of where to seek
	 *      SEEK_SET to start from beginning of file,
	 *      SEEK_CUR to start from current position in file,
	 *      SEEK_END to start from end of file
	 *  @return         The new offset of the file, negative error code on failure
	 */
	virtual off_t seek(off_t offset, int whence) {
		return -ESPIPE;
	}

	/** Flush any buffers associated with the file
	 *
	 *  @return         0 on success, negative error code on failure
	 */
	virtual int sync() {
		while (!tx_empty()) {
			rtos::ThisThread::sleep_for(1);
		}
		return 0;
	}

	/** Set blocking or non-blocking mode
	 *  The default is blocking.
	 *
	 *  @param blocking true for blocking mode, false for non-blocking mode.
	 */
	virtual int set_blocking(bool blocking) {
		_blocking = blocking;
		return 0;
	}

	/** Check current blocking or non-blocking mode for file operations.
	 *
	 *  @return             true for blocking mode, false for non-blocking mode.
	 */
	virtual bool is_blocking() const {
		return _blocking;
	}

	/** Register a callback on state change of the file.
	 *
	 *  The specified callback will be called on state changes such as when
	 *  the file can be written to or read from.
	 *
	 *  The callback may be called in an interrupt context and should not
	 *  perform expensive operations.
	 *
	 *  Note! This is not intended as an attach-like asynchronous api, but rather
	 *  as a building block for constructing  such functionality.
	 *
	 *  The exact timing of when the registered function
	 *  is called is not guaranteed and susceptible to change. It should be used
	 *  as a cue to make read/write/poll calls to find the current state.
	 *
	 *  @param func     Function to call on state change
	 */
	virtual void sigio(Callback<void()> func) {
		core_util_critical_section_enter();
		_sigio_cb = func;
		if (_sigio_cb) {
			short current_events = poll(0x7FFF);
			if (current_events) {
				_sigio_cb();
			}
		}
		core_util_critical_section_exit();
	}

	// Expose private SerialBase::Parity as ModSerial::Parity
	using SerialBase::Parity;
	// In C++11, we wouldn't need to also have using directives for each value
	using SerialBase::None;
	using SerialBase::Odd;
	using SerialBase::Even;
	using SerialBase::Forced1;
	using SerialBase::Forced0;
	using SerialBase::format;
	using SerialBase::baud;

	/** Set the transmission format used by the serial port
	 *
	 *  @param bits The number of bits in a word (5-8; default = 8)
	 *  @param parity The parity used (None, Odd, Even, Forced1, Forced0; default = None)
	 *  @param stop_bits The number of stop bits (1 or 2; default = 1)
	 */
//	void set_format(int bits = 8, Parity parity = None, int stop_bits = 1){
//		SerialBase::format(bits, parity, stop_bits);
//	}
private:

	Callback<void()> _sigio_cb;

	bool _blocking;

	unsigned char rx_start[MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE];
	unsigned char tx_start[MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE];
	volatile unsigned char *rx_head, *rx_tail, *rx_end;
	volatile unsigned char *tx_head, *tx_tail, *tx_end;

	inline bool rx_empty() {
		return rx_head == rx_tail;
	}
	inline bool rx_full() {
		return ((rx_tail - rx_head + 1)
				% MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE) == 0;
	}
	inline bool tx_empty() {
		return tx_head == tx_tail;
	}
	inline bool tx_full() {
		return ((tx_tail - tx_head + 1)
				% MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE) == 0;
	}

	inline int available_rx() const {
		return (rx_tail - rx_head + MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE)
				% MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE;
	}

	inline int capacity_tx() const {
		return MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE
				- (tx_tail - tx_head + MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE)
						% MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE - 1;
	}

	void wake(void) {
		if (_sigio_cb) {
			_sigio_cb();
		}
	}

	PlatformMutex tx_mtx;
	PlatformMutex rx_mtx;

	DMA_HandleTypeDef tx_dma;
	DMA_HandleTypeDef rx_dma;

	static UART_HandleTypeDef *huart; // Instantiated in FastSerial.cpp for all possible template values
	static FastSerial *instance;

	static void irq_entry() {
		instance->isr();
	}

	void isr() {
		uint32_t serial_isr_flag = 0;
		// Deal with only one flag per call
		// Pre-analyze some of the flags
		if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET
				&& __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE) != RESET) {
			// Received something
			serial_isr_flag = FASTSERIAL_FLAG_RX;
		}
		else if (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) != RESET
				&& __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC) != RESET) {
			// TX complete
			serial_isr_flag = FASTSERIAL_FLAG_TX_COMPLETE;
		}
		// Hand over to STM HAL
		HAL_UART_IRQHandler(huart);
		// Error codes
		if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
			if (huart->RxState == HAL_UART_STATE_READY) {
				size_t rx_len = 0;
				if (rx_tail < rx_head) {
					rx_len = rx_head - rx_tail - 1;
				} else {
					if (rx_head != rx_start)
						rx_len = rx_end - rx_tail;
					else
						rx_len = rx_end - rx_tail - 1; // If rx_head=0, rx_tail=N-1, the queue is full and nothing can be read
				}
				if (rx_len > 0) {
					// Start next reception
					HAL_UART_Receive_IT(huart, (uint8_t*) rx_tail, rx_len);
				}
			}
		}
		else if (serial_isr_flag == FASTSERIAL_FLAG_RX) {// Received something
			rx_tail++; // Push the queue
			if (rx_tail == rx_end)
				rx_tail = rx_start;
			if (huart->RxState == HAL_UART_STATE_READY) {
				size_t rx_len = 0;
				if (rx_tail < rx_head) {
					rx_len = rx_head - rx_tail - 1;
				} else {
					if (rx_head != rx_start)
						rx_len = rx_end - rx_tail;
					else
						rx_len = rx_end - rx_tail - 1; // If rx_head=0, rx_tail=N-1, the queue is full and nothing can be read
				}
				if (rx_len > 0) {
					// Start next reception
					HAL_UART_Receive_IT(huart, (uint8_t*) rx_tail, rx_len);
				}
			}
			return;
		}
		else if (serial_isr_flag == FASTSERIAL_FLAG_TX_COMPLETE) { // TX complete, prepare next transmission
			size_t tx_len = 0;
			// Setup next transfer
			tx_head += huart->TxXferSize;
			if (tx_head >= tx_end)
				tx_head -= MBED_CONF_DRIVERS_UART_SERIAL_TXBUF_SIZE;
			if (tx_tail >= tx_head) {
				tx_len = tx_tail - tx_head;
			} else {
				tx_len = tx_end - tx_head;
			}
			if (tx_len > 0) {
				// Start next transmission
				HAL_UART_Transmit_DMA(huart, (uint8_t *) tx_head, tx_len);
			}
		}
	}

	static void irq_dma_tx_entry() {
		instance->isr_dma_tx();
	}

	void isr_dma_tx() {
		HAL_DMA_IRQHandler(&tx_dma);
	}

//	static serial_t _serial;
};

template<UARTName uart> FastSerial<uart> *FastSerial<uart>::instance = NULL;
}

#endif /* FASTSERIAL_H_ */
