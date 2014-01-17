#include "tests.h"
#undef time_t
#define main uC_main
#include "../rollo.c"
#undef main

char test1 = "/ajax?cmd=timer&id=2&out=1023&days=31&sod=27000&name=Wochentags%20Hoch%20kl";
char test2 = "/ajax?cmd=down&out=1";
char test3 = "/ajax?cmd=timer&id=new&out=256&days=0&sod=0&name=Neuer%20Timer";
char test4 = "/ajax";
char test5 = "/ajax?cmd=up&out=1023";
char test6 = "/ajax?cmd=down&out=1023";
char test7 = "/ajax?cmd=setTime&sod=85723&weekDay=3";




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
        printf("{\"name\":\"%s\",\"maxTime\":%d,\"id\":%d,\"state\":%d}\n", output[i].name, output[i].maxTime, output[i].state, i);
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
