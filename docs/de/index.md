<!-- Recovered from: docs_old/html/de/de/index.html -->
<!-- Language: de | Section: index -->

# Allgemeines

## Entstehung

KADAS Albireo ist ein Kartendarstellungsystem, welches auf der professionellen Open-Source GIS Software "QGIS" basiert, und sich an nicht-spezialisierte Anwender richtet. In Zusammenarbeit mit der Firma Ergnomen wurde eine neue Benutzeroberfläche entwickelt, in der Funktionalitäten für fortgeschrittene Anwender ausgeblendet wurden. Stattdessen werden verbesserte Funktionen in Bereichen wie Zeichnen, Geländeanalyse, Drucken und Interoperabilität angeboten.

## Nutzungsbedingungen

KADAS Albireo steht unter der General Public License 2.0 (GPLv2).

Die MSS/MilX Komponente ist Eigentum der Firma gs-soft AG.

Die Nutzungsbedingungen für die Daten sind in der Anwendung unter Hilfe → Über aufgelistet.

## Versionsprotokoll

### Version 2.3 (August 2025)

Mit dem Release von KADAS 2.3 im Q3 2025 sind zahlreiche Neuerungen, Verbesserungen und Fehlerbehebungen umgesetzt worden.

- _Neue Funktionalitäten und Geodaten_
    - Geodienste und Geodaten mit einer Zeitkomponente können als chronologische Animation abgespielt werden.
    - Themenvergleiche (SWIPE-Tool) bei dem zwei Kartenansichten interaktiv mithilfe eines Schiebereglers ein- und ausgeblendet werden können.
    - Geöffnete Attributtabellen werden im Projekt gespeichert und beim Öffnen des Projektes angezeigt.
    - Unterstützung von NetCDF Rasterdatenformat (wird vor allem für Meteorologie verwendet).
    - Ein neuer «Feedback»-Knopf. Dieser Knopf führt direkt zu einem Formular auf dem Geo Info Portal V und ermöglicht Rückmeldungen auf Software-Funktionalitäten und Bugs.
    - Das Erstellen von Höhenprofilen liefert zusätzliche Statistiken, wie z.B höchster/tiefster Punkt, Länge und Höhenunterschied zur Profillinie.
    - Vector Tiles Geodienste der MGDI werden im Geokatalog gelistet und können geladen werden.
    - Neu bestehen die Offline-Daten aus der World Briefing Map.
    - Der MSS Export ist neu als KML möglich (nach dem KML-Export nicht weiter bearbeitbar).
- _Verbesserungen_
    - Plugin Manager: Bei Start von KADAS wird geprüft, ob aktuellere Versionen der installierten Plugins im Repository vorhanden sind. Falls dem so ist, wird automatisch die neueste Version des Plugins installiert und der Benutzer mit einer Meldung informiert.
    - Beim Abfragen via Klicken ist die Darstellung der Resultate, insbesondere wenn mehrere Resultate zurückgegeben werden, verbessert worden.
    - Die Funktion Bookmarks speichert nun auch Layer in Gruppen und Untergruppen.
    - Das Routing Plugin ist nun ein Standard-Plugin und wird automatisch vom Repository installiert. Es können auch weitere Plugins als obligatorisch definiert werden.
    - Eingeschränkte Geodienste in der MGDI können von berechtigten Benutzern in KADAS angezeigt werden.
    - Die Koordinatensuche wurde verbessert und standardisiert.
    - Der Support von WCS Geodiensten ermöglicht nun auch Analysen mit hochaufgelösten Höhenmodellen.
    - Die Textfunktion in Redlining enthält zusätzliche Einstellungsmöglichkeiten.
    - Die Darstellung und Beschriftung des MGRS Gitters wurde optimiert.
    - Die Druckvorlagen wurden aktualisiert und enthalten neu defaultmässig das Erstelldatum und das Projektionssystem.
    - Analysen lassen sich direkt über einen Rechtsklick mit der Maus starten.
    - Die Ausdehnung des GPKG-Exports von Vektordaten kann definiert werden.
- _Fehlerbehebungen_
    - Crash beim Export von GeoPDF wurde behoben.
    - Beim Export von GeoPDF werden alle Layer berücksichtigt und lagekorrekt dargestellt.
    - Crash bei der Verwendung des Ephemeris-Werkzeuges wurde behoben.
    - Crash beim Export von Geopackages wurde behoben.
    - Fehlende Übersetzungen in den Sprachen (DE, FR, IT) behoben.
- _Technische Anpassungen_
    - KADAS 2.3 basiert auf der QGIS-Version 3.44.0-Solothurn.
    - Die beiden Höhenmodelle Schweiz und weltweit (dtm\_analysis.tif + general\_dtm\_globe.tif) sind im COG-Format (Cloud Optimized GeoTIFF) vorhanden. Das erlaubt eine schnellere Prozessierung und Visualisierung.
    - Der 3D Viewer (Globe) musste ausgetauscht werden, da die verwendete Technologie End-of-life ist. Neu wird der 3D Viewer von der QGIS-Applikation verwendet.
    - Die verwendete MSS/MilX Bibliothek von gs-soft ist neu MSS-2025.
    - Die Valhalla Routing Engine wurde aktualisiert und ermöglicht die Miteinbeziehung der Höhe in die Routen- und Distanzberechnung.
    - Alle produktiven Plugins wurden für die Version 2.3 angepasst.
    - Hilfe wurde in den Browser externalisiert.

### Version 2.2.0 (Juni 2023)

- _Allgemein_:
    - Unterstützung für das Laden von Esri VectorTile-Ebenen hinzugefügt
    - Unterstützung für das Laden von Esri MapService-Ebenen hinzugefügt
    - Layerbaum: Unterstützung der Konfiguration des Aktualisierungsintervalls der Datenquelle für die automatische Aktualisierung der Ebenen
    - Unterstützung des GeoPDF-Druckexports
    - Sperren des Kartenmassstabs
    - Konfigurierbarer News-Popup-Dialog
    - Verbesserter Import von 3D-Geometrien aus KML-Dateien
- _Ansicht_:
    - Aufnahme von Schnappschüssen der 3D-Ansicht
    - Verbesserte MGRS-Gitterbeschriftung
- _Analyse_:
    - Neues Min/Max-Tool zur Abfrage des niedrigsten/höchsten Punktes im ausgewählten Bereich
    - Auswahl der Zeitzone im Ephemeriden-Werkzeug
    - Korrekte Behandlung von NODATA-Werten im Höhenprofil
- _Zeichnen_:
    - Rückgängig/Wiederherstellen für die gesamte Zeichensitzung zulassen
    - Ändern der Z-Reihenfolge von Zeichnungen
    - Hinzufügen von Bildern aus einer URL
- _MSS_:
    - Rückgängig machen/Wiederholen während der gesamten Zeichnungssitzung erlauben
    - Gestaltung von Führungslinien (Breite, Farbe)
    - Aktualisierung auf MSS-2024
- _Hilfe_:
    - Suche in der Hilfe

### Version 2.1.0 (Dezember 2021)

- _Allgemeines_:
    - Drucken: Korrekte Skalierung von Symbolen (MSS, Pins, Bilder, ...) entsprechend der Druck-DPI
    - GPKG: Möglichkeit, Projektebenen zu importieren
    - Ebenenbaum: Möglichkeit zum Zoomen und Entfernen aller ausgewählten Ebenen
    - Skalenbasierte Sichtbarkeit auch für Redlining/MSS-Ebenen
    - Attributtabelle: Verschiedene neue Auswahl- und Zoom-Werkzeuge
- _Ansicht_:
    - Neue Lesezeichen-Funktion
- _Analyse_:
    - Viewshed: Möglichkeit, den vertikalen Winkelbereich des Beobachters zu begrenzen
    - Höhenprofil / Sichtlinie: Anzeige der Markierung in der Grafik, wenn man mit der Maus über die Linie auf der Karte fährt
    - Neues Ephemeriden-Werkzeug
- _Zeichnen_:
    - Pins: neuer Rich-Text-Editor
    - Stecknadeln: Möglichkeit, mit der Mouse mit dem Tooltipinhalt zu interagieren
    - Führungsraster: Beschriftung nur eines Quadranten zulassen
    - Bullseye: Beschriftung von Quandranten
    - Neues Element zum Zeichnen von Passkreuzen
- _MSS_:
    - Symbol-Einstellungen pro Ebene
    - Aktualisierung auf MSS-2022

### Version 2.0.0 (Juli 2020)

- Vollständige Neugestaltung der Programmarchitektur: KADAS ist jetzt eine separate Anwendung, die auf den QGIS 3.x-Bibliotheken aufbaut
- Überarbeitete Architektur für Kartenelemente, für ein konsistentes Verhalten beim Zeichnen und Bearbeiten sämtlicher Objekte (Redlining, MSS, usw.)
- Verwendet das neue qgz-Dateiformat, wobei der bisherige Ordner `<Projektname>_files` vermieden wird
- Projekt Autosave
- Neuer Plugin-Manager zur Verwaltung externer Plugins direkt aus KADAS heraus
- Vollbildmodus
- Neue Kartengitter-Implementierung, die auch UTM/MGRS-Gitter in der Hauptkarte unterstützt
- Nach Boundingbox eingrenzbarer KML/KMZ-Export
- Nach Boundingbox eingrenzbarer GPKG-Datenexport
- Stile von Redlining-Geometrien werden bei der Darstellung als 2,5D- oder 3D-Objekte auf dem Globus berücksichtigt
- Erweitertes Führungsgitter
- Aktualisierung zu MSS-2021

### Version 1.2 (Dezember 2018)

- _Allgemein_:
    - Verbesserte KML/KMZ Export Funktionalität
    - Neue KML/KMZ Import Funktionalität
    - Neue GeoPackage Export und Import Funktionalität
    - Erlaube das Hinzufügen von CSV/WMS/WFS/WCS Ebenen im Ribbon GUI
    - Erlaube das Hinzufügen von Aktionen zum Ribbon GUI via Python Schnittstelle
    - Setzte Tastenkürzel für zahlreiche Aktionen des Ribbon GUIs
    - Verbessertes "Fuzzy-Matching" bei der Koordinatensuche
- _Analyse_:
    - Darstellung von Knotenpunkte der Messlinie im Höhenprofil
- _Zeichnen_:
    - Unterstützung für numerische Eingabe beim Zeichnen von Redlining Objekte
    - Erlaube das Setzen von Skalierungsfaktoren für Annotationenebenen
    - Erlaube das aktivieren/deaktivieren des Rahmen der Bild-Annotationen
    - Erlaube des Manipulieren von Gruppen von Annotationen
    - Neue Funktionalität: Führungsraster
    - Neue Funktionalität: Bullseye
- _GPS_:
    - Erlaube das Konvertieren zwischen Waypoints und Stecknadeln
    - Erlaube das Ändern der Farbe von Waypoints un Routen
- _MSS_:
    - Upgrade auf MSS-2019

### Version 1.1 (November 2017)

- _Allgemein_:
    - Frei setzbaren Cursor im Suchfeld
    - Höhenanzeige in der Statusleiste
    - Geschwindigkeitsverbesserungen bei der Kartendarstellung
    - Attributtabelle für Vektorebenen
- _Analyse_:
    - Geodätische Distanz- und Flächenmessung
    - Azimut wählbar relativ zum Kartennorden oder geographischen Norden
- _Zeichnen_:
    - Einschaltbares Snapping beim Zeichnen
    - Undo/Redo beim Zeichnen
    - Zeichnung können verschoben, kopiert, ausgschnitten und eingefügt werden, einzeln oder als Gruppe
    - Bestehende Geometrien können fortgesetzt werden
    - Laden von SVG Graphiken (u.a. SymTaZ Graphiken)
    - Laden von nicht georeferenzierte Bilder
    - Bilder und Stecknadeln werden nun in entsprechende Ebenen abgelegt
- _MSS_:
    - Upgrade auf MSS-2018
    - Korrektes Grössenverhältnis von MSS Symbole beim Drucken
    - Kartuscheinhalt kann von und nach MilX oder XML Dateien importiert bzw. exportiert werden
    - Numerische Eingabe von numerischen Attributen beim Zeichnen von MSS Symbole
- _3D_:
    - Unterstützung für 3D Geometrien in der 3D-Ansicht
- _Drucken_:
    - Im Projekt enthaltene Druckvorlagen können verwaltet werden

### Version 1.0 (September 2016)

- Initiale Version
