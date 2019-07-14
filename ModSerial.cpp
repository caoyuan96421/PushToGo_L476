/*
 * ModSerial.cpp
 *
 *  Created on: Jul 11, 2019
 *      Author: caoyu
 */

#include <ModSerial.h>

/* mbed Microcontroller Library
 * Copyright (c) 2006-2017 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if (DEVICE_SERIAL && DEVICE_INTERRUPTIN)

#include "platform/mbed_poll.h"

#if MBED_CONF_RTOS_PRESENT
#include "rtos/ThisThread.h"
#else
#include "platform/mbed_wait_api.h"
#endif


namespace mbed {

uint32_t timestamp[1024];
uint32_t count = 0;

ModSerial::ModSerial(PinName tx, PinName rx, int baud) :
    SerialBase(tx, rx, baud),
	_txqueue(this),
	_rxqueue(this),
    _blocking(true),
    _dcd_irq(NULL),
	rxbuf(0),
	txbuf(0)
{
	/*Set queue notification*/
	_rxqueue.notify(inotify);
	_txqueue.notify(onotify);

    /* Attatch IRQ routines to the serial device. */
//    enable_rx_irq();
	SerialBase::read(&rxbuf, 1, callback(this, &ModSerial::rx_complete_cb), SERIAL_EVENT_RX_COMPLETE, 0);
}

ModSerial::~ModSerial()
{
    delete _dcd_irq;
}

void ModSerial::dcd_irq()
{
    wake();
}

void ModSerial::set_baud(int baud)
{
    SerialBase::baud(baud);
}

void ModSerial::set_data_carrier_detect(PinName dcd_pin, bool active_high)
{
    delete _dcd_irq;
    _dcd_irq = NULL;

    if (dcd_pin != NC) {
        _dcd_irq = new InterruptIn(dcd_pin);
        if (active_high) {
            _dcd_irq->fall(callback(this, &ModSerial::dcd_irq));
        } else {
            _dcd_irq->rise(callback(this, &ModSerial::dcd_irq));
        }
    }
}

void ModSerial::set_format(int bits, Parity parity, int stop_bits)
{
    api_lock();
    SerialBase::format(bits, parity, stop_bits);
    api_unlock();
}

#if DEVICE_SERIAL_FC
void ModSerial::set_flow_control(Flow type, PinName flow1, PinName flow2)
{
    api_lock();
    SerialBase::set_flow_control(type, flow1, flow2);
    api_unlock();
}
#endif

int ModSerial::close()
{
    /* Does not let us pass a file descriptor. So how to close ?
     * Also, does it make sense to close a device type file descriptor*/
    return 0;
}

int ModSerial::isatty()
{
    return 1;

}

off_t ModSerial::seek(off_t offset, int whence)
{
    /*XXX lseek can be done theoratically, but is it sane to mark positions on a dynamically growing/shrinking
     * buffer system (from an interrupt context) */
    return -ESPIPE;
}

int ModSerial::sync()
{
    api_lock();

    while (!_txqueue.empty()) {
        api_unlock();
        // Doing better than wait would require TxIRQ to also do wake() when becoming empty. Worth it?
        wait_ms(1);
        api_lock();
    }

    api_unlock();

    return 0;
}

void ModSerial::sigio(Callback<void()> func)
{
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

/* Special synchronous write designed to work from critical section, such
 * as in mbed_error_vprintf.
 */
ssize_t ModSerial::write_unbuffered(const char *buf_ptr, size_t length)
{
    while (!_txqueue.empty()) {
    	_txqueue.get(&txbuf);
        SerialBase::_base_putc(txbuf);
    }

    for (size_t data_written = 0; data_written < length; data_written++) {
        SerialBase::_base_putc(*buf_ptr++);
    }

    return length;
}

ssize_t ModSerial::write(const void *buffer, size_t length)
{
    const char *buf_ptr = static_cast<const char *>(buffer);

    if (length == 0) {
        return 0;
    }

    if (core_util_in_critical_section()) {
        return write_unbuffered(buf_ptr, length);
    }

    api_lock();
	unsigned int timeout = _blocking ? osWaitForever : 0;
	unsigned int i;
	for (i = 0; i < length; i++) {

		timestamp[count++] = *((uint32_t *)0xE0001004)+0000000000;
		count %= 1024;
		if (_txqueue.put(buf_ptr[i], timeout) == osErrorTimeout) {
			break;
		}
	}
    api_unlock();

    return i != 0 ? (ssize_t) i : (ssize_t) - EAGAIN;
}

ssize_t ModSerial::read(void *buffer, size_t length) {
	api_lock();
	unsigned char *p = (unsigned char*) buffer;
	unsigned int timeout = _blocking ? osWaitForever : 0;
	unsigned int i;
	for (i = 0; i < length; i++) {
		if (_rxqueue.get(&p[i], timeout) == osErrorTimeout) {
			break;
		}
	}
	api_unlock();
	return i;
}

bool ModSerial::hup() const
{
    return _dcd_irq && _dcd_irq->read() != 0;
}

void ModSerial::wake()
{
    if (_sigio_cb) {
        _sigio_cb();
    }
}

short ModSerial::poll(short events) const
{

    short revents = 0;
    /* Check the Circular Buffer if space available for writing out */


    if (!_rxqueue.empty()) {
        revents |= POLLIN;
    }

    /* POLLHUP and POLLOUT are mutually exclusive */
    if (hup()) {
        revents |= POLLHUP;
    } else if (!_txqueue.full()) {
        revents |= POLLOUT;
    }

    /*TODO Handle other event types */

    return revents;
}

void ModSerial::lock()
{
    // This is the override for SerialBase.
    // No lock required as we only use SerialBase from interrupt or from
    // inside our own critical section.
}

void ModSerial::unlock()
{
    // This is the override for SerialBase.
}

void ModSerial::api_lock(void)
{
    _mutex.lock();
}

void ModSerial::api_unlock(void)
{
    _mutex.unlock();
}

void ModSerial::inotify(InputQueue<unsigned char,  MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE> *rxq, void *param){
	// Called when rxqueue is changing to non-full
	// Called by read()
	ModSerial *ms = (ModSerial *)param;
//	core_util_critical_section_enter();
    bool was_empty = rxq->empty();

    if (!ms->_rx_asynch_set && !rxq->full()){ // If no ongoing transmission, start one
    	ms->SerialBase::read(&ms->rxbuf, 1, callback(ms, &ModSerial::rx_complete_cb), SERIAL_EVENT_RX_COMPLETE, 0);
    }

    /* Report the File handler that data is ready to be read from the buffer. */
    if (was_empty && !rxq->empty()) {
        ms->wake();
    }
//	core_util_critical_section_exit();
}


void ModSerial::onotify(OutputQueue<unsigned char,  MBED_CONF_DRIVERS_UART_SERIAL_RXBUF_SIZE> *txq, void *param){
	// Called when txqueue is changing to non-empty
	// Called by write()
	ModSerial *ms = (ModSerial *)param;
//	core_util_critical_section_enter();
    bool was_full = txq->full();

	if (!ms->_tx_asynch_set && !txq->empty()){ // If no ongoing transmission, start one
		txq->get(&ms->txbuf);
		ms->SerialBase::write(&ms->txbuf, 1, callback(ms, &ModSerial::tx_complete_cb), SERIAL_EVENT_TX_COMPLETE);
	}

	timestamp[count++] = *((uint32_t *)0xE0001004)+1000000000;
	count %= 1024;

    /* Report the File handler that data can be written to peripheral. */
    if (was_full && !txq->full() && !ms->hup()) {
        ms->wake();
    }
//	core_util_critical_section_exit();
}
void ModSerial::rx_complete_cb(int flag)
{
    // Guaranteed not full
	_rxqueue.put(rxbuf);

    // Start next read if still have space
    if (!_rxqueue.full()) {
    	SerialBase::read(&rxbuf, 1, callback(this, &ModSerial::rx_complete_cb), SERIAL_EVENT_RX_COMPLETE, 0);
    }

}

// Also called from write to start transfer
void ModSerial::tx_complete_cb(int flag)
{
	timestamp[count++] = *((uint32_t *)0xE0001004)+2000000000;
	count %= 1024;
    if (!_txqueue.empty()) {
    	_txqueue.get(&txbuf);
    	SerialBase::write(&txbuf, 1, callback(this, &ModSerial::tx_complete_cb), SERIAL_EVENT_TX_COMPLETE);
    }
}

int ModSerial::enable_input(bool enabled)
{
    return 0;
}

int ModSerial::enable_output(bool enabled)
{
    return 0;
}

void ModSerial::wait_ms(uint32_t millisec)
{
    /* wait_ms implementation for RTOS spins until exact microseconds - we
     * want to just sleep until next tick.
     */
#if MBED_CONF_RTOS_PRESENT
    rtos::ThisThread::sleep_for(millisec);
#else
    ::wait_ms(millisec);
#endif
}
} //namespace mbed

#endif //(DEVICE_SERIAL && DEVICE_INTERRUPTIN)
