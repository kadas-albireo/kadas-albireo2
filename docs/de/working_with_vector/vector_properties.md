<!-- Recovered from: share/docs/html/de/de/working_with_vector/vector_properties/index.html -->
<!-- Language: de | Section: working_with_vector/vector_properties -->

# Vektorlayereigenschaften

Der _Layereigenschaften_-Dialog stellt Informationen über den Layer, Darstellungseinstellungen und Beschriftungsoptionen bereit. Wenn ein Vektorlayer aus einer PostgreSQL/PostGIS Datenbank geladen wurde, können über den Dialog _Layereigenschaften_ auch SQL-Abfragen mit dem _Objektuntermenge_-Dialog im Menü _Allgemein_ angewendet werden. Um den _Layereigenschaften_-Dialog zu erreichen doppelklicken Sie einen Layer in der Legende oder machen Sie einen Rechtsklick auf den Layer und wählen Sie _Eigenschaften_ aus dem Popupmenü.

![](../../../../images/vector_general_menu.png)

## Menü Stil

Das Menü Stil stellt Ihnen ein umfassendes Werkzeug zum Darstellen und symbolisieren Ihrer Vektordaten zur Verfügung. Sie können _Layerdarstellung ‣_ Werkzeuge, die für alle Vektordaten gleich sind, genauso wie spezielle Symbolisierungstools, die für die verschiedenen Arten von Vektordaten konzipiert wurden, verwenden.

### Darstellungen

Der Renderer ist dafür verantwortlich ein Objekt zusammen mit dem richtigen Symbol zu zeichnen. Es gibt vier Arten von Renderern: Einzelsymbol, Kategorisiert, Abgestuft, Regelbasierend und Punktverdrängung. Es gibt keinen kontinuirliche Farbe Renderer da es in der Tat einfach ein spezieller Fall des Abgestuft Renderers ist. Die Kategorisiert und Abgestuft Renderer können erstellt werden indem ein Symbol und ein Farbverlauf festgelegt werden - Sie werden die Farben für Symbole angemessen einsetzen. Für Punktlayer ist ein Punktverdrängung Renderer erhältlich. Für jeden Datentyp (Punkte, Linien und Polygone) sind Symbollayertypen erhältlich. Abhängig vom ausgesuchten Renderer gibt es im Menü _Stil_ verschiedene zusätzliche Bereiche. Im rechten unteren Teil des Symbologie Dialogs gibt es einen **[Symbol]** Knopf der den Zugang zum Stilmanager ermöglicht (siehe Abschnitt [vector\_style\_manager\_](#id2)). Mit dem Stilmanager können Sie bestehende Symbole bearbeiten und entfernen als auch neue Symbole hinzufügen.

Nachdem alle nötigen Veränderungen vorgenommen wurden kann das Symbol zur Liste der aktuellen Stilsymbole hinzugefügt werden (indem Sie **[Symbol]** ![](../../../../images/selectstring.png) _Speichern_ verwenden) und dann kann es auf einfache Weise später benutzt werden. Darüberhinaus können Sie den **[Stil speichern ...]** ![](../../../../images/selectstring.png) Knopf um das Symbol als QGIS-Layerstildatei (.qml) oder SLD-Datei (.sld) zu speichern benutzen. SLDs können von jedem Darstellungstyp - Einzelsymbol, Kategorisiert, Abgestuft oder Regelbasierend - exportiert werden, aber wenn Sie ein SLD importieren wird entweder nur Einzelsymbol oder oder Regelbasierend erstellt. Das heisst dass Kategorisierte oder Abgestufte Stile nach Regelbasierend konvertiert werden. Wenn Sie diese Darstellungsarten beibehalten wollen müssen Sie zum QML-Format greifen. Auf der anderen Seite kann es sehr nützlich sein diese einfache Art, Stile nach Regelbasierend zu konvertieren, anzuwenden.

Wenn Sie den Darstellungstyp beim Einstellen des Stils eines Vektorlayers ändern werden die Einstellungen für das Symbol beibehalten. Beachten Sie dass dieses Vorgehen nur für eine Änderunge funktioniert. Wenn Sie den Darstellungstyp wiederholt ändern gehen die Einstellungen für das Symbol verloren.

Wenn die Datenquelle der Schicht eine Datenbank ist (z.B. PostGIS oder Spatialite), können Sie Ihren Layerstil in einer Tabelle der Datenbank speichern. Klicken Sie einfach auf die _Styl speichern_ Combox und wählen Sie **In Datenbank speichern**, und geben Sie im Dialog Style-Namen und Beschreibung ein, sowie eine ui-Datei und ob der Style ein Standardstyle ist. Wenn beim Laden einer Layer aus der Datenbank bereits ein Style für diese Layer existiert, lädt KADAS die Layer und deren Style. Sie können mehrere Styles in der Datenbank hinzufügen. Nur einer wird ohnehin der Standardstil sein.

![](../../../../images/save_style_database.png)

**Auswahl und Ändern von Mehrfachsymbolen**

Mit der Symbologie können Sie Mehrfachsymbole auswählen und Rechtsklicken um Farbe, Transparenz, Größe oder Breite der ausgewählten Einträge zu ändern.

**Einzelsymbol Darstellung**

Der Einzelsymbol Renderer wird verwendet um alle Objekte des Layers mit einem einfachen benutzerdefinierten Symbol darzustellen. Die Eigenschaften, die im Menü _Stil_ angepasst werden können, hängen teilweise von dem Typ des Layers ab, wobei alle Typen die folgende Dialogstruktur befolgen. Im oberen linken Teil des Menüs gibt es einen Preview von den aktuellen Symbolen die dargestellt werden sollen. Im rechten Teil des Menüs gibt es eine Liste von Symbolen die bereits für den aktuellen Stil definiert wurden, welche durch Auswählen aus der Liste benutzt werden können. Das aktuelle Symbol kann mit dem Menü auf der rechten Seite verändert werden.

Wenn Sie auf die erste Ebene im _Symbollayer_ Dialog auf der linken Seite klicken ist es möglich grundlegende Parameter wie _Größe_, _Transparenz_, _Farbe_ und _Drehung_ zu definieren. Hier werden die Ebenen zusammengeführt.

![](../../../../images/singlesymbol_ng_line.png)

In jeder Spinbox in diesem Dialogfeld können Sie Ausdrücke eingeben. Z.B. können Sie einfache Mathematik berechnen, wie z.B. die vorhandene Größe eines Punktes mit 3 multiplizieren, ohne auf einen Taschenrechner zurückgreifen zu müssen.

![](../../../../images/expression_symbol_size_spinbox.png)

Wenn Sie im Dialog _Symbolebenen_ auf die zweite Ebene klicken, ist eine 'Datendefinierte Übersteuerung' für fast alle Einstellungen möglich. Bei der Verwendung einer datendefinierten Farbe kann es sinnvoll sein, die Farbe mit einem Feld "geknickt" zu verknüpfen. Hier wird eine Kommentarfunktion eingefügt.

```
/* This expression will return a color code depending on the field value.
 * Negative value: red
 * 0 value: yellow
 * Positive value: green
 */
CASE
  WHEN value < 0 THEN '#DC143C' -- Negative value: red
  WHEN value = 0 THEN '#CCCC00' -- Value 0: yellow
  ELSE '#228B22'                -- Positive value: green
END
```

**Kategorisierte Darstellung**

Der Kategorisiert Renderer wird verwendet um alle Objekte eines Layers darzustellen indem man ein einfaches benutzerdefiniertes Symbol, dessen Farbe den Wert des ausgewählten Attributs des Objekts wiedergibt. Im _Stil_ Menü können Sie folgendes auswählen:

- Das Attribut (indem Sie die Spalten Listbox oder die ![](../../../../images/mIconExpressionEditorOpen.png) _Spaltenausdruck einstellen_ Funktion benutzen, siehe [_Ausdrücke_](expression.html#vector-expressions))
- Das Symbol (über die Auswahl Symbol)
- Die Farben (indem Sie die Farbverlauf Listbox verwenden)

Klicken Sie dann auf den **Klassifizieren** Knopf um Klassen aus den unterschiedlichen Werten der Attributspalte zu erstellen. Jede Klasse kann ausgeschaltet werden indem Sie das Kontrollkästchen links vom Klassennamen deaktivieren.

Sie können Symbol, Wert und/oder Bezeichnung der Klasse ändern, indem Sie einfach auf das Element doppelklicken, das Sie ändern möchten.

Ein Rechtsklick zeigt ein Kontextmenü zu **Kopieren/Einfügen**, **Farbe ändern**, **Transparenz ändern**, **Ausgabeeinheit ändern**, **Symbolbreite ändern**.

Mit dem **[Erweitert]** Knopf in der rechten unteren Ecke des Dialogs können Sie die Felder, die Drehungs- und Größenskalierungsinformationen enthalten, einstellen. Der Einfachheit halber werden in der Mitte des Menüs die Werte aller aktuell ausgewählten Attribute zusammen aufgeführt, inklusive der Symbole die dargestellt werden sollen.

Das folgende Bild zeigt den Dialog zur Darstellung der Kategorie, der für die Flusslage des KADAS-Beispieldatensatzes verwendet wird.

![](../../../../images/categorysymbol_ng_line.png)

**Abgestufte Darstellung**

Der Abgestuft Renderer wird verwendet alle Objekte eines Layers darzustellen indem ein einfaches benutzerdefiniertes Symbol dessen Farbe die Zuweisung eines Objektattributes zu einer Klasse reflektiert.

![](../../../../images/graduatesymbol_ng_line.png)

Wie beim Kategorisiert Renderer können Sie mit dem Abgestuft Renderer die Drehung und Größenskalierung von angegebenen Spalten definieren.

Genauso können Sie -analog zum Kategorisierten Renderer - im Menü _Stil_ auswählen:

- Das Attribut (indem Sie die Spalten Listbox oder die ![](../../../../images/mIconExpressionEditorOpen.png) _Spaltenausdruck einstellen_ Funktion benutzen, siehe das [_Ausdrücke_](expression.html#vector-expressions) Kapitel)
- Das Symbol (über die Auswahl Symbol)
- Die Farben (indem man die Farbverlaufsliste verwendet)

Zusätzlich können Sie die Anzahl der Klassen und auch den Modus für das Klassifizieren von Objekten innerhalb der Klassen (indem man die Modus-Liste verwendet). Die möglichen Modi sind:

- Gleiches Intervall: jede Klasse hat die gleich Größe (z.B. Werte von 0 bis 16 und 4 Klassen, jede Klasse hat eine Größe von 4);
- Quantil: jede Klasse beinhaltet die gleiche Anzahl von Elementen (nach der Idee eines Boxplots);
- Natürliche Unterbrechungen (Jenks): die Varianz innerhalb jeder Klasse ist minimal währenddessen die Varianz zwischen Klassen maximal ist;
- Standardabweichung: Klassen werden abhängig von der Standardabweichung der Werte erstellt;
- Schöne Unterbrechungen: das gleiche wie Natürliche Unterbrechungen nur dass die extremen Werte jeder Klasse Ganzzahlen sind.

Das Listenfeld im mittleren Teill des _Stil_ Menüs führt die Klassen zusammen mit ihren Bereichen, Beschriftungen und Symbolen die dargestellt werden auf.

Klicken Sie auf den **Klassifizieren** Knopf um Klassen anhand des ausgewählten Modus zu erstellen. Jede Klasse kann anhand des Deaktivierens des Kontrollkästchens links neben dem Klassennamen ausgeschaltet werden.

Sie können das Symbol, den Wert oder die Beschriftung verändern, klicken Sie einfach auf das Element, das Sie ändern wollen.

Ein Rechtsklick zeigt ein Kontextmenü zu **Kopieren/Einfügen**, **Farbe ändern**, **Transparenz ändern**, **Ausgabeeinheit ändern**, **Symbolbreite ändern**.

**Thematische Karten anhand von Ausdrücken erstellen**

Kategorisierte und Abgestufte thematische Karten können jetzt anhand des Ergebnisses eines Ausdrucks erstellt werden. Im Eigenschaftendialog für Vektorlayer wurde die Attributauswahl um eine ![](../../../../images/mIconExpressionEditorOpen.png) _Set column expression_ Funktion ergänzt. Jetzt brauchen Sie nicht länger das Klassifikationsattribut in eine neue Spalte in Ihrer Attributtabelle schreiben wenn Sie wollen, dass das Klassifikationsattribut eine Zusammenstellung von mehreren Feldern wie z.B. eine Formel irgendeiner Art ist.

**Regelbasierende Darstellung**

Der regelbasierte Renderer wird verwendet um alle Objekte eines Layers anhand eines regelbasierten Symbols dessen Farbe die Zuordnung eines ausgewählten Objektattributs zu einer Klasse wiedergibt, darzustellen. Die Regeln basieren auf SQL-Anweisungen. Mit dem Dialog können Sie anhand von Filtern oder Maßstäben gruppieren und Sie können entscheiden ob Sie die Zeichenreihenfolge benutzen wollen oder nur die erste zutreffende Regel benutzen wollen.

Um eine Regel zu erstellen, aktivieren Sie eine bestehende Zeile durch Doppelklick oder klicken Sie auf "+" und klicken Sie auf die neue Regel. Im Dialog _Regeleigenschaften_ können Sie eine Bezeichnung für die Regel definieren. Drücken Sie die Schaltfläche ![](../../../../images/browsebutton.png), um den Expression String Builder zu öffnen. Klicken Sie in der **Funktionsliste** auf _Felder und Werte_, um alle Attribute der zu durchsuchenden Attributtabelle anzuzeigen. Um dem Feld **Ausdruck** des Feldrechners ein Attribut hinzuzufügen, doppelklicken Sie in der Liste _Felder und Werte_ auf seinen Namen. Im Allgemeinen können Sie die verschiedenen Felder, Werte und Funktionen verwenden, um den Berechnungsausdruck zu konstruieren, oder Sie können ihn einfach in das Feld eingeben. Sie können eine neue Regel erstellen, indem Sie eine bestehende Regel mit der rechten Maustaste kopieren und einfügen. Sie können auch die Regel'ELSE' verwenden, die ausgeführt wird, wenn keine der anderen Regeln auf dieser Ebene übereinstimmt. Die Regeln erscheinen in einer Baumhierarchie in der Kartenlegende. Mit einem Doppelklick auf die Regeln in der Kartenlegende erscheint das Menü Style der Layereigenschaften mit der Regel, die den Hintergrund für das Symbol im Baum darstellt.

![](../../../../images/rulesymbol_ng_line.png)

**Punkt-Verschiebung**

Für Punktlayer gibt es eine Darstellungsart, mit der es möglich ist, sämtliche Punkte eines Layers auch dann darzustellen, wenn sie sich teilweise an derselben Stelle befinden. Die Punkte werden dabei um ein Zentrumssymbol herum auf einem Versatzkreis angeordnet und dargestellt.

![](../../../../images/poi_displacement.png)

**Symbologie exportierten**

Sie haben die Möglichkeit, die Vektorsymbologie aus KADAS in Google \*.kml, \*.dxf und MapInfo \*.tab Dateien zu exportieren. Öffnen Sie einfach das rechte Maustaste-Menü der Ebene und klicken Sie auf _Auswahl speichern unter ‣_, um den Namen der Ausgabedatei und deren Format anzugeben. Verwenden Sie im Dialogfeld das Menü _Symbologie-Export_, um die Symbologie entweder als _Feature-Symbologie ‣_ oder als _Symbollagen-Symbologie ‣_ zu speichern. Wenn Sie Symbolebenen verwendet haben, wird empfohlen, die zweite Einstellung zu verwenden.

**Umgekehrte Polygone**

Der invertierte Polygon-Renderer ermöglicht es dem Benutzer, ein Symbol zu definieren, das außerhalb der Polygone der Schicht ausgefüllt werden soll. Wie bisher können Sie Subrenderer auswählen. Diese Subrenderer sind die gleichen wie bei den Haupt-Renderern.

![](../../../../images/inverted_polygon_symbol.png)

**Schneller Wechsel zwischen den Styles**

Sobald Sie einen der oben genannten Styles erstellt haben, können Sie mit der rechten Maustaste auf die Ebene klicken und _Styles ‣ Add_ wählen, um Ihren Style zu speichern. Jetzt können Sie ganz einfach wieder zwischen den Stilen wechseln, die Sie über das Menü _Stile ‣_ erstellt haben.

**Heatmap**

Mit dem Heatmap-Renderer können Sie dynamische Live-Heatmap für (Mehr-)Punktebenen erstellen. Sie können den Heatmap-Radius in Pixeln, mm oder Karteneinheiten angeben, eine Farbrampe für den Heatmap-Stil auswählen und einen Schieberegler verwenden, um einen Kompromiss zwischen Rendergeschwindigkeit und Qualität auszuwählen. Beim Hinzufügen oder Entfernen eines Features aktualisiert der Heatmap-Renderer automatisch den Heatmap-Stil.

### Farbwahl

Unabhängig vom Stiltyp, der verwendet wird, zeigt der _Farbe wählen_ Dialog e san wenn Sie klicken um eine Farbe zu wählen - entweder Füllungs- oder Rahmenfarbe. Der Dialog besitzt 4 verschiedene Reiter, die es Ihnen ermöglichen Farben anhand des ![](../../../../images/mIconColorBox.png) _Farbverlaufs_, ![](../../../../images/mIconColorWheel.png) _Farbkreises_, der ![](../../../../images/mIconColorSwatches.png) _Farbproben_ oder der ![](../../../../images/mIconColorPicker.png) _Farbwahl_ auszuwählen.

Egal welche Methode Sie verwenden, die ausgewählte Farbe wird immer durch Farbregler für HSV- (Hue, Saturation, Value) und RGB- (Rot, Blau, Grün) Werte beschrieben. Es gibt auch einen _Deckkraft_-Regler um den Transparenzgrad einzustellen. Im linken unteren Teil des Dialogs können Sie einen Vergleich zwischen _Aktuell_ und _Alt_ Farbe, die Sie zur Zeit auswählen und im rechten unteren Teil haben Sie die Option eine Farbe hinzuzufügen, aus der Sie gerade einen Farbknopf gemacht haben.

![](../../../../images/color_picker_ramp.png)

Mit dem ![](../../../../images/mIconColorBox.png) _Farbverlauf_ oder mit dem ![](../../../../images/mIconColorWheel.png) _Farbkreis_ können Sie alle möglichen Farbkombinationen suchen. Dennoch gibt es auch noch andere Möglichkeiten. Indem Sie Farbproben ![](../../../../images/mIconColorSwatches.png) verwenden können Sie aus einer vorausgewählten Liste wählen. Diese ausgewählte Liste wird mit einer drei Methoden _Recent colors_, _Standard colors_ oder _Project colors_ gefüllt.

![](../../../../images/color_picker_recent_colors.png)

Eine andere Option ist die ![](../../../../images/mIconColorPicker.png) _Farbwahl_ zu benutzen, die es Ihnen ermöglicht eine Farbprobe unter Ihrem Mauszeiger in jedem Teil von QGIS oder sogar von einer anderen Anwendung zu nehmen, indem Sie die Leertaste drücken. Bitte beachten Sie, dass die Farbwahl betriebssystemabhängig ist und aktuell nicht von OSX unterstützt wird.

**Schnelle Farbwahl + Farben kopieren/einfügen**

Sie können schnell aus _Recent colors_, _Standard colors_ auswählen oder einfach eine Farbe _Kopieren_ oder _Einfügen_ indem Sie den Drop-down-Pfeil klicken, der einer aktuellen Farbbox folgt.

![](../../../../images/quick_color_picker.png)

### Layerdarstellung

- _Layertransparenz_ ![slider](../../../../images/slider.png): Sie können den unten liegenden Layer in der Kartenansicht mit diesem Werkzeug sichtbar machen. Verwenden Sie den Slider um die Sichtbarkeit Ihres Vektorlayers an Ihre Bedürfnisse anzupassen. Sie können auch eine genaue Definition des Prozentgrades der Sichtbarkeit im Menü neben dem Slider vornehmen.

- Layermischmodi und _Objektmischmodi_: Sie können spezielle Darstellungseffekte mit diesen Werkzeugen, die Sie vorher nur von Grafikprogrammen gekannt haben, erzielen. Die Pixel der oben auf liegenden und darunter liegenden Layer werden anhand der unten beschriebenen Einstellungen gemischt.
    - Normal: Dies ist der Standardmischmodus, der den Alphakanal des oben liegenden Pixels mit dem darunter liegenden Pixel vermischt. Die Farben werden nicht vermischt.
    - Heller: Dies wählt das Maximum jeder Komponente der Vordergrund- und Hintergrundpixel. Seien Sie sich bewusst dass die Ergebnisse zackig und hart aussehen können.
    - Bildschirm: Helle Pixel der Quelle werden über die des Ziels gezeichnet wohingegen dunkle Pixel nicht verwendet werden. Dieser Modus ist am nützlichsten für das Mischen der Textur eines Layers mit einem anderen Layer (z.B. kann man eine Schummerung dazu verwenden einen anderen Layer mit einer Textur zu versehen).
    - Abwedeln: das Abwedeln erhellt und sättigt unten liegende Pixel auf Basis der Helligkeit des oben liegenden Pixels. Demzufolge erhöhen hellere oben liegende Pixel die Sättigung und Helligkeit des unten liegenden Pixels. Dies funktioniert am Besten wenn die oben liegenden Pixel nicht zu hell sind; andernfalls ist der Effekt zu extrem.
    - Addition: Dieser Mischmodus fügt einfach die Pixelwerte eines Layers denen eines anderen Layers hinzu. Im Falle von Werten größer 1 (im Fall von RGB) wird weiß dargestellt. Dieser Modus ist dafür geeignet Objekte hervorzuheben.
    - Dunkler: Dies erstellt ein Ergebnispixel das die kleinste Komponente der Vordergrund und Hintergrundpixel erhält. Wie das Aufhellen neigen die Ergebnisse dazu zackig und hart zu sein.
    - Multiplizieren: Hier werden die Nummern für jedes Pixel des oben liegenden Layers mit den entsprechenden Pixeln des unteren Layers multipliziert. Das Ergebnis sind dunklere Bilder.
    - Einbrennen: Dunklere Farben im oben liegenden Layer bewirken ein Verdunkeln des unten liegenden Layers. Einbrennen kann dazu benutzt werden um unten liegende Layer zu optimieren und zu colorieren.
    - Überlagern: Dieser Modus kombiniert die Multiplizieren und Bilschirm Mischmodi. Im Ergebnispixel werden helle Bereiche heller und dunkle Bereiche dunkler.
    - Weiches Licht: Dieses ist dem Überlagern sehr ähnlich nur dass anstelle Multiplizieren/Bildschirm Einbrennen/Abwedeln verwendet wird. Hier soll das Leuchten eines weichen Lichtes auf ein Bild nachgeahmt werden.
    - Hartes Licht: Auch Hartes Licht ist dem Überlagerungsmodus sehr ähnlich. Hier soll die Projektion eines sehr intensiven Lichts auf ein Bild nachgeahmt werden.
    - Unterschied: Unterschied subtrahiert das oben liegende Pixel von dem unten liegenden Pixel oder andersherum um immer einen positiven Wert zu bekommen. Das Mischen mit Schwarz produziert keinen Unterschied, da die Differenz mit allen Farben Null ist.
    - Abziehen: Dieser Mischmodus zieht einfach die Pixelwerte eines Layers von dem anderen ab. Im Fall von negativen Werten wird Schwarz dargestellt.

## Menü Beschriftungen

Die ![](../../../../images/mActionLabeling.png) _Beschriftungen_ Kernanwendung stellt intelligentes Beschriften für Punkt-, Linien- und Polygonlayer zur Verfügung und erfordert nur wenige Parameter. Diese neue Anwendung untersützt auch spontan transformierte Layer. Die Kernfunktionen der Anwendung wurden überarbeitet. In KADAS gibt es eine Anzahl von anderen Funktionen die das Beschriften verbessern. Die folgenden Menüs wurden erstellt um die Vektorlayer zu beschriften:

- Text
- Formatierung
- Puffer
- Hintergrund
- Schatten
- Platzierung
- Darstellung

Lassen Sie uns sehen wie die Menüs für verschiedene Vektorlayer benutzt werden können.

**Punktlayer beschriften**

Starten Sie KADAS und laden Sie einen Punktlayer. Aktivieren Sie den Layer in der Legende und klicken Sie auf das ![](../../../../images/mActionLabeling.png) _Layerbeschriftungseinstellungen_ Icon in der KADAS Werkzeugleiste.

Der erste Schritt besteht darin das ![](../../../../images/checkbox.png) _Layer beschriften mit_ Kontrollkästchen zu aktivieren und eine Attributspalte für das Beschriften auszuwählen. Klicken Sie ![](../../../../images/mIconExpressionEditorOpen.png) wenn Sie ausdrucksbasierte Beschriftungen definieren wollen.

Die folgenden Schritte beschreiben ein einfaches Beschriften ohne die Verwendung der _Datendefinierten Übersteuerung_ Funktionen, die neben den Drop-Down Menüs untergebracht sind.

Sie können den Textstil im _Text_ Menü definieren. Verwenden Sie die _Schriftart Groß-/Kleinschreibung_ Option um die Textdarstellung zu beeinflussen. Sie haben die Möglichkeit den Text mit ‘Nur Großbuchstaben’, ‘Nur Kleinbuchstaben’ oder ‘Erstes Zeichen groß’ darzustellen. Verwenden Sie Mischmodi um Effekte die Sie von Grafikprogrammen kennen zu erstellen.

Im Menü _Formatierung_ können Sie einen Buchstaben für einen Zeilenumbruch in den Beschriftungen mit der ‘Bei Zeichen umbrechen’ Funktion definieren. Verwenden Sie die ![](../../../../images/checkbox.png) _Zahlenformatierung_ Option um die Nummern in einer Attributtabelle zu formatieren. Hier können Dezimalstellen eingefügt werden. Wenn Sie diese Option aktivieren werden erst einmal drei Dezimalstellen als Standard eingestellt.

Um einen Puffer zu erstellen aktivieren Sie einfach die ![](../../../../images/checkbox.png) _Textpuffer zeichnen_ im Menü _Puffer_. Die Pufferfarbe ist variabel. Auch hier können Sie Mischmodi verwenden.

Wenn das ![](../../../../images/checkbox.png) _Pufferfüllung einfärben_ Kontrollkästchen aktivieren, wird es mit teiltransparentem Text interagieren und gemischte Farbtransparenzergebnisse liefern. Das Abschalten der Pufferfüllung behebt das Problem (es sein denn der innere Aspekt der Pufferausdehnung überschneidet sich mit der Textfüllung) und ermöglicht das Erstellen von umrandetem Text.

Im Menü _Hintergrund_ können Sie mit _Größe X_ und _Größe Y_ die Form Ihres Hintergrunds definieren. Verwenden Sie _Größenart_ um einen zusätzlichen ‘Puffer’ in Ihren Hintergrund einzufügen. Der Hintergrund besteht dann aus dem Puffer mit dem Hintergrund in _Größe X_ und _Größe Y_. Sie können eine _Drehung_ festlegen wobei Sie zwischen ‘Mit Beschriftung abgleichen’, ‘Beschriftungsversatz’ und ‘Fest’ wählen können. Indem Sie ‘Beschriftungsversatz’ und ‘Fest’ verwenden, können Sie den Hintergrund rotieren. Definieren Sie _X-,Y-Versatz_ mit X und Y Werten und der Hintergrund wird versetzt. Wenn Sie _X-, Y-Radius_ verwenden erhält der Hintergrund runde Ecken. Auch hier ist es möglich den Hintergrund mit den darunterliegenden Layern in der Kartenansicht anhand von _Mischmodi_ zu mischen.

Verwenden Sie das _Schatten_ Menü für einen benutzerdefinierten _Schattenwurf_. Das Zeichnen des Hintergrunds ist sehr variabel. Wählen Sie zwischen ‘Niedrigste Beschriftungskomponente’, ‘Text’, ‘Puffer’ und ‘Hintergrund’. Der _Versatz_-Winkel hängt von der Orientierung der Beschriftung ab. Wenn Sie das Kontrollkästchen ![](../../../../images/checkbox.png) _Globalen Schatten verwenden_ dann ist der Ausgangspunkt des Winkels immer nach Norden orientiert und hängt nicht von der Orientierung der Beschriftung ab. Sie können das Erscheinungsbild des Schattens mit _Radius verschmieren_ beeinflussen. Je höher die Nummer desto weicher sind die Schatten. Das Erscheinungsbild des Schattenwurfs kann auch durch das Benutzen eines Mischmodus verändert werden.

Wählen Sie das Menü _Platzierung_ für die Etikettenplatzierung und die Priorität der Beschriftung. Mit der Einstellung ![radiobuttonon](../../../../images/radiobuttonon.png) _Offset from point_ haben Sie nun die Möglichkeit, _Quadranten_ zum Platzieren Ihres Labels zu verwenden. Zusätzlich können Sie den Winkel der Etikettenplatzierung mit der Einstellung _Rotation_ ändern. Somit ist eine Platzierung in einem bestimmten Quadranten mit einer bestimmten Rotation möglich. Im Abschnitt _Priorität_ können Sie festlegen, mit welcher Priorität die Labels gerendert werden. Es interagiert mit den Beschriftungen der anderen Vektorebenen auf der Karten-Leinwand. Wenn sich Etiketten von verschiedenen Ebenen an der gleichen Stelle befinden, wird das Etikett mit der höheren Priorität angezeigt und das andere weggelassen.

Im Menü _Darstellung_ können sie Beschriftungs- und Objektoptionen definieren. Unter _Beschriftungsoptionen_ können Sie jetzt die maßstabsabhängige Sichtbarkeitseinstellung vornehmen. Sie können KADAS davon abhalten nur ausgewählte Beschriftungen darzustellen indem Sie das ![](../../../../images/checkbox.png) _Alle Beschriftungen auf diesem Layer anzeigen (einschließlich kollidierender)_ Kontrollkästchen benutzen. Unter _Objektoptionen_ können Sie definieren ob jeder Teil eines Mulitpart Features beschriftet werden soll. Es ist möglich zu definieren ob die Nummer von Objekten die beschriftet werden soll begrenzt ist und ![](../../../../images/checkbox.png) _Möglichst keine Objekte durch Beschriftungen verdecken_.

![](../../../../images/label_points.png)

**Linienlayer beschriften**

Der erste Schritt ist es das ![](../../../../images/checkbox.png) _Layer beschriften mit_ Kontrollkästchen im _Beschriftungen_ Menü zu aktivieren und eine Attributspalte für das Beschriften auszuwählen. Klicken Sie ![](../../../../images/mIconExpressionEditorOpen.png) wenn Sie ausdrucksbasierte Beschriftungen definieren wollen.

Danach können Sie den Textstil im _Text_ Menü definieren. Hier können Sie die gleichen Einstellungen wie für Punktlayer verwenden.

Auch im Menü _Formatierung_ sind die gleichen Einstellungen wie für Punktlayer möglich.

Das Menü _Puffer_ hat die gleichen Funktionen wie in Abschnitt [Punktebenen beschriften](#labeling-point-layers).

Das Menü _Hintergrund_ hat die gleichen Einträge wie in Abschnitt [Punktebenen beschriften](#labeling-point-layers) beschrieben.

Auch das _Schatten_ Menü hat die gleichen Einträge wie in section [Punktebenen beschriften](#labeling-point-layers) beschrieben.

Im Menü _Platzierung_ finden Sie spezielle Einstellungen für Linienlayer. Die Beschriftung kann ![radiobuttonon](../../../../images/radiobuttonon.png) _Parallel_, ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Gebogen_ oder ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Horizontal_ platziert werden. Mit der ![radiobuttonon](../../../../images/radiobuttonon.png) _Parallel_ und radiobuttonoff| _Gebogen_ Option können Sie die Position ![](../../../../images/checkbox.png) _Über Linie_, ![](../../../../images/checkbox.png) _Auf der Linie_ und ![](../../../../images/checkbox.png) _Unter Linie_ definieren. Es ist möglich mehrere Optionen auf einmal auszuwählen. In diesem Fall wird KADAS nach der optimalen Position der Beschriftung suchen. Zusätzlich können Sie _Größter Winkel zwischen Zeichen auf Kurven_ definieren wenn Sie die ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Gebogen_ Option auswählen.

Sie können eine Minimaldistanz zum Wiederholen von Beschriftungen einstellen. Der Abstand kann in mm oder Karteneinheiten angegeben werden.

Some Placement setup will display more options, for example, _Curved_ and _Parallel_ Placements will allow the user to set up the position of the label (above, below or on the line), _distance_ from the line and for _Curved_, the user can also setup inside/outside max angle between curved label. As for point vector layers you have the possibility to define a _Priority_ for the labels.

Das _Darstellung_ Menü hat fast die gleichen Einträge wie das bei Punktlayern. In den _Objektoptionen_ können Sie jetzt _Objekte nicht beschriften, wenn kürzer als_.

![](../../../../images/label_line.png)

**Polygonlayer beschriften**

Der erste Schritt ist es das ![](../../../../images/checkbox.png) _Layer beschriften mit_ Kontrollkästchen zu aktivieren und eine Attributspalte für das Beschriften auszuwählen. Klicken Sie ![](../../../../images/mIconExpressionEditorOpen.png) wenn Sie ausdrucksbasierte Beschriftungen definieren wollen.

Definieren Sie den Textstil im _Text_ Menü. Die Einträge sind die gleichen wie die für Punkt- und Linienlayer.

Das Menü _Formatierung_ ermöglicht es Ihnen mehrzeilige Zeilen zu formatieren wie schon bei Punkt- und Linienlayern.

Wie bei Punkt- und Linienlayern können Sie einen Textpuffer im _Puffer_ Menü erstellen.

Verwenden Sie das Menü _Hintergrund_ um einen komplexen benutzerdefinierten Hintergrund für den Polygonlayer zu erstellen. Sie können das Menü wie bei Punkt- und Linienlayern benutzen.

Die Einträge in dem Menü _Schatten_ sind die gleichen wie für Punkt- und Linienlayer.

Im Menü _Platzierung_ finden Sie spezielle Einstellungen für Polygonlayer. ![radiobuttonon](../../../../images/radiobuttonon.png) _Abstand vom Zentrum_, ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Horizontal (langsam)_, _Um Zentrum_, ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Frei (langsam)_ und ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Nach Umfang_ sind möglich.

In den ![radiobuttonon](../../../../images/radiobuttonon.png) _Abstand vom Zentrum_ Einstellungen können Sie festlegen ob der Zentroid sich auf ![radiobuttonon](../../../../images/radiobuttonon.png) _sichtbarem Polygon_ oder ![radiobuttonoff](../../../../images/radiobuttonoff.png) _ganzem Polygon_ bezieht. Das heißt dass entweder der Zentroid für das Polygon das Sie auf der Karte sehen verwendet wird oder der Zentroid für das ganze Polygon bestimmt wird egal ob Sie das ganze Objekt auf der Karte sehen. Sie können hier Ihre Beschriftung anhand von Quadranten platzieren sowie Versatz und Drehung definieren. Die ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Um Zentrum_ Einstellung macht es möglich die Beschriftung um einen Zentroiden herum mit einer bestimmten Distanz zu platzieren. Auch hier können Sie ![radiobuttonon](../../../../images/radiobuttonon.png) _sichtbarem Polygon_ oder ![radiobuttonoff](../../../../images/radiobuttonoff.png) _ganzem Polygon_ für den Zentroiden definieren. Mit den ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Nach Umfang_ Einstellungen können Sie eine Position und einen Abstand für die Beschriftung definieren. Für die Position sind ![](../../../../images/checkbox.png) _Über Linie_, ![](../../../../images/checkbox.png) _Auf Linie_, _Unter Linie_ und ![](../../../../images/checkbox.png) _Linienrichtungsabhängige Position_ möglich.

Bezogen auf die Wahl der Etikettenplatzierung werden mehrere Optionen angezeigt. Wie bei der Punktplatzierung können Sie den Abstand für die Polygonumrandung wählen, wiederholen Sie die Beschriftung um den Polygonumfang herum.

Wie bei Punkt- und Linienvektorlagen haben Sie die Möglichkeit, eine _Priorität_ für die Polygonvektorlage zu definieren.

Die Einträge in das Menü _Darstellung_ sind die gleichen wie für Linienlayer. Sie können auch _Objekte nicht beschriften wenn kürzer als_ in den _Objektoptionen_.

![](../../../../images/label_area.png)

**Ausdrucksbasierte Beschriftungen definieren**

Mit KADAS können Sie Ausdrücke benutzen um Objekte zu beschriften. Klicken Sie einfach das ![](../../../../images/mIconExpressionEditorOpen.png) Icon im Menü ![](../../../../images/mActionLabeling.png) _Beschriftungen_ des Eigenschaften Dialogs. Im folgenden Bild sehen Sie einen Beispielausdruck um die Alaska Regionen mit Namen- und Flächengröße, abhängig vom Feld ‘NAME\_2’, etwas beschreibenden Text und die Funktion ‘$area()’ in Kombination mit ‘format\_number()’ damit sie besser aussieht.

![](../../../../images/label_expression.png)

Mit dem ausdrucksbasierten Beschriften können Sie auf einfache Art und Weise arbeiten. Alles worum Sie sich kümmern müssen ist dass Sie alle Elemente (Zeichenketten, Felder und Funktionen) mit einem String-Verkettungszeichen ‘||’ kombinieren und dass Felder in “doppelten Anführungszeichen” und Zeichenketten in ‘einfachen Anführungszeichen’ geschrieben werden. Lassen Sie uns einen Blick auf einige Beispiele werfen:

```
 # label based on two fields 'name' and 'place' with a comma as separater
 "name" || ', ' || "place"

 -> John Smith, Paris

 # label based on two fields 'name' and 'place' separated by comma
 'My name is ' || "name" || 'and I live in ' || "place"

 -> My name is John Smith and I live in Paris

 # label based on two fields 'name' and 'place' with a descriptive text
 # and a line break (\n)
 'My name is ' || "name" || '\nI live in ' || "place"

 -> My name is John Smith
    I live in Paris

 # create a multi-line label based on a field and the $area function
 # to show the place name and its area size based on unit meter.
 'The area of ' || "place" || 'has a size of ' || $area || 'm²'

 -> The area of Paris has a size of 105000000 m²

 # create a CASE ELSE condition. If the population value in field
 # population is <= 50000 it is a town, otherwise a city.
 'This place is a ' || CASE WHEN "population <= 50000" THEN 'town' ELSE 'city' END

-> This place is a town
```

Wie Sie im Expression Builder sehen können, stehen Ihnen Hunderte von Funktionen zur Verfügung, um einfache und sehr komplexe Ausdrücke zu erstellen, mit denen Sie Ihre Daten in QGIS beschriften können.

**Datendefinierte Übersteuerung für das Beschriften**

Mit der datendefinierten Übersteuerung werden die Einstellungen für das Beschriften von Einträgen in der Attributtabelle überschrieben. Sie können die Funktion mit dem Rechte-Maus-Knopf aktivieren und deaktivieren. Fahren Sie über das Symbol und Sie sehen die Information über die datendefinierte Übersteuerung einschließlich des aktuellen Definitionsfeldes. Wir beschreiben jetzt ein Beispiel indem wir die datendefinierte Übersteuerungsfunktion für die ![](../../../../images/mActionMoveLabel.png)_Beschriftung verschieben_ Funktion verwenden.

1. Importieren Sie `lakes.shp` aus dem KADAS Beispieldatensatz.
2. Doppelklicken Sie den Layer um die Layereigenschaften zu öffnen. Klicken Sie auf _Beschriftungen_ und _Platzierung_. Wählen Sie ![radiobuttonon](../../../../images/radiobuttonon.png) _Abstand vom Punkt_.
3. Suchen Sie nach den _Datendefiniert_ Einträgen. Klicken Sie das ![](../../../../images/mIconDataDefine.png) Icon um den Feldtyp für die _Koordinate_ zu definieren. Wählen Sie ‘xlabel’ für X und ‘ylabel’ für Y aus. Die Icons sind jetzt in gelb hervorgehoben.
4. Zoomen Sie auf einen See.
5. Gehen Sie zur Beschriftung Werkzeugleiste und klicken Sie das ![](../../../../images/mActionMoveLabel.png) Icon. Jetzt können Sie die Beschriftung manuell in eine andere Position verschieben. Die neue Position der Beschriftung ist in den ‘xlabel’ und ‘ylabel’ Spalten der Attributtabelle gespeichert.

![](../../../../images/label_data_defined.png)

![](../../../../images/move_label.png)

## Menü Felder

![](../../../../images/attributes.png) Innerhalb des _Felder_ Menüs können die Feldattribute des ausgewählten Datensatzes manipuliert werden. Die Knöpfe ![](../../../../images/mActionNewAttribute.png) _Neue Spalte_ und ![](../../../../images/mActionDeleteAttribute.png) _Spalte löschen_ können benutzt werden wenn der Datensatz im ![](../../../../images/mActionToggleEditing.png) _Bearbeitungsstatus umschalten_ Modus ist.

**Bearbeitungselement**

![](../../../../images/editwidgetsdialog.png)

Innerhalb des Menüs _Felder_ finden Sie auch eine **Bearbeitungselement** Spalte. Diese Spalte kann dazu benutzt werden Werte oder eine Spanne von Werten zu definieren die zu der bestimmten Attributtabellenspalte hinzugefügt werden dürfen. Wenn Sie auf den **[Eingabezeile]** Knopf klicken öffnet sich ein Dialog indem Sie verschiedene Elemente definieren können. Diese Elemente sind:

- **Kontrollkästchen**: Gibt ein Kontrollkästchen wieder und Sie können definieren welches Attribut der Spalte hinzugefügt wird wenn das Kontrollkästchen aktiviert ist oder nicht.
- **Klassifikation**: Auswahlliste mit den Attributwerten, die im Menü _Stil_ als Legendentyp Eindeutiger Wert für die Klassifikation benutzt werden.
- [\*\*](#id1)Farbe\*: Stellt einen Farbknopf dar, der es dem Anwender ermöglicht eine Farbe von einem Farbdialogfenster auszuwählen.
- **Datum/Zeit**: Zeigt ein Zeilenfeld an, das ein Kalender-Widget öffnen kann, um ein Datum, eine Uhrzeit oder beides einzugeben. Die Spaltenart muss Text sein. Sie können ein benutzerdefiniertes Format auswählen, einen Kalender öffnen usw.
- **Aufzählung**: Öffnet eine Kombobox mit Werten die innerhalb eines Spaltentyps benutzt werden können. Dieses wird aktuell nur vom PostgreSQL Provider untersützt.
- **Dateiname**: Vereinfacht die Dateiauswahl durch einen Dateiauswahldialog.
- **Versteckt**: Ein verstecktes Attribut ist unsichtbar. Der Anwender kann den Inhalt nicht sehen.
- **Foto**: Feld enthält einen Dateinamen für ein Bild. Die Breite und Höhe des Feldes kann definiert werden.
- **Bereich**: Erlaubt Ihnen numerische Werte eines bestimmten Wertebereichs festzulegen. Das Bearbeitungselement kann entweder ein Schieber oder ein Drehfeld sein.
- **Beziehungsreferenz**: Mit diesem Element können Sie das Objektformular des referenzierten Layers in das Objektformular des aktuellen Layers einbetten. Siehe _vector\_relations_.
- **Texteditor** (voreingestellt): Dies öffnet einen Textbearbeitungsfeld mit dem Sie einfachen Text oder mehrere Zeilen verwenden können. Wenn Sie mehrzeilig gewählt haben können Sie auch HTML wählen.
- **Eindeutige Werte**: Sie können einen der Werte die bereits in der Attributtabelle verwendet werden aussuchen. Wenn ‘Änderbar’ aktiviert ist, wird eine Eingabezeile mit Autovervollständigungsunterstützung gezeigt, andernfalls wird eine Kombobox verwendet.
- **UUID Generator**: Erstellt ein schreibgeschütztes UUID (Universally Unique Identifiers)-Feld wenn es leer ist.
- **Wertabbildung**: Eine Kombobox mit vordefinierten Elementen. Der Wert ist im Attribut gespeichert, die Beschreibung wird in der Kombobox gezeigt. Sie können Werte manuell definieren oder sie aus einem Layer oder einer CSV-Datei laden.
- **Wertbeziehung**: Bietet Werte aus einer verknüpften Tabelle in einem Auswahlmenü an. Sie können Layer, Schlüsselspalte und Wertspalte auswählen.
- **Webansicht**: Feld enthält eine URL. Die Breite und Höhe des Feldes ist variabel.

KADAS hat eine erweiterte "versteckte" Option, um Ihr eigenes Feld-Widget mit Python zu definieren und es zu dieser beeindruckenden Liste von Widgets hinzuzufügen. Es ist knifflig, aber es ist sehr gut erklärt in folgendem exzellenten Blog, der erklärt, wie man ein Echtzeit-Validierungs-Widget erstellt, das wie die beschriebenen Widgets verwendet werden kann. Siehe <http://blog.vitu.ch/10142013-1847/write-your-own-qgis-form-elements>

Mit dem Layout des **Attributlayout-Editors** können Sie nun integrierte Formulare definieren. Dies ist nützlich für Dateneingabeaufträge oder um Objekte mit der Option Auto Open Form zu identifizieren, wenn Sie Objekte mit vielen Attributen haben. Sie können einen Editor mit mehreren Registerkarten und benannten Gruppen erstellen, um die Attributfelder darzustellen.

Wählen Sie 'Drag and Drop Designer' und eine Attributspalte. Verwenden Sie das Symbol ![](../../../../images/mActionSignPlus.png), um eine Kategorie zum Einfügen einer Registerkarte oder einer benannten Gruppe zu erstellen. Beim Erstellen einer neuen Kategorie fügt KADAS eine neue Registerkarte oder benannte Gruppe für die Kategorie in das integrierte Formular ein. Im nächsten Schritt werden die relevanten Felder einer ausgewählten Kategorie mit dem Symbol ![](../../../../images/mActionArrowRight.png) zugeordnet. Sie können weitere Kategorien anlegen und die gleichen Felder wieder verwenden.

Weitere Optionen im Dialog sind _Autogenerieren_ und _UI-Datei bereitstellen_.

- _Autogenerieren_ erstellt einfach Editoren für alle Felder und tabelliert sie.
- Mit der Option _UI-Datei bereitstellen_ können Sie komplexe Dialoge verwenden, die mit dem Qt-Designer erstellt wurden. Die Verwendung einer UI-Datei bietet viel Freiheit bei der Erstellung eines Dialogs. Detaillierte Informationen finden Sie unter <http://nathanw.net/2011/09/05/qgis-tips-custom-feature-forms-with-python-logic/>.

KADAS Dialoge können eine Python-Funktion beinhalten, die aufgerufen wird wenn der Dialog geöffnet wird. Verwenden Sie diese Funktion um Ihren Dialogen eine zusätzliche Logik hinzuzufügen. Ein Beispiel ist (im Modul MyForms.py):

```
def open(dialog,layer,feature):
geom = feature.geometry()
control = dialog.findChild(QWidged,"My line edit")
```

Die Referenz in der Python Init Function sieht in etwa so aus: MyForms.open

MyForms.py muss in den PYTHONPATH wie in .qgis2/python oder innerhalb des Projektordners eingetragen werden.

![](../../../../images/attribute_editor_layout.png)

![](../../../../images/resulting_feature_form.png)

## Menü Allgemein

![](../../../../images/general.png) Verwenden Sie dieses Menü um allgemeine Einstellungen für den Vektorlayer zu machen. Es stehen mehrere Optionen zur Verfügung:

Layerinformation

- Ändern Sie den Anzeigenamen des Layers in _angezeigt als_
- Definieren Sie die _Layerquelle_ des Vektorlayers
- Definieren Sie _Datenquellenkodierung_ um providerspezifische Optionen zu definieren und um in der Lage zu sein die Datei zu lesen.

Koordinatenbezugssystem

- Verwenden Sie _Festlegen_ für das Koordinatensystem. Hier können Sie die Projektion eines bestimmten Vektorlayers ansehen oder verändern.
- Erstellen Sie einen _Räumlichen Index_ (nur OGR-unterstützte Formate)
- Die _Ausmaße aktualisieren_ für einen Layer
- Sehen Sie sich die Projektion eins spezifischen Vektorlayers an oder ändern Sie diese indem Sie auf _Festlegen ..._ klicken.

![](../../../../images/checkbox.png) _Maßstabsabhängige Sichtbarkeit_

- Sie können den _Maximum (inklusive)_ und _Minimum (exklusiv)_ Maßstab einstellen. Der Maßstab kann auch mit den **[Aktuell]** Knöpfen eingestellt werden.

Objektuntermenge

- Mit dem **[Abfrageerstellung]** Knopf können Sie eine Untermenge von Objekten im Layer erstellen, die dann dargestellt wird (lesen Sie auch die Informationen in _vector\_query\_builder_).

![](../../../../images/vector_general_menu.png)

## Menü Darstellung

KADAS unterstützt die On-the-fly-Feature-Verallgemeinerung. Dies kann die Renderingzeiten beim Zeichnen vieler komplexer Features in kleinem Maßstab verbessern. Diese Funktion kann in den Layereinstellungen über die Option ![](../../../../images/checkbox.png) _Geometrie vereinfachen_ ein- oder ausgeschaltet werden. Es gibt auch eine neue globale Einstellung, die standardmäßig eine Generalisierung für neu hinzugefügte Layer ermöglicht (siehe Abschnitt [_Optionen_](../introduction/qgis_configuration.html#gui-options)). **Hinweis**: Die Generalisierung von Features kann in einigen Fällen Artefakte in Ihre gerenderte Ausgabe einbringen. Dazu gehören beispielsweise Sliver zwischen Polygonen und ungenaues Rendern bei der Verwendung von Offset-basierten Symbolebenen.

## Menü Darstellung

![](../../../../images/mActionMapTips.png) Dieses Menü ist speziell für Kartenhinweise erstellt worden. Es enthält eine neue Funktion: Kartentippanzeigetext in HTML. Während Sie weiterhin ein ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Feld_ das angezeigt werden soll wenn Sie über ein Objekt auf der Karte gehen wählen können ist es jetzt möglich HTML Code der eine komplexe Anzeige erstellt wenn man darüber geht einzugeben. Um Kartenhinweise zu aktivieren wählen Sie die Menüoption _Ansicht ‣ Kartenhinweise_. Figure Display 1 zeigt ein Beispiel für HTML Code.

![](../../../../images/display_html.png)

![](../../../../images/map_tip.png)

## Menü Aktionen

![](../../../../images/action.png) KADAS bietet die Möglichkeit, Aktionen auf Basis von Attributen einer Ebene durchzuführen. Dies kann für eine Vielzahl von Aktionen genutzt werden, z.B. um ein Programm mit Abfragen aus der Attributdatenbank zu füttern oder um Parameter an ein Web-Reporting-Tool weiterzugeben.

**Figure Actions 1:**

![](../../../../images/action_dialog.png)
Überblick über den Dialog Aktionen mit einigen Beispielaktionen

Aktionen auf Basis von Attributen sind sinnvoll wenn sie häufig eine externe Anwendung starten oder eine Internetseite auf Basis von einem oder mehreren Werten in Ihrem Vektorlayer visualisieren wollen. Sie sind in 6 Typen aufgeteilt und können wie folgt verwendet werden:

- Allgemein, Mac, Windows und Unix Aktionen starten einen externen Prozess.
- Python Aktionen führen einen Python-Ausdruck aus.
- Allgemeine und Pythonaktionen sind überall sichtbar.
- Mac, Windows und Unix Aktionen sind nur sichtbar auf der entsprechenden Plattform (z.B. können Sie drei ‘Bearbeiten’ Aktionen definieren um einen Editor zu öffnen und die Benutzer können nur die eine ‘Bearbeiten’ Aktion für Ihr Betriebssystem sehen und ausführen um den Editor zu starten).

Es gibt verschiedene Beispiele in diesem Dialog. Sie können Ihn laden indem Sie auf **[Voreingestellte Aktion]** klicken. In einem Beispiel wird eine Suche auf Basis eines Attributwertes durchgeführt. Dieses Konzept wird in der folgenden Diskussion verwendet.

**Aktionen definieren**

Attributaktionen werden im Vektor _Layer Eigenschaften_ Dialog definiert. Um eine Aktion zu definieren öffnen Sie den Vektor _Layereigenschaften_ Dialog und klicken Sie auf das Menü _Aktionen_. Gehen Sie zu den _Aktionseigenschaften_. Wählen Sie ‘Allgemein’ als Typ und vergeben Sie einen beschreibenden Namen für die Aktion. Die Aktion selbst muss den Namen der Anwendung, die ausgeführt wird wenn die Aktion zum Einsatz kommt, enhalten. Sie können einen oder mehrere Attributfeldwerte als Argumente der Applikation hinzufügen. Wenn die Aktion aufgerufen wird wird jeder Satz von Buchstaben, der mit einem `%` beginnt und auf den der Name eines Feldes folgt, durch den Wert des entsprechenden Feldes ersetzt. Die speziellen Buchstaben %% werden durch den Wert des Feldes, das durch das Objekte abfragen Werkzeug oder die Attributtabelle ausgewählt wurde, ersetzt. Anführungszeichen werden ignoriert wenn Ihnen ein Backslash vorausgeht.

Wenn Sie Feldnamen haben, die Teilzeichenketten anderer Feldnamen sind (z.B. `col1` und `col10`), sollten Sie angeben, dass Sie den Feldnamen (und das %-Zeichen) mit eckigen Klammern umgeben (z.B. `[%col10]`). Dadurch wird verhindert, dass der Feldname `%col10` mit dem Feldnamen `%col1` mit einer `0` am Ende verwechselt wird. Die Klammern werden von KADAS entfernt, wenn sie durch den Wert des Feldes ersetzt werden. Wenn Sie möchten, dass das substituierte Feld von eckigen Klammern umgeben ist, verwenden Sie einen zweiten Satz wie diesen: `[[%col10]]`.

Wenn Sie das _Objekte abfragen_ Werkzeug verwenden, können Sie den _Identifikationsergebnis_ Dialog öffnen. Es enthält eine _(Abgeleitet)_ Item das layertyprelevante Informationen enthält. Die Werte in diesem Element können auf ähnliche Weise den anderen Feldern zugeordnet werden indem dem Abgeleitet Feldnamen ein `(Derived)` vorangeht. Zum Beispiel hat ein Punktlayer ein `X` und `Y` Feld und die Werte dieser Felder können in der Aktion mit `%(Derived).X` und `%(Derived).Y` verwendet werden. Die Abgeleitet Attribute sind nur in der _Objekte abfragen_ Dialog Box erhältlich, jedoch nicht in der _Attributtabelle_ Dialogbox.

Nachfolgend werden zwei Beispielaktionen gezeigt:

- `konqueror http://www.google.com/search?q=%nam`
- `konqueror http://www.google.com/search?q=%%`

Im ersten Beispiel wird der Webbrowser Konqueror eingebunden und öffnet eine URL. Die URL führt eine Googlesuche für den Wert des `nam` Feldes unseres Vektorlayers durch. Beachten Sie das die Anwendung oder das Skript das von der Anwendung aufgerufen wird im Pfad sein muss oder den vollen Pfad vermitteln muss. Um sicher zu sein könnten wir das erste Beispiel wie folgt umschreiben: `/opt/kde3/bin/konqueror http://www.google.com/search?q=%nam`. Dies wird versichern dass die Konqueroranwendung ausgeführt wird wenn die Aktion aufgerufen wird.

Das zweite Beispiel verwendet die %%-Notation, die sich für ihren Wert nicht auf ein bestimmtes Feld verlässt. Wenn die Aktion aufgerufen wird, wird %% durch den Wert des ausgewählten Feldes in der Identifizierungsergebnis- oder Attributtabelle ersetzt.

**Aktionen anwenden**

Aktionen können entweder über den _Objekte abfragen_ Dialog, den _Attributtabelle_ Dialog oder über _Objektaktion durchführen_ aufgerufen werden (erinnern Sie sich daran dass diese Dialoge durch Klicken von ![](../../../../images/mActionIdentify.png) _Objekte abfragen_ oder ![](../../../../images/mActionOpenTable.png) _Attributtabelle öffnen_ oder ![](../../../../images/mAction.png) _Objektaktion ausführen_ geöffnet werden können). Um eine Aktion aufzurufen, klicken Sie mit der rechten Maustaste auf einen Eintrag im Popup Menü und wählen die gewünschte Aktion aus der Liste aus. Aktionen sind anhand des Namens den Sie beim Definieren der Aktion vergeben haben im Popup Menü aufgeführt. Klicken Sie auf die Aktion die sie aufrufen wollen.

Wenn Sie eine Aktion mit `%%` Notation verwenden, machen Sie einen Rechtsklick auf den Feldwert im _Objekte abfragen_ Dialog oder im _Attributtabelle_ Dialog den Sie der Anwendung oder dem Skript übergeben wollen.

In einem weiteren Beispiel soll gezeigt werden, wie Attributwerte eines Vektorlayers abgefragt und in eine Textdatei mit Hilfe der Bash und des `echo` Kommandos geschrieben werden (funktioniert also nur unter und evtl. ![](../../../../images/osx.png) ). Der Abfragelayer enthält die Felder Art `taxon_name`, Latitude `lat` und Longitude `long`. Wir möchten jetzt eine räumliche Selektion von Örtlichkeiten machen und diese Feldwerte in eine Textdatei für den ausgewählten Datensatz (in der KADAS Kartenansicht in gelb gezeigt) exportieren. Hier ist die Aktion, um dies zu erreichen:

```
bash -c "echo \"%taxon_name %lat %long\" >> /tmp/species_localities.txt"
```

Nachdem ein paar Orte auf dem Bildschirm ausgewählt wurden (diese erscheinen gelb hinterlegt), starten wir die Aktion mit der rechten Maustaste über den Dialog _Abfrageergebnisse_ und können danach in der Textdatei die Ergebnisse ansehen:

```
Acacia mearnsii -34.0800000000 150.0800000000
Acacia mearnsii -34.9000000000 150.1200000000
Acacia mearnsii -35.2200000000 149.9300000000
Acacia mearnsii -32.2700000000 150.4100000000
```

Als Übung können wir eine Aktion erstellen die eine Googlesuche auf Basis des `lakes` Layers durchführt. Zuerst müssen wir die URL, die gebraucht wird um eine Suche nach einem Stichwort durchzuführen, festlegen. Dies lässt sich einfach durchführen indem man einfach Google aufruft und eine einfache Suche durchführt und dann die URL aus der Adressleiste Ihres Browsers entnimmt. Mit diesem kleinen Aufwand können wir sehen dass das Format <http://google.com/search?q=qgis> ist, wobei `QGIS` das Suchwort ist. Anhand dieser Informationen können wir fortfahren:

1. Laden Sie den Layer file:lakes.shp.
2. Öffnen Sie den _Layereigenschaften_ Dialog indem Sie einen Doppelklick auf den Layer in der Legende machen und wählen Sie _Eigenschaften_ aus dem Popup-Menü.
3. Klicken Sie auf das Menü _Aktionen_
4. Geben Sie einen Namen für die Aktion ein, z.B. `Google Search`.
5. Für diese Aktion ist es notwendig den Namen des externen Programms anzugeben. In diesem Fall können wir Firefox verwenden. Wenn das Programm sich nicht im Pfad befindet müssen Sie den vollständigen Pfad angeben.
6. Hinter dem Namen des Programms geben wir die URL ein, die wir für die Internetsuche benutzen wollen, aber ohne das Schlüsselwort: `http://google.com/search?q=`
7. Der Text imm Feld _Aktion_ sollte nun folgendermaßen aussehen: `firefox http://google.com/search?q=`
8. Klicken Sie nun auf die Drop–Down Box mit dem Spaltennamen der Attributtabelle des Layers `lakes`. Der Knopf ist gleich links neben dem Knopf **[Attribut einfügen]**.
9. Selektieren Sie ‘NAMES’ aus der Drop–Down Box und klicken Sie **[Attribut einfügen]**.
10. Die Aktion sieht nun so aus:

    `firefox http://google.com/search?q=%NAMES`
11. Um die Aktion abzuschließen klicken Sie auf den **[Zur Aktionsliste hinzufügen]** Knopf.

Damit ist die Aktion fertig für den Einsatz. Der gesamte Befehl der Aktion sollte folgendermaßen aussehen:

```
firefox http://google.com/search?q=%NAMES
```

Damit ist die Aktion fertig für den Einsatz. Schließen Sie den _Eigenschaften_ Dialog und zoomen Sie in einen Bereich Ihrer Wahl. Stellen Sie sicher, dass der Layer `lakes` in der Legende aktiviert ist. Nun identifizieren Sie einen See. In der Ergebnisanzeige sollte nun die Aktion sichtbar sein:

![](../../../../images/action_identifyaction.png)

Wenn wir nun auf das Wort `action` klicken, öffnet sich der Webbrowser Firefox und zeigt uns das Ergebnis der Internetrecherche z.B. nach dem See Tustumena an <http://www.google.com/search?q=Tustumena>. Es ist übrigens auch möglich, weitere Attributspalten zu ergänzen. Dazu fügen Sie einfach ein ‘+’-Zeichen an das Ende der Aktion, wählen eine weitere Attributspalte und klicken wieder auf den Knopf **[Attribut einfügen]**. In unserem Datensatz ist leider keine weitere sinnvolle Attributspalte vorhanden, nach der man im Internet suchen könnte.

Sie können auch mehrere Aktionen für einen Layer definieren. Sie alle werden dann bei der Abfrage von Objekten im _Identifikationsergebnis_ Dialog angezeigt.

Sie sehen, man kann sich eine Vielzahl interessanter Aktionen ausdenken. Wenn Sie z.B. einen Punktlayer mit einzelnen Punkten haben, an denen Photos geschossen wurden, dann können Sie eine Aktion erstellen, über die Sie dann das entsprechende Foto anzeigen lassen können, wenn Sie auf den Punkt in der Karte klicken. Man kann auch zu bestimmten Attributen webbasierte Information ablegen (z.B. in einer HTML-Datei) und diese dann über eine Aktion anzeigen lassen, etwa so wie in dem Google Beispiel.

Wir können auch komplexere Beispiele erstellen, indem wir z.B. **Python** Aktionen verwenden.

Normalerweise wenn wir beim Erstellen von Aktionen zum Öffnen einer Datei mit einer externen Anwendung absolute Pfade, oder letztendlich relative Pfade verwenden ist im zweiten Fall der Pfad relativ zum Ort der ausführbaren Datei. Was aber wenn wir relative Pfade, die relativ zum ausgewählten Layer (eine dateibasierte, wie ein Shape oder SpatiaLite) sind, benutzen müssen ? Mit dem folgenden Code können wir einen Trick anwenden:

```
command = "firefox";
imagerelpath = "images_test/test_image.jpg";
layer = qgis.utils.iface.activeLayer();
import os.path;
layerpath = layer.source() if layer.providerType() == 'ogr'
  else (qgis.core.QgsDataSourceURI(layer.source()).database()
  if layer.providerType() == 'spatialite' else None);
path = os.path.dirname(str(layerpath));
image = os.path.join(path,imagerelpath);
import subprocess;
subprocess.Popen( [command, image ] );
```

Wir müssen uns nur ins Gedächtnis rufen dass es sich um eine _Python_ Aktion handelt und dass das Ändern der _command_ und _imagerelpath_ Variablen auf unsere Bedürfnisse angepasst wird.

Was aber wenn der relative Pfad relativ zur (gespeicherten) Projektdatei sein muss? Der Code der Python Aktion würde wie folgt lauten:

```
command="firefox";
imagerelpath="images/test_image.jpg";
projectpath=qgis.core.QgsProject.instance().fileName();
import os.path; path=os.path.dirname(str(projectpath)) if projectpath != '' else None;
image=os.path.join(path, imagerelpath);
import subprocess;
subprocess.Popen( [command, image ] );
```

Ein anderes Python Aktion Beispiel ist das mit wir dem Projekt neue Layer hinzufügen können. Z.B. wird in den folgenden Beispielen dem Projekt ein Vektorlayer beziehungsweise ein Rasterlayer hinzugefügt. Die Namen der Dateien, die dem Projekt hinzugefügt werden sollen, und die Namen, die den Layern gegeben werden, sind datengesteuert (_filename_ und _layername_ sind Spaltennamen der Attributtabelle des Vektorlayers in dem die Aktion erstellt wurde):

```
qgis.utils.iface.addVectorLayer('/yourpath/[% "filename" %].shp','[% "layername" %]',
  'ogr')
```

Um eine Rasterdatei hinzuzufügen (ein TIF-Bild in diesem Beispiel) wird daraus:

```
qgis.utils.iface.addRasterLayer('/yourpath/[% "filename" %].tif','[% "layername" %]
')
```

## Menü Verknüpfungen

![](../../../../images/join.png) Mit dem _Verknüpfungen_ Menü können Sie eine geladene Attributtabelle mit einem geladenen Vektorlayer verknüpfen. Nach dem Klicken von ![](../../../../images/mActionSignPlus.png) öffnet sich der _Vektorverknüpfung hinzufügen_ Dialog. Als Schlüsselspalten müssen Sie einen Joinlayer definieren, den Sie mit dem Zielvektorlayer verbinden wollen. Dann müssen Sie das Verknüpfungsfeld, das der Joinlayer und der Zielvektorlayer gemeinsam haben, festlegen. Jetzt können Sie auch eine Untermenge von Feldern aus dem verknüpften Layer auf Basis des Kontrollkästchens ![](../../../../images/checkbox.png) _Verknüpfte Felder wählen_ festlegen. Als Ergebnis der Verknüpfung werden alle Informationen des Joinlayers und des Zielvektorlayers in der Attributtabelle des Zielvektorlayers als verknüpfte Information dargestellt. wenn Sie eine Untermenge von Feldern festgelegt haben dann werden nur diese Felder in der Attributtabelle des Zielvektorlayers dargestellt.

KADAS bietet zur Zeit Unterstützung für das Verknüpfen von nicht-räumlichen Tabellenformaten die von OGR unterstützt werden (z.B. CSV, DBF und Excel) und von Delimited Text und für den PostgreSQL Provider.

![](../../../../images/join_attributes.png)

Zusätzlich können Sie mit dem _Vektorverknüpfung hinzufügen_ Dialog:

- ![](../../../../images/checkbox.png) _Verknüpfung im Speicher cachen_
- ![](../../../../images/checkbox.png) _Index auf Feld erzeugen_
- ![](../../../../images/checkbox.png) _Felden wählen, die zusammengejoined werden sollen_
- Create a ![](../../../../images/checkbox.png) _Benutzerdefinierter Feldnamenpräfix_

## Menü Diagramme

![](../../../../images/diagram.png) Das Menü _Diagramme_ ermöglicht es, ein Diagramm als Grafik über einen Vektorlayer zu visualisieren.

Die aktuelle Kernimplementation von Diagrammen bietet Unterstützung von Kuchendiagrammen, Textdiagrammen und Histogrammen.

Das Menü ist in vier Reiter aufgeteilt: _Darstellung_, _Größe_, _Position_ und _Optionen_.

Im Fall des Textdiagramms und Kuchendiagramms werden Textwerte aus verschiedenen Datenspalten untereinander mit einem Kreis oder einer Box mit Teilern dargestellt. Im _Größe_ Reiter basiert die Diagrammgröße auf einer festen Größe oder auf linearer Skalierung entsprechend eines Klassifikationsattributs. Die Platzierung der Diagramme, die im _Position_ Reiter vorgenommen wird, interagiert mit dem neuen Beschriften, also werden Positionskonflikte zwischen Diagrammen und Beschriftungen aufgedeckt und gelöst. Zusätzlich können die Diagramme manuell befestigt werden.

![](../../../../images/diagram_tab.png)

Wir werden ein Beispiel zeigen und dem Alaskagrenzlayer ein Textdiagramm das Temperaturdaten von einem climate Vektorlayer zeigt überlagern. Beide Vektorlayer sind Teil des KADAS Beispieldatensatzes (siehe Abschnitt [_Beispieldaten_](../introduction/getting_started.html#label-sampledata)).

1. Klicken Sie erst auf das ![](../../../../images/mActionAddOgrLayer.png) _Vektorlayer hinzufügen_ Icon, browsen Sie zum KADAS Beispieldatensatzordner und laden Sie die beiden Vektorlayer `alaska.shp` und `climate.shp`.
2. Doppelklicken Sie auf den `climate` Layer in der Kartenlegende um den Dialog _Layereigenschaften_ zu öffnen.
3. Klicken Sie auf das Menü _Diagramme_, aktivieren Sie ![](../../../../images/checkbox.png)_Diagramme anzeigen_ und wählen Sie ‘Textdiagramm’ aus der _Diagrammtyp_ ![](../../../../images/selectstring.png) Kombo-Box aus.
4. Im _Darstellung_ Reiter wählen wir ein Hellblau als Hintergrundfarbe und im Reiter _Größe_ stellen wir eine feste Größe von 18 mm ein.
5. Im Reiter _Position_ könnte die Platzierung auf ‘Um Punkt’ eingestellt werden.
6. Im Diagramm wollen wir die Werte der drei Spalten `T_F_JAN`, `T_F_JUL` und `T_F_MEAN` darstellen. Wählen Sie erst `T_F_JAN` als _Attribute´ und klicken Sie den |mActionSignPlus| Knopf, dann ``T\_F\_JUL`_ und schließlich `T_F_MEAN`.
7. Klicken Sie jetzt **[Anwenden]** um das Diagramm in KADAS anzuzeigen.
8. Sie können die Diagrammgröße im _Größe_ Reiter anpassen. Deaktivieren Sie ![](../../../../images/checkbox.png) _Feste Größe_ und stellen Sie die Größe des Diagramms auf Basis eines Attributes mit dem **[Maximalwert suchen]** Knopf und dem _Größe_ Menü ein. Wenn die Diagramme auf dem Bildschirm zu klein erscheinen können Sie das ![](../../../../images/checkbox.png) _Kleine Diagramme vergrößern_ Kontrollkästchen aktivieren und die Minimalgröße des Diagramms definieren.
9. Verändern Sie die Attributfarben indem Sie auf die Farbwerte im _Zugewiesene Attribute_ Feld doppelklicken.
10. Klicken Sie schließlich auf **[Ok]**.

![](../../../../images/climate_diagram.png)

Behalten Sie im Hinterkopf dass im Reiter _Position_ eine _Datendefinierte Position_ der Diagramme möglich ist. Sie können hier Attribute verwenden um die Position des Diagramms zu definieren. Sie können auch eine maßstabsabhängige Sichtbarkeit im _Darstellung_ Reiter einstellen.

Die Größe und die Attribute können auch ein Ausdruck sein. Verwenden Sie den ![](../../../../images/mIconExpressionEditorOpen.png) Knopf um einen Ausdrück einzufügen. Siehe Kapitel [_Ausdrücke_](expression.html#vector-expressions) für weitere Informationen und Beispiele.

## Menü Metadaten

![](../../../../images/metadata.png) Das _Metadaten_ Menü besteht aus _Beschreibung_, _Beschreibung_, _Metadaten-URL_ und _Eigenschaften_ Abschnitten.

Im Abschnitt _Eigenschaften_ erhalten Sie allgemeine Informationen über den Layer, darunter Einzelheiten über den Typ und die Verortung, Anzahl der Objekte, Objekttyp und Bearbeitungsmöglichkeiten. Die _Ausdehnung_ Tabelle zeigt Ihnen Layerausdehnungsinformationen und das _Räumliches Bezugssystem des Layers_ Informationen über das CRS des Layers. Dies ist ein schneller Weg Informationen über den Layer herauszufinden.

Zusätzlich können Sie einen Titel und eine Zusammenfassung für den Layer im Abschnitt _Beschreibung_ hinzufügen oder bearbeiten. Es ist hier außerdem möglich eine _Stichwortliste_ zu definieren. Diese Stichwortlisten können in einem Metadatenkatalog verwendet werden. Wenn Sie einen Titel aus einer XML Metadatendatei verwenden wollen müssen Sie einen Link im _DateURL_ Feld ausfüllen. Verwenden Sie _Beschreibung_ um Attributdaten aus einem XML-Metadatenkatalog zu erhalten. In _Metadaten-URL_ können Sie den allgemeinen Pfad zum XML-Metadatenkatalog definieren. Diese Information wird in der KADAS Projektdatei für nachfolgende Sitzungen gespeichert und wird für KADAS Server verwendet.

![](../../../../images/vector_metadata_tab.png)
