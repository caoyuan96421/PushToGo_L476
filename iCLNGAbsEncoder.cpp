/*
 * iCLNGAbsEncoder.cpp
 *
 *  Created on: Aug 13, 2019
 *      Author: caoyu
 */

#include "iCLNGAbsEncoder.h"
#include "Logger.h"

iCLNGAbsEncoder::iCLNGAbsEncoder(SPI *spi) :
		spi(spi), initialized(false) {
	activate(true);

	write_reg(OUT1, 0x03);	// Set gain to x2
	write_reg(GC, 0x3F);	// Set gain to x2
	write_reg(GS, 0x3F);	// Set gain to x2
	write_reg(LCSET, 0x3F); // Set LED to max output
	write_reg(OUT2, 0x10);  // Set output 16bit

	// Try to read sensor
	int count;
	for(count=10 ;count && !data_available();count--);
	if (count == 0){
		// Cannot get data
		initialized = false;
		Logger::logError("iC-LNG cannot initialize.");
	}
	else{
		initialized = true;
		Logger::log("iC-LNG initialized succesfully.");
	}
}

iCLNGAbsEncoder::~iCLNGAbsEncoder() {
}

uint32_t iCLNGAbsEncoder::readPosGray() {
	if (initialized) {
		while (!data_available())
			;
		return (uint32_t) get_sensor_data();
	}
	else{
		return 0;
	}
}
int iCLNGAbsEncoder::spi_xchg(unsigned int size, OpCode opcode, char *txbuf,
		char *rxbuf) {
	char tx[8], rx[8];
	tx[0] = (uint8_t) opcode;
	for (unsigned int i = 0; i < size; i++) {
		tx[i + 1] = txbuf[i];
	}
	wait_ns(50); // Probably unnecessary
	int ret = spi->write(tx, size + 1, rx, size + 1);
	wait_ns(50);
	for (unsigned int i = 0; i < size; i++) {
		rxbuf[i] = rx[i+1];
	}
	return ret;
}

// Activate register and data channels
void iCLNGAbsEncoder::activate(bool rapa) {
	char tx = rapa ? 0x03 : 0, rx;
	spi_xchg(1, ACTIVATE, &tx, &rx);
}

uint16_t iCLNGAbsEncoder::get_sensor_data() {
	char tx[2]={0}, rx[2];
	spi_xchg(2, READ_SENSOR, tx, rx);
	return (uint16_t) rx[1] | ((uint16_t) rx[0] << 8); // Big endian
}

bool iCLNGAbsEncoder::data_available() {
	char tx = 0, rx;
	spi_xchg(1, SENSOR_STATUS, &tx, &rx);
	return (rx & 0x80); // SVALID
}

uint8_t iCLNGAbsEncoder::reg_status() {
	char tx[2], rx[2];
	spi_xchg(2, REG_STATUS, tx, rx);
	return rx[0];
}

uint8_t iCLNGAbsEncoder::read_reg(Register addr) {
	char tx[2], rx[2];
	tx[0] = (char) addr;
	tx[1] = 0;
	spi_xchg(2, READ_REG, tx, rx);
	return rx[1]; // rx[0] is the returned address
}

void iCLNGAbsEncoder::write_reg(Register addr, uint8_t data) {
	char tx[2], rx[2];
	tx[0] = (char) addr;
	tx[1] = parity(data); // Calculate parity
	spi_xchg(2, WRITE_REG, tx, rx);
}

uint8_t iCLNGAbsEncoder::parity(uint8_t data) {
	data &= 0x7F; // Ignore MSB
	uint8_t x = data; // 76543210
	x ^= (x >> 1); // 7(76)(65)(54)(43)(32)(21)(10)
	x ^= (x >> 2); // 7(76)(765)(7654)(6543)(5432)(4321)(3210)
	x ^= (x >> 4); // 7(76)(765)(7654)(76543)(765432)(7654321)(76543210)
	data |= ((~x) & 1) << 7; // Set MSB to parity, which is !(76543210), the LSB of x
	return data;
}
