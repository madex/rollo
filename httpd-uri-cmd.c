#include "httpd-uri-cmd.h"
#include "rollo.h"
#include <string.h>

#define MIN(x, y) ((x)<(y)?(x):(y))
#define MAX_ELEMETS 10

typedef struct {
    unsigned int para_off, para_len;
    unsigned int value_off, value_len;
} uriParseElement_t;

typedef struct {
    unsigned char length;
    char *uriString;
    uriParseElement_t elem[MAX_ELEMETS];
} uriParse_t;

typedef enum {
    PARAM_START,
    PARAM,
    VALUE_START,
    VALUE
} parseState_t;

int atoi(const char *c) {
    int result = 0;
    int sign   = 1;
    if (!c) {
        return 0;
    }
    // skip starting spaces
    while (*c == ' ') {
        c++;
    }
    if (*c == '-') {
        sign = -1;
        c++;
    }
    while (*c >= '0' && *c <= '9') {
        result *= 10;
        result += *c++ - '0';
    }
    return result * sign;
}

void decodeString(char *dest, char *src, unsigned int len) {
    char c;
    while ((c = *src++) && len--) {
        if (c == '%') {
            if (strncmp(src, "20", 2) == 0 && len >= 2) {
                *dest++ = ' ';
                len -= 2;
                src += 2;
            } else if (strncmp(src, "C3%84;", 5) == 0 && len >= 5) {
                *dest++ = 'A';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            }  else if (strncmp(src, "C3%9C;", 5) == 0 && len >= 5) {
                *dest++ = 'U';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            } else if (strncmp(src, "C3%96;", 5) == 0 && len >= 5) {
                *dest++ = 'O';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            } else if (strncmp(src, "C3%A4;", 5) == 0 && len >= 5) {
                *dest++ = 'a';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            } else if (strncmp(src, "C3%BC;", 5) == 0 && len >= 5) {
                *dest++ = 'u';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            } else if (strncmp(src, "C3%B6;", 5) == 0 && len >= 5) {
                *dest++ = 'o';
                *dest++ = 'e';
                len -= 5;
                src += 5;
            } else if (strncmp(src, "C3%9F;", 5) == 0 && len >= 5) {
                *dest++ = 's';
                *dest++ = 's';
                len -= 5;
                src += 5;
            }
        } else {
            *dest++ = c;
        }
    }
    *dest++ = 0;
}


int parseUri(char *uri, int len, uriParse_t *ps) {
    unsigned int n = 0, valueLen = 0;
    char c;
    parseState_t st = PARAM_START;
    ps->uriString = uri;
    ps->length = 0;
    ps->elem[ps->length].para_off = 0;
    if (strncmp(uri, "/ajax.cgi?", MIN(9, len)) != 0) {
        return 0; // fehler
    }
    if (len < 9) {
        return 0; // fehler
    } else if (len == 9) {
        return 1;
    }
    len -= 10;
    n += 10;
    while (len) {
        c = uri[n];
        switch (st) {
            case PARAM_START:
                ps->elem[ps->length].para_off = n;
                ps->elem[ps->length].para_len = 0;
                ps->elem[ps->length].value_off = 0;
                ps->elem[ps->length].value_len = 0;
                st = PARAM;
                valueLen = 0;
                break;
                
            case PARAM:
                if (c == '=') {
                    ps->elem[ps->length].para_len = valueLen;
                    st = VALUE_START;
                }
                break;
                
            case VALUE_START:
                ps->elem[ps->length].value_off = n;
                st = VALUE;
                valueLen = 0;
                break;
                
            case VALUE:
                if (c == '&') {
                    ps->elem[ps->length].value_len = valueLen;
                    st = PARAM_START;
                    ps->length++;
                    if (ps->length == MAX_ELEMETS) {
                        return 0;
                    }
                }
                break;
        }
        n++;
        valueLen++;
        len--;
    }
    if (ps->elem[ps->length].value_len == 0 && ps->elem[ps->length].value_off) {
        ps->elem[ps->length].value_len = valueLen;
        ps->length++;
    }
    return 1;
}

static unsigned char idx;

unsigned char nextParam(char *paramPtr, char *valuePtr, unsigned char len, uriParse_t *u) {
    if (!u || !u->uriString || !valuePtr || !paramPtr  || idx >= u->length) {
        return 0;
    }
    decodeString(paramPtr, &(u->uriString[u->elem[idx].para_off]),
                 MIN(u->elem[idx].para_len, len));
    decodeString(valuePtr, &(u->uriString[u->elem[idx].value_off]),
                 MIN(u->elem[idx].value_len, len));
    idx++;
    return 1;
}

unsigned char firstParam(char *paramPtr, char *valuePtr, unsigned char len, uriParse_t *u) {
    idx = 0;
    return nextParam(paramPtr, valuePtr, len, u);
}

#define BUF 30
void httpd_uri_cmd(char* uri) {
    uriParse_t u;
    char param[BUF], value[BUF];
    if (parseUri(uri, strlen(uri), &u)) {
        if (u.length) {
            if (firstParam(param, value, BUF-1, &u)) {
                if (strncmp(param, "cmd", 3) == 0) {
                    
                    if (strncmp(value, "setTime", 7) == 0) {
                        while (nextParam(param, value, BUF-1, &u)) {
                            if (strncmp(param, "sod", 3) == 0) {
                                setTimeSod(atoi(value));
                            } else if (strncmp(param, "weekDay", 3) == 0) {
                                setWeekday(atoi(value));
                            }
                        }
                    } else if (strncmp(value, "up", 2) == 0) {
                        while (nextParam(param, value, BUF-1, &u)) {
                            if (strncmp(param, "out", 3) == 0) {
                                setEvent(EVT_UP, atoi(value), "Up ajax");
                            }
                        }
                    } else if (strncmp(value, "down", 4) == 0) {
                        while (nextParam(param, value, BUF-1, &u)) {
                            if (strncmp(param, "out", 3) == 0) {
                                setEvent(EVT_DOWN, atoi(value), "Down ajax");
                            }
                        }
                    } else if (strncmp(value, "timer", 5) == 0) {
                        timeEvent_t timer = {0, 0, 0, 0, ""};
                        signed char id = -1;
                        while (nextParam(param, value, BUF-1, &u)) {
                            if (strncmp(param, "id", 2) == 0) {
                                if (strncmp(value, "new", 3) == 0) {
                                    id = -1;
                                } else {
                                    id = atoi(value);
                                }
                            } else if (strncmp(param, "out", 3) == 0) {
                                timer.outputs = atoi(value);
                            } else if (strncmp(param, "sod", 3) == 0) {
                                timer.secOfDay = atoi(value);
                            } else if (strncmp(param, "event", 5) == 0) {
                                if (strncmp(value, "hoch", 4) == 0) {
                                    timer.event = EVT_UP;
                                } else {
                                    timer.event = EVT_DOWN;
                                }
                            } else if (strncmp(param, "days", 4) == 0) {
                                timer.days = atoi(value);
                            } else if (strncmp(param, "name", 4) == 0) {
                                strncpy(timer.name, value, BUF-1);
                                timer.name[BUF-1] = 0;
                            }
                        }
                        setTimeEvent(id, &timer);
                    } else if (strncmp(value, "delTimer", 8) == 0) {
                        if (nextParam(param, value, BUF-1, &u)) {
                            if (strncmp(param, "id", 2) == 0) {
                                delTimer(atoi(value));
                            }
                        }
                    }
                }
            }
        }
    }
}
