/*
 * ADL355.h
 *
 *  Created on: Aug 27, 2019
 *      Author: caoyu
 */

#ifndef ADL355_H_
#define ADL355_H_

#include "mbed.h"

class ADL355 : I2C{
public:
	ADL355(PinName sda, PinName scl);
	virtual ~ADL355();

	void getAcceleration(double &ax, double &ay, double &az);
	/**
	 * Get tilt (along x) and tip (along y)
	 */
	void getTilt(double &tilt, double &tip);
private:
	void write_reg(uint8_t addr, uint8_t data);
	uint8_t read_reg(uint8_t addr);
	void read_reg_cont(uint8_t addr, uint8_t *buf, unsigned int nbytes);
};

#endif /* ADL355_H_ */
