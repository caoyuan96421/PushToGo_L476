/*
 * iCLNGAbsEncoder.h
 *
 *  Created on: Aug 13, 2019
 *      Author: caoyu
 */

#ifndef ICLNGABSENCODER_H_

#define ICLNGABSENCODER_H_

#include "mbed.h"
#include "Encoder.h"

class iCLNGAbsEncoder: public GrayAbsEncoder<16> {
public:
	// Construct iC-LNG Absolute Encoder object
	// Note: iC-LNG uses a POSITIVE CS signal
	// The following should be set before passing SPI into the constructor
	// Mode: SPI Mode 0
	// Max frequency: 10MHz
	iCLNGAbsEncoder(SPI *spi);
	virtual ~iCLNGAbsEncoder();


	virtual uint32_t readPosGray();

private:
	enum OpCode {
		ACTIVATE = 0xB0,
		READ_SENSOR = 0xA6,
		SENSOR_STATUS = 0xF5,
		READ_REG = 0x8A,
		WRITE_REG = 0xCF,
		REG_STATUS = 0xAD
	};

	enum Register {
		GS = 0,
		GC = 1,
		OSP = 2,
		OSN = 3,
		OCP = 4,
		OCN = 5,
		LCSET = 6,
		OUT1 = 7,
		OUT2 = 8,
		TEST = 9
	};

	SPI *spi;

	int spi_xchg(unsigned int size, OpCode opcode, char *txbuf, char *rxbuf);
	void activate(bool rapa);
	uint16_t get_sensor_data();
	bool data_available();
	uint8_t reg_status();

	uint8_t read_reg(Register addr);
	void write_reg(Register addr, uint8_t data);
	uint8_t parity(uint8_t data);
};

#endif /* ICLNGABSENCODER_H_ */
