/*
 * ADL355.h
 *
 *  Created on: Aug 27, 2019
 *      Author: caoyu
 */

#ifndef ADL355_H_
#define ADL355_H_

#include "mbed.h"
#include "pushtogo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class ADL355: I2C, public Inclinometer {
public:
	ADL355(PinName sda, PinName scl);
	virtual ~ADL355();

	bool getAcceleration(double &ax, double &ay, double &az);
	/**
	 * Get tilt (along x) and tip (along y)
	 */
	bool getTilt(double &tilt, double &tip);

	/// Implements Inclinometer
	double getTilt() {
		return -t * 180 / M_PI;
	}
	void refresh() {
		double x, y, z;
		if (!getAcceleration(x, y, z)) {
			return;
		}
		double theta = atan2(x, sqrt(y * y + z * z));
		double phi = atan2(y, z);
		t = asin(cos(theta) * sin(phi));
	}

private:
	double t;
	void write_reg(uint8_t addr, uint8_t data);
	uint8_t read_reg(uint8_t addr);
	void read_reg_cont(uint8_t addr, uint8_t *buf, unsigned int nbytes);
};

#endif /* ADL355_H_ */
