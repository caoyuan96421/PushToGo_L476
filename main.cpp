#include "mbed.h"
#include <stdio.h>
#include "FastSerial.h"
#include "Board.h"
#include "LCD1602.h"
#include "LED.h"
#include "TMC2130.h"
#include "SharedSPI.h"
#include "Axis.h"
#include "TelescopeConfiguration.h"

//FastSerial<UART_3> uart3(PC_10, PC_11, 115200);
//FastSerial<UART_5> uart5(PC_12, PD_2, 115200);
FastSerial<UART_2> pc(USBTX, USBRX, 1024000);
PinName lcdpins[] = { NC, NC, NC, NC, LCD_D4, LCD_D5, LCD_D6, LCD_D7 };
LCD1602 lcd(LCD_RS, LCD_RW, LCD_EN, lcdpins, LCD_BRIGHTNESS);
LED led(LED1);
SharedSPI spi(MOTOR_MOSI, MOTOR_MISO, MOTOR_SCK, 8, 3, 1000000);

TMC2130 test(*spi.getInterface(MOTOR1_CS), MOTOR1_STEP, MOTOR1_DIR, MOTOR1_DIAG,
		MOTOR1_IREF);
TMC2130 test2(*spi.getInterface(MOTOR2_CS), MOTOR2_STEP, NC, NC, NC);

class _Axis: public Axis {
public:
	_Axis() :
			Axis(800, &test) {
	}
	virtual void idle_mode() {
		((TMC2130*) stepper)->setStealthChop(false);
		stepper->setCurrent(0.3);
		stepper->setMicroStep(256);
	}
	virtual void slew_mode() {
		((TMC2130*) stepper)->setStealthChop(false);
		stepper->setCurrent(0.7);
		stepper->setMicroStep(32);
	}
	virtual void track_mode() {
		((TMC2130*) stepper)->setStealthChop(true);
		stepper->setCurrent(0.3);
		stepper->setMicroStep(256);
	}
} test_axis;
//DigitalOut dk(MOTOR1_IREF, 0);

//PwmOut lp1(MOTOR1_STEP);

//const char s[] = "qwertyuioasdfhzxcvnmb,z ljzxcl v.zxcv lau46o5t ualsd7ugou zxjncbvlk 7uzxoibuj lkajklsdg yu8ozpfsugob jzxlcj vlk jzxklcbv7 890zxc7ubvo jzxclbvjkl; zxunc0b987 890zpxc87bvo zhxclv jlzxjvio zx7b890p7u zxo0cbhj lkzxcjgkljzsop89g 7zsf90d78g90p zdujflgbknzjkl;dfhjg9 8pz7sr0gt n8aeorg;zsdfr fgkl;zjxkl;fvj oipzxc7v09- 7890-347850-9b8 a340tajsrklg jklzsjg 7z89fdg790- a0243 t50ja34o5 jaer9-t8 0=afsujvgoipj zkopxjcvk jz90x=c8 ";

Timer us_ticker;

Thread rd_thd;

//void re_cb(int flag){
//	rd_thd.flags_set(0x7FFFFFFF);
//}

volatile unsigned int *DWT_CYCCNT;
volatile unsigned int *DWT_CONTROL;
volatile unsigned int *SCB_DEMCR;
void reset_timer() {
	DWT_CYCCNT = (volatile unsigned int*) 0xE0001004; //address of the register
	DWT_CONTROL = (volatile unsigned int*) 0xE0001000; //address of the register
	SCB_DEMCR = (volatile unsigned int*) 0xE000EDFC; //address of the register
	*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
	*DWT_CYCCNT = 0; // reset the counter
//    *DWT_CONTROL = 0;
}

void start_timer() {
	*DWT_CONTROL = *DWT_CONTROL | 1; // enable the counter
}

void stop_timer() {
	*DWT_CONTROL = *DWT_CONTROL | 0; // disable the counter
}

inline uint32_t get_tick() {
	return *DWT_CYCCNT;
}

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

		for (int i = 0; i < 4; ++i) {
			gpio_write(&led_err, 1);
			wait_us(400000);
			gpio_write(&led_err, 0);
			wait_us(400000);
		}
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

	// Print multi-row message
	int row = 0;
	char *p = buffer;
	while (size > 0) {
		lcd.setPosition(row, 0);
		lcd.write(p, size < 16 ? size : 16);
		p += 16;
		size -= 16;
		row++;
		if (row == 2 && size > 0) {
			// Scroll
			wait_us(1000000);
			lcd.clear();
			lcd.setPosition(0, 0);
			lcd.write(p - 16, 16);
			row = 1;
		}
	}
#endif

	mbed_die();

	core_util_critical_section_exit(); // Will not reach here
}

extern "C" MBED_NORETURN mbed_error_status_t mbed_error(
		mbed_error_status_t error_status, const char *error_msg,
		unsigned int error_value, const char *filename, int line_number) {
	error("Error: 0x%x, %s", error_status, error_msg);
}

void blinker() {
	while (1) {
		led.on();
		wait(0.2);
		led.off();
		wait(0.2);
		test.debug();
	}
}

Thread blinker_thread;

int main() {
	reset_timer();
	start_timer();
	blinker_thread.start(blinker);
	pc.write("start\r\n", 8);
//	rd_thd.start(reader);
	us_ticker.start();
//	uart3.set_blocking(false);
//	set_time(186786934);

	lcd.setBrightness(0.4);

//	test.poweron();
//	test.setMicroStep(64);
//
//	test.setCurrent(0.4);
//	test.setFrequency(1600);
//	test_axis.startTracking(AXIS_ROTATE_POSITIVE);
//	test_axis.setSlewSpeed(6);
	TelescopeConfiguration::setDouble("goto_slew_speed", 4);
//	test_axis.startSlewingIndefinite(AXIS_ROTATE_POSITIVE);

	while (1) {
//		uart3.write("abcdefghijklmnopqrstuvwxyz123456", 32);
//		uart3.write(s, sizeof(s));
		ThisThread::sleep_for(200);
		time_t t = time(NULL);
		lcd.setPosition(0, 0);
		lcd.printf("%lld", t);
//		pcprintf("%lld\r\n", t);
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

		test_axis.slewTo(AXIS_ROTATE_POSITIVE, 10);
		wait(2);
		test_axis.slewTo(AXIS_ROTATE_NEGATIVE, 0);

//		test_axis.startTracking(AXIS_ROTATE_POSITIVE);
//		wait(10);
		test_axis.stop();
//		test.poweroff();
	}
}
