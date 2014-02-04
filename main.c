#include <inc/lm3s9b96.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/ethernet.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/uart.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/timer.h>
#include <utils/uartstdio.h>
#include <httpserver_raw/httpd.h>
#include <utils/locator.h>
#include <utils/lwiplib.h>
#include "rollo.h"

volatile unsigned char ticks;

// A twirling line used to indicate that DHCP/AutoIP address acquisition is in
// progress.
static char g_pcTwirl[4] = { '\\', '|', '/', '-' };

// The index into the twirling line array of the next line orientation to be
// printed.
static unsigned long g_ulTwirlPos = 0;

//*****************************************************************************
//
// The most recently assigned IP address.  This is used to detect when the IP
// address has changed (due to DHCP/AutoIP) so that the new address can be
// printed.
//
//*****************************************************************************
static unsigned long g_ulLastIPAddr = 0;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void lwIPHostTimerHandler(void) {
    unsigned long ulIPAddress;

    // Get the local IP address.
    ulIPAddress = lwIPLocalIPAddrGet();

    // See if an IP address has been assigned.
    if(ulIPAddress == 0) {
        // Draw a spinning line to indicate that the IP address is being
        // discoverd.
        UARTprintf("\b%c", g_pcTwirl[g_ulTwirlPos]);

        // Update the index into the twirl.
        g_ulTwirlPos = (g_ulTwirlPos + 1) & 3;
    } else if(ulIPAddress != g_ulLastIPAddr) { // Check if IP address has changed, and display if it has.
        // Display the new IP address.
        UARTprintf("\rIP: %d.%d.%d.%d       \n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);

        // Save the new IP address.
        g_ulLastIPAddr = ulIPAddress;

        // Display the new network mask.
        ulIPAddress = lwIPLocalNetMaskGet();
        UARTprintf("Netmask: %d.%d.%d.%d\n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);

        // Display the new gateway address.
        ulIPAddress = lwIPLocalGWAddrGet();
        UARTprintf("Gateway: %d.%d.%d.%d\n", ulIPAddress & 0xff,
                   (ulIPAddress >> 8) & 0xff, (ulIPAddress >> 16) & 0xff,
                   (ulIPAddress >> 24) & 0xff);
    }
}

// Aufruf jede Millisekunde (interrupt)
void SysTickHandler(void) {
	ticks++;
	rollo_Tick();
}

int main(void) {
    int i;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ); // 80 MHz
    ROM_SysTickPeriodSet(80000L); // 1 ms Tick
	ROM_SysTickEnable();
	ROM_SysTickIntEnable();
	ROM_IntMasterEnable();
    rollo_init();
    SysTickIntRegister(SysTickHandler);
	UARTprintf("\nRollocontrol v0.4 (Martin Ongsiek)\n");
    //readSettingsFromEerpom();
	//initialRelayTest();
	while (1) {
	 	if (ticks >= 10) {
	 		ticks -= 10;
			rollo_Cont();
			
	 	}
	}
	return 0;
}