<!-- Recovered from: share/docs/html/de/de/working_with_raster/supported_data/index.html -->
<!-- Language: de | Section: working_with_raster/supported_data -->

# Arbeiten mit Rasterdaten

Dieser Abschnitt beschreibt wie man Rasterlayereigenschaften visualisiert und einstellt. KADAS verwendet die GDAL Bibliothek um Rasterdatenformate zu lesen und zu schreiben. Dies umfasst Arc/Info Binary Grid, Arc/Info ASCII Grid, GeoTIFF, Erdas Imagine und viele weitere Formate. GRASS Rastersupport wird durch einen eigenen QGIS Datenprovider bereitgestellt. Die Rasterdaten können auch im Lesemodus aus zip und gzip Archiven in KADAS geladen werden.

Zum Zeitpunkt der Erstellung diese Dokumentes werden mehr als 100 Rasterdatenformate von der GDAL-Bibliothek unterstützt (siehe GDAL-SOFTWARE-SUITE _literature\_and\_web_). Eine vollständige Liste ist unter <http://www.gdal.org/formats_list.html>. erhältlich.

Einige der aufgelisteten Formate können auf ihrem Rechner aus unterschiedlichen Gründen nicht unterstützt werden. Einige brauchen z.B. kommerzielle Bibliotheken oder die GDAL Installation auf Ihrem Rechner, wurde ohne die Unterstützung für das entsprechende Format erstellt. Nur Formate, die getestet wurden, können ausgewählt werden, wenn Sie eine Rasterdatei in QGIS laden. Andere nicht getestete Formate können anhand des `[GDAL] Alle Dateien (*)` Filter geladen werden.

## Was ist ein Rasterlayer?

Rasterlayer in QGIS sind Gitter aus diskreten Zellen oder Pixeln, die Objekte über, auf oder unter der Erdoberfläche beschreiben. Jede Zelle eines Rasters hat die gleiche Größe und ist meistens rechteckig (in QGIS ist dies immer so). Typische Rasterlayer sind Fernerkungsdaten wie Luft- oder Satellitenbilddaten oder modellierte Daten wie Höhenmodelle.

Im Gegensatz zu Vektorlayern haben Rasterlayer keine Verknüpfung zu einer Attributtabelle mit Werten für jede Zelle. Sie sind anhand der Pixelauflösung und der x/y Koordinate eines Eckpixels des Rasterlayers geocodiert. Dies ermöglicht es KADAS die Daten korrekt im Kartenfenster zu positionieren.

QGIS liest die Projektionsinformationen innerhalb des Rasterlayers (e.g. GeoTiff) oder aus einem passenden Worldfile um die Daten richtig darzustellen.
