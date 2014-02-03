//*****************************************************************************
//
// lmi_fs.c - File System Processing for enet_io application.
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"

//*****************************************************************************
//
// Include the file system data for this application.  This file is generated
// by the makefsfile utility, using the following command (all on one line):
//
//     makefsfile -i fs -o io_fsdata.h -r -h
//
// If any changes are made to the static content of the web pages served by the
// application, this script must be used to regenerate io_fsdata.h in order
// for those changes to be picked up by the web server.
//
//*****************************************************************************
#include "io_fsdata.h"


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
    int sign;
    
    if (c && *c == '-') {
        sign = -1;
        c++;
    } else {
        sign = 1;
    }
    while (*c) {
        if (*c < '0' || *c > '9') {
            return result * sign;
        }
        result += *c - '0';
        c++;
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
                        return 0;
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

unsigned char parameterNameEqual(unsigned char paraIdx,
                                 uriParse_t *u,
                                 const char *compareName) {
    if (!u || !u->uriString || !compareName || paraIdx >= u->length)
		return 0;
    return strncmp(&(u->uriString[(u->elem[paraIdx]).para_off]),
                   compareName, (u->elem[paraIdx]).para_len) == 0;
}

unsigned short getParameterValue(unsigned char paraIdx,
                                 uriParse_t *u,
                                 unsigned char *error) {
    unsigned char temp;
    if (!error)
        error = &temp;
    if (!u || !u->uriString || paraIdx >= u->length) {
		*error = 1;
        return 0;
    }
    *error = 0;
    return atoi(&(u->uriString[(u->elem[paraIdx]).value_off]));
}


//*****************************************************************************
//
// Open a file and return a handle to the file, if found.  Otherwise,
// return NULL.  This function also looks for special filenames used to
// provide specific status information or to control various subsystems.
// These filenames are used by the JavaScript on the "IO Control Demo 1"
// example web page.
//
//*****************************************************************************
struct fs_file * fs_open(char *name) {
    const struct fsdata_file *ptTree;
    struct fs_file *ptFile = NULL;
    uriParse_t u;
    char help[80], error = 0;
    //
    // Allocate memory for the file system structure.
    //
    ptFile = mem_malloc(sizeof(struct fs_file));
    if(NULL == ptFile)
    {
        return(NULL);
    }
    
    if (parseUri(name, strlen(name), &u)) {
        if (!u || !u->uriString || !u->length)
            return;
        if (parameterNameEqual(0, &u, "cmd")) {
            
        }
        
        
        for (i = 0; i < u->length; i++) {
            decodeString(help, &(u->uriString[(u->elem[i]).para_off]),  (u->elem[i]).para_len);
            decodeString(help2, &(u->uriString[(u->elem[i]).value_off]), (u->elem[i]).value_len);
            printf("%s = %s\n", help, help2);
        }
        
    }
    
    /*
    //
    // Process request to toggle STATUS LED
    //
    if(strncmp(name, "/cgi-bin/toggle_led", 19) == 0)
    {
        //
        // Toggle the STATUS LED
        //
        io_set_led(!io_is_led_on());

        //
        // Setup the file structure to return whatever.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = NULL;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }
    //
    // Request for LED State?
    //
    else if(strncmp(name, "/ledstate?id", 12) == 0)
    {
        static char pcBuf[4];

        //
        // Get the state of the LED
        //
        io_get_ledstate(pcBuf, 4);

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }
    //
    // Request for the animation speed?
    //
    else if(strncmp(name, "/get_speed?id", 12) == 0)
    {
        static char pcBuf[6];

        //
        // Get the current animation speed as a string.
        //
        io_get_animation_speed_string(pcBuf, 6);

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }
    //
    // Set the animation speed?
    //
    else if(strncmp(name, "/cgi-bin/set_speed?percent=", 12) == 0)
    {
        static char pcBuf[6];

        //
        // Extract the parameter and set the actual speed requested.
        //
        io_set_animation_speed_string(name + 27);

        //
        // Get the current speed setting as a string to send back.
        //
        io_get_animation_speed_string(pcBuf, 6);

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }
    //
    // If I can't find it there, look in the rest of the main file system
    //
    else */
    {
        //
        // Initialize the file system tree pointer to the root of the linked list.
        //
        ptTree = FS_ROOT;

        //
        // Begin processing the linked list, looking for the requested file name.
        //
        while(NULL != ptTree)
        {
            //
            // Compare the requested file "name" to the file name in the
            // current node.
            //
            if(strncmp(name, (char *)ptTree->name, ptTree->len) == 0)
            {
                //
                // Fill in the data pointer and length values from the
                // linked list node.
                //
                ptFile->data = (char *)ptTree->data;
                ptFile->len = ptTree->len;

                //
                // For now, we setup the read index to the end of the file,
                // indicating that all data has been read.
                //
                ptFile->index = ptTree->len;

                //
                // We are not using any file system extensions in this
                // application, so set the pointer to NULL.
                //
                ptFile->pextension = NULL;

                //
                // Exit the loop and return the file system pointer.
                //
                break;
            }

            //
            // If we get here, we did not find the file at this node of the linked
            // list.  Get the next element in the list.
            //
            ptTree = ptTree->next;
        }
    }

    //
    // If we didn't find the file, ptTee will be NULL.  Make sure we
    // return a NULL pointer if this happens.
    //
    if(NULL == ptTree)
    {
        mem_free(ptFile);
        ptFile = NULL;
    }

    //
    // Return the file system pointer.
    //
    return(ptFile);
}

//*****************************************************************************
//
// Close an opened file designated by the handle.
//
//*****************************************************************************
void
fs_close(struct fs_file *file)
{
    //
    // Free the main file system object.
    //
    mem_free(file);
}

//*****************************************************************************
//
// Read the next chunck of data from the file.  Return the count of data
// that was read.  Return 0 if no data is currently available.  Return
// a -1 if at the end of file.
//
//*****************************************************************************
int
fs_read(struct fs_file *file, char *buffer, int count)
{
    int iAvailable;

    //
    // Check to see if a command (pextension = 1).
    //
    if(file->pextension == (void *)1)
    {
        //
        // Nothting to do for this file type.
        //
        file->pextension = NULL;
        return(-1);
    }

    //
    // Check to see if more data is available.
    //
    if(file->len == file->index)
    {
        //
        // There is no remaining data.  Return a -1 for EOF indication.
        //
        return(-1);
    }

    //
    // Determine how much data we can copy.  The minimum of the 'count'
    // parameter or the available data in the file system buffer.
    //
    iAvailable = file->len - file->index;
    if(iAvailable > count)
    {
        iAvailable = count;
    }

    //
    // Copy the data.
    //
    memcpy(buffer, file->data + iAvailable, iAvailable);
    file->index += iAvailable;

    //
    // Return the count of data that we copied.
    //
    return(iAvailable);
}
