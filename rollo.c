/* Konzeption
 * ==========
 *
 * Eingänge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *          clk !clk rck_i !rck_i einlesen (stecker 2)
 *          clk !clk rck_i !rck_i einlesen (stecker 3)
 *          clk !clk rck_i !rck_i einlesen (stecker 4)
 *          clk !clk rck_i !rck_i einlesen (stecker 5)
 *          clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 Eingänge entprellen. Jedes Rollo (2 Eingänge) hat eine eigene
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs können Events erzeugen. EVT_UP, EVT_DOWN, EVT_STOP ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren können mit
 *    einer GroupId addressiert werden.
 *  - Neben den Eingängen kann die Uhr auch Events erzeugen, die
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events kënnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen / Groupmanager.
 *  - An der Steuerung kënnen auch für alle Gruppen Events erzeugt werden.
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle Ausgänge die angesprochen werden sollen.
 *    - eine Priorität? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der Priorität die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 Relaisausgänge könnten andersweitig
 *    z.B. für zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen
 *  - 10 ms Takt 
 * 
 * Rollos   KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 * Gruppen  KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 *          Kü(KüFe KüTü) Wz(WZLi WZRe) Unten(KüFe KüTü WZLi WZRe GäWC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 *
 * written by Martin Ongsiek
 */
#include <inc/lm3s9b96.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/timer.h>
#include <utils/uartstdio.h>
#include <utils/flash_pb.h>

#define SWITCH_TIME 30 // *10ms

//#undef DEBUG
//#define DEBUG
#define NUM_INPUTS 32
#define NUM_GROUPS 16
#define DEBOUNCE_TIME 10

#define PORTA GPIO_PORTA_DATA_R
#define PORTB GPIO_PORTB_DATA_R
#define PORTD GPIO_PORTD_DATA_R

#define OUT(x) (1UL << x)

unsigned long outputs;
volatile unsigned int timeInMs;


#define P_SER   GPIO_PIN_5 // PORTB
#define P_RCK_O GPIO_PIN_6 // PORTB
#define P_SCK   GPIO_PIN_0 // PORTD
#define P_RCK_I GPIO_PIN_7 // PORTA

// PORTA
#define IN_H1  2  // I1 PORTD
#define IN_R1  6  // I2
#define IN_H2  3  // I3
#define IN_R2  2  // I4
#define IN_H3  5  // I5
#define IN_R3  4  // I6

// A
typedef enum {
	EVT_OFF,
	EVT_ON,
	EVT_UP,
	EVT_DOWN,
	EVT_CONT,
} event_t;

// A
typedef struct {
	unsigned short timer;
	char           name[30];
	event_t        event;
    unsigned long  outputs;
} input_t;

// C
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char hour, min;
	unsigned long outputs;
	event_t       event;
} time_t;

// C
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char sonnenaufgang;  // 0 = sonnenuntergang
	short         timeDiff;       // Zeit bevor (-) oder nach dem Sonnenaufgang oder Sonnenuntergang.
	unsigned long outputs;
	event_t       event;          // event
} smart_time_t;


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
 {0, "Wohnzimmmer rechts", EVT_UP,   OUT(0)},
 {0, "Wohnzimmmer rechts", EVT_DOWN, OUT(0)},
 {0, "Wohnzimmmer links",  EVT_UP,   OUT(1)},
 {0, "Wohnzimmmer links",  EVT_DOWN, OUT(1)},
 {0, "Kueche Tuer",        EVT_UP,   OUT(2)},
 {0, "Kueche Tuer",        EVT_DOWN, OUT(2)},
 {0, "Kueche Fenster",     EVT_UP,   OUT(3)},
 {0, "Kueche Fenster",     EVT_DOWN, OUT(3)},
 {0, "Gaeste WC",          EVT_UP,   OUT(4)},
 {0, "Gaeste WC",          EVT_DOWN, OUT(4)},

 {0, "Eltern",             EVT_UP,   OUT(5)},
 {0, "Eltern",             EVT_DOWN, OUT(5)},
 {0, "Kind links",         EVT_UP,   OUT(6)},
 {0, "Kind links",         EVT_DOWN, OUT(6)},

 {0, "Kind rechts",        EVT_UP,   OUT(7)},
 {0, "Kind rechts",        EVT_DOWN, OUT(7)},

 {0, "Bad",                EVT_UP,   OUT(8)},
 {0, "Bad",                EVT_DOWN, OUT(8)},

 {0, "Alle",               EVT_UP,   0x0000001f},
 {0, "Alle",               EVT_DOWN, 0x0000001f},
};

typedef struct {
    outputState_t  state;
    outputType_t   type;
    unsigned char  outUpOrOn;
    unsigned char  outPower;
    unsigned long  timer;
    unsigned long  maxTime;
    char           name[20];
} output_t;

output_t output[32] = {
{STOP, ROLLO, 16, 17, 0, 1500, "Rollo 0"},
{STOP, ROLLO,  2,  3, 0, 1500, "Rollo 1"},
{STOP, ROLLO,  4,  5, 0, 1500, "Rollo 2"},
{STOP, ROLLO,  6,  7, 0, 1500, "Rollo 3"},
{STOP, ROLLO,  8,  9, 0, 1500, "Rollo 4"},
};

// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
unsigned char inputs_new[6], inputs_debounced[6];

void setEvent(event_t event_id, input_t *input);
void readInputs(void);
void timeManager(void);
void rolloControl(event_t event, unsigned char out_id);
void setOutputs(void);
void readSettingsFromEerpom(void);
void saveSettingsInEeprom(void);

static inline unsigned char GetBit(unsigned char bitfield, unsigned char bit) {
    if (bit < 8)                             // Sicherheitsabgrabe
        return (bitfield >> bit) & 1;        // Wert des Bittes zur¸ckgeben.
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
            setEvent(EVT_OFF, input);
        }
    } else { // aktueller Eingang aktiv
        if (GetBit(*inputs_debounced, bit) &&  // aber enptrellter Eingang aktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced &= ~(1 << bit);
            setEvent(input->event, input);
        }
    }
}

static void wait() {
	int i;
	for (i = 0; i < 3; i++) {
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

void readInputs(void) {
	unsigned char row, i, val, changes;
	input_t *in_ptr = inputs;
	// 6 nullen laden
	PORTB &= ~P_SER;
	for (i = 0; i < 6; i++) {
		wait();
		PORTD |= P_SCK;
		wait();
		PORTD &= ~P_SCK;
	}
	PORTB |= P_SER;
	wait();
	for (row = 0; row < 6; row++) {
		PORTD |= P_SCK;
		wait();
		PORTA |= P_RCK_I;
		PORTD &= ~P_SCK;
		PORTB &= ~P_SER;

	 	wait();
		PORTA &= ~P_RCK_I;

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
	PORTD |= P_SCK;
	wait();
	PORTD &= ~P_SCK;
	wait();
	PORTA |= P_RCK_I;
	wait();
	PORTA &= ~P_RCK_I;

	//UARTprintf("inputs_new %02x %02x %02x %02x %02x %02x\n",
	//            inputs_new[0], inputs_new[1], inputs_new[2], inputs_new[3], inputs_new[4], inputs_new[5]);
}

void setEvent(event_t event, input_t *input) {
	int i;
	unsigned long out = input->outputs;

	for (i = 0; i < 5; i++)  {
		if (out & 1) {
			rolloControl(event, i);
		}
		out >>= 1;
	}
#ifdef DEBUG
    UARTprintf("Event %d %s\n", event, input->name);
#endif
}

void setOutputs(void) {
    unsigned long val = ~outputs, i;
    for (i = 0; i < 32; i++) {
        if (val & 1)
        	PORTB |= P_SER;
        else
        	PORTB &= ~P_SER;

    	PORTD |= P_SCK;
    	wait();
        val >>= 1;
        PORTD &= ~P_SCK;
        wait();
    }
    PORTB |= P_RCK_O;
    wait();
    PORTB &= ~P_RCK_O;
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

void rolloControl(event_t event, unsigned char out_id) {
	output_t *out;
	if (out_id >= 6)
		return;
	out = &output[out_id];
	if (out->type == ROLLO) {
		switch (out->state) {
		case UP_START:
			out->timer = out->maxTime + SWITCH_TIME;
			out->state = UP;
			outputs |=  OUT(out->outUpOrOn);
			outputs &= ~OUT(out->outPower);
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

			case EVT_CONT:
				// nach 500 ms Power einschalten
				if (out->timer < out->maxTime)
					outputs |= OUT(out->outPower);
				if (out->timer)
					out->timer--;
				else {
					outputs &= ~OUT(out->outPower);
					outputs &= ~OUT(out->outUpOrOn);
				}
				break;

			default:
				break;
			}
			break;

		case DOWN_START:
			out->timer = out->maxTime + SWITCH_TIME;
			out->state = DOWN;
			outputs &= ~OUT(out->outUpOrOn);
			outputs &= ~OUT(out->outPower);
			UARTprintf("DOWN_START out %s (%d)\n", out->name, out_id);
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

			case EVT_CONT:
				// nach 500 ms Power einschalten
				if (out->timer < out->maxTime)
					outputs |= OUT(out->outPower);
				if (out->timer)
					out->timer--;
				else {
					outputs &= ~OUT(out->outPower);
				}
				break;

			default:
				break;
			}
			break;

		case STOP_START:
			outputs &= ~OUT(out->outUpOrOn);
			outputs &= ~OUT(out->outPower);
			out->state = STOP;
			UARTprintf("STOP_START out %s (%d)\n", out->name, out_id);
		case STOP:
			switch (event) {
			case EVT_UP:
				out->state = UP_START;
				break;

			case EVT_DOWN:
				out->state = DOWN_START;
				break;
			}
			break;

		}
	} else {

	}
}

void timeManager(void) {

}

void saveSettingsInEeprom(void) {

}

#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine) {
        UARTprintf("ERROR: %s:%d\n", pcFilename, ulLine);
}
#endif


void SysTickHandler(void) {
	timeInMs++;
	/*if (timeInMs & 1)
		PORTD |= OUT(3);
	else
		PORTD &= ~OUT(3);
	*/
}

int main(void) {
    int i;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ); // 80 MHz
    ROM_SysTickPeriodSet(80000L); // 1 ms Tick
	ROM_SysTickEnable();
	ROM_SysTickIntEnable();
	ROM_IntMasterEnable();
    // Initialize the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB | SYSCTL_PERIPH_GPIOD);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    wait();
    UARTStdioInit(0);

	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, OUT(IN_R1) | OUT(IN_H2) | OUT(IN_R2) | OUT(IN_H3) | OUT(IN_R3));
	GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, OUT(IN_H1));
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, P_RCK_I);
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, P_SER | P_RCK_O);

	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, P_SCK | OUT(3));
	SysTickIntRegister(SysTickHandler);
	UARTprintf("\nRollocontrol v0.1 (Martin Ongsiek)\n");
    readSettingsFromEerpom();
    
	while (1) {
	 	if (timeInMs > 10) {
	 		timeInMs -= 10;
			readInputs();
			timeManager();
			for (i = 0; i < 6; i++)
				rolloControl(EVT_CONT, i);
			//UARTprintf("%x\n", outputs);
			setOutputs();
		}
	}
}
