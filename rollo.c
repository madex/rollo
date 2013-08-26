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

#define NUM_INPUTS 32
#define NUM_GROUPS 16
#define DEBOUNCE_TIME 10

#define PORTA GPIO_PORTA_DATA_R
#define PORTB GPIO_PORTB_DATA_R

unsigned long outputs;
unsigned char timeInMs;


#define SER   (1 << 5)  // PORTB
#define RCK_O (1 << 6)  // PORTB
#define SCK   (1 << 7)  // PORTB
#define RCK_I (1 << 7)  // PORTA

// PORTA
#define IN_H1 (1 << 1)  // I1
#define IN_R1 (1 << 6)  // I2
#define IN_H2 (1 << 3)  // I3
#define IN_R2 (1 << 2)  // I4
#define IN_H3 (1 << 5)  // I5
#define IN_R3 (1 << 4)  // I6

// A
typedef enum {
	EVT_OFF,
	EVT_ON,
	EVT_UP,
	EVT_DOWN,
} event_t;

// A
typedef struct {
	unsigned short timer;
	unsigned char  groupId;
	char name[10];
	event_t event;
} input_t;

// C
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char hour, min;
	unsigned char groupId;
	event_t event;
} time_t;

// C
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char sonnenaufgang;  // 0 = sonnenuntergang
	short         timeDiff;       // Zeit bevor (-) oder nach dem Sonnenaufgang oder Sonnenuntergang.
	unsigned char groupId;        // GruppenId
	event_t event;                // event
} smart_time_t;

// A
typedef struct {
	unsigned long outputs;
	char          name[20];
} group_t;

typedef enum {
    UP_START,
    UP,
    DOWN_START,
    DOWN,
    STOP_START,
    STOP
} outputState_t;

typedef enum {
    ROLLO,   // events UP, EVT_DOWN und OFF
    OUTPUT   // ON und OFF
} outputType_t;

typedef struct {
    char           name[20];
    outputState_t  state;
    outputType_t   type;
    unsigned short timer;
    unsigned short maxTime;
} output_t;

// Speicher für die Eingänge.
input_t inputs[NUM_INPUTS];
// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
unsigned char intputs_new[6], inputs_debounced[6];
// Speicher für Gruppen
group_t gruppen[NUM_GROUPS];

void setEvent(event_t event_id, input_t *input);
void readInputs(void);
void timeManager(void);  //
void rolloControl(void);
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

void readInputs(void) {
	unsigned char row, i, val, changes;
	input_t *in_ptr = inputs;

	for (row = 0; row < 6; row++) {
		val = (1 << row);
		for (i = 0; i < 6; i++) {
			if (val & 1)
				PORTB |= SER;
			else
				PORTB &= ~SER;
			wait();
			PORTB |= SCK;
			wait();
			//GPIOPinWrite(GPIO_PORTB_BASE, SER, val & 1);
			//GPIOPinWrite(GPIO_PORTB_BASE, SCK, 1);
			val >>= 1;
			PORTB &= ~SCK;
			//GPIOPinWrite(GPIO_PORTB_BASE, SCK, 0);
		}
		PORTB &= ~SER;
		wait();
	 	PORTA |= RCK_I;
	 	wait();
		PORTA &= ~RCK_I;
    	//GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 1);
    	//GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 0);
    	val =  (GPIOPinRead(GPIO_PORTA_BASE, IN_H1) << 0) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R1) << 1) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_H2) << 2) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R2) << 3) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_H3) << 4) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R3) << 5);
    	changes = val ^ intputs_new[row];
		intputs_new[row] = val;
		for (i = 0; i < 6; i++) {
			  procInput(in_ptr, &intputs_new[row], &inputs_debounced[row], changes, i);
			  in_ptr++;
		}
    }	
	
	// Am ende strom sparen
	//GPIOPinWrite(GPIO_PORTB_BASE, SER, 0)
	PORTB &= ~SER;
	for (i = 0; i < 6; i++) {
		PORTB |= SCK;
		wait();
		//GPIOPinWrite(GPIO_PORTB_BASE, SCK, 1);
		val >>= 1;
		PORTB &= ~SCK;
		//GPIOPinWrite(GPIO_PORTB_BASE, SCK, 0);
		wait();
	}
}

void setEvent(event_t event_id, input_t *input) {
    group_t *group;
#ifdef DEBUG
    UARTprintf("Event %d group=%d %s\n", event_id, input->groupId, input->name);
#endif
    if (input->groupId >= NUM_GROUPS)
        return;
    group = &gruppen[input->groupId];
    
}

void setOutputs(void) {
    unsigned long val = outputs, i;
    for (i = 0; i < 32; i++) {
        GPIOPinWrite(GPIO_PORTB_BASE, SER, val & 1);
        GPIOPinWrite(GPIO_PORTB_BASE, SCK, 1);
        val >>= 1;
        GPIOPinWrite(GPIO_PORTB_BASE, SCK, 0);
    }
    GPIOPinWrite(GPIO_PORTB_BASE, RCK_O, 1);
    GPIOPinWrite(GPIO_PORTB_BASE, RCK_O, 0);
}

void readSettingsFromEerpom(void) {
 	// hier wird erstmal initialisiert. später eeprom

}

void rolloControl(void) {

}

void saveSettingsInEeprom(void) {

}

#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
        UARTprintf("ERROR: %s:%d\n", pcFilename, ulLine);
}
#endif


void SysTickHandler(void) {
	timeInMs--;
}

int main(void) {
    int i = 500, mode = 0, time;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ); // 80 MHz
    //
    // Initialize the UART.
    // TODO andere serielle suchen
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB);
    //ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    //UARTStdioInit(0);
    //UARTprintf("\nFarbborg Cortex new\n");
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, RCK_I);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, SER | RCK_O | SCK);
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, IN_H1 | IN_R1 | IN_H2 | IN_R2 | IN_H3 | IN_R3);
    SysTickIntRegister(SysTickHandler);
    ROM_SysTickPeriodSet(80000L); // 1 ms Tick
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();
    ROM_IntMasterEnable();
    readSettingsFromEerpom();
    
	while (1) {
	 	if (timeInMs) {
	 		timeInMs--;
			readInputs();
			rolloControl();
			//setOutputs();
		}
	}
}
