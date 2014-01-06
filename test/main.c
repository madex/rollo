#include "tests.h"
#undef time_t
#define main uC_main
#include "../rollo.c"
#undef main

static inline void jsonPrintTimeEvents() {
    int i;
    printf("\"timeEvents\":[");
    for (i = 0; i < NUM_TIMERS; i++) {
        if (timeEvents[i].days) { // Falls kein Tag gesetzt? ungesetzter Timer
            if (i) // am ende darf nicht ,} stehen da es sonst kein json ist.
                printf(",");
            printf("{\"name\":\"%s\",\"days\":%d,\"event\":\"%s\",\"secoundOfDay\":%d,\"id\":%d}\n", 
                   timeEvents[i].name, timeEvents[i].days, 
                   timeEvents[i].event == EVT_UP ? "hoch": timeEvents[i].event == EVT_DOWN ? "runter":"reserviert", 
                   timeEvents[i].secOfDay , i);
        }
    }
    printf("],\n");
}

static inline void jsonPrintTime() {
    printf("\"time\":{\"secoundsOfDay\":%d,\"weekDay\":%d},\n", secoundsOfDay, weekDay);
}

static inline void jsonPrintOutputs() {
    int i;
    printf("\"outputs\":[");
    for (i = 0; i < NUM_OUTPUTS; i++) {
        if (i)
            printf(",");
        printf("{\"name\":\"%s\",\"maxTime\":%d,\"id\":%d}\n", output[i].name, output[i].maxTime, i);
    }
    printf("]\n");
}

static inline void makeJson() {
    printf("{");
    jsonPrintTimeEvents();
    jsonPrintTime();
    jsonPrintOutputs();
    printf("}");
} 

START_TESTS()

START_TEST("make json")
    makeJson();
END_TEST()

START_TEST("make json")
    
END_TEST()

END_TESTS()
