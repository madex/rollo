/*
 * Konzeption
 * ==========
 *
 * Eingänge abfragen:
 *  - Ablauf
 *    - ser clk !clk !ser rck_i !rck_i einlesen (stecker 1)
 *				 clk !clk rck_i !rck_i einlesen (stecker 2)
 *				 clk !clk rck_i !rck_i einlesen (stecker 3)
 *				 clk !clk rck_i !rck_i einlesen (stecker 4)
 *				 clk !clk rck_i !rck_i einlesen (stecker 5)
 *				 clk !clk rck_i !rck_i einlesen (stecker 6)
 *  - 36 Eingänge entprellen. Jedes Rollo (2 Eingänge) hat eine eigene 
 *    Struct mit Timer und Zustand, GroupId und einen freiparametrierbaren Namen.
 *  - Die 18 Rollo Inputs können Events erzeugen. Up, Down, Stop ..
 *  - Jedes Rollo hat eine GroupId. Mehrere RolloMotoren können mit 
 *    einer GroupId addressiert werden.
 *  - Neben den Eingängen kann die Uhr auch Events erzeugen, die 
 *    jeweils auch mit GroupId verbunden sind.
 *  - Events könnnen zu Debug Zwecken dargestellt werden.
 * Events verteilen Groupmanager.
 *  - An der Steuerung können auch für alle Gruppen Events erzeugt werden.  
 *  - Die Gruppen beinhalten
 *    - einen parametrierbaren Namen (20 Zeichen)
 *    - eine Ausgangsbitmaske mit alle Ausgänge die angesprochen werden sollen.
 *    - eine Priorität? (Noch unklar wie sie funktionieren soll)
 *    - Es gibt maximal 30 Gruppen. (siehe unten)
 *    - Der GroupManager wertet entsprechend der Gruppe und der Priorität die
 *      Events aus und leitet sie an die entsprechenden RolloControls weiter.
 *  - Es gibt 10 Rollos. Die restlichen 12 Relaisausgänge könnten andersweitig
 *    z.B. für zeitschaltfunktionen benutzt werden.
 * Rollos Motoren betreiben. RolloControl
 *  - Die Rolladen sollen 
 *
 * - 10 ms Takt 
 * 
 * Rollos   KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 * Gruppen  KüFe KüTü WZLi WZRe GäWC HWR BAD KiRe KiLi SZim
 *          Kü(KüFe KüTü) Wz(WZLi WZRe) Unten(KüFe KüTü WZLi WZRe GäWC HWR)
 *          Oben(BAD KiRe KiLi SZim) Alle(..)
 */


