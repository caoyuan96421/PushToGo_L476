/*
 * ButtonGroup.h
 *
 *  Created on: Aug 24, 2019
 *      Author: caoyu
 */

#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "mbed.h"
#include "mbed_events.h"
#include "Board.h"

class ButtonListener {
public:
	ButtonListener() {
	}
	virtual ~ButtonListener(){}

	/**
	 * Callback for button state change, must be implemented by subclasses
	 */
	virtual void buttonStateChanged(int oldstate, int newstate) = 0;
};

class Buttons {
public:
	Buttons();
	virtual ~Buttons();

	void registerListener(ButtonListener *listener){
		this->listener = listener;
	}

private:
	ButtonListener *listener;
	InterruptIn *buttons[BUTTON_NUM];
	EventQueue evt_queue;
	Thread thread;
	int previous_state;

	void _irq();
	void debounce_cb(int newstate);
};

#endif /* _BUTTONS_H_ */
