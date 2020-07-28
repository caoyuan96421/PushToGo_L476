#include "Buttons.h"
/*
 * ButtonGroup.cpp
 *
 *  Created on: Aug 24, 2019
 *      Author: caoyu
 */
#define DEBOUNCE_TIME	20
#define DEBOUNCE_NUM	3

Buttons::Buttons() :
		listener(NULL), evt_queue(4 * EVENTS_EVENT_SIZE), thread(
				osPriorityAboveNormal, 1024, NULL, "btns_evt_q"), previous_state(
				0) {
	for (int i = 0; i < BUTTON_NUM; i++) {
		buttons[i] = new InterruptIn(Button_All[i], PullUp);
		buttons[i]->fall(callback(this, &Buttons::_irq));
		buttons[i]->rise(callback(this, &Buttons::_irq));
	}
	thread.start(callback(&evt_queue, &EventQueue::dispatch_forever));
}

Buttons::~Buttons() {
	evt_queue.break_dispatch();
	thread.terminate();
	for (int i = 0; i < BUTTON_NUM; i++) {
		delete buttons[i];
	}
}

void Buttons::_irq() {
	int state = 0;
	for (int i = 0; i < BUTTON_NUM; i++) {
		// Pressed is 1, unpressed is 0 (invert of the voltage level)
		state |= ((!buttons[i]->read()) << i);
		// Disable irq
		buttons[i]->fall(NULL);
		buttons[i]->rise(NULL);
	}
	evt_queue.call(this, &Buttons::debounce_cb, state);
}

void Buttons::debounce_cb(int newstate) {
	int statebuf[DEBOUNCE_NUM];
	int i;
	bool match = false;
	memset(statebuf, 0xFF, sizeof(statebuf)); // Set all old_states to -1, so it will never match newstate
	statebuf[0] = newstate;
	i = 1;
	while (!match) {
		// Read state after debounce time
		ThisThread::sleep_for(DEBOUNCE_TIME);
		statebuf[i] = 0;
		for (int j = 0; j < BUTTON_NUM; j++) {
			// Pressed is 1, unpressed is 0 (invert of the voltage level)
			statebuf[i] |= ((!buttons[j]->read()) << j);
		}
		i = (i + 1) % DEBOUNCE_NUM; // Wraps when reaches DEBOUNCE_NUM

		// Check if all state match
		match = true;
		for (int j = 1; j < DEBOUNCE_NUM; j++)
			if (statebuf[j - 1] != statebuf[j]) {
				match = false;
				break;
			}
	}
	newstate = statebuf[0]; // Final state after settling
	if (newstate != previous_state) {
		// Call listener
		if (listener) {
			listener->buttonStateChanged(previous_state, newstate);
		}

		// Update previous state
		previous_state = newstate;
	}

	// Enable irq to detect next event
	core_util_critical_section_enter();
	for (int i = 0; i < BUTTON_NUM; i++) {
		buttons[i]->fall(callback(this, &Buttons::_irq));
		buttons[i]->rise(callback(this, &Buttons::_irq));
	}
	core_util_critical_section_exit();
}
