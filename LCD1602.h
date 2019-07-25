/*
 * LCD1602.h
 *
 *  Created on: Jul 18, 2019
 *      Author: caoyu
 */

#ifndef LCD1602_H_
#define LCD1602_H_

#include "DigitalOut.h"
#include "DigitalInOut.h"
#include "PWMOut.h"

using namespace mbed;

class LCD1602 {
public:
	LCD1602(PinName rs, PinName rw, PinName en, PinName d[], PinName brighness = NC, bool fourwire=true);
	virtual ~LCD1602();

	void clear();
	void print(const char *s);
	void printf(const char *fmt, ...);
	void setCursor(bool on);
	void setDisplay(bool on);
	void setPosition(uint8_t row, uint8_t column);

	void addGlyph(char code, uint8_t glyph[]);

	void setBrightness(float level);

private:
	enum RAM {
		CGRAM=0,
		DDRAM
	};
	void init();
	void send_high_nibble(uint8_t nibble);
	void send_byte(uint8_t byte);
	uint8_t read_byte();
	void busy_wait();
	void command(uint8_t cmd);
	uint8_t read_ram(uint8_t addr, RAM ram);
	void write_ram(uint8_t addr, uint8_t data, RAM ram);

	DigitalOut rs;
	DigitalOut rw;
	DigitalOut en;
	PwmOut *br;
	bool fw;
	DigitalInOut d[8];
};

#endif /* LCD1602_H_ */
