#include <inc/lm3s9b96.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/uart.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/timer.h>
#include <utils/uartstdio.h>
#include "rollo.h"

int main(void) {
    int i;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ); // 80 MHz
    ROM_SysTickPeriodSet(80000L); // 1 ms Tick
	ROM_SysTickEnable();
	ROM_SysTickIntEnable();
	ROM_IntMasterEnable();
    rollo_init();
	UARTprintf("\nRollocontrol v0.4 (Martin Ongsiek)\n");
    //readSettingsFromEerpom();
	//initialRelayTest();
	while (1) {
	 	if (ticks >= 10) {
	 		ticks -= 10;
			rolloCont();
			
	 	}
	}
	return 0;
}