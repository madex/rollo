#ifndef WIFI_H
#define WIFI_H

/** WLAN-Station starten (SSID/Passwort aus menuconfig), inkl.
 *  automatischem, endlosem Reconnect bei Verbindungsabbruch. */
void wifi_init_sta(void);

#endif
