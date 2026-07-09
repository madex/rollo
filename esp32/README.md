# Rollocontrol auf ESP32-C3

Portierung der Rollladensteuerung vom LM3S9B96 (Stellaris, Ethernet/lwIP)
auf ESP32-C3 mit WLAN. Hintergrund: das Ethernet-Interface des alten
Boards stürzt nach ca. einem Tag ab (vermutlich Hardwaredefekt, tritt
mit uIP und lwIP gleichermaßen auf).

Die Kernlogik (State-Machine, Entprellung, Timer, JSON, Web-UI) ist
unverändert übernommen. Neu bzw. geändert:

* **WLAN statt Ethernet** — mit endlosem Auto-Reconnect; das Gerät heilt
  sich selbst, wenn das WLAN wegbricht.
* **NTP-Zeitsynchronisation** — die Uhr stellt sich selbst, inklusive
  automatischer Sommer-/Winterzeit (Zeitzone Europe/Berlin, per
  menuconfig änderbar). Der „Stellen“-Knopf im Web-UI funktioniert
  weiter als Fallback, solange kein NTP verfügbar ist.
* **Timer überleben Stromausfall** — die Timer-Tabelle wird bei jeder
  Änderung in NVS (Flash) gesichert. (Im Original war
  `readSettingsFromEerpom()` nie implementiert.)
* **Web-UI unverändert** — dieselbe `client.html`, gepollt über
  `/ajax.cgi` wie bisher. Erreichbar unter `http://rollo/` (DHCP-
  Hostname) bzw. der IP aus dem Log.
* **Entfallen**: die seriellen Einzeltasten-Kommandos (`u`, `t`, `m` …).
  Debug-Ausgaben kommen weiter über die serielle Konsole.

## Verdrahtung

| Signal  | Funktion                        | alt (LM3S) | neu (ESP32-C3) |
|---------|---------------------------------|------------|----------------|
| SER_O   | Daten Ausgangs-SR (Relais)      | PB5        | GPIO2          |
| SCK_O   | Takt Ausgangs-SR                | PD0        | GPIO19         |
| RCK_O   | Latch Ausgangs-SR               | PB6        | GPIO10         |
| SER_I   | Daten Eingangs-Scan-SR          | PD3        | GPIO9          |
| SCK_I   | Takt Eingangs-Scan-SR           | PD4        | GPIO0          |
| RCK_I   | Latch Eingangs-Scan-SR          | PA7        | GPIO18         |
| IN_H1   | Matrix-Rücklesung 1             | PD2        | GPIO3          |
| IN_R1   | Matrix-Rücklesung 2             | PA6        | GPIO1          |
| IN_H2   | Matrix-Rücklesung 3             | PA3        | GPIO4          |
| IN_R2   | Matrix-Rücklesung 4             | PA2        | GPIO5          |
| IN_H3   | Matrix-Rücklesung 5             | PA5        | GPIO6          |
| IN_R3   | Matrix-Rücklesung 6             | PA4        | GPIO7          |

Die Belegung liegt zentral in `main/config.h` und ist leicht änderbar.

**Wichtige Hardware-Hinweise:**

* Der ESP32-C3 ist **nicht 5V-tolerant**. Laufen die Schieberegister /
  die Tastenmatrix mit 5V, müssen die 6 Rückleseeingänge über
  Spannungsteiler (z.B. 10k/15k) oder Pegelwandler angebunden werden.
  Der LM3S9B96 lief ebenfalls mit 3,3V-Logikpegeln — wenn die alte
  Schaltung direkt am Stellaris hing, passt es vermutlich auch so.
* **10k-Pulldown an RCK_O** (GPIO10) vorsehen, damit während
  Reset/Flashen keine zufälligen Zustände in die Relaisregister
  gelatcht werden.
* Die Strapping-Pins GPIO2/8/9 sind bewusst als ESP-*Ausgänge* belegt
  (dort hängen nur hochohmige Schieberegister-Eingänge, die den Boot
  nicht stören). GPIO9 ist auf DevKits der BOOT-Taster — das ist ok.
* GPIO18 ist normalerweise USB D-; USB ist deshalb deaktiviert, die
  Konsole läuft über UART0 (GPIO20/21 — auf dem DevKit der
  USB-UART-Chip, also einfach der normale USB-Anschluss des DevKits).

## Bauen und Flashen

ESP-IDF (≥ v5.1) installieren: https://docs.espressif.com/projects/esp-idf/

```sh
cd esp32
idf.py set-target esp32c3
idf.py menuconfig        # -> "Rollo Konfiguration": WLAN SSID + Passwort
idf.py build
idf.py flash monitor
```

Im Monitor erscheint nach dem Verbinden die IP-Adresse. Web-UI:
`http://rollo/` oder `http://<IP>/`.

## Struktur

```
main/
  config.h          Pinbelegung
  rollo.c/.h        Kernlogik (portiert, weitgehend 1:1)
  httpd-uri-cmd.c   /ajax.cgi-Parser (unverändert)
  web_server.c      esp_http_server: "/" + "/ajax.cgi"
  wifi.c            WLAN-Station mit Auto-Reconnect
  storage.c         Timer-Persistenz in NVS
  client.html       Web-UI (unverändert aus test/client.html)
  main.c            Init + 10-ms-Regelschleife
```

## Nebenläufigkeit

Anders als auf dem Stellaris (eine große `while(1)`-Schleife) läuft der
HTTP-Server in einem eigenen FreeRTOS-Task. Der gemeinsame Zustand
(Timer, Ausgänge) ist mit einem Mutex geschützt (`rollo_lock()` /
`rollo_unlock()`): die 10-ms-Regelschleife hält ihn während
`rollo_Cont()`, der Webserver während Kommando + JSON-Erzeugung.
