#ifndef ROLLO_H
#define ROLLO_H

#define OUT(x) (1L << x)

#define NUM_TIMERS 32

typedef enum {
	EVT_OFF,
	EVT_ON,
	EVT_UP,
	EVT_DOWN,
	EVT_CONT,
} event_t;

typedef struct {
	unsigned char days;           // bit 0 mon bit 1 die ... 6 son
	unsigned long secOfDay;
	event_t       event;
	unsigned long outputs;
    char          name[30];
} timeEvent_t;

/* Timer-Tabelle; wird von storage.c als Blob in NVS gesichert/geladen. */
extern timeEvent_t timeEvents[NUM_TIMERS];

/**
 * Change the content of a TimeEvent, or create a new (idx = -1)
 * @param idx adress or index of the Timeevent. -1 create a new.
 * @param newTimeEvent a Pointer of a struct from witch is copied.
 * @return 1 for Error. Dataformat or Memory full   0 changed.
 */
unsigned char setTimeEvent(signed char idx, timeEvent_t *newTimeEvent);

/**
 * Generates a Json representation of the current data for sending over http.
 * @param buf Pointer to the buffer for the JSON data.
 * @param size Size of the buffer in bytes.
 */
char *genJson(char *buf, unsigned int size);

/**
 * Init of hardware and variables for the rollo control.
 */
void rollo_init(void);

void delTimer(unsigned char id);

/**
 * Set a event to the outputs bitcoded in the variable outputs.
 * @param event Event typically EVT_UP or EVT_DOWN
 * @param outputs bitcoded outputs selection. For Example 0x3ff for all rollos.
 * @param name c-string for serial debug output.
 */
void setEvent(event_t event, unsigned long outputs, char *name);

/**
 * trigger all 10 ms all output control state machines.
 */
void rollo_Cont(void);

/**
 * Einmal pro Sekunde aufrufen: uebernimmt die Systemzeit (NTP) in
 * secoundsOfDay/weekDay. Solange keine gueltige Systemzeit vorliegt,
 * laeuft die interne Uhr wie frueher einfach weiter.
 */
void rollo_secondTick(void);

/**
 * Set the time (manueller Fallback, solange kein NTP vorhanden).
 * @param sod secounds of day, from website js.
 * @param day weekday from website.
 */
void setTimeSod(unsigned long sod);
void setWeekday(unsigned char day);

/**
 * Mutex um den gemeinsamen Zustand (Web-Server-Task vs. Regelschleife).
 */
void rollo_lock(void);
void rollo_unlock(void);

#endif
