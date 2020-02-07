# bispp_lab_01
Bezpečnosť  informačných systémov z pohľadu praxe - Zadanie 1

### Kompilácia a spustenie
Pre skompilovanie treba zadať príkaz:

```Bash
make
```

Pre spustenie servera treba zadať príkaz:

```Bash
./fbankld fbank-exstack.conf
```

alebo

```Bash
./fbankld  fbank-nxstack.conf
```

Server bude štandardne bežať na localhoste, na porte 8080 (možno zmeniť v konfiguračnom súbore). Server možno spustiť s dvoma nastaveniami. Pomocou konfiguračného súboru *fbank-exstack.conf* bude proces servera bežať s vypnutým NX flagom, t.j. zásobník ma nastavený X flag a dáta v ňom môžu byť spustiteľné. Konfiguračný súbor *fbank-exstack.conf* naopak spúšťa server so zapnutým NX flagom a dáta na zásobníku už nie je možné vykonávať.

### Architektúra servera

Server obsahuje nasledujúce komponenty:
* *fbankld*; hlavný démon, ktorý spúšťa servisy definované v konfiguračnom súbore *fbank.conf*.
* *fbankd*; servis, ktorý preposiela všetky HTTP requesty korešpondujúcim servisom.
* *fbankfs*; servis, ktorý poskytuje statické súbory alebo spúšťa dynamické skripty.