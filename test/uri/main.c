#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MIN(x,y) ((x<y)?x:y)

char test1[] = "/ajax?cmd=setTime&sod=50226&weekDay=4";
char test2[] = "/ajax?cmd=up&out=1023";
char test3[] = "/ajax?cmd=timer&id=new&out=0&days=0&sod=0&name=%C3%84%20%C3%9Cmme%20%C3%96strich";
char test4[] = "/ajax?cmd=timer&id=0&out=1023&days=96&sod=34200&name=WE%20H%C3%B6ch%20%C3%9F%20%C3%96ma";
char test5[] = "/ajax?cmd=delTimer&id=5";
char test6[] = "/ajax";
char test7[] = "/ajax?cmd=timer&id=new&out=5&days=96&sod=21780&name=Neuer%20Timer%20wer";
char test8[] = "/ajax?cmd=timer&id=new&out=0&days=0&sod=0&name=%C3%84%C3%9C%C3%96%C3%A4%C3%BC%C3%B6%C3%9F"; // ÄÜÖäüöß
char test9[] = "/ajax?cmd=down&out=128";

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

int decodeString(char *dest, char *src, unsigned int len) {
	char c;
	while ((c = *src++) && len--) {
		if (c == '%') {
			if (strncmp(src, "20", 2) == 0 && len > 2) {
				*dest++ = ' ';
				len -= 2;
				src += 2;
			} else if (strncmp(src, "C3%84;", 5) == 0 && len > 5) {
			    *dest++ = 'A';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			}  else if (strncmp(src, "C3%9C;", 5) == 0 && len > 5) {
			    *dest++ = 'U';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			} else if (strncmp(src, "C3%96;", 5) == 0 && len > 5) {
			    *dest++ = 'O';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			} else if (strncmp(src, "C3%A4;", 5) == 0 && len > 5) {
			    *dest++ = 'a';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			} else if (strncmp(src, "C3%BC;", 5) == 0 && len > 5) {
			    *dest++ = 'u';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			} else if (strncmp(src, "C3%B6;", 5) == 0 && len > 5) {
			    *dest++ = 'o';
			    *dest++ = 'e';
			    len -= 5;
				src += 5;
			} else if (strncmp(src, "C3%9F;", 5) == 0 && len > 5) {
			    *dest++ = 's';
			    *dest++ = 's';
			    len -= 5;
				src += 5;
			}
		} else
			*dest++ = c;
	}
	*dest++ = 0;
}

int parseUri(char *uri, int len, uriParse_t *ps) {
	unsigned int n = 0, valueLen;
	char c;
	parseState_t st = PARAM_START;
	ps->uriString = uri;
	ps->length = 0;
	ps->elem[ps->length].para_off = 0;
	if (strncmp(uri, "/ajax?", MIN(6, len)) != 0)
		return 0; // fehler
	if (len < 6)
		return 0; // fehler
	len -= 6;
	n += 6;
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
					if (ps->length == MAX_ELEMETS)
                        return;
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

void printParams(uriParse_t *u) {
	int i;
	char help[100], help2[100];
	if (!u || !u->uriString)
		return;
	for (i = 0; i < u->length; i++) {
        decodeString(help, &(u->uriString[(u->elem[i]).para_off]),  (u->elem[i]).para_len);
        decodeString(help2, &(u->uriString[(u->elem[i]).value_off]), (u->elem[i]).value_len);
	 	printf("%s = %s\n", help, help2);
	}
}

int main() {
    uriParse_t u;
    char help[80];
    printf("\ntest1 = %s\n", test1);
    if (parseUri(test1, strlen(test1), &u))
    	printParams(&u);
    printf("\ntest2 = %s\n", test2);
    if (parseUri(test2, strlen(test2), &u))
    	printParams(&u);
    printf("\ntest3 = %s\n", test3);
    if (parseUri(test3, strlen(test3), &u))
    	printParams(&u);
    printf("\ntest4 = %s\n", test4);
    if (parseUri(test4, strlen(test4), &u))
    	printParams(&u);
    printf("\ntest5 = %s\n", test5);
    if (parseUri(test5, strlen(test5), &u))
    	printParams(&u);
    printf("\ntest6 = %s\n", test6);
    if (parseUri(test6, strlen(test6), &u))
    	printParams(&u);
    printf("\ntest7 = %s\n", test7);
    if (parseUri(test7, strlen(test7), &u))
    	printParams(&u);
    printf("\ntest8 = %s\n", test8);
    if (parseUri(test8, strlen(test8), &u))
    	printParams(&u);
    printf("\ntest9 = %s\n", test9);
    if (parseUri(test9, strlen(test9), &u))
    	printParams(&u);
}
