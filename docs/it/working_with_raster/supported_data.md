<!-- Recovered from: share/docs/html/it/it/working_with_raster/supported_data/index.html -->
<!-- Language: it | Section: working_with_raster/supported_data -->

# Lavorare con i dati raster

Questa sezione descrive come visualizzare ed impostare le proprietà dei dati raster. KADAS usa la libreria GDAL per l’accesso alla lettura/scrittura a formati raster, tipo ArcInfo Binary Grid, ArcInfo ASCII Grid, GeoTIFF, ERDAS IMAGINE e molti altri. Il supporto ai raster GRASS è garantito da un plugin nativo di KADAS. Puoi caricare i dati raster in KADAS anche in sola lettura da archivi zip e gzip.

Attualmente, la libreria GDAL supporta più di 100 formati raster (vedi GDAL-SOFTWARE-SUITE _literature\_and\_web_). La lista completa è disponibile alla pagina web <http://www.gdal.org/formats_list.html>.

Per diverse ragioni, non tutti i formati elencati potrebbero funzionare in KADAS. Per esempio, alcuni formati richiedono la presenza di librerie commerciali di terze parti oppure l’installazione di GDAL potrebbe essere avvenuta senza il supporto al formato che intendi usare. Quando carichi un raster in KADAS, solo i formati ben testati appariranno nell’elenco dei tipi di file; altri formati non testati possono essere caricati selezionando l’opzione `[GDAL] Tutti i file (*)`.

## Cosa sono i dati raster?

I dati raster sono matrici di celle discrete che rappresentano elementi della superficie terrestre o dell’ambiente al di sopra o al di sotto di essa. Ogni cella nella matrice ha la stessa dimensione e le celle sono solitamente rettangolari (in KADAS sono sempre rettangolari). Esempi tipici di dati raster sono quelli provenienti dal telerilevamento come le fotografie aeree, le immagini da satellite e dati modellati come le matrici dell’elevazione.

Diversamente dai vettori, i dati raster di solito non hanno associato un database contenente i dati descrittivi di ogni cella e sono geocodificati in base alla risoluzione del pixel e alle coordinate x/y di un angolo del raster. Questo permette a KADAS di posizionare correttamente il dato sulla mappa.

Per posizionare e visualizzare correttamente un raster, KADAS legge le informazioni di georeferenziazione incorporate direttamente nel file raster (ad es. GeoTiff) o gestite da un apposito file (world file).
