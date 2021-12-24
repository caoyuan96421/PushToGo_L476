/*
 * USB.h
 *
 *  Created on: Jul 28, 2020
 *      Author: caoyu
 */

#ifndef _USB_H_
#define _USB_H_

#include "USBSerial.h"
#include "EquatorialMount.h"


void usbServerInit(EquatorialMount *);
void usbServerDeinit();

bool usbMSDStart();
void usbMSDStop();

//void xprintf(const char* format, ...);
//void pcprintf(const char *fmt, ...);
//int get_us();

#endif /* _USB_H_ */
