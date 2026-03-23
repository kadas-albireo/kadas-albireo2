<!-- Recovered from: docs_old/html/de/de/working_with_projections/working_with_projections/index.html -->
<!-- Language: de | Section: working_with_projections/working_with_projections -->

# Arbeiten mit Projektionen

KADAS ermöglicht es, globale und projektbezogene KBS (Koordinatenbezugssysteme) für Layer ohne vordefinierte KBS zu definieren. Es können benutzerdefinierte Koordinatenbezugssysteme erstellt werden und für Raster- und Vektorlayer wird On-The-Fly (OTF) Projektion unterstützt, um Layer gemeinsam und lagegenau darzustellen, auch wenn sie unterschiedliche KBS besitzen.

## Überblick zur Projektionsunterstützung

KADAS unterstützt etwa 2700 bekannte Koordinatenbezugssysteme (KBS). Diese sind in einer SQlite-Datenbank abgelegt, die mit QGIS installiert wird. Normalerweise muss diese Datenbank nicht editiert werden, und es kann Probleme verursachen, wenn Sie es dennoch versuchen. Selbstdefinierte KBS sind in einer Benutzerdatenbank abgelegt. Informationen zum Anlegen einer Benutzerdatenbank finden Sie im Abschnitt _sec\_custom\_projections_.

Die Koordinatenbezugssysteme in KADAS basieren auf EPSG Codes der European Petroleum Search Group (EPSG) und dem Institut Geographic National de France (IGNF) und entsprechen weitestgehend den spatial reference Tabellen der Software GDAL. Die EPSG IDs sind in einer SQlite-Datenbank abgelegt und werden benutzt, um KBS in KADAS zu spezifizieren.

Um OTF Projektion zu verwenden, müssen die Daten Informationen über ihr Koordinatenbezugssystem enthalten oder Sie müssen ein globales, layer- oder projektbezogenes KBS definieren. Bei PostGIS-Layern benutzt KADAS die spatial reference ID, die bei der Erstellung des Layers festgelegt wurde. Bei Daten, die von der OGR-Bibliothek unterstützt werden, bezieht sich KADAS auf das Vorhandensein eines KBS bei den Daten. Bei Shapes bedeutet dies, dass eine Datei mit der Endung .prj vorhanden sein muss, in der das KBS im Well Known Text (WKT) Format angegeben ist. Für ein Shape mit dem Namen `alaska.shp` gäbe es also eine entsprechende Projektionsdatei `alaska.prj`.

The _CRS_ tab contains the following important components:

1. **Filter** - wenn Sie den EPSG Code, die ID oder den Namen für ein Koordinatenbezugssystem kennen können Sie diese benutzen, um ihr Koordinatenbezugssystem zu finden. Geben Sie einfach einen EPSG Code, eine ID oder einen Namen ein.
2. **Kürzlich benutzte Koordinatenbezugssysteme** -Wenn Sie bestimmte Koordinatenbezugssysteme regelmäßig für ihre tägliche GIS Arbeit verwenden, werden diese für den ‘schnellen’ Zugriff unterhalb des Fensters mit den vorhandenen KBS angezeigt. Klicken Sie auf einen der Knöpfe, um das enstprechende KBS direkt auszuwählen.
3. **Koordinatenbezugssystem der Welt** – Dies ist eine Liste von allen KBS die von KADAS unterstützt werden, darunter Geographische, Projezierte und Benutzerdefinierte Koordinatenbezugssysteme. Um ein KBS zu definieren wählen Sie es aus der Liste indem Sie den entsprechenden Knoten aufklappen und das KBS auswählen. Das aktive KBS ist vorgewählt.
4. **Proj4Text** - dies ist ein Ausdruck der von der PROJ4-Bibliothek genutzt wird. Er dient nur zu Information und kann nicht verändert werden.

## Standard Datumtransformationen

Eine Spontan-Reprojektion hängt davon ab ob Daten in ein ‘Standard-KBS’ transformiert werden können, KADAS benutzt hierbei WGS84. Für einige KBS sind eine Reihe von Transformationen verfügbar. Sie können unter KADAS die benutzte Transformation definieren sonst benutzt es eine Standard-Transformation.

KADAS fragt welche Transformation benutzt werden soll indem es eine Dialogbox, die den PROJ.4 Text der wiederum die Quell- und Ziel-Datumstransformation beschreibt, öffnet. Weitere Informationen sind zu finden indem man mit der Maus über eine Transformation geht. Benutzereinstellungen können gespeichert werden indem Sie ![radiobuttonon](../../images/radiobuttonon.png) _Speichere Auswahl_ auswählen.
