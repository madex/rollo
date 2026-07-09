#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

/*
 * Pinbelegung ESP32-C3
 *
 * Der C3 hat genau genug frei nutzbare GPIOs fuer die 12 Signale.
 * Strapping-Pins (GPIO2, GPIO8, GPIO9) sind bewusst als ESP-AUSGAENGE
 * belegt: dort haengen nur hochohmige Schieberegister-Eingaenge dran,
 * die den Boot-Vorgang nicht stoeren koennen.
 *
 * GPIO18 ist normalerweise USB D-. USB wird nicht genutzt, Konsole
 * laeuft ueber UART0 (GPIO20/21, auf dem DevKit der USB-UART-Chip).
 *
 * WICHTIG (Hardware):
 *  - Der ESP32-C3 ist NICHT 5V-tolerant. Wenn die Eingangsmatrix bzw.
 *    die Schieberegister mit 5V laufen, muessen die 6 Ruecklese-
 *    leitungen ueber Spannungsteiler oder Pegelwandler angeschlossen
 *    werden. (Der alte LM3S9B96 lief ebenfalls mit 3,3V-I/O.)
 *  - 10k-Pulldown an RCK_O empfohlen, damit die Relais-Register
 *    waehrend Reset/Boot nicht zufaellig gelatcht werden.
 */

/* Steuerleitungen Schieberegister (ESP-Ausgaenge)
 *
 * Board-spezifische Erkenntnisse (GOOUUU-ESP32-C3, empirisch ermittelt):
 *  - GPIO3/4/5 = RGB-LED (rot/gruen/blau, gemeinsame Anode, aktiv low).
 *    Nur als hochohmige Eingaenge verwenden, sonst leuchtet die LED.
 *  - GPIO0 darf NUR Eingang sein: als getakteter Ausgang bricht die
 *    Versorgung zusammen -> endloser POWERON-Reset-Bootloop.
 *  - GPIO19 (USB D+) vorsichtshalber ebenfalls nicht als Ausgang nutzen.
 *  - GPIO8 ist auf diesem Board frei (keine LED, anders als beim
 *    DevKitM-1/SuperMini, wo dort die Onboard-LED haengt). */
#define PIN_SER_O   GPIO_NUM_2   /* Daten     Ausgangs-SR (Relais)   - alt: PB5 */
#define PIN_SCK_O   GPIO_NUM_8   /* Takt      Ausgangs-SR            - alt: PD0 */
#define PIN_RCK_O   GPIO_NUM_10  /* Latch     Ausgangs-SR            - alt: PB6 */
#define PIN_SER_I   GPIO_NUM_9   /* Daten     Eingangs-Scan-SR       - alt: PD3 */
#define PIN_SCK_I   GPIO_NUM_6   /* Takt      Eingangs-Scan-SR       - alt: PD4 */
#define PIN_RCK_I   GPIO_NUM_18  /* Latch     Eingangs-Scan-SR       - alt: PA7 */

/* Ruecklese-Eingaenge der Tastenmatrix (ESP-Eingaenge)
 * GPIO3/4/5 sind auf dem GOOUUU-ESP32-C3 die RGB-LED (rot/gruen/blau,
 * gemeinsame Anode, aktiv low). Als hochohmige EINGAENGE sind sie ok
 * (LED bleibt dunkel), als Ausgaenge wuerden sie die LED einschalten.
 * Bit-Reihenfolge wie im Original: H1, R1, H2, R2, H3, R3 */
#define PIN_IN_H1   GPIO_NUM_3   /* alt: PD2 */
#define PIN_IN_R1   GPIO_NUM_1   /* alt: PA6 */
#define PIN_IN_H2   GPIO_NUM_4   /* alt: PA3 */
#define PIN_IN_R2   GPIO_NUM_5   /* alt: PA2 */
#define PIN_IN_H3   GPIO_NUM_0   /* alt: PA5 */
#define PIN_IN_R3   GPIO_NUM_7   /* alt: PA4 */

/* Interne Pull-ups an den Ruecklese-Eingaengen aktivieren (1) oder
 * nicht (0), je nachdem was die externe Beschaltung braucht. */
#define ROLLO_IN_PULLUP 0

#endif
