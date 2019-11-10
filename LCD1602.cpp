/*
 * LCD1602.cpp
 *
 *  Created on: Jul 18, 2019
 *      Author: caoyu
 */

#include "LCD1602.h"
#include "mbed_wait_api.h"
#include "printf.h"
#include <stdarg.h>

LCD1602::LCD1602(PinName rs, PinName rw, PinName en, PinName d[],
		PinName brightness, bool fourwire) :
		rs(rs), rw(rw), en(en), br(NULL), fw(fourwire), d(
				{ fourwire ? NC : d[0], fourwire ? NC : d[1],
						fourwire ? NC : d[2], fourwire ? NC : d[3], d[4], d[5],
						d[6], d[7] }) {
	this->en = 0;
	for (int i = fourwire ? 4 : 0; i < 8; i++) {
		this->d[i].output();
	}
	if (brightness != NC) {
		br = new PwmOut(brightness);
		br->period_us(64);
		*br = 1.0;
	}
	init();
}

LCD1602::~LCD1602() {
	if (br) {
		delete br;
	}
}

void LCD1602::clear() {
	command(0x01);
}

void LCD1602::write(const char *buf, int len) {
	for (int i = 0; i < len; i++) {
		rw = 0;
		rs = 1;
		send_byte(buf[i]);
		busy_wait();
	}
}

void LCD1602::print(const char *fmt, ...) {
	char buf[128];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	for (int i = 0; i < len; i++) {
		rw = 0;
		rs = 1;
		send_byte(buf[i]);
		busy_wait();
	}
}

void LCD1602::setCursor(bool on) {
	if (on) {
		command(0x0F);
	} else {
		command(0x0C);
	}
}

void LCD1602::setDisplay(bool on) {
	if (on) {
		command(0x08);
	} else {
		command(0x0C);
	}
}

void LCD1602::setPosition(uint8_t row, uint8_t column) {
	uint8_t addr = row * 0x40 + column;
	command(0x80 | (addr & 0x7F));
}

void LCD1602::addGlyph(char code, uint8_t glyph[]) {
}

void LCD1602::send_high_nibble(uint8_t nibble) {
	d[7] = nibble & 0x80;
	d[6] = nibble & 0x40;
	d[5] = nibble & 0x20;
	d[4] = nibble & 0x10;
	en = 1;
	wait_ns(50);
	en = 0;
}

void LCD1602::send_byte(uint8_t byte) {
	d[7] = byte & 0x80;
	d[6] = byte & 0x40;
	d[5] = byte & 0x20;
	d[4] = byte & 0x10;
	if (fw) {
		// Send high byte
		en = 1;
		wait_ns(500);
		en = 0;
		d[7] = byte & 0x08;
		d[6] = byte & 0x04;
		d[5] = byte & 0x02;
		d[4] = byte & 0x01;
		// Send low byte
		en = 1;
		wait_ns(500);
		en = 0;
	} else {
		d[3] = byte & 0x08;
		d[2] = byte & 0x04;
		d[1] = byte & 0x02;
		d[0] = byte & 0x01;
		// Send whole byte
		en = 1;
		wait_ns(500);
		en = 0;
	}
}

uint8_t LCD1602::read_byte() {
	uint8_t data = 0;
	uint8_t ep = fw ? 4 : 0;
	for (int i = ep; i < 8; i++)
		d[i].input();
	en = 1;
	wait_ns(500);
	for (int i = 7; i >= ep; i--) {
		data |= d[i] ? 1 : 0;
		data <<= 1;
	}
	en = 0;
	wait_ns(500);
	if (fw) {
		// Do again for lower nibble
		en = 1;
		wait_ns(500);
		for (int i = 7; i >= ep; i--) {
			data |= d[i] ? 1 : 0;
			data <<= 1;
		}
		en = 0;
		wait_ns(500);
	}
	for (int i = ep; i < 8; i++)
		d[i].output();
	return data;
}

void LCD1602::busy_wait() {
//	wait_ms(5);
	bool busy = true;
	rs = 0;
	rw = 1;
	for (int i = fw ? 4 : 0; i < 8; i++)
		d[i].input();
	while (busy) {
		en = 1;
		wait_ns(500);
		busy = d[7];
		en = 0;
		wait_ns(500);
		if (fw) {
			// Have to do twice to complete one read
			en = 1;
			wait_ns(500);
			en = 0;
			wait_ns(500);
		}
	}
	for (int i = fw ? 4 : 0; i < 8; i++)
		d[i].output();
}

void LCD1602::init() {
	rs = 0;
	rw = 0;
	// Powerup delay
	wait_ms(100);
	// Init four wire
	if (fw) {
		send_high_nibble(0x30);
		wait_ms(5);
		send_high_nibble(0x30);
		wait_ms(5);
		send_high_nibble(0x20);
		wait_ms(5);
		send_byte(0x28);
		wait_ms(5);
	} else {
		send_byte(0x38);
		wait_ms(5);
	}
	command(0x0C); // No cursor
	command(0x06); // Increment addr after read/write, no shifting
	command(0x02); // Home
	command(0x01); // Clear display
}

void LCD1602::command(uint8_t cmd) {
	rs = 0;
	rw = 0;
	send_byte(cmd);
	busy_wait();
}

void LCD1602::setBrightness(float level) {
	if (br) {
		br->write(level);
	}
}

void LCD1602::write_ram(uint8_t addr, uint8_t data, RAM ram) {
	// Set address
	if (ram == CGRAM) {
		command(0x40 | (addr & 0x3F));
	} else {
		command(0x80 | (addr & 0x7F));
	}
	rw = 0;
	rs = 1;
	send_byte(data);
	busy_wait();
}

uint8_t LCD1602::read_ram(uint8_t addr, RAM ram) {
	// Set address
	if (ram == CGRAM) {
		command(0x40 | (addr & 0x3F));
	} else {
		command(0x80 | (addr & 0x7F));
	}
	rw = 1;
	rs = 1;
	uint8_t data = read_byte();
	busy_wait();
	return data;
}

void LCD1602::fillWith(char ch, int cnt) {
	for (int i = 0; i < cnt; i++) {
		rw = 0;
		rs = 1;
		send_byte(ch);
		busy_wait();
	}
}
