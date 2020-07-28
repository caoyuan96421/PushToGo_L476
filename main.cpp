#include "mbed.h"
#include <stdio.h>
#include <TMC2130/TMC2130.h>
#include "FastSerial.h"
#include "Board.h"
#include "LCD1602.h"
#include "LED.h"
#include "iCLNGAbsEncoder.h"
#include "SharedSPI.h"
#include "Axis.h"
#include "pushtogo.h"
#include "PushToGoGUI.h"
#include "Buttons.h"
#include "ADL355.h"
#include "printf.h"
#include "USBSerial.h"

//FastSerial<UART_3> uart3(PC_10, PC_11, 115200);
//FastSerial<UART_5> uart5(PC_12, PD_2, 115200);
FastSerial<UART_2> pc(USBTX, USBRX, 1024000);
PinName lcdpins[] = { NC, NC, NC, NC, LCD_D4, LCD_D5, LCD_D6, LCD_D7 };
LCD1602 lcd(LCD_RS, LCD_RW, LCD_EN, lcdpins, LCD_BRIGHTNESS);
//ADL355 accel(PB_14, PB_13);

//SPI spi2(ENCODER_MOSI, ENCODER_MISO, ENCODER_SCK, ENCODER1_CS);
//SharedSPI spi2(ENCODER_MOSI, ENCODER_MISO, ENCODER_SCK, 8, 0, 1000000, true);

//SharedSPI spi(MOTOR_MOSI, MOTOR_MISO, MOTOR_SCK, 8, 3, 1000000);
//
//TMC2130 test(*spi.getInterface(MOTOR1_CS), MOTOR1_STEP, MOTOR1_DIR, MOTOR1_DIAG,
//		MOTOR1_IREF);
//TMC2130 test2(*spi.getInterface(MOTOR2_CS), MOTOR2_STEP, NC, NC, NC);
//
//Timer us_ticker;

//Thread rd_thd;

//void re_cb(int flag){
//	rd_thd.flags_set(0x7FFFFFFF);
//}

void pcprintf(const char *fmt, ...) {
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (len > 0)
		pc.write(buf, len);
}

//void reader(){
//	char ret[100];
//	while(1){
//		uart5.read((unsigned char *)ret, sizeof(ret), re_cb, SERIAL_EVENT_RX_ALL, 0xFF);
//		ThisThread::flags_wait_any(0x7FFFFFFF);
//		printf(ret);
//		printf("\n");
//		ThisThread::sleep_for(2000);
//		uint32_t tick_start = get_tick();
//		int len = uart3.read(ret, sizeof(ret));
//		uint32_t tick = (get_tick() - tick_start);
//		pcprintf("%d us\n", tick/80);
//		if (len == -EAGAIN){
//			pc.write("ERROR\n", 6);
//		}
//		else{
//			pc.write(ret, len);
//			pc.write(".", 1);
//		}
//	}
//}

// Override the default fatal error handlers
extern "C" void mbed_die(void) {
#if !defined (NRF51_H) && !defined(TARGET_EFM32)
	core_util_critical_section_enter();
#endif
	gpio_t led_err;
	gpio_init_out(&led_err, LED3);

	while (1) {
		for (int i = 0; i < 4; ++i) {
			gpio_write(&led_err, 1);
			wait_us(150000);
			gpio_write(&led_err, 0);
			wait_us(150000);
		}

//		for (int i = 0; i < 4; ++i) {
//			gpio_write(&led_err, 1);
//			wait_us(400000);
//			gpio_write(&led_err, 0);
//			wait_us(400000);
//		}
	}
}

extern "C" void error(const char *format, ...) {
	static char buffer[257];
	core_util_critical_section_enter();
#ifndef NDEBUG
	va_list arg;
	va_start(arg, format);
	int size = vsnprintf(buffer, sizeof(buffer) - 1, format, arg);
	if (size >= (int) sizeof(buffer)) {
		// Properly terminate the string
		buffer[sizeof(buffer) - 1] = '\0';
	}
	va_end(arg);


	gpio_t led_err;
	gpio_init_out(&led_err, LED3);

	while(1){
		for (int i = 0; i < 5; ++i) {
			gpio_write(&led_err, 1);
			wait_us(150000);
			gpio_write(&led_err, 0);
			wait_us(150000);
		}

		lcd.clear();
		// Print multi-row message
		int row = 0;
		char *p = buffer;
		int ss = size;
		while (ss > 0) {
			lcd.setPosition(row, 0);
			lcd.write(p, ss < 16 ? ss : 16);
			p += 16;
			ss -= 16;
			row++;
			if (row == 2 && ss > 0) {
				// Scroll
				wait_us(1000000);
				lcd.clear();
				lcd.setPosition(0, 0);
				lcd.write(p - 16, 16);
				row = 1;
			}
		}
	}
#endif

//	mbed_die();

	core_util_critical_section_exit(); // Will not reach here
}

extern "C" MBED_NORETURN mbed_error_status_t mbed_error(
		mbed_error_status_t error_status, const char *error_msg,
		unsigned int error_value, const char *filename, int line_number) {
	error("Error: 0x%x, %s", error_status, error_msg);
}


EquatorialMount *eqMount;
extern USBSerial *serial_usb;
Thread led1_thread(osPriorityBelowNormal, 512, NULL, "LED1 thread");
Thread led2_thread(osPriorityBelowNormal, 512, NULL, "LED2 thread");


LED led1(LED1);
LED led2(LED2);

PushToGo_GUI *gui;

Buttons buttons;

void led1_thread_entry() {
	float val = 0;
	int step = 10; // ms
	int dir = 1;
	while (1) {
		mountstatus_t status = eqMount->getStatus();
		switch(status){
		case MOUNT_TRACKING:
		case MOUNT_TRACKING_GUIDING:
			val += dir * 0.01;
			if (val > 1){
				val = 1;
				dir = -1;
			}
			else if (val < 0){
				val	= 0;
				dir = 1;
			}
			led1 = val;
			ThisThread::sleep_for(step);
			break;
		case MOUNT_SLEWING:
		case MOUNT_NUDGING:
		case MOUNT_NUDGING_TRACKING:
			led1.on();
			ThisThread::sleep_for(100);
			led1.off();
			ThisThread::sleep_for(100);
			break;
		case MOUNT_STOPPED:
			led1.off();
			ThisThread::sleep_for(100);
			break;
		default:
			break;
		}
	}
}

void _usb_cb(){
	led2=0.5;
}

void led2_thread_entry() {
	serial_usb->attach(_usb_cb);
	while(1){
		if (led2){
			ThisThread::sleep_for(200);
			led2=0;
		}
		ThisThread::sleep_for(10);
	}
}

#include "mbed_mem_trace.h"

int main() {
//	NVStore::get_instance().reset();
    mbed_mem_trace_set_callback(mbed_mem_trace_default_callback);

	eqMount = &telescopeHardwareInit();
	telescopeServerInit();

	gui = new PushToGo_GUI(&lcd, eqMount);
	gui->listenTo(&buttons);
	gui->startGUI();


	// Start LED1 thread
	led1_thread.start(led1_thread_entry);
	// Start LED2 thread
	led2_thread.start(led2_thread_entry);


	while (1) {
//		uart3.write("abcdefghijklmnopqrstuvwxyz123456", 32);
//		uart3.write(s, sizeof(s));
		ThisThread::sleep_for(100);
//		double x, y, z;
//		accel.getAcceleration(x, y, z);
//		accel.getTilt(x,y);
//		time_t t = time(NULL);
//		lcd.clear();
//		lcd.setPosition(0, 0);
//		int now = dec_encoder->readPos();
//		lcd.printf("%5d", now);
//		lcd.fillWith(' ', 5);
//		lcd.setPosition(1, 0);
//		lcd.printf("%5d", now - last);
//		last = now;
//		pcprintf("x=%lf y=%lf z=%lf\r\n", x, y, z);
//		pcprintf("x=%lf y=%lf\r\n", x, y);
//		test.testmove();
//		Thread::wait(500);
//		test.stop();
//		test.setCurrent(1.1);
//		test.setFrequency(2000);
//		test.start(STEP_BACKWARD);
//		wait(1.5);
//		test.stop();
//		test.start(STEP_FORWARD);
//		wait(3);
//		test.stop();
//		wait(5);

//		test_axis.slewTo(AXIS_ROTATE_POSITIVE, 10);
//		wait(2);
//		test_axis.slewTo(AXIS_ROTATE_NEGATIVE, 0);

//		test_axis.startTracking(AXIS_ROTATE_POSITIVE);
//		wait(10);
//		test_axis.stop();
//		test.poweroff();
	}
}
