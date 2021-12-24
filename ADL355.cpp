/*
 * ADL355.cpp
 *
 *  Created on: Aug 27, 2019
 *      Author: caoyu
 */

#include "ADL355.h"
#include "Logger.h"

#define ADL355_ADDR	0x1D
#define ADL355_FULLSCALE	2.0 // Full scale acceleration, +-2g for 20-bit
#define ADL355_STATUS	0x04
#define ADL355_TEMP1	0x07
#define ADL355_FILTER	0x28
#define ADL355_PWRCTL	0x2D
#define ADL355_XDATA3	0x08 // X,Y,Z data registers start from here

ADL355::ADL355(PinName sda, PinName scl) : I2C(sda, scl), t(0) {
    pin_mode(sda, OpenDrainPullUp);
    pin_mode(scl, OpenDrainPullUp);

//	frequency(400000);
	// Init
	write_reg(ADL355_FILTER, 0x0A); // LPF ~4Hz
	// Start measurement
	write_reg(ADL355_PWRCTL, 0x00); // STANDBY=0
}

ADL355::~ADL355() {
}

bool ADL355::getAcceleration(double &ax, double &ay, double &az) {
	// Init
	write_reg(ADL355_FILTER, 0x0A); // LPF ~4Hz
	// Start measurement
	write_reg(ADL355_PWRCTL, 0x00); // STANDBY=0
	// Read STATUS to force DATA_RDY to clear
	read_reg(ADL355_STATUS);
	// Wait for next data to be taken
	int timeout = 500;
	while (!(read_reg(ADL355_STATUS) & 0x01)) {
		ThisThread::sleep_for(10);
		if((timeout-=10) < 0){
			abort_transfer();
			Logger::logError("ADL355 Accelerometor not connected.");
			return false;
		}
	}
	// Read X,Y,Z data
	uint8_t buf[9];
	read_reg_cont(ADL355_XDATA3, buf, 9);
	int32_t dx = (((uint32_t) buf[0]) << 12) | (((uint32_t) buf[1]) << 4)
			| (((uint32_t) buf[2]) >> 4);
	int32_t dy = (((uint32_t) buf[3]) << 12) | (((uint32_t) buf[4]) << 4)
			| (((uint32_t) buf[5]) >> 4);
	int32_t dz = (((uint32_t) buf[6]) << 12) | (((uint32_t) buf[7]) << 4)
			| (((uint32_t) buf[8]) >> 4);
	// Sign extension from 20-bit to 32-bit
	if (dx & 0x00080000) {
		dx |= 0xFFF00000;
	}
	if (dy & 0x00080000) {
		dy |= 0xFFF00000;
	}
	if (dz & 0x00080000) {
		dz |= 0xFFF00000;
	}
	ax = (double) dx / (1 << 19) * ADL355_FULLSCALE;
	ay = (double) dy / (1 << 19) * ADL355_FULLSCALE;
	az = (double) dz / (1 << 19) * ADL355_FULLSCALE;

	Logger::log("ADL355 ax=%.5f, ay=%.5f, az=%.5f", ax, ay, az);

	return true;
}

bool ADL355::getTilt(double &tilt_x, double &tilt_y) {
	double x,y,z;
	if(!getAcceleration(x,y,z))
		return false;
	tilt_x = atan2(x, z) * 180 / M_PI;
	tilt_y = atan2(y, z) * 180 / M_PI;
	return true;
}

void ADL355::write_reg(uint8_t addr, uint8_t data) {
	char buf[2] = { addr, data };
	write((ADL355_ADDR << 1) | 0x00, buf, 2, false); // Write
}

uint8_t ADL355::read_reg(uint8_t addr) {
	char buf = addr;
	write((ADL355_ADDR << 1) | 0x00, &buf, 1, true); // Write address, with repeated start for reading
	read((ADL355_ADDR << 1) | 0x01, &buf, 1, false); // Read data
	return buf;
}

void ADL355::read_reg_cont(uint8_t addr, uint8_t *buf, unsigned int nbytes) {
	write((ADL355_ADDR << 1) | 0x00, (char*) &addr, 1, true); // Write address, with repeated start for reading
	read((ADL355_ADDR << 1) | 0x01, (char*) buf, nbytes, false); // Read data
}
