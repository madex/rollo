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
#include <stdio.h>
#include "rollo.h"

#define SWITCH_TIME 30 // *10ms

//#undef DEBUG
//#define DEBUG
#define NUM_INPUTS    36
#define NUM_TIMERS    32
#define NUM_OUTPUTS   10
#define DEBOUNCE_TIME  5

#define PORTA GPIO_PORTA_DATA_R
#define PORTB GPIO_PORTB_DATA_R
#define PORTD GPIO_PORTD_DATA_R


#define OUT_ALLE   (OUT(0) | OUT(1) | OUT(2) | OUT(3) | OUT(4) |  \
                    OUT(5) | OUT(6) | OUT(7) | OUT(8) | OUT(9))
#define OUT_TUEREN (OUT(0) | OUT(4) | OUT(5))
#define SET_TIME(hour, minute)   (hour*60*60 + minute*60)

volatile int secoundsOfDay;
volatile unsigned int timeInMs;
volatile unsigned char weekDay; // 0 montag 1 dienstag
unsigned long outputs;

#define P_SER_O GPIO_PIN_5 // PORTB
#define P_SER_I GPIO_PIN_3 // PORTD
#define P_RCK_O GPIO_PIN_6 // PORTB
#define P_SCK_O GPIO_PIN_0 // PORTD
#define P_SCK_I GPIO_PIN_4 // PORTD
#define P_RCK_I GPIO_PIN_7 // PORTA

#define IN_H1  2  // I1 PORTD 2
#define IN_R1  6  // I2 PORTA 6
#define IN_H2  3  // I3 PORTA 3
#define IN_R2  2  // I4 PORTA 2
#define IN_H3  5  // I5 PORTA 5
#define IN_R3  4  // I6 PORTA 4

typedef struct {
	unsigned short timer;
	char           name[30];
	event_t        event;
    unsigned long  outputs;
} input_t;

#define MO (1 << 0)
#define DI (1 << 1)
#define MI (1 << 2)
#define DO (1 << 3)
#define FR (1 << 4)
#define SA (1 << 5)
#define SO (1 << 6)

#define MO_FR    (MO | DI | MI | DO | FR)
#define WE       (SA | SO)

timeEvent_t timeEvents[NUM_TIMERS] = {
{WE,    SET_TIME( 9,30), EVT_UP,   OUT_ALLE,   "WE Hoch"},
{WE,    SET_TIME(19,30), EVT_DOWN, OUT_ALLE,   "WE Runter"},
{MO_FR, SET_TIME( 7,30), EVT_UP,   OUT_ALLE,   "Wochentags Hoch"},
{MO_FR, SET_TIME(19,30), EVT_DOWN, OUT_ALLE,   "Wochentags Runter"},
{WE,    SET_TIME(18,45), EVT_DOWN, OUT_TUEREN, "WE Tueren"},
{MO_FR, SET_TIME(18,45), EVT_DOWN, OUT_TUEREN, "Wochentags Tueren"},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
{0,0,0,0,""},
};

typedef enum {
    UP_START,
    UP,
    DOWN_START,
    DOWN,
    STOP_START,
    STOP,
} outputState_t;

typedef enum {
    ROLLO,   // events UP, EVT_DOWN und OFF
    OUTPUT   // ON und OFF
} outputType_t;

// Speicher für die Eingänge.
input_t inputs[NUM_INPUTS] = {
 {0, "Wohnzimmmer rechts", EVT_DOWN,  OUT(0)},
 {0, "Wohnzimmmer rechts", EVT_UP,    OUT(0)},
 {0, "Wohnzimmmer links",  EVT_DOWN,  OUT(4)},
 {0, "Wohnzimmmer links",  EVT_UP,    OUT(4)},
 {0, "Kueche Tuer",        EVT_DOWN,  OUT(5)},
 {0, "Kueche Tuer",        EVT_UP,    OUT(5)},
 {0, "Kueche Fenster",     EVT_DOWN,  OUT(2)},
 {0, "Kueche Fenster",     EVT_UP,    OUT(2)},
 {0, "Gaeste WC",          EVT_DOWN,  OUT(1)},
 {0, "Gaeste WC",          EVT_UP,    OUT(1)},
 {0, "Technik",            EVT_DOWN,  OUT(3)},
 {0, "Technik",            EVT_UP,    OUT(3)},
 {0, "Eltern",             EVT_DOWN,  OUT(6)},
 {0, "Eltern",             EVT_UP,    OUT(6)},
 {0, "Kind rechts",        EVT_DOWN,  OUT(7)},
 {0, "Kind rechts",        EVT_UP,    OUT(7)},
 {0, "Kind links",         EVT_DOWN,  OUT(8)},
 {0, "Kind links",         EVT_UP,    OUT(8)},
 {0, "Bad",                EVT_DOWN,  OUT(9)},
 {0, "Bad",                EVT_UP,    OUT(9)},
 {0, "Alle",               EVT_DOWN,  OUT_ALLE},
 {0, "Alle",               EVT_UP,    OUT_ALLE},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
 {0, "nicht belegt",       EVT_ON,    0},
};

typedef struct {
    outputState_t  state;
    outputType_t   type;
    unsigned char  outUpOrOn;
    unsigned char  outPower;
    unsigned long  timer;
    unsigned short relaySaveTimer;
    unsigned long  maxTime;
    char           name[20];
} output_t;

output_t output[NUM_OUTPUTS] = {
{STOP, ROLLO, 15, 14, 0, 0, 3500, "Wohnzimmmer rechts"},
{STOP, ROLLO, 19, 18, 0, 0, 2500, "Gaeste WC"},
{STOP, ROLLO, 21, 20, 0, 0, 2500, "Kueche Fenster"},
{STOP, ROLLO,  9,  8, 0, 0, 2500, "Technik"},
{STOP, ROLLO,  1,  0, 0, 0, 3500, "Wohnzimmmer links"},
{STOP, ROLLO,  3,  2, 0, 0, 3500, "Kueche Tuer"},
{STOP, ROLLO,  7,  6, 0, 0, 2500, "Eltern"},
{STOP, ROLLO, 31, 30, 0, 0, 2500, "Kind rechts",},
{STOP, ROLLO, 27, 26, 0, 0, 2500, "Kind links"},
{STOP, ROLLO, 29, 28, 0, 0, 2500, "Bad"},
};

// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
unsigned char inputs_new[6], inputs_debounced[6];

/**
 * Reads multiplexed inputs, debounce intputs and generate Events. 
 */ 
static void readInputs(void);

/**
 * If the current time is in the timeEvents array defiened it generates a 
 * event.
 */  
static void timeManager(void);

/**
 * Sets outputshiftregisters with gpio
 */ 
static void setOutputs(void);
static void rolloControl(event_t event, unsigned char out_id, unsigned short delay);
static void timeOverflowCorrecter(void);
static void serialControl(void);


static void print_TimerEvents() {
	int size = NUM_TIMERS, i, j;
	timeEvent_t *tePtr = timeEvents;
	for (i = 0; i < size; i++) {
		if (tePtr->days != 0) {

			UARTprintf("%d %02d:%02d ", i, tePtr->secOfDay/3600,
										  (tePtr->secOfDay % 3600) / 60);
			if (tePtr->days == MO_FR)
				UARTprintf("MO-FR ");
			else if (tePtr->days == WE)
				UARTprintf("WE    ");
			else {
				if (tePtr->days & MO)
					UARTprintf("MO ");
				if (tePtr->days & DI)
					UARTprintf("DI ");
				if (tePtr->days & MI)
					UARTprintf("MI ");
				if (tePtr->days & DO)
					UARTprintf("DO ");
				if (tePtr->days & FR)
					UARTprintf("FR ");
				if (tePtr->days & SA)
					UARTprintf("SA ");
				if (tePtr->days & SO)
					UARTprintf("SO ");
			}

			if (tePtr->event == EVT_UP)
				UARTprintf("hoch   ");
			else
				UARTprintf("runter ");

			if (tePtr->outputs == OUT_ALLE)
				UARTprintf("Alles  ");
			else if (tePtr->outputs == OUT_TUEREN)
				UARTprintf("Tueren ");
			else {
				for (j = 0; j < NUM_OUTPUTS; j++)
					if ((1 << j) & tePtr->outputs)
						UARTprintf("%s", output[j].name);
			}
		}
		tePtr++;
	}
}


static inline unsigned char GetBit(unsigned char bitfield, unsigned char bit) {
    if (bit < 8)        
        return (bitfield >> bit) & 1;
    else
        return 0;
}

/**
 * Verarbeite einen Eingang und entprelle ihn. Bei Änderungen wird ein Event erzeugt.
 * @param *input Zeiger auf Datenstrucktur des Einganges
 * @param *inputs_new Zeiger auf den neuen Zustand der eingelesen Eingänge. (6 Eingänge bitweise)
 * @param *inputs_debounced Zeiger auf den enprellten Zustand der Eingänge. (6 Eingänge bitweise)
 * @param changes Die Veränderungen der Eingänge bitweise (inputs_new  xor  neu eingelesene Eingänge)
 * @param bit das aktuelle bit des Einganges für inputs_new, inputs_debounced und changes.
 */
static void procInput(input_t *input,
                      unsigned char *inputs_new,
                      unsigned char *inputs_debounced,
                      unsigned char changes,
                      unsigned char bit) {
    // Entprellzeit herunterzählen
    if (input->timer > 0)
        input->timer--;
    
    // Prüfen ob das Bit verändert wurde und dementsprechend den Entprelltimer zurücksetzen.
    if (GetBit(changes, bit))
        input->timer = DEBOUNCE_TIME;

    if (GetBit(*inputs_new, bit)) {            // aktueller Eingang aktiv
        if (!GetBit(*inputs_debounced, bit) && // aber enptrellter Eingang inaktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced |= (1 << bit);
            setEvent(EVT_OFF, input->outputs, input->name);
        }
    } else { // aktueller Eingang aktiv
        if (GetBit(*inputs_debounced, bit) &&  // aber enptrellter Eingang aktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced &= ~(1 << bit);
            setEvent(input->event, input->outputs, input->name);
        }
    }
}

/**
 * Wait loop, optimiesed for gpio serial loading
 */
static void wait() {
	int i;
	for (i = 0; i < 11; i++) {
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
	}
}

unsigned char setTimeEvent(signed char idx, timeEvent_t *newTimeEvent) {
	UARTprintf("setTimeEvent\n");
	return 0;
}

/* not used
static void setTime(unsigned char hour, unsigned char min, unsigned char day) {
    if (hour > 23 || min > 59 || day > 6)
        return;
    secoundsOfDay = SET_TIME(hour, min);
    weekDay       = day;
} */

static void readInputs(void) {
	unsigned char row, i, val, changes;
	input_t *in_ptr = inputs;
	// 6 nullen laden
	PORTB &= ~P_SER_I;
	for (i = 0; i < 6; i++) {
		wait();
		PORTD |= P_SCK_I;
		wait();
		PORTD &= ~P_SCK_I;
	}
	PORTD |= P_SER_I;
	wait();
	for (row = 0; row < 6; row++) {
		PORTD |= P_SCK_I;
		wait();
		PORTA |= P_RCK_I;
		PORTD &= ~P_SCK_I;
		PORTD &= ~P_SER_I;

	 	wait();

	 	PORTA &= ~P_RCK_I;
	 	wait();
	 	wait();
	 	wait();

		val = ~((((PORTD >> IN_H1) & 1) << 0) |
			    (((PORTA >> IN_R1) & 1) << 1) |
			    (((PORTA >> IN_H2) & 1) << 2) |
			    (((PORTA >> IN_R2) & 1) << 3) |
			    (((PORTA >> IN_H3) & 1) << 4) |
			    (((PORTA >> IN_R3) & 1) << 5));
    	changes = val ^ inputs_new[row];
		inputs_new[row] = val;
		for (i = 0; i < 6; i++) {
			  procInput(in_ptr, &inputs_new[row], &inputs_debounced[row], changes, i);
			  in_ptr++;
		}

    }	
	// Am ende strom sparen
	PORTD |= P_SCK_I;
	wait();
	PORTD &= ~P_SCK_I;
	wait();
	PORTA |= P_RCK_I;
	wait();
	PORTA &= ~P_RCK_I;
	//UARTprintf("inputs_new %02x %02x %02x %02x %02x %02x\n",
	//            inputs_new[0], inputs_new[1], inputs_new[2], inputs_new[3], inputs_new[4], inputs_new[5]);
}

void setEvent(event_t event, unsigned long outputs, char *name) {
	int i;
	unsigned short delay = 0;
	for (i = 0; i < NUM_OUTPUTS; i++)  {
		if (outputs & 1) {
			rolloControl(event, i, delay);
			delay += 20;
		}
		outputs >>= 1;
	}
#ifdef DEBUG
    UARTprintf("Event %d %s\n", event, name);
#endif
}

void setOutputs(void) {
	unsigned long val = ~outputs, i;
	PORTB &= ~P_SER_O;
	PORTD &= ~P_SCK_O;
	wait();
	PORTD |= P_SCK_O;
	wait();
	PORTD &= ~P_SCK_O;
	wait();
	wait();
    for (i = 0; i < 32; i++) {
        if (val & 1)
        	PORTB |= P_SER_O;
        else
        	PORTB &= ~P_SER_O;

    	PORTD |= P_SCK_O;
    	wait();
        val >>= 1;
        PORTD &= ~P_SCK_O;
        wait();
    }
    PORTB |= P_RCK_O;
    wait();
    PORTB &= ~P_RCK_O;  
}



void setTimeSod(unsigned short sod) {
    secoundsOfDay = sod;
    timeOverflowCorrecter();
}

void setWeekday(unsigned char day) {
    if (day < 7) {
        weekDay = day;
        timeOverflowCorrecter();
    }
}

void delTimer(unsigned char id) {
    
}

void readSettingsFromEerpom(void) {
 	/* hier wird erstmal initialisiert.
	 unsigned char *FlashPBGet(void);

	 void FlashPBInit(unsigned long ulStart,
	                  unsigned long ulEnd,
	                  unsigned long ulSize);

	 void FlashPBSave(unsigned char *pucBuffer);
beispiel:
    unsigned char pucBuffer[16], *pucPB;
    //
    // Initialize the flash parameter block module, using the last two pages of
    // a 64 KB device as the parameter block.
    //
    FlashPBInit(0xf800, 0x10000, 16);
    //
    // Read the current parameter block.
    //
    pucPB = FlashPBGet();
    if(pucPB)
    {
        memcpy(pucBuffer, pucPB);
    }

 	*/
}

void rollo_init(void) {
 	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB | SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    wait();
    UARTStdioInit(0);

	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, OUT(IN_R1) | OUT(IN_H2) | OUT(IN_R2) | OUT(IN_H3) | OUT(IN_R3));
	GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, OUT(IN_H1));
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, P_RCK_I);
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, P_SER_O | P_RCK_O);

	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, P_SCK_O | P_SCK_I | P_SER_I | OUT(3));
	
	UARTEchoSet(false);
	outputs = 0;
}

static void switchPowerOff(output_t *out) {
	if (outputs & OUT(out->outPower)) { 
		outputs &= ~OUT(out->outPower);
		outputs &= ~OUT(out->outUpOrOn);
		out->relaySaveTimer = SWITCH_TIME;
	}
}    

static void rolloControl(event_t event, unsigned char out_id, 
                         unsigned short delay) {
	output_t *out;
	if (out_id >= NUM_OUTPUTS)
		return;
	out = &output[out_id];
	if (out->type == ROLLO) {
		// gemeinsammes Verhalten für alle Conts
		if (event == EVT_CONT) {
			// verzögert Power einschalten
			if (out->relaySaveTimer)
				out->relaySaveTimer--;
			else if (out->timer) {
				out->timer--;
				outputs |= OUT(out->outPower);
			} else {
				switchPowerOff(out);
			}
		}
		switch (out->state) {
		case UP_START:
			if (delay > out->relaySaveTimer)
				out->relaySaveTimer = delay;
			out->state = UP;
			out->timer = out->maxTime; 
			outputs |=  OUT(out->outUpOrOn);
			UARTprintf("UP_START out %s (%d)\n", out->name, out_id);
		case UP:
			switch (event) {
			case EVT_DOWN:
				if (out->timer)
					out->state = STOP_START;
				else
					out->state = DOWN_START;
				break;

			case EVT_UP:
				if (!out->timer)
					out->state = UP_START;
				break;

			case EVT_OFF:
				break;

			default:
				break;
			}
			break;

		case DOWN_START:
			if (delay > out->relaySaveTimer)
				out->relaySaveTimer = delay;
			out->timer = out->maxTime;
			out->state = DOWN;
			outputs &= ~OUT(out->outUpOrOn);
			UARTprintf("DOWN_START out %s (%d)\n", out->name, out_id);
			/*FALLTHROUGH*/
		case DOWN:
			switch (event) {
			case EVT_UP:
				if (out->timer)
					out->state = STOP_START;
				else
					out->state = UP_START;
				break;

			case EVT_OFF:
				break;


			default:
				break;
			}
			break;

		case STOP_START:
			outputs &= ~OUT(out->outUpOrOn);
			switchPowerOff(out);
			out->timer = 0;
			out->state = STOP;
			UARTprintf("STOP_START out %s (%d)\n", out->name, out_id);
			/*FALLTHROUGH*/
		case STOP:
			switch (event) {
			case EVT_UP:
				out->state = UP_START;
				break;

			case EVT_DOWN:
				out->state = DOWN_START;
				break;

			default:
				break;
			}
			break;

		}
	} else {
        	//UARTprintf("OUTPUT Mode not implementetd only Rollo");
	}
}

void timeManager(void) {
    static unsigned long sod_old = 0xffffffff;
    unsigned char i;
    if (secoundsOfDay != sod_old) {
       sod_old = secoundsOfDay;
       for (i = 0; i < NUM_TIMERS; i++) {
            if (((1 << weekDay) & timeEvents[i].days) &&
                secoundsOfDay == timeEvents[i].secOfDay) {
                setEvent(timeEvents[i].event, timeEvents[i].outputs, timeEvents[i].name);
            }
       }
    }
}

void rollo_Cont(void) {
	unsigned char i;
	readInputs();
	timeManager();
	for (i = 0; i < NUM_OUTPUTS; i++)
		rolloControl(EVT_CONT, i, 0);
	//UARTprintf("%x\n", outputs);
	setOutputs();
	serialControl();	
}


/**
 * Test routine that triggers all relays seperate, to see if they are working.
 */ 
/*
void initialRelayTest(void) {
	unsigned char i;
    for (i = 0; i < 32; i++) {
    	while (ticks < 250);
		ticks = 0;
		outputs = OUT(i);
		setOutputs();
    } 
}*/

void saveSettingsInEeprom(void) {

}

static void timeOverflowCorrecter(void) {
   if (secoundsOfDay < 0) {
	   secoundsOfDay += 60*60*24;
	   weekDay--;
	   if (weekDay >= 7)
		   weekDay = 6;
   } else if (secoundsOfDay >= 60*60*24) {
       secoundsOfDay = 0;
       weekDay++; 
   }
   if (weekDay >= 7)
		weekDay = 0; 
}

void rollo_Tick(void) {
  	timeInMs++;
	if (timeInMs == 1000) {
   		timeInMs = 0;
		secoundsOfDay++; 
   }
   timeOverflowCorrecter();
}

static char getHour(int secOD) {
	return secOD / (60*60);
}

static char getMin(int secOD) {
	return (secOD % (60*60)) / 60; 
}

static void timeIncMin(void) {
    secoundsOfDay += 60;
	timeOverflowCorrecter();
} 

void timeDecMin(void) {
    secoundsOfDay -= 60;
	timeOverflowCorrecter();
}

void timeIncHour(void) {
    secoundsOfDay += 60*60;
	timeOverflowCorrecter();
} 

void timeDecHour(void) {
    secoundsOfDay -= 60*60;
	timeOverflowCorrecter();
}

void timeIncDay(void) {
    weekDay++;
	timeOverflowCorrecter();
}

unsigned int jsonSize;

char *addStringToBuffer(char *buffer, char *string) {
	while (*string && jsonSize) {
		*buffer++ = *string++;
		jsonSize--;
	}
	*buffer = 0;
	return buffer;
}

char* itoa(int val){
	static char buf[32] = {0};
	int i = 30, sign = val < 0;
	if (sign)
		val = -val;
	if (val == 0)
		buf[i--] = '0';
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	if (sign)
		buf[i--] = '-';
	return &buf[i+1];
}


char* genJson(char *buf, unsigned int size) {
	int i;
#ifdef JSONTEST   	
	unsigned short sizeStart = size;
	char *bufStart = buf;
#endif
	jsonSize = size;

	buf = addStringToBuffer(buf,  "{\"timeEvents\":[");
    for (i = 0; i < NUM_TIMERS; i++) {
		if (timeEvents[i].days) { // Falls kein Tag gesetzt? ungesetzter Timer
			buf = addStringToBuffer(buf, i?",":"");
			buf = addStringToBuffer(buf, "{\"name\":\"");
			buf = addStringToBuffer(buf,  timeEvents[i].name?timeEvents[i].name:"");
			buf = addStringToBuffer(buf, "\",\"days\":");
			buf = addStringToBuffer(buf, itoa(timeEvents[i].days));
			buf = addStringToBuffer(buf, ",\"event\":\"");
			buf = addStringToBuffer(buf, timeEvents[i].event == EVT_UP ? "hoch":
					                     timeEvents[i].event == EVT_DOWN ? "runter":"reserviert");
			buf = addStringToBuffer(buf, "\",\"secoundOfDay\":");
			buf = addStringToBuffer(buf, itoa(timeEvents[i].secOfDay));
			buf = addStringToBuffer(buf, ",\"id\":");
			buf = addStringToBuffer(buf, itoa(i));
			buf = addStringToBuffer(buf, "}");
		}
	}
	buf = addStringToBuffer(buf, "],\"time\":{\"secoundsOfDay\":");
	buf = addStringToBuffer(buf, itoa(secoundsOfDay));
	buf = addStringToBuffer(buf, ",\"weekDay\":");
	buf = addStringToBuffer(buf, itoa(weekDay));
	buf = addStringToBuffer(buf, "},\"outputs\":[");
	for (i = 0; i < NUM_OUTPUTS; i++) {
		buf = addStringToBuffer(buf, i?",":"");
		buf = addStringToBuffer(buf, "{\"name\":\"");
		buf = addStringToBuffer(buf, output[i].name);
		buf = addStringToBuffer(buf, "\",\"maxTime\":");
		buf = addStringToBuffer(buf, itoa(output[i].maxTime));
		buf = addStringToBuffer(buf, ",\"state\":\"");
		buf = addStringToBuffer(buf, (output[i].timer?(output[i].state == UP?"faehrt hoch":"faehrt runter"):
				                (output[i].state == UP?"oben":(output[i].state == DOWN?"unten":"gestoppt"))));
		buf = addStringToBuffer(buf, "\"}");
    }

	buf = addStringToBuffer(buf, "]}");
#ifdef JSONTEST    
  	// debug output  
    printf("bufStart:%x, buf:%x, used:%d, sizeStart:%d, size:%d, size diff:%d",
	       bufStart, buf, buf-bufStart, sizeStart, size, sizeStart-size);
#endif
    return buf;
}

void printTime(void) {
	UARTprintf("%02d:%02d ", getHour(secoundsOfDay), getMin(secoundsOfDay));
	switch (weekDay) {
	case 0:
		UARTprintf("Montag");
		break;
	case 1:
		UARTprintf("Dienstag");
		break;
	case 2:
		UARTprintf("Mittwoch");
		break;
	case 3:
		UARTprintf("Donnerstag");
		break;
	case 4:
		UARTprintf("Freitag");
		break;
	case 5:
		UARTprintf("Samstag");
		break;
	case 6:
		UARTprintf("Sonntag");
		break;
	default:
		UARTprintf("%d ist ein fehlerhafter Wochentag", weekDay);
		break;	
	}
	UARTprintf("\r\n");
}

static void serialControl(void) {
	while (UARTRxBytesAvail()) {
		switch (UARTgetc()) {

		case 'u':
			setEvent(EVT_UP,  OUT_ALLE, "Alle Hoch (serial)");
			break;

		case 'U':
			setEvent(EVT_DOWN, OUT_ALLE, "Alle Runter (serial)");
			break;

		case 'm':
			timeIncMin();
			printTime();
			break;

		case 'T':
			print_TimerEvents();
			break;

		case 'M':
			timeDecMin();
			printTime();
			break;

		case 'h':
			timeIncHour();
			printTime();
			break;

		case 'H':
			timeDecHour();
			printTime();
			break;

		case 'd':
            timeIncDay();
			printTime();
			break;

		case 't':
			printTime();
			break;
		}
	}
}


