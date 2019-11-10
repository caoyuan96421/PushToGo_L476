/*
 * LED.cpp
 *
 *  Created on: Jul 25, 2019
 *      Author: caoyu
 */

#include "LED.h"
#include "PeripheralPins.h"
#include "mbed.h"

LED::LED(PinName pin, bool use_pwm) :
		dig(NULL), pwm(NULL), has_pwm(use_pwm) {
	// Check if that pin has pwm
	uint32_t function = pinmap_function(pin, PinMap_PWM);
	if (function == (uint32_t) NC) {
		has_pwm = false;
	}

	if (has_pwm) {
		pwm = new PwmOut(pin);
		pwm->period_us(1000);
		pwm->write(0);
	} else {
		dig = new DigitalOut(pin);
		dig->write(0);
	}
}

LED::~LED() {
	if (pwm)
		delete pwm;
	if (dig)
		delete dig;
}

void LED::flash(float duration, float frequency) {
	uint32_t half_period_us = ceilf(5E5f / frequency);
	int64_t time_remaining_us = ceilf(duration * 1000000ULL);
	// Start from on
	on();
	while (duration < 0 || time_remaining_us > 0) {
		wait_us(half_period_us);
		toggle();
		time_remaining_us -= half_period_us;
	}
	// Turn off led after flashing
	off();
}

LED& LED::operator =(float val) {
	if (has_pwm) {
		pwm->write(val);
	} else {
		dig->write((val > 0) ? 1 : 0);
	}
	return *this;
}

void LED::period(uint32_t us) {
	if (has_pwm) {
		pwm->period_us(us);
	}
}

LED::operator int() {
	if (has_pwm) {
		return ((float) (*pwm) > 0);
	} else {
		return ((int) (*dig) > 0);
	}
}

void LED::breath(float duration, float start, float end) {
	if (!has_pwm) {
		// Can't breath without PWM capability
		wait(duration);
		return;
	}
	const float step = 10e-3; // 10ms update
	unsigned nsteps = unsigned(duration / step);
	float delta = (end - start) / nsteps;

	*this = start;
	for (unsigned i = 0; i < nsteps; i++) {
		wait_ms(step*1000);
		*this = start + i * delta;
	}
	*this = end;
}

void LED::test() {
// On
	on();
	wait(1);
// Off
	off();
	wait(1);
// Toggle
	for (int i = 0; i < 10; i++) {
		toggle();
		wait(0.2);
	}
	wait(1);
// flash
	flash(2, 15);
	flash(2, 3);
	flash(2, 20);
	wait(2);
// Breath
	breath(2, 0, 1);
	breath(1, 1, 0.5);
	breath(2, 0.5, 0);
}
