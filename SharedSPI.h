/*
 * SharedSPI.h
 *
 *  Created on: Jul 14, 2019
 *      Author: caoyu
 */

#ifndef SHAREDSPI_H_
#define SHAREDSPI_H_

#include "drivers/SPI.h"
#include "drivers/DigitalOut.h"

namespace mbed {

#define SHAREDSPI_MAX_INSTANCE 4

// Dummy for use in the inner class
template<typename T>
class _NonCopyable: public NonCopyable<T> {
};

class SharedSPI: private NonCopyable<SharedSPI> {
public:
	SharedSPI(PinName mosi, PinName miso, PinName sclk, int bit = 8, int mode =
			0, int freq = 100000, bool pol = false, PinMode pinMode = PullNone);
	virtual ~SharedSPI();

	/**
	 * Interface to access the shared SPI resource using separate Chip Select lines
	 * Note: changing format, speed, etc of the interface will NOT work
	 * Change the parameters of the parent SharedSPI object instead, which is a real SPI object
	 */
	class SPI_Interface: public SPI, private _NonCopyable<SPI_Interface> {
	public:
		SPI_Interface(SharedSPI *p, PinName cs);
		virtual ~SPI_Interface() {
		}
		virtual int write(int value);
		virtual int write(const char *tx_buffer, int tx_length, char *rx_buffer,
				int rx_length);
		void format(int bits, int mode = 0);
		void frequency(int hz = 1000000);
	protected:
		SharedSPI *parent;
		DigitalOut cs;
	};

	/** Set polarity of CS
	 *  pol=false: active low, idle high
	 *  pol=true: active high, idle low
	 */
	void setCSPolarity(bool pol) {
		polarity = pol;
	}

	// Do not lock on SPI level, instead lock when the interface is called
	virtual void lock() {
	}
	virtual void unlock() {
	}

	void acquire() {
		mutex.lock();
	}
	void release() {
		mutex.unlock();
	}

	/**
	 * Get an interface to the shared SPI using provided chipselect
	 */
	SPI_Interface* getInterface(PinName chipselect);
protected:
	SPI spi;
	bool polarity;
	size_t num_ifcs;
	SPI_Interface *ifcs[SHAREDSPI_MAX_INSTANCE];
	PlatformMutex mutex;

	void assertCS(DigitalOut &cs);
	void deassertCS(DigitalOut &cs);
};

}

#endif /* SHAREDSPI_H_ */
