/**
 * Harware setup is implemented in this file
 */

#include "telescope_hardware.h"
#include "TMC2130.h"
#include "iCLNGAbsEncoder.h"
#include "EquatorialMount.h"
#include "RTCClock.h"
#include "TelescopeConfiguration.h"
#include "EqMountServer.h"
#include "USBSerial.h"
#include "FastSerial.h"
#include "SharedSPI.h"
#include "Board.h"
#include "Axis.h"
#include "ADL355.h"

// Motor SPI
SharedSPI spi1(MOTOR_MOSI, MOTOR_MISO, MOTOR_SCK, 8, 3, 1000000);

// Encoder SPI
// Pullup on MISO
SharedSPI spi2(ENCODER_MOSI, ENCODER_MISO, ENCODER_SCK, 8, 3, 4000000, true);

//class DummyStepper: public StepperMotor {
//public:
//	DummyStepper() {
//		state = IDLE;
//		dir = STEP_FORWARD;
//		freq = 0;
//		stepCount = 0;
//		tim.start();
//	}
//	virtual ~DummyStepper() {
//	}
//	virtual void start(stepdir_t dir) {
//		if (state == IDLE) {
//			this->dir = dir;
//			tim.reset();
//			state = RUNNING;
//		}
//	}
//	virtual void stop() {
//		if (state == RUNNING) {
//			stepCount += freq * tim.read_high_resolution_us() / 1E6
//					* (dir == STEP_BACKWARD ? -1 : 1);
//			state = IDLE;
//		}
//	}
//	virtual double setFrequency(double freq) {
//		if (state == RUNNING) {
//			stop();
//			start(dir);
//		}
//		this->freq = freq;
//		return freq;
//	}
//	virtual double getFrequency() {
//		return freq;
//	}
//	virtual void setStepCount(double sc) {
//		stepCount = sc;
//	}
//	virtual double getStepCount() {
//		if (state == RUNNING)
//			return stepCount
//					+ freq * tim.read_high_resolution_us() / 1E6
//							* (dir == STEP_BACKWARD ? -1 : 1);
//		else
//			return stepCount;
//	}
//private:
//	enum {
//		IDLE, RUNNING
//	} state;
//	double freq;
//	double stepCount;
//	stepdir_t dir;
//	Timer tim;
//};

// Steppers
StepperMotor *ra_stepper;
//TMC2130 *dec_stepper;
StepperMotor *dec_stepper; // Dummy

// Encoders
iCLNGAbsEncoder *ra_encoder;
iCLNGAbsEncoder *dec_encoder;

#include "mbed_mktime.h"
/**
 * Clock & Location object
 */
class RTCClock_HR: public RTCClock {
public:
	double getTimeHighResolution() {
		return rtc_read_hr();
	}
private:
	static double rtc_read_hr(void) {
#if TARGET_STM32F1

	    RtcHandle.Instance = RTC;
	    return RTC_ReadTimeCounter(&RtcHandle);

	#else /* TARGET_STM32F1 */

		struct tm timeinfo;

		/* Since the shadow registers are bypassed we have to read the time twice and compare them until both times are the same */
		uint32_t Read_time = RTC->TR & RTC_TR_RESERVED_MASK;
		uint32_t Read_date = RTC->DR & RTC_DR_RESERVED_MASK;
		uint32_t Read_subsec = RTC->SSR;

		while ((Read_time != (RTC->TR & RTC_TR_RESERVED_MASK))
				|| (Read_date != (RTC->DR & RTC_DR_RESERVED_MASK))
				|| (Read_subsec != RTC->SSR)) {
			Read_time = RTC->TR & RTC_TR_RESERVED_MASK;
			Read_date = RTC->DR & RTC_DR_RESERVED_MASK;
			Read_subsec = RTC->SSR;
		}

		/* Setup a tm structure based on the RTC
		 struct tm :
		 tm_sec      seconds after the minute 0-61
		 tm_min      minutes after the hour 0-59
		 tm_hour     hours since midnight 0-23
		 tm_mday     day of the month 1-31
		 tm_mon      months since January 0-11
		 tm_year     years since 1900
		 tm_yday     information is ignored by _rtc_maketime
		 tm_wday     information is ignored by _rtc_maketime
		 tm_isdst    information is ignored by _rtc_maketime
		 */
		timeinfo.tm_mday = RTC_Bcd2ToByte(
				(uint8_t) (Read_date & (RTC_DR_DT | RTC_DR_DU)));
		timeinfo.tm_mon = RTC_Bcd2ToByte(
				(uint8_t) ((Read_date & (RTC_DR_MT | RTC_DR_MU)) >> 8)) - 1;
		timeinfo.tm_year = RTC_Bcd2ToByte(
				(uint8_t) ((Read_date & (RTC_DR_YT | RTC_DR_YU)) >> 16)) + 68;
		timeinfo.tm_hour = RTC_Bcd2ToByte(
				(uint8_t) ((Read_time & (RTC_TR_HT | RTC_TR_HU)) >> 16));
		timeinfo.tm_min = RTC_Bcd2ToByte(
				(uint8_t) ((Read_time & (RTC_TR_MNT | RTC_TR_MNU)) >> 8));
		timeinfo.tm_sec = RTC_Bcd2ToByte(
				(uint8_t) ((Read_time & (RTC_TR_ST | RTC_TR_SU)) >> 0));

		// Convert to timestamp
		time_t t;
		if (_rtc_maketime(&timeinfo, &t, RTC_4_YEAR_LEAP_YEAR_SUPPORT)
				== false) {
			return 0;
		}

		uint32_t PREDIV_S = (RTC->PRER & RTC_PRER_PREDIV_S_Msk);

		// Add fractional part
		return (double) t + (double) (PREDIV_S - Read_subsec) / (PREDIV_S + 1);

#endif /* TARGET_STM32F1 */
	}
} clk;
LocationProvider location;

class TMCAxis: public Axis {
public:
	TMCAxis(double stepsPerDeg, StepperMotor *stepper, Encoder *enc = NULL,
			const char *name = "Axis") :
			Axis(stepsPerDeg, stepper, enc, name) {
	}
	virtual void idle_mode() {
		((TMC2130*) stepper)->setStealthChop(true);
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
};

Axis *ra_axis = NULL;
//TMCAxis *dec_axis = NULL;
Axis *dec_axis = NULL; // Dummy
EquatorialMount *eq_mount = NULL;

// Accelerameter
ADL355 accel(PB_14, PB_13);

static void add_sys_commands();

EquatorialMount& telescopeHardwareInit() {
	// Read configuration from NVStore
	TelescopeConfiguration::readConfig_NV();

	// Object re-initialization
	if (ra_axis != NULL) {
		delete ra_axis;
	}
	if (dec_axis != NULL) {
		delete dec_axis;
	}
	if (eq_mount != NULL) {
		delete eq_mount;
	}
	if (ra_stepper != NULL) {
		delete ra_stepper;
	}
	if (dec_stepper != NULL) {
		delete dec_stepper;
	}
	if (ra_encoder != NULL) {
		delete ra_encoder;
	}
	if (dec_encoder != NULL) {
		delete dec_encoder;
	}

	double stepsPerDeg = TelescopeConfiguration::getDouble("motor_steps")
			* TelescopeConfiguration::getDouble("gear_reduction")
			* TelescopeConfiguration::getDouble("worm_teeth") / 360.0;

	ra_encoder = new iCLNGAbsEncoder(spi2.getInterface(ENCODER1_CS));
	dec_encoder = new iCLNGAbsEncoder(spi2.getInterface(ENCODER2_CS));

	ra_encoder->setDirection(TelescopeConfiguration::getBool("ra_enc_invert"));
	dec_encoder->setDirection(
			TelescopeConfiguration::getBool("dec_enc_invert"));

	ra_stepper = new TMC2130(*spi1.getInterface(MOTOR1_CS), MOTOR1_STEP,
	MOTOR1_DIR, MOTOR1_DIAG,
	MOTOR1_IREF, TelescopeConfiguration::getBool("ra_invert"));

	dec_stepper = new TMC2130(*spi1.getInterface(MOTOR2_CS), MOTOR2_STEP,
	MOTOR2_DIR, MOTOR2_DIAG,
	MOTOR2_IREF, TelescopeConfiguration::getBool("dec_invert"));

	ra_axis = new TMCAxis(stepsPerDeg, ra_stepper,
			TelescopeConfiguration::getBool("ra_use_encoder") ?
					ra_encoder : NULL, "RA_Axis");
	dec_axis = new TMCAxis(stepsPerDeg, dec_stepper,
			TelescopeConfiguration::getBool("dec_use_encoder") ?
					dec_encoder : NULL, "DEC_Axis");
	eq_mount = new EquatorialMount(*ra_axis, *dec_axis, clk, location);

	// Set accelerator if available
	eq_mount->setInclinometer(&accel);
	eq_mount->getInclinometer()->refresh();

	return (*eq_mount); // Return reference to eq_mount
}

/* Serial connection from hand controller*/
typedef FastSerial<UART_3> HC_Serial;
HC_Serial *serial_hc = NULL;
EqMountServer *server_hc = NULL;

/* USB connection from PC*/
USBSerial *serial_usb = new USBSerial(false);
EqMountServer *server_usb = NULL;

bool serverInitialized = false;


osStatus telescopeServerInit() {
	if (eq_mount == NULL)
		return osErrorResource;
	if (!serverInitialized) {
		// Only run once
		serverInitialized = true;
		add_sys_commands();
	} else {
		return osErrorResource;
	}

	if (!serial_hc) {
		serial_hc = new HC_Serial(HC_TX, HC_RX,
				TelescopeConfiguration::getInt("serial_baud"));
		server_hc = new EqMountServer(*serial_hc, false);
	}
	server_hc->bind(*eq_mount);

	if (!server_usb) {
		serial_usb->connect();
		server_usb = new EqMountServer(*serial_usb, false);
	}
	server_usb->bind(*eq_mount);

	return osOK;
}

#include "rtx_lib.h"

static int eqmount_sys(EqMountServer *server, const char *cmd, int argn,
		char *argv[]) {
	const int THD_MAX = 32;
	osThreadId_t thdlist[THD_MAX];
	int nt = osThreadEnumerate(thdlist, THD_MAX);

	svprintf(server, "Thread list: \r\n");
	for (int i = 0; i < nt; i++) {
		osThreadState_t state = osThreadGetState(thdlist[i]);
		const char *s = "";
		const char *n;
		osPriority_t prio = osThreadGetPriority(thdlist[i]);

//		if (prio == osPriorityIdle) {
//			n = "Idle thread";
//		} else {
		n = osThreadGetName(thdlist[i]);
		if (n == NULL)
			n = "System thread";
//		}

		switch (state) {
		case osThreadInactive:
			s = "Inactive";
			break;
		case osThreadReady:
			s = "Ready";
			break;
		case osThreadRunning:
			s = "Running";
			break;
		case osThreadBlocked:
			s = "Blocked";
			break;
		case osThreadTerminated:
			s = "Terminated";
			break;
		case osThreadError:
			s = "Error";
			break;
		default:
			s = "Unknown";
			break;
		}
		uint32_t pc;

		if (thdlist[i] == ThisThread::get_id()) {
			uint32_t _pc;
			__asm__ __volatile__ ("mov %0, pc" : "=r" (_pc));
			pc = _pc;
		} else {
			osRtxThread_t *thd = osRtxThreadId(thdlist[i]);
			uint32_t sp = thd->sp;
			pc = *((uint32_t*) (sp
					+ (((thd->stack_frame & 0x10) == 0) ? 0x78 : 0x38))); // Get PC from the stack frame
		}

		svprintf(server, " - %10s 0x%08x %3d %s \r\n", s, pc, (int) prio, n);
	}

//	stprintf(server->getStream(), "\r\nRecent CPU usage: %.1f%%\r\n",
//			MCULoadMeasurement::getInstance().getCPUUsage() * 100);
	return 0;
}

static int eqmount_systime(EqMountServer *server, const char *cmd, int argn,
		char *argv[]) {
	char buf[32];
	time_t t = time(NULL);

#if !( defined(__ARMCC_VERSION) || defined(__CC_ARM) )
	ctime_r(&t, buf);
#else
	core_util_critical_section_enter();
	char *ibuf = ctime(&t);
	strncpy(buf, ibuf, sizeof(buf));
	core_util_critical_section_exit();
#endif

	svprintf(server, "Current UTC time: %s\r\n", buf);

	return 0;
}

static int eqmount_reboot(EqMountServer *server, const char *cmd, int argn,
		char *argv[]) {
	NVIC_SystemReset();
	return 0;
}

static int eqmount_save(EqMountServer *server, const char *cmd, int argn,
		char *argv[]) {
	if (argn != 0) {
		svprintf(server, "Usage: save\r\n");
		return -1;
	}
	TelescopeConfiguration::saveConfig_NV();
	return 0;
}

static void add_sys_commands() {
	EqMountServer::addCommand(
			ServerCommand("sys", "Print system information", eqmount_sys));
	EqMountServer::addCommand(
			ServerCommand("systime", "Print system time", eqmount_systime));
	EqMountServer::addCommand(
			ServerCommand("reboot", "Reboot the system", eqmount_reboot));
	EqMountServer::addCommand(
			ServerCommand("save", "Save configuration file", eqmount_save));
}

