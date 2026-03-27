<!-- Recovered from: docs_old/html/de/de/working_with_vector/supported_data/index.html -->
<!-- Language: de | Section: working_with_vector/supported_data -->

# Unterstützte Datenformate

KADAS verwendet die OGR-bibliothek um Vektordatenformate zu lesen und zu schreiben, einschließlich ESRI Shapedateien, MapInfo und MicroStation Dateiformate, AutoCAD DXF, PostGIS, SpatiaLite, Oracle Spatial und MSSQL Spatial Datenbanken und viele mehr. GRASS Vektor und PostgreSQL Support wird duch native Datenprovider Plugins bereitgestellt. Die Vektordaten können auch im Lesemodus aus zip- und gzip-Archiven ins KADAS geladen werden. Zum Zeitpunkt der Erstellung dieses Dokumentes werden 69 Vektorformate von der OGR-Bibliothek unterstützt (siehe OGR-SOFTWARE-SUITE in _literature\_and\_web_). Die vollständige Liste ist auf <https://gdal.org/en/stable/drivers/vector/index.html> zu finden.

## ESRI Shapes

Die ESRI Shapedatei ist das Standard Vektorformat in QGIS und wird durch die OGR Simple Feature Library (<http://www.gdal.org/> ) bereitgestellt.

Ein Shape besteht derzeit aus mehreren Dateien. Die folgenden drei sind erforderlich:

1. `shp` Datei (enthält die Geometrien)
2. `.dbf` Datei (enthält die Attribute im dBase-Format)
3. `.shx` Indexdatei

Darüber hinaus kann eine Datei mit .prj Endung existieren. Diese enthält die Projektionsinformationen des Shapes. Während es sehr nützlich ist eine Projektionsdatei zu verwenden ist dies nicht zwingend erforderlich. Ein Shape-Datensatz kann zusätzliche Dateien enthalten. Details dazu finden sich in der technischen Spezifikation von ESRI unter <http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf>.

### Shape Layer laden

Beim Laden einer Vektorebene öffnet sich der folgende Dialog:

![](../../images/addvectorlayerdialog.png)

Wählen Sie aus den möglichen Quelltypen ![radiobuttonon](../../images/radiobuttonon.png) _Datei_ und klicken Sie auf den Knopf **[Durchsuchen]**. Dadurch erscheint ein weiterer Dialog zum Öffnen mit dem Sie im Dateisystem navigieren können und Sie ein Shape oder eine andere unterstützte Datenquelle laden können. Die Auswahlbox _Filter_ ![](../../images/selectstring.png) ermöglicht es Ihnen einige OGR-untersützte Dateiformate vorzuwählen.

Außerdem kann auch der Kodierungstyp für die Shapedatei eingestellt werden, falls dies notwendig ist.

![](../../images/shapefileopendialog.png)

Wenn Sie ein Shapefile aus der Liste auswählen und auf **[Öffnen]** klicken, wird es in KADAS geladen.

**Farben von Vektorlayern**

Wenn Sie einen neuen Vektorlayer in QGIS laden, werden Farben zufällig zugewiesen. Wenn Sie mehrere neue Vektorlayer laden, werden jeweils unterschiedliche Farben zugewiesen.

Nach dem Laden können Sie mit den Navigationstools aus der Werkzeugleiste beliebig zoomen. Um den Stil eines Layers zu verändern öffnen Sie den _Layereigenschaften_ Dialog in dem Sie auf den Layernamen doppelklicken oder indem Sie einen Rechtsklick auf den Namen in der Legende machen und _Eigenschaften_ im Popupmenu wählen. Vergleichen Sie Abschnitt _vector\_style\_tab_ für weitere Informationen zum Editieren der Eigenschaften von Vektorlayern.

### Die Darstellungsgeschwindigkeit von Shapdedateien verbessern

Um die Darstellungsgeschwindigkeit zu optimieren, kann ein räumlicher Index erstellt werden. Ein räumlicher Index erhöht die Geschwindigkeit beim Zoomen und Verschieben. Räumliche Indizes haben in KADAS die Endung `.qix`.

Benutzen Sie folgende Schritte zum Erstellen eines räumlichen Index:

- Um eine Shapedatei zu laden klicken Sie auf den ![](../../images/mActionAddOgrLayer.png) _Vektorlayer hinzufügen_ Knopf in der Werkzeugleiste oder drücken einfach `Strg+Umschalt+V`.
- Öffnen Sie den _Eigenschaften_-Dialog des Vektorlayers, indem Sie auf den Namen des Layers in der Legende doppelklicken oder mit der rechten Maustaste _Eigenschaften_ auswählen.
- Im Menü _Allgemein_ klicken Sie auf den **[Räumlichen Index erzeugen]** Knopf.

### Problem beim Laden eines Shapes mit .prj Datei

Wenn Sie eine Shapedatei mit `.prj`-Datei laden und KADAS ist nicht in der Lage, die Projektionsinformationen korrekt auszulesen, ist es notwendig das Koordinatenbezugsystem (KBS) manuell im _Allgemein_ Menü des _Layereigenschaften_ Dialog anhand des **[Festlegen ...]** Knopfs anzugeben. Hintergrund ist, dass `.prj` Dateien oftmals nicht die vollständigen Projektionsparameter enthalten, so wie KADAS sie benötigt und auch im _KBS_ Dialog anzeigt.

Aus diesem Grund, wenn Sie ein neues Shapefile mit KADAS erstellen, werden derzeit zwei unterschiedliche Projektionsdateien angelegt. Eine `.prj` Datei, mit den unvollständigen Projektionsparametern, wie sie z.B. von ESRI Software gelesen und erstellt wird, und eine `.qpj` Datei, in der die vollständigen Projektionsparameter anthalten sind. Wenn Sie dann ein Shape in KADAS laden, und QGIS findet eine `.qpj` Datei, dann wird diese anstelle der `.prj` Datei benutzt.
