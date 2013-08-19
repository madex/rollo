/*
 * Konzeption
 * ==========
 *
 * Eing�nge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *				 clk !clk rck_i !rck_i einlesen (stecker 2)
 *				 clk !clk rck_i !rck_i einlesen (stecker 3)
 *				 clk !clk rck_i !rck_i einlesen (stecker 4)
 *				 clk !clk rck_i !rck_i einlesen (stecker 5)
 *				 clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 Eing�nge entprellen. Jedes Rollo (2 Eing�nge) hat eine eigene 
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs k�nnen Events erzeugen. Up, Down, Stop ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren k�nnen mit 
 *    einer GroupId addressiert werden.
 *  - Neben den Eing�ngen kann die Uhr auch Events erzeugen, die 
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events k�nnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen Groupmanager.
 *  - An der Steuerung k�nnen auch f�r alle Gruppen Events erzeugt werden.  
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle Ausg�nge die angesprochen werden sollen.
 *    - eine Priorit�t? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der Priorit�t die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 Relaisausg�nge k�nnten andersweitig
 *    z.B. f�r zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen 
 *
 * - 10 ms Takt 
 * 
 * Rollos   K�Fe K�T� WZLi WZRe G�WC HWR BAD KiRe KiLi SZim
 * Gruppen  K�Fe K�T� WZLi WZRe G�WC HWR BAD KiRe KiLi SZim
 *          K�(K�Fe K�T�) Wz(WZLi WZRe) Unten(K�Fe K�T� WZLi WZRe G�WC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 */

// Rollocontrol

// Eingaenge erfassen.

#define NUM_INPUTS 32
#define DEBOUNCE_TIME 10
// TODO 32 Bits reichen nicht, was tun?
unsigned long intputs_new, inputs_debounced;

typedef enum {
	OFF,
	ON,
	UP,
	DOWN,
} event_t;

typedef struct {
	unsigned short timer;
	unsigned char  groupId;
	char name[10];
	event_t event;
} intput_t;

typedef struct {
	unsigned char days; // bit 7 mon bit 6 die ... 1 son
	unsigned char hour, min;
	unsigned char groupId;
	event_t event;
} time_t;

typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char sonnenaufgang;  // 0 = sonnenuntergang
	short timeDiff;               // Zeit bevor (-) oder nach dem Sonnenaufgang oder Sonnenuntergang.
	unsigned char groupId;        // GruppenId
	event_t event;                // event
} smart_time_t;

typedef struct {
	unsigned long outputs;
	char name[10];
} group_t;

intput_t inputs[NUM_INPUTS];

void setEvent(event_t event_id, unsigned char  groupId);
void readInputs(void);
void debounceInputs(void);
void timeManager(void);
void rolloControl(void);
void setOutputs(void);

static inline unsigned char GetBit(unsigned long bitfield, unsigned char bit);
static void procInput(unsigned long inputs_new, unsigned long changes,
                      unsigned char bit);

static inline unsigned char GetBit(unsigned long bitfield, unsigned char bit) {
    if (bit < 32)                             // Sicherheitsabgrabe
        return (bitfield >> bit) & 1;         // Wert des Bittes zur�ckgeben.
    else
        return 0;
}

static void procInput(unsigned long inputs_new, unsigned long changes,
                      unsigned char bit)
{
    // Entprellzeit herunterz�hlen
    if (inputs[bit].timer > 0)          
        inputs[bit].timer--;
    
    // Pr�fen ob das Bit ver�ndert wurde und dementsprechend den Entprelltimer zur�cksetzen.
    if (GetBit(changes, bit))
        inputs[bit].timer = DEBOUNCE_TIME;

    if (GetBit(hw_inputs_cur, bit)) {         // aktueller Eingang aktiv
        if (!GetBit(inputs_debounced, bit) && // aber enptrellter Eingang inaktiv
            inputs[bit].timer == 0) {         // und Entprelltimer abgelaufen
            inputs_debounced |= (1UL << bit);
            setEvent(OFF, inputs[bit].groupId);
        }
    } else { // aktueller Eingang aktiv
        if (GetBit(inputs_debounced, bit) &&  // aber enptrellter Eingang aktiv
            inputs[bit].timer == 0) {         // und Entprelltimer abgelaufen
            inputs_debounced &= ~(1UL << bit);
            setEvent(inputs[bit].event, inputs[bit].groupId);
        }
    }
}

void readInputs(void) {
	unsigned long help = 0;
	// 

}

