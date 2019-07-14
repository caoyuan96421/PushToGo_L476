#include "mbed.h"
#include <stdio.h>
#include "FastSerial.h"

FastSerial<UART_3> uart3(PC_10, PC_11, 115200);
FastSerial<UART_5> uart5(PC_12, PD_2, 115200);
UARTSerial pc(USBTX, USBRX, 1024000);

const char s[] = "qwertyuioasdfhzxcvnmb,z ljzxcl v.zxcv lau46o5t ualsd7ugou zxjncbvlk 7uzxoibuj lkajklsdg yu8ozpfsugob jzxlcj vlk jzxklcbv7 890zxc7ubvo jzxclbvjkl; zxunc0b987 890zpxc87bvo zhxclv jlzxjvio zx7b890p7u zxo0cbhj lkzxcjgkljzsop89g 7zsf90d78g90p zdujflgbknzjkl;dfhjg9 8pz7sr0gt n8aeorg;zsdfr fgkl;zjxkl;fvj oipzxc7v09- 7890-347850-9b8 a340tajsrklg jklzsjg 7z89fdg790- a0243 t50ja34o5 jaer9-t8 0=afsujvgoipj zkopxjcvk jz90x=c8 ";

Timer us_ticker;

Thread rd_thd;

//void re_cb(int flag){
//	rd_thd.flags_set(0x7FFFFFFF);
//}

void reader(){
	char ret[32];
	while(1){
//		uart5.read((unsigned char *)ret, sizeof(ret), re_cb, SERIAL_EVENT_RX_ALL, 0xFF);
//		ThisThread::flags_wait_any(0x7FFFFFFF);
//		printf(ret);
//		printf("\n");
		uart3.read(ret, sizeof(ret));
		pc.write(ret, sizeof(ret));
		pc.write(".", 1);
	}
}

volatile unsigned int *DWT_CYCCNT  ;
volatile unsigned int *DWT_CONTROL ;
volatile unsigned int *SCB_DEMCR   ;
void reset_timer(){
    DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
    DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register
    SCB_DEMCR    = (volatile unsigned int *)0xE000EDFC; //address of the register
    *SCB_DEMCR   = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT  = 0; // reset the counter
//    *DWT_CONTROL = 0;
}

void start_timer(){
    *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter
}

void stop_timer(){
    *DWT_CONTROL = *DWT_CONTROL | 0 ; // disable the counter
}

int main(){
//	__HAL_FLASH_PREFETCH_BUFFER_ENABLE(); // Enable prefetch to enhance
//	__HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
//	__HAL_FLASH_DATA_CACHE_ENABLE();
	reset_timer();
	start_timer();
	pc.write("start\r\n", 8);
	rd_thd.start(reader);
	us_ticker.start();
	uart3.write("abcdefghijklmnopqrstuvwxyz123456", 32);
	while(1){
//		uart3.write(s, sizeof(s));
		Thread::wait(500);
		pc.write("\r\n", 2);
		Thread::wait(500);
	}
}
