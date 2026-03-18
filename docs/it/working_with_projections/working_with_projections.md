<!-- Recovered from: share/docs/html/it/it/working_with_projections/working_with_projections/index.html -->
<!-- Language: it | Section: working_with_projections/working_with_projections -->

# Lavorare con le proiezioni

KADAS ti consente di definire un sistema di riferimento - SR - (Coordinate Reference System, ovvero sistema di riferimento delle coordinate) globale o a livello di singolo progetto per i layer privi di un SR predefinito. Ti consente inoltre di definire sistemi di coordinate personalizzati e supporta anche la riproiezione al volo (on-the-fly, OTF) di vettori e raster. Tutte queste funzionalità ti permettono di visualizzare contemporaneamente layer con SR diversi.

## Panoramica sul supporto alle proiezioni

KADAS supporta all’incirca 2.700 SR. Le definizioni di ognuno di questi SR sono memorizzate in un database SQLite che viene installato insieme a KADAS. Normalmente non è necessario manipolare il database direttamente, infatti potresti causare il malfunzionamento del supporto alla proiezione. I SR personalizzati invece, sono salvati in un database utente. Vedi la sezione _sec\_custom\_projections_ per informazioni sulla gestione dei SR personalizzati.

I SR disponibili in KADAS sono basati su quelli definiti dall’European Petroleum Survey Group - EPSG - e dall’Institut Geographique National francese (IGN) e sono ricavati essenzialmente dalle tabelle di riferimento spaziale usate da GDAL. I codici EPSG sono presenti nel database e li puoi usare per idetificare e specificare i SR in KADAS.

Per usare la riproiezione al volo (OTF), i dati devono contenere informazioni sul proprio sistema di riferimento, altrimenti devi definire un SR per il layer, a livello di progetto o a livello globale. Per i layer PostGIS, KADAS usa l’identificatore del riferimento spaziale specificato al momento della creazione del layer. Per i dati supportati da OGR, KADAS fa affidamento sulla presenza di un “mezzo” specifico per ciascun formato, che definisce il SR. Nel caso degli shapefile, ad esempio, si stratta di un file contenente l’indicazione del SR in formato Well Known Text (WKT). Il file della proiezione ha lo stesso nome dello shapefile, ma ha estensione `.prj`. Per esempio lo shapefile chiamato `alaska.shp`. avrà un corrispondente file di proiezione chiamato `alaska.prj`.

The _CRS_ tab contains the following important components:

1. **Filtro** — se conosci il codice EPSG, l’identificatore o il nome del SR che vuoi impostare, puoi utilizzare questa area di ricerca per trovarlo nell’elenco. Inserisci il codice EPSG, l’identificatore o il nome.
2. **Sistemi di riferimento usati di recente** — se ci sono dei SR che usi frequentemente, questi verranno visualizzati in questa sezione della finestra di dialogo. Clicca su una voce per impostare il SR associato.
3. **Sistemi di riferimento mondiali** — questa è una lista di tutti i SR supportati da KADAS, compresi quelli geografici, proiettati e personalizzati. Per specificare un SR, selezionalo dalla lista: il SR attivo verrà evidenziato.
4. **Testo PROJ.4** - è la stringa SR usata dal motore di proiezione Proj4. È un testo di sola lettura, a solo scopo informativo.

## Trasformazioni datum predefinite

La riproiezione al volo dipende dalla capacità di trasformare i dati in un ‘SR predefinito’ che in KADAS è WGS84. Per alcuni SR sono disponibili molti tipi di trasformazione. KADAS ti permette di definire la trasformazione da usare, altrimenti verrà usata la trasformazione predefinita.

In una finestra di dialogo, KADAS chiede quale trasformazione deve usare visualizzando il informazioni di PROJ.4 che descrivono la trasformazione di partenza e quella di destinazione. Puoi ottenere altre informazioni fermando il cursore del mouse sopra una trasformazione. Puoi salvare le impostazioni selezionando il pulsante ![radiobuttonon](../../images/radiobuttonon.png) _Ricorda la selezione_.
