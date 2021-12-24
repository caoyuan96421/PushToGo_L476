/*
 * USB.cpp
 *
 *  Created on: Jul 28, 2020
 *      Author: caoyu
 */

#include "USB.h"
#include "mbed.h"

#include "Board.h"
#include "USBMSD.h"
#include "FlashIAPBlockDevice.h"
#include "SDBlockDevice.h"
#include "USBPhyHw.h"
#include "usb_phy_api.h"

#include "FastSerial.h"
#include "printf.h"
#include "Logger.h"

//Thread printer_th(osPriorityNormal, 2048, NULL, "Printer Thread");

/* USB connection from PC*/
USBSerial *serial_usb = NULL;
static EqMountServer *server_usb = NULL;
extern void _usb_cb();
//void printer();
//Timer tim;

void usbServerInit(EquatorialMount *eq_mount){
	if (!serial_usb) {
		 serial_usb= new USBSerial(false);
		 serial_usb->connect();
		// Start LED2 thread
		serial_usb->attach(_usb_cb);
	}
	if (!server_usb) {
		server_usb = new EqMountServer(*serial_usb, false);
	}
	server_usb->bind(*eq_mount);

	// Fix for this particular board: set PA_10 to GPIO for use as SPI CS
	pin_function(MOTOR2_CS, STM_PIN_DATA(STM_MODE_OUTPUT_PP, PullNone, 0));

    Logger::log("USB Serial Server Started.");
}

void usbServerDeinit(){
	// First disconnect usb to break the server out of its loop
	if (serial_usb){
		serial_usb->deinit();
	}
	if (server_usb) {
		delete server_usb;
		server_usb = NULL;
	}
	if (serial_usb) {
		delete serial_usb;
		serial_usb = NULL;
	}

    Logger::log("USB Serial Server Stopped.");
}

extern SDBlockDevice sd_bd;
Semaphore msd_sem(0,1);
bool should_terminate = false;

static void usb_msd_signal();
static void usb_msd_serve();
static Thread *usb_msd_thread = NULL;


#define	MSD_TERMINATE 	1
#define MSD_PROCESS		2

bool usbMSDStart(){
	if (usb_msd_thread){
		// Already started
		return false;
	}

	// Init
	msd_sem.try_acquire(); // Make sure it's empty
	should_terminate = false;

	// Stop Logger

    Logger::log("Mounting on USB...");
	Logger::deinit();


	// Init SD card
	if(sd_bd.init() != BD_ERROR_OK)
		return false;


	// Start processing thread
	usb_msd_thread = new Thread(osPriorityHigh1, 4096, NULL, "USB MSD Thread");
	usb_msd_thread->start(usb_msd_serve);
	return true;
}
void usbMSDStop(){
	if (!usb_msd_thread){
		return;
	}
	if (usb_msd_thread->get_state() == Thread::Deleted){
		delete usb_msd_thread;
		usb_msd_thread = NULL;
		return;
	}

	// Set flag to terminate and delete msd
	should_terminate = true;

	// Wait for the thread to finish
	usb_msd_thread->join();

	// Delete thread
	delete usb_msd_thread;
	usb_msd_thread = NULL;

	// Re-init Logger
	Logger::init(&sd_bd);
    Logger::log("USB Unmounted.");

}

static void usb_msd_signal(){
	// Called by USB_MSD to signal it needs processing
	msd_sem.release();
}

static void usb_msd_serve(){
	// Mount on USB
	USBMSD *msd = new USBMSD(&sd_bd, true, 0x1234, 0x5678);

	// Fix for this particular board: set PA_10 to GPIO for use as SPI CS
	pin_function(MOTOR2_CS, STM_PIN_DATA(STM_MODE_OUTPUT_PP, PullNone, 0));

	// Attach ISR
	msd->attach(usb_msd_signal);
	while(!should_terminate){
		if(msd_sem.try_acquire_for(100ms)) {
			msd->process();
		}
	}
	// Delete msd
	delete msd;
	msd = NULL;
}


///* Mail */
//typedef struct
//{
//	int time;
//	char msg[12];
//} mail_t;
//
//typedef Mail<mail_t, 16> MB_t;
//MB_t mbox;
//
//FastSerial<UART_2> pc(USBTX, USBRX, 1024000, false);
//
//void pcprintf(const char *fmt, ...) {
//	char buf[1024];
//
////	itoa(get_us(), buf, 10);
////	pc.write(buf, strlen(buf));
//
//	va_list args;
//	va_start(args, fmt);
//	int len = vsnprintf(buf, sizeof(buf), fmt, args);
//	va_end(args);
//
//	if (len > 0) {
//		pc.write(buf, len);
//		pc.write("\r\n", 2);
//	}
//}
//
//void printer()
//{
//	while (true)
//	{
//		mail_t *m = (mail_t *) mbox.get().value.p;
//		pcprintf("[%d]%s\r\n", m->time, m->msg);
//		mbox.free(m);
//	}
//}
//
///**
// * Printf for debugging use. Takes about 20us for each call. Can be called from any context
// */
//void xprintf(const char* format, ...)
//{
//	va_list argptr;
//	va_start(argptr, format);
//
//	core_util_critical_section_enter();
//	mail_t *m = mbox.alloc();
//	if (m == NULL){
//		core_util_critical_section_exit();
//		va_end(argptr);
//		return;
//	}
//	m->time = tim.read_us();
//	vsnprintf(m->msg, sizeof(m->msg), format, argptr);
//	m->msg[15] = 0;
//	mbox.put(m);
//	core_util_critical_section_exit();
//	va_end(argptr);
//}
//
//int get_us(){
//	return tim.read_us();
//}
