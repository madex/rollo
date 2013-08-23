/* Konzeption
 * ==========
 *
 * EingŠnge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *          clk !clk rck_i !rck_i einlesen (stecker 2)
 *          clk !clk rck_i !rck_i einlesen (stecker 3)
 *          clk !clk rck_i !rck_i einlesen (stecker 4)
 *          clk !clk rck_i !rck_i einlesen (stecker 5)
 *          clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 EingŠnge entprellen. Jedes Rollo (2 EingŠnge) hat eine eigene
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs kšnnen Events erzeugen. Up, Down, Stop ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren kšnnen mit 
 *    einer GroupId addressiert werden.
 *  - Neben den EingŠngen kann die Uhr auch Events erzeugen, die
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events k‘nnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen / Groupmanager.
 *  - An der Steuerung k‘nnen auch fŸr alle Gruppen Events erzeugt werden.  
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle AusgŠnge die angesprochen werden sollen.
 *    - eine PrioritŠt? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der PrioritŠt die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 RelaisausgŠnge kšnnten andersweitig
 *    z.B. fŸr zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen
 *  - 10 ms Takt 
 * 
 * Rollos   KŸFe KŸTŸ WZLi WZRe GŠWC HWR BAD KiRe KiLi SZim
 * Gruppen  KŸFe KŸTŸ WZLi WZRe GŠWC HWR BAD KiRe KiLi SZim
 *          KŸ(KŸFe KŸTŸ) Wz(WZLi WZRe) Unten(KŸFe KŸTŸ WZLi WZRe GŠWC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 *
 * written by Martin Ongsiek
 */
#define NUM_INPUTS 32
#define NUM_GROUPS 16
#define DEBOUNCE_TIME 10
#define DEBUG

#define PORTA GPIO_PORTA_DATA_R
#define PORTB GPIO_PORTB_DATA_R
unsigned long timeInMs;

unsigned long outputs;

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
	OFF,
	ON,
	UP,
	DOWN,
} event_t;

// A
typedef struct {
	unsigned short timer;
	unsigned char  groupId;
	char name[10];
	event_t event;
} intput_t;

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

// TODO define was das seien soll.
typedef enum {
    UP_START,
    UP,
    DOWN_START,
    DOWN,
    STOP_START
    STOP,
} outputState_t;

typedef enum {
    ROLLO,   // events UP, DOWN und OFF
    OUTPUT   // ON und OFF
} outputType_t;

typedef struct {
    char           name[20];
    outputState_t  state;
    outputType_t   type;
    unsigned short timer;
    unsigned short maxTime;
} output_t;

// Speicher fŸr die EingŠnge.
intput_t inputs[NUM_INPUTS];
// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
unsigned char intputs_new[6], inputs_debounced[6];
// Speicher fŸr Gruppen
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
        return (bitfield >> bit) & 1;        // Wert des Bittes zurückgeben.
    else
        return 0;
}

/**
 * Verarbeite einen Eingang und entprelle ihn. Bei €nderungen wird ein Event erzeugt.
 * @param *input Zeiger auf Datenstrucktur des Einganges
 * @param *inputs_new Zeiger auf den neuen Zustand der eingelesen EingŠnge. (6 EingŠnge bitweise)
 * @param *inputs_debounced Zeiger auf den enprellten Zustand der EingŠnge. (6 EingŠnge bitweise)
 * @param changes Die VerŠnderungen der EingŠnge bitweise (inputs_new  xor  neu eingelesene EingŠnge)
 * @param bit das aktuelle bit des Einganges fŸr inputs_new, inputs_debounced und changes.
 */
static void procInput(input_t *input,
                      unsigned char *inputs_new,
                      unsigned char *inputs_debounced,
                      unsigned char changes,
                      unsigned char bit) {
    // Entprellzeit herunterzŠhlen
    if (input->timer > 0)
        input->timer--;
    
    // PrŸfen ob das Bit verŠndert wurde und dementsprechend den Entprelltimer zurŸcksetzen.
    if (GetBit(changes, bit))
        input->timer = DEBOUNCE_TIME;

    if (GetBit(*inputs_new, bit)) {            // aktueller Eingang aktiv
        if (!GetBit(*inputs_debounced, bit) && // aber enptrellter Eingang inaktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced |= (1 << bit);
            setEvent(OFF, input);
        }
    } else { // aktueller Eingang aktiv
        if (GetBit(*inputs_debounced, bit) &&  // aber enptrellter Eingang aktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced &= ~(1 << bit);
            setEvent(input->event, input);
        }
    }
}

void readInputs(void) {
	unsigned char row, i, val, changes;
	intput_t *in_ptr = inputs;

	for (row = 0; row < 6; row++) {
		val = (1 << row);
		for (i = 0; i < 6; i++) {
		    GPIOPinWrite(GPIO_PORTB_BASE, SER, c & 1);
		    GPIOPinWrite(GPIO_PORTB_BASE, SCK, 1);
		    val >>= 1;
		    GPIOPinWrite(GPIO_PORTB_BASE, SCK, 0);
        }
    	GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 1);
    	GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 0);
    	val =  (GPIOPinRead(GPIO_PORTA_BASE, IN_H1) << 0) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R1) << 1) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_H2) << 2) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R2) << 3) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_H3) << 4) |
			   (GPIOPinRead(GPIO_PORTA_BASE, IN_R3) << 5);
    	changes = val ^ intputs_new[row];
		intputs_new[row] = val;
		for (i = 0; i < 6; i++) {
			  procInput(in_ptr, &intputs_new[row], &intputs_debounced[row], changes, i);
			  in_ptr++;
		}
    }	
	
	// Am ende strom sparen
	GPIOPinWrite(GPIO_PORTB_BASE, SER, 0);
	for (i = 0; i < 6; i++) {
	    GPIOPinWrite(GPIO_PORTB_BASE, SCK, 1);
	    GPIOPinWrite(GPIO_PORTB_BASE, SCK, 0);
    }
	GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 1);
	GPIOPinWrite(GPIO_PORTA_BASE, RCK_I, 0);
}

void setEvent(event_t event_id, input_t *input) {
    group_t *group;
#ifdef DEBUG
    UARTprintf("Event %d group=%d %s\n", event_id, input->group_id, input->name);
#endif
    if (input->groupId >= NUM_GROUPS)
        return;
    group = gruppen[input->groupId];
    
}

void setOutputs() {
    unsigned long val = outputs, i;
	int i;
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
 	// hier wird erstmal initialisiert. spaeter eeprom

}

void rolloControl(void) {

}

void setOutputs(void) {

}

void saveSettingsInEeprom(void) {

}

/*
int main(void)
{
    unsigned short i, reload = 23000, value = 0xffff;
    unsigned char buf[7];
    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);
    
    
    //GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    //UARTStdioInit(0);
    //UARTprintf("\nFarbborg Cortex new\n");
    
    SysTickIntRegister(SysTickHandler);
    SysTickPeriodSet(8000L); // 1 ms Tick
    SysTickEnable();
    SysTickIntEnable();
    IntMasterEnable();
    
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    i=1000;
    hw_relays = 0xffffffff;
    //
    // Finished.
    //
    while (1) {
    	if (bit_ms) {
    		bit_ms--;
    		hw_relays -= 3000;
            
    		if ((timeInMs % 100) == 0)
                RIT128x96x4StringDraw(PrintSignedShortFormated(timeInMs, buf), 0, 16, 8);
            
            
    		
    	}
    }
}
 

 */


int main(void) {
    int i = 500, mode = 0, time;
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ); // 80 MHz
    //
    // Initialize the UART.
    //         TODO andere serielle suchen
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB | SYSCTL_PERIPH_GPIOD | SYSCTL_PERIPH_GPIOF);
    //ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    //UARTStdioInit(0);
    //UARTprintf("\nFarbborg Cortex new\n");
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, RCK_O);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, SER | RCK_O | SCK);
    SysTickIntRegister(SysTickHandler);
    ROM_SysTickPeriodSet(80000L); // 1 ms Tick
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();
    ROM_IntMasterEnable();
    readSettingsFromEerpom();
    
	while (1) {
	 	if (ms) {
			ms--;
			readInputs();
			
		
		}
	}
}
