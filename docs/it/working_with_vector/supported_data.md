<!-- Recovered from: share/docs/html/it/it/working_with_vector/supported_data/index.html -->
<!-- Language: it | Section: working_with_vector/supported_data -->

# Formati supportati

KADAS usa la libreria OGR per leggere e creare vettori, inclusi i formati ESRI shapefile, MapInfo e MicroStation, AutoCAD DXF, PostGIS, SpatiaLite, database Oracle Spatial e MSSQL, e molti altri ancora. Il supporto ai vettori GRASS e a PostgreSQL è fornito da plugin nativi. I vettori possono anche essere caricati in modalità lettura da archivi zip e gzip. Attualmente la libreria OGR supporta 69 formati di vettori (vedi OGR-SOFTWARE-SUITE in _literature\_and\_web_). La lista completa è diponibile all’indirizzo <http://www.gdal.org/ogr/ogr_formats.html>.

## Shapefile ESRI

Il formato di file predefinito in KADAS è ESRI shapefile. Il supporto al formato è fornito dalla libreria OGR Simple Feature Library (<http://www.gdal.org/ogr/>).

Uno shapefile è costituito da di un minimo di tre file:

1. `.shp` contente le geometrie
2. `.dbf` contenente gli attributi in formato dBase
3. `.shx` contenente l’indice

Uno shapefile può anche includere un file con suffisso `.prj` che contiene le informazioni sulla proiezione. Anche se non è obbligatorio, è molto utile avere informazioni sulla proiezione del file. Un insieme di dati shapefile può contenere anche altri tipi di file. Per ulteriori informazioni, vedi le specifiche tecniche di ESRI all’indirizzo <http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf>.

### Caricare uno shapefile

Quando si carica un livello vettoriale, si apre la seguente finestra di dialogo:

![](../../images/addvectorlayerdialog.png)

Nella finestra di dialogo seleziona ![radiobuttonon](../../images/radiobuttonon.png) _File_ e clicca su **[Sfoglia]**. Si aprirà cosi una finestra di dialogo standard che ti consentirà di cercare nel computer lo shapefile o qualunque altro dato vettoriale che vuoi caricare. La casella di controllo _Tipo file_ ![](../../images/selectstring.png) consente di selezionare in anticipo specifici formati supportati da OGR.

Se vuoi, puoi selezionare il tipo di codifica per lo shapefile.

![](../../images/shapefileopendialog.png)

Selezionando un shapefile dalla lista e cliccando **[Apri]** lo carica in KADAS.

**Colori dei vettori**

Quando aggiungi un vettore alla mappa, gli viene assegnato un colore casuale. Se aggiungi più vettori in una sola volta, ciascuno avrà un colore diverso.

Una volta caricato lo shapefile, puoi interagire con la mappa usando gli strumenti di navigazione. Per cambiare lo stile di un vettore, apri la finestra di dialogo _Proprietà layer_ facendo doppio click sul nome del vettore oppure cliccando con il tasto destro sul nome del vettore e scegliendo _Proprietà_. Vedi la sezione [_Menu Stile_](vector_properties.md#vector-style-menu) per ulteriori informazioni su come impostare la simbologia dei vettori.

### Ottimizzare le prestazioni per gli shapefile

Per migliorare le prestazioni di visualizzazione di uno shapefile, puoi creare un indice spaziale. L’indice spaziale migliora la velocità di visualizzazione quando usi le funzioni di zoom e di spostamento. Gli indici spaziali usati da KADAS hanno estensione `.qix`.

Segui questi passi per creare un indice spaziale:

- Carica uno shapefile cliccando sul pulsante ![](../../images/mActionAddOgrLayer.png) _Aggiungi vettore_ oppure premi `Ctrl+Shift+V`.
- Apri la finestra di dialogo _Proprietà layer_ facendo doppio click sul nome dello shapefile nella legenda o cliccandoci con il tasto destro e scegliendo _Proprietà_ dal menu contestuale.
- Nella scheda _Generale_ clicca sul pulsante **[Crea indice spaziale]**.

### Problemi nel caricare un file .prj

Se carichi uno shapefile con file `.prj` associato e KADAS non riesce a leggere le informazioni della proiezione, devi inserire manualmente queste informazioni nella scheda _Generale_ della finestra di dialogo _Proprietà layer_ cliccando su **[Specifica...]**. Questo è dovuto al fatto che spesso i file `.prj` non forniscono i parametri di proiezione completi che richiede KADAS e che sono elencati nella finestra di dialogo _SR_.

Per la stessa ragione, quando crei un nuovo shapefile in KADAS, vengono creati due differenti file di proiezione. Un file `.prj` che ha un insieme limitato di parametri compatibili con il software ESRI, e un file `.qpj` che memorizza l’insieme completo dei parametri del SR utilizzato. Quando KADAS trova un file `.qpj` utilizza quest’ultimo invece del file `.prj`.
