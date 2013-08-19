/*
 * Konzeption
 * ==========
 *
 * EingŠnge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *				 clk !clk rck_i !rck_i einlesen (stecker 2)
 *				 clk !clk rck_i !rck_i einlesen (stecker 3)
 *				 clk !clk rck_i !rck_i einlesen (stecker 4)
 *				 clk !clk rck_i !rck_i einlesen (stecker 5)
 *				 clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 EingŠnge entprellen. Jedes Rollo (2 EingŠnge) hat eine eigene
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs kšnnen Events erzeugen. Up, Down, Stop ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren kšnnen mit 
 *    einer GroupId addressiert werden.
 *  - Neben den Eingängen kann die Uhr auch Events erzeugen, die 
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events kšnnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen Groupmanager.
 *  - An der Steuerung kšnnen auch fŸr alle Gruppen Events erzeugt werden.  
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle AusgŠnge die angesprochen werden sollen.
 *    - eine Priorität? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der PrioritŠt die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 RelaisausgŠnge kšnnten andersweitig
 *    z.B. fŸr zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen 
 *
 *  - 10 ms Takt 
 * 
 * Rollos   KŸFe KŸTŸ WZLi WZRe GŠWC HWR BAD KiRe KiLi SZim
 * Gruppen  KŸFe KŸTŸ WZLi WZRe GŠWC HWR BAD KiRe KiLi SZim
 *          KŸ(KŸFe KŸTŸ) Wz(WZLi WZRe) Unten(KŸFe KŸTŸ WZLi WZRe GŠWC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 */

// Rollocontrol

// EingŠnge erfassen.

unsigned long intputs_new, inputs_debounced;

//
typedef enum {
	OFF,
	ON,
	UP,
	DOWN,
} event_t;

//
typedef struct {
	unsigned short timer;
	unsigned char  groupId;
	char name[10];
	event_t event;
} intput_t;

//
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char hour, min;
	unsigned char groupId;
	event_t event;
} time_t;

//
typedef struct {
	unsigned char days;           // bit 7 mon bit 6 die ... 1 son
	unsigned char sonnenaufgang;  // 0 = sonnenuntergang
	short timeDiff;               // Zeit bevor (-) oder nach dem Sonnenaufgang oder Sonnenuntergang.
	unsigned char groupId;        // GruppenId
	event_t event;                // event
} smart_time_t;

//
typedef struct {
	unsigned long outputs;
	char name[10];
} group_t;





