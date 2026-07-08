#ifndef HTTPD_URI_CMD_H
#define HTTPD_URI_CMD_H

/**
 * Verarbeitet eine /ajax.cgi-URI (Query-Parameter) und fuehrt die
 * entsprechenden Kommandos aus (setTime, up, down, timer, delTimer).
 * Der Aufrufer muss rollo_lock() halten.
 */
void httpd_uri_cmd(const char *uri);

#endif
