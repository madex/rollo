/*
 * Rollladensteuerung - Kernlogik, portiert vom LM3S9B96 (Stellaris) auf
 * ESP32-C3 / ESP-IDF. Die State-Machine, Entprellung und Timerverwaltung
 * sind unveraendert; ersetzt wurden nur GPIO-Zugriffe, die Zeitbasis
 * (NTP statt manuell gestellter Software-RTC) und UARTprintf -> printf.
 */
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

#include "config.h"
#include "rollo.h"
#include "storage.h"

#define SWITCH_TIME 30 // *10ms

//#undef DEBUG
#define DEBUG
#define NUM_INPUTS    36
#define NUM_OUTPUTS   10
#define DEBOUNCE_TIME  5

#define OUT_ALLE   (OUT(0) | OUT(1) | OUT(2) | OUT(3) | OUT(4) |  \
                    OUT(5) | OUT(6) | OUT(7) | OUT(8) | OUT(9))
#define OUT_ALLE2   (OUT(0) | OUT(1) | OUT(2) | OUT(3) | OUT(4) |  \
                    OUT(5) | OUT(6) | OUT(7) | OUT(9))
#define OUT_OHNE_MO (OUT(1) | OUT(2) | OUT(3) |  \
                     OUT(6) | OUT(7) | OUT(9))
#define OUT_TUEREN (OUT(0) | OUT(4) | OUT(5))
#define SET_TIME(hour, minute)   (hour*60*60 + minute*60)

volatile long secoundsOfDay;
volatile unsigned char weekDay; // 0 montag 1 dienstag
unsigned long outputs;

static SemaphoreHandle_t rolloMutex;

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
#define SO_DO    (SO | MO | DI | MI | DO)
#define SA_SO    (SA | SO)
#define FR_SA    (FR | SA)

timeEvent_t timeEvents[NUM_TIMERS] = {
{SA_SO, SET_TIME( 8,30), EVT_UP,   OUT_ALLE2,   "WE Hoch"},
{FR_SA, SET_TIME(18,45), EVT_DOWN, OUT_TUEREN,  "WE Tueren Runter"},
{FR_SA, SET_TIME(19,30), EVT_DOWN, OUT_OHNE_MO, "WE Runter"},
{MO_FR, SET_TIME( 7,30), EVT_UP,   OUT_ALLE2,   "Wochentags Hoch"},
{SO_DO, SET_TIME(18,45), EVT_DOWN, OUT_TUEREN,  "Wochentags Tueren"},
{SO_DO, SET_TIME(19,30), EVT_DOWN, OUT_OHNE_MO, "Wochentags Runter"},
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

// Speicher fuer die Eingaenge.
static input_t inputs[NUM_INPUTS] = {
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
 {0, "Nele",               EVT_DOWN,  OUT(7)},
 {0, "Nele",               EVT_UP,    OUT(7)},
 {0, "Moritz",             EVT_DOWN,  OUT(8)},
 {0, "Moritz",             EVT_UP,    OUT(8)},
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

static output_t output[NUM_OUTPUTS] = {
{STOP, ROLLO, 15, 14, 0, 0, 3500, "Wohnzimmmer rechts"},
{STOP, ROLLO, 19, 18, 0, 0, 2500, "Gaeste WC"},
{STOP, ROLLO, 21, 20, 0, 0, 2500, "Kueche Fenster"},
{STOP, ROLLO,  9,  8, 0, 0, 2500, "Technik"},
{STOP, ROLLO,  1,  0, 0, 0, 3500, "Wohnzimmmer links"},
{STOP, ROLLO,  3,  2, 0, 0, 3500, "Kueche Tuer"},
{STOP, ROLLO,  7,  6, 0, 0, 2500, "Eltern"},
{STOP, ROLLO, 31, 30, 0, 0, 2500, "Nele",},
{STOP, ROLLO, 27, 26, 0, 0, 2500, "Moritz"},
{STOP, ROLLO, 29, 28, 0, 0, 2500, "Bad"},
};

// jeweils nur 6 Bit pro Byte. Wie bei der Hardware.
static unsigned char inputs_new[6], inputs_debounced[6];

static void readInputs(void);
static char *rollo_itoa(signed long val);
static void timeManager(void);
static void setOutputs(void);
static void rolloControl(event_t event, unsigned char out_id, unsigned short delay);
static void timeOverflowCorrecter(void);

void rollo_lock(void) {
	xSemaphoreTake(rolloMutex, portMAX_DELAY);
}

void rollo_unlock(void) {
	xSemaphoreGive(rolloMutex);
}

static inline unsigned char GetBit(unsigned char bitfield, unsigned char bit) {
    if (bit < 8)
        return (bitfield >> bit) & 1;
    else
        return 0;
}

/**
 * Verarbeite einen Eingang und entprelle ihn. Bei Aenderungen wird ein Event erzeugt.
 */
static void procInput(input_t *input,
                      unsigned char *inputs_new,
                      unsigned char *inputs_debounced,
                      unsigned char changes,
                      unsigned char bit) {
    // Entprellzeit herunterzaehlen
    if (input->timer > 0)
        input->timer--;

    // Pruefen ob das Bit veraendert wurde und dementsprechend den Entprelltimer zuruecksetzen.
    if (GetBit(changes, bit))
        input->timer = DEBOUNCE_TIME;

    if (GetBit(*inputs_new, bit)) {            // aktueller Eingang aktiv
        if (!GetBit(*inputs_debounced, bit) && // aber enptrellter Eingang inaktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced |= (1 << bit);
            setEvent(EVT_OFF, input->outputs, input->name);
        }
    } else { // aktueller Eingang inaktiv
        if (GetBit(*inputs_debounced, bit) &&  // aber enptrellter Eingang aktiv
            input->timer == 0) {               // und Entprelltimer abgelaufen
            *inputs_debounced &= ~(1 << bit);
            setEvent(input->event, input->outputs, input->name);
        }
    }
}

/**
 * Kurze Wartezeit fuers Schieberegister-Timing (~3 us, wie die alte
 * nop-Schleife bei 80 MHz).
 */
static void wait(void) {
	esp_rom_delay_us(3);
}

static signed char findFreeTimeEvent(void) {
	int i;
	for (i = 0; i < NUM_TIMERS; i++) {
		if (timeEvents[i].days == 0)
			return i;
	}
	return -1;
}

void delTimer(unsigned char id) {
	if (id >= NUM_TIMERS)
		return;
	timeEvents[id].days = 0;
	storage_save();
}

unsigned char setTimeEvent(signed char idx, timeEvent_t *newTimeEvent) {
	printf("setTimeEvent(%d) days:%d out:%lu evt:%d sod:%lu name:%s\n", idx, newTimeEvent->days,
		   newTimeEvent->outputs, newTimeEvent->event, newTimeEvent->secOfDay, newTimeEvent->name);
	if (idx >= NUM_TIMERS) {
		return 1;
	} else if (idx < 0) {
		idx = findFreeTimeEvent();
		if (idx < 0)
			return 1;
	}
	memcpy(&timeEvents[idx], newTimeEvent, sizeof (timeEvent_t));
	storage_save();
	return 0;
}

static void readInputs(void) {
	unsigned char row, i, val, changes;
	input_t *in_ptr = inputs;
	// 6 nullen laden
	gpio_set_level(PIN_SER_I, 0);
	for (i = 0; i < 6; i++) {
		wait();
		gpio_set_level(PIN_SCK_I, 1);
		wait();
		gpio_set_level(PIN_SCK_I, 0);
	}
	gpio_set_level(PIN_SER_I, 1);
	wait();
	for (row = 0; row < 6; row++) {
		gpio_set_level(PIN_SCK_I, 1);
		wait();
		gpio_set_level(PIN_RCK_I, 1);
		gpio_set_level(PIN_SCK_I, 0);
		gpio_set_level(PIN_SER_I, 0);

		wait();

		gpio_set_level(PIN_RCK_I, 0);
		wait();
		wait();
		wait();

		val = ~((gpio_get_level(PIN_IN_H1) << 0) |
			    (gpio_get_level(PIN_IN_R1) << 1) |
			    (gpio_get_level(PIN_IN_H2) << 2) |
			    (gpio_get_level(PIN_IN_R2) << 3) |
			    (gpio_get_level(PIN_IN_H3) << 4) |
			    (gpio_get_level(PIN_IN_R3) << 5)) & 0x3f;
    	changes = val ^ inputs_new[row];
		inputs_new[row] = val;
		for (i = 0; i < 6; i++) {
			  procInput(in_ptr, &inputs_new[row], &inputs_debounced[row], changes, i);
			  in_ptr++;
		}
    }
	// Am ende strom sparen
	gpio_set_level(PIN_SCK_I, 1);
	wait();
	gpio_set_level(PIN_SCK_I, 0);
	wait();
	gpio_set_level(PIN_RCK_I, 1);
	wait();
	gpio_set_level(PIN_RCK_I, 0);
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
    printf("Event %d %s\n", event, name);
#endif
}

static void setOutputs(void) {
	unsigned long val = ~outputs, i;
	gpio_set_level(PIN_SER_O, 0);
	gpio_set_level(PIN_SCK_O, 0);
	wait();
	gpio_set_level(PIN_SCK_O, 1);
	wait();
	gpio_set_level(PIN_SCK_O, 0);
	wait();
	wait();
    for (i = 0; i < 32; i++) {
        gpio_set_level(PIN_SER_O, val & 1);
    	gpio_set_level(PIN_SCK_O, 1);
    	wait();
        val >>= 1;
        gpio_set_level(PIN_SCK_O, 0);
        wait();
    }
    gpio_set_level(PIN_RCK_O, 1);
    wait();
    gpio_set_level(PIN_RCK_O, 0);
}

void setTimeSod(unsigned long sod) {
	printf("Sod = %ld\n", (long)sod);
    secoundsOfDay = sod;
    timeOverflowCorrecter();
}

void setWeekday(unsigned char day) {
    if (day < 7) {
        weekDay = day;
        timeOverflowCorrecter();
    }
}

void rollo_init(void) {
	rolloMutex = xSemaphoreCreateMutex();

	gpio_config_t out_conf = {
		.pin_bit_mask = (1ULL << PIN_SER_O) | (1ULL << PIN_SCK_O) |
		                (1ULL << PIN_RCK_O) | (1ULL << PIN_SER_I) |
		                (1ULL << PIN_SCK_I) | (1ULL << PIN_RCK_I),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&out_conf);

	gpio_config_t in_conf = {
		.pin_bit_mask = (1ULL << PIN_IN_H1) | (1ULL << PIN_IN_R1) |
		                (1ULL << PIN_IN_H2) | (1ULL << PIN_IN_R2) |
		                (1ULL << PIN_IN_H3) | (1ULL << PIN_IN_R3),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = ROLLO_IN_PULLUP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&in_conf);

	outputs = 0;
	setOutputs(); // definierten Grundzustand (alles aus) in die Relais-Register latchen
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
		// gemeinsammes Verhalten fuer alle Conts
		if (event == EVT_CONT) {
			// verzoegert Power einschalten
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
			printf("UP_START out %s (%d)\n", out->name, out_id);
			/*FALLTHROUGH*/
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
			printf("DOWN_START out %s (%d)\n", out->name, out_id);
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
			printf("STOP_START out %s (%d)\n", out->name, out_id);
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
        	//printf("OUTPUT Mode not implementetd only Rollo");
	}
}

/**
 * Loest Timer-Events aus. Statt nur exakt die aktuelle Sekunde zu pruefen
 * (Original), wird der Bereich seit dem letzten Aufruf nachgeholt, damit
 * bei kleinen Zeitspruengen (NTP-Nachfuehrung, verpasste Ticks) kein Event
 * verloren geht. Grosse Spruenge (> 60 s, z.B. erste NTP-Synchronisation)
 * werden uebersprungen.
 */
static void timeManager(void) {
    static long sod_old = -1;
    long sod = secoundsOfDay;
    unsigned char i;
    long diff;
    if (sod_old < 0) {
        sod_old = sod;
        return;
    }
    if (sod == sod_old)
        return;
    diff = sod - sod_old;
    if (diff < 0)
        diff += 24*60*60; // Mitternachtsueberlauf
    if (diff > 60) {      // Zeitsprung: nichts nachholen
        sod_old = sod;
        return;
    }
    while (sod_old != sod) {
        sod_old++;
        if (sod_old >= 24*60*60)
            sod_old = 0;
        for (i = 0; i < NUM_TIMERS; i++) {
            if (((1 << weekDay) & timeEvents[i].days) &&
                sod_old == (long)timeEvents[i].secOfDay) {
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
	setOutputs();
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

void rollo_secondTick(void) {
	time_t now;
	struct tm tm;
	time(&now);
	localtime_r(&now, &tm);
	if (tm.tm_year >= (2020 - 1900)) {
		// gueltige Systemzeit (NTP) uebernehmen
		secoundsOfDay = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
		weekDay = (tm.tm_wday + 6) % 7; // tm_wday: 0=Sonntag -> 0=Montag
	} else {
		// Fallback: interne Uhr laeuft wie frueher sekundenweise weiter
		secoundsOfDay++;
		timeOverflowCorrecter();
	}
}

static unsigned int jsonSize;

static char *addStringToBuffer(char *buffer, const char *string) {
	while (*string && jsonSize) {
		*buffer++ = *string++;
		jsonSize--;
	}
	*buffer = 0;
	return buffer;
}

static char* rollo_itoa(signed long val) {
	/** Die Ausgabe von rollo_itoa(i) ist identisch zu
	 *  sprintf(buf, "%d", i);
	 *  fuer den gesamten signed 32 bit Bereich.
	 */
	static char buf[32]; // vorsicht beim naechsten Aufruf ist str weg.
	char *sBuf = &buf[31];
	unsigned char negative = val < 0;
	unsigned long value, valueOld;
	*sBuf = 0;
	if (negative)
		value = (unsigned long) -val;
	else
		value = (unsigned long) val;
	do {
		valueOld = value;
		value /= 10;
		*--sBuf = '0' + valueOld - (value * 10); // schneller als % 10
	}  while (value);
	if (negative)
		*--sBuf = '-';
	return sBuf;
}

char* genJson(char *buf, unsigned int size) {
	int i, firstTimeEvent = 1;
	jsonSize = size;

	buf = addStringToBuffer(buf,  "{\"timeEvents\":[");
    for (i = 0; i < NUM_TIMERS; i++) {
		if (timeEvents[i].days) { // Falls kein Tag gesetzt? ungesetzter Timer
			if (!firstTimeEvent)
				buf = addStringToBuffer(buf, ",");
			buf = addStringToBuffer(buf, "{\"name\":\"");
			buf = addStringToBuffer(buf,  timeEvents[i].name);
			buf = addStringToBuffer(buf, "\",\"days\":");
			buf = addStringToBuffer(buf, rollo_itoa(timeEvents[i].days));
			buf = addStringToBuffer(buf, ",\"event\":\"");
			buf = addStringToBuffer(buf, timeEvents[i].event == EVT_UP ? "hoch":
					                     timeEvents[i].event == EVT_DOWN ? "runter":"reserviert");
			buf = addStringToBuffer(buf, "\",\"secoundOfDay\":");
			buf = addStringToBuffer(buf, rollo_itoa(timeEvents[i].secOfDay));
			buf = addStringToBuffer(buf, ",\"out\":");
			buf = addStringToBuffer(buf, rollo_itoa(timeEvents[i].outputs));
			buf = addStringToBuffer(buf, ",\"id\":");
			buf = addStringToBuffer(buf, rollo_itoa(i));
			buf = addStringToBuffer(buf, "}");
			firstTimeEvent = 0;
		}
	}
	buf = addStringToBuffer(buf, "],\"time\":{\"secoundsOfDay\":");
	buf = addStringToBuffer(buf, rollo_itoa(secoundsOfDay));
	buf = addStringToBuffer(buf, ",\"weekDay\":");
	buf = addStringToBuffer(buf, rollo_itoa(weekDay));
	buf = addStringToBuffer(buf, "},\"outputs\":[");
	for (i = 0; i < NUM_OUTPUTS; i++) {
		buf = addStringToBuffer(buf, i?",":"");
		buf = addStringToBuffer(buf, "{\"name\":\"");
		buf = addStringToBuffer(buf, output[i].name);
		buf = addStringToBuffer(buf, "\",\"maxTime\":");
		buf = addStringToBuffer(buf, rollo_itoa(output[i].maxTime));
		buf = addStringToBuffer(buf, ",\"state\":\"");
		buf = addStringToBuffer(buf, (output[i].timer?(output[i].state == UP?"faehrt hoch":"faehrt runter"):
				                (output[i].state == UP?"oben":(output[i].state == DOWN?"unten":"gestoppt"))));
		buf = addStringToBuffer(buf, "\"}");
    }

	buf = addStringToBuffer(buf, "]}");
    return buf;
}
