/*
 * SharedSPI.cpp
 *
 *  Created on: Jul 14, 2019
 *      Author: caoyu
 */

#include "SharedSPI.h"

using namespace mbed;

SharedSPI::SharedSPI(PinName mosi, PinName miso, PinName sclk) :
		SPI(mosi, miso, sclk, NC), polarity(false), num_ifcs(0) {
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
	int ret = parent->write(value);
	parent->deassertCS(cs);
	parent->release();
	return ret;
}

int SharedSPI::SPI_Interface::write(const char* tx_buffer, int tx_length,
		char* rx_buffer, int rx_length) {
	parent->acquire();
	parent->assertCS(cs);
	int ret = parent->write(tx_buffer, tx_length, rx_buffer, rx_length);
	parent->deassertCS(cs);
	parent->release();
	return ret;
}

SharedSPI::SPI_Interface::SPI_Interface(SharedSPI* p, PinName c) :
		SPI(NC, NC, NC), parent(p), cs(c) {
	// The parent SPI will be still be initiated to SPIName of NC, but will not be used
	// The "NC" SPI object will be shared among interfaces
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