<!-- Recovered from: share/docs/html/it/it/index.html -->
<!-- Language: it | Section: index -->

# Generalità

## Origine

KADAS Albireo è un'applicazione per la visualizzazione di mappe basata sul software professionale open source GIS "QGIS" e rivolta ad utenti non specializzati. In collaborazione con la ditta Ergnomen, è stata sviluppata una nuova interfaccia che nasconde gran parte delle funzionalità rivolte ad utenti avanzati, migliorando al contempo le funzionalità in ambito di disegno, analisi del terreno, stampa ed interoperabilità.

## Condizioni d'uso

KADAS Albireo è protetto dalla General Public License 2.0 (GPLv2).

I componenti MSS/MilX sono di proprietà della società gs-soft SA.

Le condizioni d'uso dei dati sono riportate nell'applicazione alla voce Aiuto → Informazioni.

## Protocollo delle versioni

### Versione 2.3 (agosto 2025)

Con il rilascio di KADAS 2.3 nel terzo trimestre del 2025 sono state implementate numerose novità, miglioramenti e correzioni di bug.

- _Nuove funzionalità e geodati_
    - I geoservizi e i dati geografici con una componente temporale possono essere riprodotti come animazioni cronologiche.
    - Confronto tematico (strumento SWIPE) in cui è possibile visualizzare e nascondere in modo interattivo due mappe utilizzando un cursore.
    - Le tabelle degli attributi aperte vengono salvate nel progetto e visualizzate all'apertura del progetto.
    - Supporto del formato di dati raster NetCDF (utilizzato principalmente in meteorologia)
    - Un nuovo pulsante «Feedback». Questo pulsante porta direttamente a un modulo sul Geo Info Portal V e consente di inviare feedback sulle funzionalità del software e sui bug.
    - La creazione di profili altimetrici fornisce ulteriori statistiche, come ad esempio il punto più alto/più basso, la lunghezza e il dislivello rispetto alla linea del profilo.
    - Vector Tiles Geodienste der MGDI werden im Geokatalog gelistet und können geladen werden.
    - I dati offline sono ora costituiti dalla World Briefing Map.
    - L'esportazione MSS è ora possibile come KML (non modificabile dopo l'esportazione KML).
- _Miglioramenti_
    - Plugin Manager: all'avvio di KADAS viene verificato se nel repository sono disponibili versioni più recenti dei plugin installati. In tal caso, viene installata automaticamente la versione più recente del plugin e l'utente viene informato con un messaggio.
    - Durante la ricerca tramite clic, è stata migliorata la visualizzazione dei risultati, in particolare quando vengono restituiti più risultati.
    - La funzione Segnalibri ora salva anche i livelli in gruppi e sottogruppi.
    - Il plugin Routing è ora un plugin standard e viene installato automaticamente dal repository. È possibile definire come obbligatori anche altri plugin.
    - I geoservizi limitati nell'MGDI possono essere visualizzati dagli utenti autorizzati in KADAS.
    - La ricerca delle coordinate è stata migliorata e standardizzata.
    - Il supporto dei geoservizi WCS consente ora anche analisi con modelli altimetrici ad alta risoluzione.
    - La funzione di testo in Redlining contiene ulteriori opzioni di impostazione.
    - La visualizzazione e l'etichettatura della griglia MGRS sono state ottimizzate.
    - I modelli di stampa sono stati aggiornati e ora contengono di default la data di creazione e il sistema di proiezione.
    - Le analisi possono essere avviate direttamente con un clic destro del mouse.
    - È possibile definire l'estensione dell'esportazione GPKG dei dati vettoriali.
- _Correzione di errori_
    - Risolto il crash durante l'esportazione di GeoPDF
    - Durante l'esportazione di GeoPDF vengono considerati tutti i livelli e visualizzati correttamente.
    - Risolto il crash durante l'utilizzo dello strumento Ephemeris.
    - Risolto il crash durante l'esportazione di Geopackage.
    - Risolto: traduzioni mancanti nelle lingue (DE, FR, IT)
- _Modifiche tecniche_
    - KADAS 2.3 si basa sulla versione QGIS 3.44.0-Solothurn
    - I due modelli altimetrici della Svizzera e del mondo (dtm\_analysis.tif + general\_dtm\_globe.tif) sono disponibili in formato COG (Cloud Optimized GeoTIFF). Ciò consente un'elaborazione e una visualizzazione più rapide.
    - Il visualizzatore 3D (Globe) ha dovuto essere sostituito perché la tecnologia utilizzata è giunta al termine del suo ciclo di vita. Ora il visualizzatore 3D viene utilizzato dall'applicazione QGIS.
    - La libreria MSS/MilX di gs-soft utilizzata è ora MSS-2025.
    - Il Valhalla Routing Engine è stato aggiornato e consente di includere l'altitudine nel calcolo del percorso e della distanza.
    - Tutti i plugin produttivi sono stati adattati alla versione 2.3.
    - La guida è stata esternalizzata nel browser.

### Versione 2.2.0 (giugno 2023)

- _Generale_:
    - Permette il caricamento di livelli Esri VectorTile
    - Permette il caricamento di livelli Esri MapService
    - Albero dei livelli: permette la configurazione dell'intervallo di aggiornamento dell'origine dati per l'aggiornamento automatico dei livelli
    - Supporto dell'esportazione di stampa GeoPDF
    - Possibilità di bloccare la scala della mappa
    - Finestra di dialogo "novità" configurabile
    - Miglioramento dell'importazione di geometrie 3D da file KML
- _Vista_:
    - Possibilità di scattare foto istantanee della vista 3D
    - Miglioramento dell'etichettatura della griglia MGRS
- _Analisi_:
    - Nuovo strumento min/max per determinare il punto più basso/più alto nell'area selezionata
    - Selezione del fuso orario nello strumento effemeridi
    - Gestione corretta dei valori NODATA nel profilo altimetrico
- _Disegno_:
    - Consentire l'annullamento/ripristino durante l'intera sessione di disegno.
    - Possibilità di modificare l'ordine z dei disegni
    - Possibilità di aggiungere immagini da URL
- _MSS_:
    - Consente l'annullamento/ripristino durante l'intera sessione di disegno.
    - Possibilità di stilizzare le linee guida (larghezza, colore)
    - Aggiornamento a MSS-2024
- _Aiuto_:
    - Consente la ricerca nella guida

### Versione 2.1.0 (dicembre 2021)

- _Generale_:
    - Stampa: Scalare correttamente i simboli (MSS, pin, immagini, ...) in base ai DPI di stampa
    - GPKG: Permettere l'importazione dei livelli di progetti
    - Indice: Possibilità di zoomare e rimuovere tutti i layer selezionati
    - Visibilità basata sulla scala anche per i layer redlining/MSS
    - Tabella attributi: vari nuovi strumenti di selezione e zoom per
- _Vista_:
    - Nuova funzione segnalibri
- _Analisi_:
    - Viewshed: Possibilità di limitare gli angoli verticali dell'osservatore
    - Profilo altimetrico / linea di vista: mostrare l'indicatore nel grafico quando si passa sopra la linea sulla mappa
    - Nuovo strumento effemeridi
- _Disegno_:
    - Spilli: Nuovo editor richt-text
    - Spilli: Permettere di interagire con il contenuto del tooltip con il mouse
    - Griglia di guida: Consentire l'etichettatura di un solo quadrante
    - Bullseye: Etichettatura dei quandranti
    - Nuovo elemento di disegno a croce di coordinate
- _MSS_:
    - Impostazioni dei simboli per livello
    - Aggiornamento a MSS-2022

### Versione 2.0.0 (luglio 2020)

- Nuova architettura dell'applicazione: KADAS è ora un'applicazione separata, basata sulle librerie QGIS 3.x
- Nuova architettura comune per tutti gli oggetti posizionabili sulla mappa (redlining, symboli MSS; ecc.), per un flusso di lavoro coerente durante il disegno e la modifica degli elementi
- Utilizza il nuovo formato di file qgz, evitando la precedente cartella `<projectname>_files`
- Autosalvataggio del progetto
- Nuovo strumento per la gestione di plugin direttamente dall'interno di KADAS
- Modalità a schermo intero
- Nuova implementazione della griglia della mappa, supporta griglie UTM/MGRS sulla mappa principale
- Esportazione KML/KMZ confinata ad un'estensione
- Esportazione dati GPKG confinata ad un'estensione
- Stili delle geometrie di redlining sono applicati quando visualizzate come oggetti 2.5D o 3D sul globo
- Griglia di guida migliorata
- Aggiornamento a MSS-2021

### Versione 1.2 (Dicembre 2018)

- _Generale_:
    - Funzionalità di esportazione KML/KMZ migliorata
    - Nuova funtionalità di importazione KML/KMZ
    - Nuova funzionalità di esportazione e importazione GeoPackage
    - Possibilità di aggiungere layer CSV/WMS/WFS/WCS dall'interfaccia ribbon
    - Possibilità di aggiungere funzionalità all'interfaccia ribbon dall'API Python
    - Aggiungi tasti di scelta rapida per varie funzionalità dell'interfaccia ribbon
    - Migliora il "fuzzy-matching" nella ricerca di coordinate
- _Analisi_:
    - Disegna i vertici della linea di misura nel profilo di elevazione
- _Disegnare_:
    - Supporta l'input numerico nel disegno di oggetti redlining
    - Possibilità di scalare il contanuto di interi layer di annotazioni
    - Possibilità di attivare e disattivare le cornici di immagini
    - Possibilità di manipulare gruppi di annotazioni
    - Nuova funtionalità: griglia di guida
    - Nuova funtionalità: Bullseye
- _GPS_:
    - Possibilità di convertire tra waypoints e spilli
    - Possibilità di modificare il colore di waypoints e rotte
- _MSS_:
    - Aggiornamento a MSS-2019

### Versione 1.1 (Novembre 2017)

- _Generale_:
    - Cursore liberamente posizionabile nel campo di ricerca
    - Indicazione della quota nella barra dello stato
    - Miglioramenti di prestazione della mappa
    - Tabella degli attributi per layer vettoriali
- _Analisi_:
    - Misurazioni geodetiche di distanze e aree
    - Azimut relativo al Nord della mappa o al Nord geografico
- _Disegnare_:
    - Possibilità di attivare lo snapping durante la modalità di disegno
    - Possibilità di annullare/ripetere nella modalità di disegno
    - Possibilità di spostare, copiare, tagliare ed incollare disegni, singolarmente o in gruppo
    - Possibilità di continuare geometrie esistenti
    - Possibilità di caricare grafiche SVG (tra cui simboli SymTaZ)
    - Possibilità di caricare immagini non georeferenziate
    - Spilli e immagini sono vengono ora raggruppate in layer
- _MSS_:
    - Aggiornamento a MSS-2018
    - Rapporto dimensionale corretto dei simboli MSS nella stampa
    - Il cartiglio può venir importato e esportato da risp. a file MilX o XML
    - Input numerico per gli attributi nella modalità di disegno di simboli MSS
- _3D_:
    - Possibilità di visualizzare geometrie come modelli 3D
- _Stampa_:
    - Possibilità di gestire i modelli di stampa contenuti nel proggetto

### Versione 1.0 (Settembre 2016)

- Versione iniziale
