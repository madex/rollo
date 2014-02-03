#ifndef ROLLO_H
#define ROLLO_H

#define OUT(x) (1L << x)
 
extern volatile unsigned char ticks; 
    
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
} time_t;

/** 
 * Change the content of a TimeEvent, or create a new (idx = -1)
 * @param idx adress or index of the Timeevent. -1 create a new.
 * @param newTimeEvent a Pointer of a struct from witch is copied.
 * @return 1 for Error. Dataformat or Memory full   0 changed.   
 */  
unsigned char setTimeEvent(signed char idx, time_t *newTimeEvent);

/** 
 * Generates a Json representation of the current data for sending over http.
 * @param buf Pointer to the buffer for the JSON data.
 * @param size Size of the buffer in bytes.   
 */ 
void genJson(unsigned char *buf, unsigned short size);

void rollo_init(void);

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
void rolloCont(void);

/**
 * Test routine that triggers all relays seperate, to see if they are working.
 */ 
void initialRelayTest(void);



/**
 * Set the time.
 * @param sod secounds of day, from website.
 * @param day weekday from website.
 */   
void setTimeSod(unsigned short sod, unsigned char day);


//void readSettingsFromEerpom(void);
//void saveSettingsInEeprom(void);

#endif