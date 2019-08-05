/**
 * Harware setup is implemented in this file
 */

#include "telescope_hardware.h"
#include "TMC2130.h"
#include "EquatorialMount.h"
#include "RTCClock.h"
#include "TelescopeConfiguration.h"
#include "EqMountServer.h"
#include "USBSerial.h"
#include "FastSerial.h"
#include "SharedSPI.h"
#include "Board.h"
#include "Axis.h"

// Motor SPI
SharedSPI spi(MOTOR_MOSI, MOTOR_MISO, MOTOR_SCK, 8, 3, 1000000);

class DummyStepper : public StepperMotor {
public:
	DummyStepper() {
		state = IDLE;
		dir = STEP_FORWARD;
		freq = 0;
		stepCount = 0;
		tim.start();
	}
	virtual ~DummyStepper(){
	}
	virtual void start(stepdir_t dir){
		this->dir = dir;
		tim.reset();
		state = RUNNING;
	}
	virtual void stop(){
		stepCount += freq * tim.read_high_resolution_us() / 1E6;
		state = IDLE;
	}
	virtual double setFrequency(double freq){
		if (RUNNING){
			stop();
			start(dir);
		}
		this->freq = freq;
		return freq;
	}
	virtual double getFrequency(){
		return freq;
	}
	virtual void setStepCount(double sc){
		stepCount= sc;
	}
	virtual double getStepCount(){
		if (state == RUNNING)
			return stepCount + freq * tim.read_high_resolution_us() / 1E6;
		else
			return stepCount;
	}
private:
	enum {
		IDLE, RUNNING
	} state;
	double freq;
	double stepCount;
	stepdir_t dir;
	Timer tim;
};

// Steppers
TMC2130 *ra_stepper;
//TMC2130 *dec_stepper;
StepperMotor *dec_stepper; // Dummy

/**
 * Clock & Location object
 */
RTCClock clk;
LocationProvider location;

class TMCAxis: public Axis {
public:
	TMCAxis(double stepsPerDeg, StepperMotor *stepper,
			const char *name = "Axis") :
			Axis(stepsPerDeg, stepper, name) {
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
};

TMCAxis *ra_axis = NULL;
//TMCAxis *dec_axis = NULL;
Axis *dec_axis = NULL; // Dummy
EquatorialMount *eq_mount = NULL;

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

	double stepsPerDeg = TelescopeConfiguration::getDouble("motor_steps")
			* TelescopeConfiguration::getDouble("gear_reduction")
			* TelescopeConfiguration::getDouble("worm_teeth") / 360.0;

	ra_stepper = new TMC2130(*spi.getInterface(MOTOR1_CS), MOTOR1_STEP,
			MOTOR1_DIR, MOTOR1_DIAG,
			MOTOR1_IREF, TelescopeConfiguration::getBool("ra_invert"));
//	dec_stepper = new TMC2130(*spi.getInterface(MOTOR2_CS), MOTOR2_STEP,
//			MOTOR2_DIR, MOTOR2_DIAG,
//			MOTOR2_IREF, TelescopeConfiguration::getBool("dec_invert"));
	dec_stepper = new DummyStepper();
	ra_axis = new TMCAxis(stepsPerDeg, ra_stepper, "RA_Axis");
//	dec_axis = new TMCAxis(stepsPerDeg, dec_stepper, "DEC_Axis");
	dec_axis = new Axis(stepsPerDeg, dec_stepper, "DEC_Axis");
	eq_mount = new EquatorialMount(*ra_axis, *dec_axis, clk, location);

	return (*eq_mount); // Return reference to eq_mount
}

/* Serial connection from hand controller*/
typedef FastSerial<UART_3> HC_Serial;
HC_Serial *serial_hc = NULL;
EqMountServer *server_hc = NULL;

/* USB connection from PC*/
USBSerial *serial_usb = NULL;
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
		serial_usb = new USBSerial(false);
		serial_usb->connect();
		server_usb = new EqMountServer(*serial_usb, false);
	}
	server_usb->bind(*eq_mount);

	return osOK;
}

static int eqmount_sys(EqMountServer *server, const char *cmd, int argn,
		char *argv[]) {
	const int THD_MAX = 32;
	osThreadId thdlist[THD_MAX];
	int nt = osThreadEnumerate(thdlist, THD_MAX);

	stprintf(server->getStream(), "Thread list: \r\n");
	for (int i = 0; i < nt; i++) {
		osThreadState_t state = osThreadGetState(thdlist[i]);
		const char *s = "";
		const char *n;
		osPriority_t prio = osThreadGetPriority(thdlist[i]);

		if (prio == osPriorityIdle) {
			n = "Idle thread";
		} else {
			n = osThreadGetName(thdlist[i]);
			if (n == NULL)
				n = "System thread";
		}

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
		stprintf(server->getStream(), " - %10s 0x%08x %3d %s \r\n", s,
				(uint32_t) thdlist[i], (int) prio, n);
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

	stprintf(server->getStream(), "Current UTC time: %s\r\n", buf);

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
		stprintf(server->getStream(), "Usage: save\r\n");
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

