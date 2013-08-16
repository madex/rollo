/*
 * Konzeption
 * ==========
 *
 * Eingänge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *				 clk !clk rck_i !rck_i einlesen (stecker 2)
 *				 clk !clk rck_i !rck_i einlesen (stecker 3)
 *				 clk !clk rck_i !rck_i einlesen (stecker 4)
 *				 clk !clk rck_i !rck_i einlesen (stecker 5)
 *				 clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 Eingänge entprellen. Jedes Rollo (2 Eingänge) hat eine eigene 
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs können Events erzeugen. Up, Down, Stop ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren kšnnen mit 
 *    einer GroupId addressiert werden.
 *  - Neben den Eingängen kann die Uhr auch Events erzeugen, die 
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events kšnnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen Groupmanager.
 *  - An der Steuerung kšnnen auch fŸr alle Gruppen Events erzeugt werden.  
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle Ausgänge die angesprochen werden sollen.
 *    - eine Priorität? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der PrioritŠt die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 Relaisausgänge könnten andersweitig
 *    z.B. fŸr zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen 
 *
 * - 10 ms Takt 
 * 
 * Rollos   KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 * Gruppen  KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 *          Kü(KüFe KüTü) Wz(WZLi WZRe) Unten(KüFe KüTü WZLi WZRe GäWC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 */

// Rollocontrol

// EingŠnge erfassen.

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





