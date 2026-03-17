<!-- Recovered from: share/docs/html/it/it/gps/index.html -->
<!-- Language: it | Section: gps -->

# Navigazione

Nella scheda GPS si trovano le funzionalità per l’interazione con un apparecchio GPS collegato e gli strumenti per disegnare, importare ed esportare waypoint e percorsi GPX (GPS Exchange Format).

## Attivare geolocalizzazione

Per poter usare un GPS con KADAS su Windows, sul sistema deve essere installata l’applicazione GpsGate Splitter, vedi [Configurazione di GPSGate](../gpsgate/gpsgate/)

Lo stato del collegamento GPS viene visualizzato nella riga di stato nell’area di programma inferiore. Questo pulsante di stato del GPS può essere attivato o disattivato per realizzare o interrompere la connessione. Il colore di riempimento del pulsante di stato cambia in base allo stato di connessione attuale:

- **Nero**: GPS non attivato
- **Blu**: connessione in fase di inizializzazione
- **Bianco**: connessione inizializzata, nessun dato ricevuto
- **Rosso**: connessione inizializzata, nessuna informazione sulla posizione disponibile
- **Giallo**: connessione inizializzata, solo 2D Fix
- **Verde**: connessione inizializzata, 3D Fix

Non appena KADAS riceve dei dati sulla posizione dal GPS, sulla mappa viene disegnato un marcatore di posizione corrispondente.

## Movimento con la posizione

Questa funzione attiva lo spostamento automatico della sezione di mappa visibile, centrata sulla posizione GPS attuale.

## Disegno di waypoint e rotte

Con queste funzioni vengono disegnati waypoint e rotte che potranno essere successivamente salvati in formato GPX, ad esempio per il caricamento su un apparecchio GPS.

I **waypoint** sono punti semplici sulla mappa che possono essere inoltre dotati di un nome.

I **rotte** sono polilinee che possono essere dotate di nome e numero.

Waypoint e rotte vengono salvat in un proprio layer **Percorsi GPS**, analogo al layer Redlining.

![](../media/image9.png)

## Esportazione e importazione GPX

Queste funzioni permettono l’esportazione dei waypoint e dei percorsi disegnati in un file GPX e l’importazione di un file GPX esistente nel layer **Percorsi GPS**.
