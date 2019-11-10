/*
 * MCULoadMeasurement.cpp
 *
 *  Created on: Aug 25, 2019
 *      Author: caoyu
 */
#include "mbed.h"
#include "MCULoadMeasurement.h"

volatile unsigned int *DWT_CYCCNT = (volatile unsigned int*) 0xE0001004;
volatile unsigned int *DWT_CONTROL = (volatile unsigned int*) 0xE0001000;
volatile unsigned int *SCB_DEMCR = (volatile unsigned int*) 0xE000EDFC;
static void init_timer() {
	*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
}

static void reset_timer() {
	*DWT_CYCCNT = 0; // reset the counter
}

static void start_timer() {
	*DWT_CONTROL = *DWT_CONTROL | 1; // enable the counter
}

static void stop_timer() {
	*DWT_CONTROL = *DWT_CONTROL | 0; // disable the counter
}

static inline uint32_t get_tick() {
	return *DWT_CYCCNT;
}

void idle_hook() {
	core_util_critical_section_enter();
	MCULoadMeasurement::getInstance().setMCUActive(false);
//	sleep_manager_lock_deep_sleep();
//	sleep();
//	__WFI();
//	sleep_manager_unlock_deep_sleep();
	int i = 1000;
	while (i--)
		;
	MCULoadMeasurement::getInstance().setMCUActive(true);
	core_util_critical_section_exit();
}

MCULoadMeasurement::MCULoadMeasurement() : thd(osPriorityBelowNormal, OS_STACK_SIZE, NULL, "mcu_load_meas"), t_active(0), t_total(0), avg_usage(0){
	init_timer();
	reset_timer();
	start_timer();

	// Register idle hook
	Thread::attach_idle_hook(idle_hook);
	MCULoadMeasurement::getInstance().reset();

	// Start thread
	thd.start(this, &MCULoadMeasurement::_thread_entry);
}

void MCULoadMeasurement::_thread_entry(){
	while(true){
		core_util_critical_section_enter();
		uint64_t extra_ticks = get_tick();
		if (!active)
			avg_usage = (float) t_active / t_total;
		else {
			avg_usage =  (float) (t_active + extra_ticks) / (t_total + extra_ticks);
		}
		reset();
		core_util_critical_section_exit();

		// Update usage every 100ms
		ThisThread::sleep_for(100);
	}
}

void MCULoadMeasurement::reset(){
	t_active = 0;
	t_total = 0;
	reset_timer();
}

void MCULoadMeasurement::setMCUActive(bool active){
	uint64_t ticks = get_tick();
	if (!this->active && active) {
		// Enabled
		reset_timer();
		t_total += ticks;
	}
	else if(this->active && !active){
		// Disabled
		reset_timer();
		t_total += ticks;
		t_active += ticks;
	}

	this->active = active;
}

float MCULoadMeasurement::getCPUUsage(){
	return avg_usage;
}
