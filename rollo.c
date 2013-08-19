/*
 * Konzeption
 * ==========
 *
 * Eing�nge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *          clk !clk rck_i !rck_i einlesen (stecker 2)
 *          clk !clk rck_i !rck_i einlesen (stecker 3)
 *          clk !clk rck_i !rck_i einlesen (stecker 4)
 *          clk !clk rck_i !rck_i einlesen (stecker 5)
 *          clk !clk rck_i !rck_i einlesen (stecker 6)
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
 *  - 10 ms Takt 
 * 
 * Rollos   K�Fe K�T� WZLi WZRe G�WC HWR BAD KiRe KiLi SZim
 * Gruppen  K�Fe K�T� WZLi WZRe G�WC HWR BAD KiRe KiLi SZim
 *          K�(K�Fe K�T�) Wz(WZLi WZRe) Unten(K�Fe K�T� WZLi WZRe G�WC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 *
 * written by Martin Ongsiek
 */
#define NUM_INPUTS 32
#define NUM_GROUPS 16
#define DEBOUNCE_TIME 10
#define DEBUG
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

// Speicher f�r die Eing�nge.
intput_t inputs[NUM_INPUTS];
// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
unsigned char intputs_new[6], inputs_debounced[6];
// Speicher f�r Gruppen
group_t gruppen[NUM_GROUPS];

void setEvent(event_t event_id, input_t *input);
void readInputs(void);
void debounceInputs(void);
void timeManager(void);
void rolloControl();
void setOutputs(void);
void readSettingsFromEerpom(void);
void saveSettintsInEeprom(void);
void showEvent(event_t event_id, input_t *input);

static inline unsigned char GetBit(unsigned char bitfield, unsigned char bit) {
    if (bit < 8)                             // Sicherheitsabgrabe
        return (bitfield >> bit) & 1;        // Wert des Bittes zur�ckgeben.
    else
        return 0;
}

/**
 * Verarbeite einen Eingang und entprelle ihn. Bei �nderungen wird ein Event erzeugt.
 * @param *input Zeiger auf Datenstrucktur des Einganges
 * @param *inputs_new Zeiger auf den neuen Zustand der eingelesen Eing�nge. (6 Eing�nge bitweise)
 * @param *inputs_debounced Zeiger auf den enprellten Zustand der Eing�nge. (6 Eing�nge bitweise)
 * @param changes Die Ver�nderungen der Eing�nge bitweise (inputs_new  xor  neu eingelesene Eing�nge)
 * @param bit das aktuelle bit des Einganges f�r inputs_new, inputs_debounced und changes.
 */
static void procInput(input_t *input,
                      unsigned char *inputs_new,
                      unsigned char *inputs_debounced,
                      unsigned char changes,
                      unsigned char bit) {
    // Entprellzeit herunterz�hlen
    if (input->timer > 0)
        input->timer--;
    
    // Pr�fen ob das Bit ver�ndert wurde und dementsprechend den Entprelltimer zur�cksetzen.
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
	unsigned long help = 0;
	// 

}

void setEvent(event_t event_id, input_t *input) {
    group_t *group;
#ifdef DEBUG
    showEvent(event_id, input);
#endif
    if (input->groupId >= NUM_GROUPS)
        return;
    group = gruppen[input->groupId];
    
}

void showEvent(event_t event_id, input_t *input) {
    // Print input, event and group
}
