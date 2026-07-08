#ifndef STORAGE_H
#define STORAGE_H

/** Timer-Tabelle aus NVS laden (falls vorhanden). Vor rollo-Start aufrufen. */
void storage_load(void);

/** Timer-Tabelle in NVS sichern. Wird bei jeder Timer-Aenderung aufgerufen. */
void storage_save(void);

#endif
