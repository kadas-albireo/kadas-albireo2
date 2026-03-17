<!-- Recovered from: share/docs/html/it/it/working_with_raster/raster_properties/index.html -->
<!-- Language: it | Section: working_with_raster/raster_properties -->

# Proprietà raster

Per visualizzare ed impostare le proprietà di un raster, fai doppio click sul nome del raster nella legenda o cliccaci sopra con il tasto destro e scegli _Proprietà_ dal menu contestuale. Si aprirà cosi la finestra di dialogo _Proprietà del layer_.

Ci sono diversi menu nella finestra di dialogo:

- _Generale_
- _Stile_
- _Trasparenza_
- _Piramidi_
- _Istogramma_
- _Metadati_

![](../../../../images/rasterPropertiesDialog.png)

## Menu Generale

### Informazioni del layer

Il menu _Generale_ contiene informazioni basilari del raster selezionato, inclusa la sorgente del file, il nome visualizzato nella legenda (che puoi modificare) e il numero di colonne, righe e valori nulli.

### Sistema Riferimento Coordinate

Qui puoi trovare il sistema di riferimento spaziale (SR) visualizzato in formato stringa PROJ.4. Se l’impostazione non è corretta la puoi modificare con il pulsante **[Specifica]**.

### Visibilità dipendente dalla scala

In questo menu puoi attivare la funzione che imposta la visibilità del raster in funzione della scala. Spuntando la casella di controllo puoi impostare l’intervallo di scala in cui vuoi che il raster venga visualizzato nella mappa.

Nella parte inferiore puoi vedere un’anteprima del raster, la sua simbologia e la tavolozza.

## Menu Stile

### Visualizzazione banda

KADAS offre quattro tipologie di _Visualizzazione del layer_. La scelta dipende dal tipo di dato.

1. Colori banda multipla - se il file è caricato come multibanda e ha diverse bande di colori (per esempio un’immagine satellitare con molte bande diverse)
2. Tavolozza - se un file ha una tavolozza indicizzata (per esempio una mappa topografica digitale)
3. Banda singola grigia - (una banda) l’immagine verrà visualizzata in grigio: KADAS sceglierà questo tipo di visualizzazione se il file non ha né bande multiple né una tavolozza indicizzata né una tavolozza continua (comune per una mappa dei rilievi)
4. Banda singola falso colore - puoi usare questo visualizzatore per i file che hanno una tavolozza continua o una mappa di colore (per esempio una mappa delle altimetrie)

**Colori banda multipla**

Con il visualizzatore colore banda multipla verranno visualizzate le tre bande selezionate dell’immagine, ognuna delle quali corrisponde alle componenti rosso, verde e blu che verranno usate per creare i colori dell’immagine stessa. Puoi scegliere fra diversi metodi di _Miglioramento contrasto_: ‘Nessun miglioramento’, ‘Stira a MinMax’, ‘Stira e taglia a MinMax’ e ‘Taglia a MinMax’.

![](../../../../images/rasterMultibandColor.png)

Questa sezione offre un’ampia gamma di opzioni per modificare l’aspetto del tuo raster. Prima di tutto scegli l’estensione dell’immagine da _Estensione_ e poi premi il pulsante **[Carica]**. KADAS può scegliere l’_Accuratezza_ stimando i valori _Min_ e _Max_ tramite i pulsanti _Stimato (veloce)_ e _Attuale (lento)_.

Ora puoi impostare i colori con l’aiuto della sezione _Carica i valori min/max_. Molte immagini hanno pochi valori estremi. Puoi eliminare questi outlier con l’impostazione ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_. L’intervallo standard è impostato dal 2% al 98% dei valori del file e può essere adattato manualmente. Con questa impostazione potrebbero sparire i caratteri grigi. Con l’opzione ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min/max_, KADAS crea una tabella di colori con tutti i valori dell’immagine originale (per esempio KADAS crea una tabella di colori con 256 valori, se la tua immagine ha bande a 8 bit). Puoi anche creare la tua tabella dei colori usando l’opzione ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Media +/- deviazione standard x_ ![](../../../../images/selectnumber.png). In questo modo solamente i valori inclusi nella deviazione standard o in multipli della deviazione standard verranno considerati nella tabella dei colori. Questo è molto utile quando hai una o due celle con valori molto grandi che avrebbero un impatto negativo nella visualizzazione del raster.

Le stesse impostazioni sono valide anche per l’estensione ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Attuale_.

**Visualizzare una singola banda di un raster multibanda**

Se vuoi vedere solamente una banda singola di un’immagine multibanda (per esempio, rossa) potresti pensare di impostare le bande verde e blu come “Non impostato”. Ma questo non è il miglior modo di agire. Per visualizzare la banda rossa, seleziona il visualizzatore ‘Banda grigia singola’ e poi seleziona il rosso come colore da usare al posto del grigio.

**Tavolozza**

Questo è il visualizzatore standard per i file a banda singola che hanno già una tavola di colori, dove a ogni valore dei pixel è associato un determinato colore. In questo caso, la tavolozza viene visualizzata automaticamente. Se vuoi cambiare i colori assegnati a certi valori fai semplicemente doppio click sul colore e si aprirà cosi la finestra _Seleziona colore_. In KADAS 2.2 puoi anche assegnare un’etichetta ai valori dei colori. L’etichetta comparirà cosi nella legenda.

![](../../../../images/rasterPaletted.png)

**Miglioramento contrasto**

Quando aggiungi un raster di GRASS, l’opzione _Miglioramento contrasto_ è sempre impostata su _Stira a MInMax_ anche se hai impostato altri valori nelle opzioni di KADAS.

**Banda singola grigia**

Questo visualizzatore ti permette di visualizzare un raster a banda singola con un _Gradiente di colore_: ‘Da nero a bianco’ o ‘Da bianco a nero’. Puoi selezionare il valore _Min_ e quello _Max_ scegliendo prima l’opzione _Estensione_ e poi premendo il pulsante **[Carica]**. KADAS può scegliere l’_Accuratezza_ stimando i valori _Min_ e _Max_ tramite i pulsanti _Stimato (veloce)_ e _Attuale (lento)_.

![](../../../../images/rasterSingleBandGray.png)

Nella sezione _Carica i valori min/max_ puoi scegliere la tabella dei colori. Puoi eliminare questi outlier con l’impostazione ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_. L’intervallo standard è impostato dal 2% al 98% dei valori del file e può essere adattato manualmente. Con questa impostazione potrebbero sparire i caratteri grigi. Puoi effettuare altri cambiamenti con le impostazioni _Min/max_ e ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Media +/- deviazione standard x_ ![](../../../../images/selectnumber.png). Mentre la prima crea una tabella di colori con tutti i valori dell’immagine originale, la seconda opzione che una tabella di colori in cui vengono considerati solamente i valori che ricascano all’interno della deviazione standard o a un multiplo di questa. Questo è molto utile quando hai una o due celle con valori molto grandi che avrebbero un impatto negativo nella visualizzazione del raster.

**Banda singola falso colore**

Questa è l’opzione di visualizzazione per i file a banda singola, inclusa una tavolozza continua. Puoi anche creare mappe di colori singoli per le bande singole.

![](../../../../images/rasterSingleBandPseudocolor.png)

Sono disponibili tre tipologie di interpolazione di colore:

1. Discreto
2. Lineare
3. Esatto

Nella parte sinistra, il pulsante ![](../../../../images/mActionSignPlus.png) _Aggiungi un valore manualmente_ aggiunge un valore alla tabella dei colori. Il pulsante ![](../../../../images/mActionSignMinus.png) _Rimuovi la riga selezionata_ cancella un valore dalla tabella dei colori e il pulsante ![](../../../../images/mActionArrowDown.png) _Ordina gli elementi della mappa dei colori_ ordina i colori della tabella in funzione dei valori dei pixel e dei valori della colonna. Facendo doppio click sul valore presente nella colonna potrai inserire un valore specifico. Facendo invece doppio click su un colore, potrai scegliere un colore specifico da assegnare a quel valore. Inoltre puoi anche aggiungere un’etichetta per ogni colore, ma questa etichetta non verrà visualizzata quando userai lo strumento Informazioni elementi. Puoi anche cliccare sul pulsante ![](../../../../images/mActionDraw.png) _Carica mappa colore dalla banda_, che prova a caricare la tabella dalla banda (se questa esiste). Puoi usare i pulsanti ![](../../../../images/mActionFileOpen.png) _Carica mappa colore da file_ oppure ![](../../../../images/mActionFileSaveAs.png) _Esporta mappa colore su file_ per caricare una tabella di colori esistente o per salvarne una per le sessioni successive.

Nella parte destra, la sezione _Genera nuova mappa colore_ ti permette di creare mappe di colore categorizzate. Per la _Modalità_ ![](../../../../images/selectstring.png) ‘Intervallo uguale’ devi solamente scegliere il _numero di classi_ ![](../../../../images/selectnumber.png) e premere il pulsante _Classifica_. Puoi invertire i colori spuntando la casella di controllo ![](../../../../images/checkbox.png) _Inverti_. Se hai scelto la _Modalità_ ![](../../../../images/selectstring.png) ‘Continuo’, KADAS crea automaticamente le classi in funzione dei valori _Min_ e _Max_. Puoi definire i valori _Min/Max_ con l’aiuto della sezione _Carica i valori min/max_. Molte immagini hanno pochi valori estremi. Puoi eliminare questi outlier con l’impostazione ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_. L’intervallo standard è impostato dal 2% al 98% dei valori del file e può essere adattato manualmente. Con questa impostazione potrebbero sparire i caratteri grigi. Con l’opzione ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min/max_, KADAS crea una tabella di colori con tutti i valori dell’immagine originale (per esempio KADAS crea una tabella di colori con 256 valori, se la tua immagine ha bande a 8 bit). Puoi anche creare la tua tabella dei colori usando l’opzione ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Media +/- deviazione standard x_ ![](../../../../images/selectnumber.png). In questo modo solamente i valori inclusi nella deviazione standard o in multipli della deviazione standard verranno considerati nella tabella dei colori.

### Visualizzazione colore

Per ogni _Visualizzazione banda_, è disponibile una _Visualizzazione colore_.

Puoi anche ottenere effetti speciali per i tuoi raster usando una delle modalità fusione.

Ulteriori impostazioni possono essere fatte modificando la _Luminosità_, la _Saturazione_ e il _Contrasto_. Puoi usare anche l’opzione _Scala di grigi_ dove puoi scegliere fra ‘Per chiarezza’, ‘Per luminosità’ e ‘Per media’. Puoi modificare la ‘Forza’ per ogni tonalità della tabella dei colori.

### Ricampionamento

La sezione _Ricampionamento_ ha effetto quando ingrandisci o rimpicciolisci l’immagine. I metodi di ricampionamento ottimizzano l’aspetto della mappa perché calcolano una nuova matrice di grigi attraverso una trasformazione geometrica.

![](../../../../images/rasterRenderAndRessampling.png)

Applicando il metodo ‘vicino più prossimo’ la mappa potrebbe avere una struttura con molti pixel quando viene ingrandita. Questo aspetto può essere migliorato usando i metodi ‘Bilineare’ o ‘Cubico’ perché creano delle geometrie più appuntite e offuscate. Il risultato è un’immagine più morbida. Puoi applicare questo metodo, per esempio, a mappe raster topografiche.

## Menu Trasparenza

KADAS riesce a visualizzare ogni raster con differenti livelli di trasparenza. Usa il cursore trasparenza ![slider](../../../../images/slider.png) per impostare il livello di trasparenza che desideri. Questa opzione è molto utile se vuoi sovrapporre diversi raster (per esempio una mappa dei rilievi sovrapposta a un raster classificato). In questo modo puoi simulare un effetto tridimensionale.

Inoltre puoi inserire nel menu _Valori nulli aggiuntivi_ un valore che deve essere trattato come _Valore nullo_.

Puoi definire la trasparenza in maniera ancora più dettagliata e personalizzata nella sezione _Opzioni di trasparenza personalizzate_, nella quale puoi impostare il grado di trasparenza di ogni singola cella (o pixel).

Per esempio, vogliamo impostare l’acqua del file `landcover.tif` con una trasparenza del 20%. Questi sono i passi necessari:

1. Carica il file
2. Apri la finestra di dialogo _Proprietà_ facendo doppio click sul nome del raster nella legenda o cliccando su di esso con il tasto destro del mouse e scegliendo _Proprietà_ dal menu contestuale.
3. Seleziona il menu _Trasparenza_.
4. Scegli ‘Nessuno’ dal menu _Banda trasparenza_.
5. Clicca sul pulsante ![](../../../../images/mActionSignPlus.png) _Aggiungi valori manualmente_. Apparirà cosi una nuova riga nell’elenco.
6. Inserisci il valore nelle colonne ‘Da’ e ‘A’ (nell’esempio viene usato 0) e aggiusta la trasparenza al 20%.
7. Clicca sul pulsante **[Applica]** per visualizzare il risultato.

Ripeti i passaggi 5 e 6 per aggiustare più valori con trasparenze personalizzate.

Come puoi vedere è molto semplice impostare una trasparenza personalizzata, però richiede comunque un po’ di lavoro. Proprio per questo puoi usare il pulsante ![](../../../../images/mActionFileSave.png) _Esporta su file_ per salvare la lista dei valori su un file esterno. Il pulsante ![](../../../../images/mActionFileOpen.png) _Importa da file_ ti permette di caricare le impostazioni di trasparenza e applicarle al raster selezionato.

## Menu Metadati

La scheda _Metadati_ mostra una serie di informazioni sul raster, incluse le statistiche di ogni banda. Da questo menu hai accesso a diverse sezioni: _Descrizione_, _Assegnazione_, _URL Metadati_ e _Proprietà_. Nella sezione _Proprietà_ le statistiche sono ottenute da una base ‘che si deve ancora conoscere’, quindi è meglio che le statistiche di questo raster non siano ancora state calcolate.

![](../../../../images/rasterMetadata.png)
