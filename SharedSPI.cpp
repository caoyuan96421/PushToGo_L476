/*
 * SharedSPI.cpp
 *
 *  Created on: Jul 14, 2019
 *      Author: caoyu
 */

#include "SharedSPI.h"
#include "pinmap.h"

using namespace mbed;

SharedSPI::SharedSPI(PinName mosi, PinName miso, PinName sclk, int bit, int mode, int freq, bool pol, PinMode pinMode) :
		spi(mosi, miso, sclk, NC), polarity(pol), num_ifcs(0), _mosi(mosi), _miso(miso), _sclk(sclk) {
	spi.format(bit, mode);
	spi.frequency(freq);
	pin_mode(miso, pinMode);
}

SharedSPI::~SharedSPI() {
	for (size_t i = 0; i < num_ifcs; i++) {
		if (ifcs[i])
			delete ifcs[i];
	}
}

int SharedSPI::SPI_Interface::write(int value) {
	parent->acquire();
	parent->assertCS(cs);
	int ret = parent->spi.write(value);
	parent->deassertCS(cs);
	parent->release();
	return ret;
}

int SharedSPI::SPI_Interface::write(const char *tx_buffer, int tx_length,
		char *rx_buffer, int rx_length) {
	parent->acquire();
	parent->assertCS(cs);
	int ret = parent->spi.write(tx_buffer, tx_length, rx_buffer, rx_length);
	parent->deassertCS(cs);
	parent->release();
	return ret;
}

SharedSPI::SPI_Interface::SPI_Interface(SharedSPI *p, PinName c) :
		SPI(p->_mosi, p->_miso, p->_sclk), parent(p), cs(c) {
	// The parent SPI will be still be initiated to same PINs, and it will map to the same SPI object as the parent
	parent->deassertCS(cs);
}

SharedSPI::SPI_Interface* SharedSPI::getInterface(PinName chipselect) {
	if (num_ifcs == SHAREDSPI_MAX_INSTANCE)
		return NULL;
	else {
		ifcs[num_ifcs++] = new SPI_Interface(this, chipselect);
		return ifcs[num_ifcs - 1];
	}
}

inline void SharedSPI::assertCS(DigitalOut &cs) {
	cs = polarity ? 1 : 0;
}

inline void SharedSPI::deassertCS(DigitalOut &cs) {
	cs = polarity ? 0 : 1;
}

// Only allow the first interface to set format and frequency

void mbed::SharedSPI::SPI_Interface::format(int bits, int mode) {
	if (this == parent->ifcs[0])
		parent->spi.format(bits, mode);
}

void mbed::SharedSPI::SPI_Interface::frequency(int hz) {
	if (this == parent->ifcs[0])
		parent->spi.frequency(hz);
}
