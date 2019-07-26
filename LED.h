/*
 * LED.h
 *
 *  Created on: Jul 25, 2019
 *      Author: caoyu
 */

#ifndef _LED_H_
#define _LED_H_

#include "DigitalOut.h"
#include "PwmOut.h"

using namespace mbed;

class LED {
public:
	LED(PinName pin, bool use_pwm = true);
	virtual ~LED();

	// Common functions
	void on() {
		*this = 1;
	}

	void off() {
		*this = 0;
	}

	void toggle() {
		*this = !(*this);
	}

	void flash(float duration, float frequency);
	void breath(float duration, float start, float end);

	// Low level functions
	LED& operator=(float val);

	void period(uint32_t us);

	operator int();

	// Run all of the above functions for fun
	void test();

private:
	DigitalOut *dig;
	PwmOut *pwm;
	bool has_pwm;
};

#endif /* _LED_H_ */
