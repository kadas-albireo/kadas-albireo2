<!-- Recovered from: docs_old/html/de/de/kadas_gui/index.html -->
<!-- Language: de | Section: kadas_gui -->

# KADAS Benutzeroberfläche

![](media/image1.png)

Die KADAS Benutzeroberfläche ist in fünf Bereiche unterteilt:

- Funktionsmenü
- Favoriten und Suche
- Kartenfenster
- Ebenen und Geodatenkatalog
- Status Bar

## Funktionsmenü

Im Funkionsmenü kann über eine Menüleiste zwischen verschiedenen Symbolleisten umgeschaltet werden. Die Symbolleisten enthalten Knöpfe für die verschiedenen Funktionen. Diese sind in separaten Kapiteln dokumentiert:

- [Karte](map/)
- [Ansicht](view/)
- [Analyse](analysis/)
- [Zeichnen](draw/)
- [Navigation](gps/)
- [MSS](mss/)
- [Einstellungen](settings/)

## Favoriten und Suche

### Favoriten

Auf den vier Platzhaltern können favorisierte Funktionen aus dem Funktionsmenü platziert werden. Dies geschieht durch Ziehen der Maus bei gedrückter Maustaste. Der Favorit kann durch Drücken der rechten Mausstaste wieder entfernt werden. Die Favoriten werden als Benutzereinstellungen gespeichert.

### Suche

Das Suchfeld bietet eine einheitliche Schnittstelle für verschiedene Suchdienste:

- Koordinaten (LV03, LV95, DD, DM, DMS, UTM, MGRS)
- Ortschaften und Adressen schweizweit
- Ortschaften weltweit
- Attribute in lokale Datensätze
- Attribute in remote Datensätze (Web-Dienste)
- Attribute in Stecknadeln

Nach der Eingabe von mindestens drei Buchstaben startet die Suche und es werden erste Resultate angezeigt.

Die Resultate werden in entsprechend bezeichnete Kategorien aufgelistet. Die Resultatliste kann mit Maus oder Tastatur-Pfeile durchsucht werden. Beim Auswählen eines Resultats mit den Pfeilen wird eine blaue Stecknadel an den entsprechenden Ort gesetzt. Beim aktivieren eines Resultats mit der Maus wird der Kartenausschnitt auf den entsprechenden Ort zentriert.

![](media/image2.png)

Rechts vom Suchfeld gibt es die Möglichkeit, einen Filter für die lokale und remote Datensatz-Suche zu definieren. Dieser Filter greift _nicht_ für Koordinaten, Ortschaft oder Stecknadelsuchen.

## Kartenfenster

Dieser zentrale Bereich von KADAS zeigt die geladenen Layer an und ermöglicht verschiedene Operationen auf der Karte.

Das Navigieren in der Karte erfolgt mit der linken oder mittleren Maustaste, das Zoomen mit dem Scrollrad oder mit den Zoomtasten in der oberen rechten Ecke des Kartenfensters. Mit der rechten Maustaste wird das Kontextmenü geöffnet. Navigations- und Drehbewegungen werden auf berührungsempfindlichen Geräten erkannt. Darüber hinaus kann auf eine bestimmte Region gezoomt werden, indem man bei gedrückter SHIFT-Taste ein Rechteck aufzieht.

Unabhängig vom aktiven Kartenwerkzeug werden die mittlere Maustaste und das Scrollrad immer für die Kartennavigation verwendet. Die Funktion der linken und rechten Maustasten ist abhängig vom aktiven Werkzeug.

Der Inhalt der Karte wird durch die im nächsten Abschnitt beschriebene Kartenlegende gesteuert.

In der Registerkarte Ansicht können zusätzliche Kartenansichten hinzugefügt werden. Diese zusätzlichen Ansichten sind passiv, d.h. es ist keine weitere Interaktion außer dem Navigieren und Zoomen möglich.

## Ebenen und Geodatenkatalog

Am linken Rand des Programmfensters befindet sich ein aufklappbarer Bereich, welche Verwaltungsfunktionen für Kartenebenen enthält. Im oberen Teil ist die Kartenlegende und im unteren Teil der Geodatenkatalog angeordnet.

### Ebenen

Der Bereich der Kartenlegende verzeichnet alle **Ebenen** des Projekts. Das Kontrollkästchen für jedes Element der Legende kann benutzt werden, um die Ebene ein- oder auszublenden.

Die Z-Anordnung der Kartenlayer kann mit der ‘drag and drop’ Funktion der Maus festgelegt werden. Z-Anordnung bedeutet, dass ein weiter oben in der Legende angeordneter Layer über einem weiter unten angeordneten Layer im Kartenfenster angezeigt wird.

Die Ebenen im Legendenfenster können als Gruppen organisiert werden.

Das Kontrollkästchen für eine Gruppe zeigt oder verbirgt alle Layer einer Gruppe mit einem Klick.

Bei einem Rechtsklick auf einen Eintrag können, abhängig vom Typ der ausgewählten Ebene, verschiedene Operationen ausgeführt werden, wie:

- Auf Ebene zoomen
- Entfernen
- Unbenennen
- Ebeneneigentschaften aufrufen

Es ist möglich mehr als einen Layer oder Gruppe zur gleichen Zeit auszuwählen indem man die Strg Taste gedrückt hält und die Layer mit der linken Maustaste auswählt.

### Geodatenkatalog

Im Geodatenkatalog können weitere Kartenebenen zur Karte hinzugefügt werden. Ist die Liste leer, besteht keine Netzwerkverbindung zum Katalogdienst.

Beim Programmstart werden nur öffentliche Daten angezeigt. Abhängig vom Benutzer können nach erfolgter Authentifizierung weitere Daten zur Verfügung stehen, siehe _SAML Authentifizierung_ unten.

Durch Eingabe von Suchbegriffen im Textfeld werden die verfügbaren Ebenen entsprechend eingeschränkt. Eine Ebene kann mittels Kontextmenü (rechte Maustaste auf dem Ebeneneintrag) oder via “Drag and Drop” der Karte hinzugefügt werden.

Oberhalb der Katalogliste stehen folgende Funktionen zur Verfügung:

- **Lokaler Datensatz hinzufügen**: Es können Vektor-, Raster- und CSV-Daten hinzugefügt werden.
- **Ebene eines Webdienstes hinzufügen**: Es können WMS, WFS, WCS, MapServer und Vektorkachel Ebenen hinzugefügt werden.
- **Katalog neu laden**: Lädt die Liste der verfügbaren Kartenebenen neu.
- **SAML Authentifizierung**: Es wird ein Fenster geöffnet, in dem ein webbasiertes Login auf dem Server durchgeführt werden kann. Nach erfolgreichem Login wird der Geodatenkatalog neu geladen und es stehen entsprechend den Berechtigungen zusätzliche Kartenebenen zur Verfügung.

## Statusbar

In der Statuszeile sind folgende Anzeigen und Bedienelemente angeordnet:

- **GPS**: Die Verwendung der GPS Schaltfläche ist im Kapitel [Kapitel _Navigation_](gps/) beschrieben.
- Mausposition\_\*\*: Die aktuelle Mausposition auf der Karte kann in Bezug auf mehrere Referenzsysteme angezeigt werden. Das gewünschte System kann im Menü links neben der Positionsanzeige ausgewählt werden. Die Einheit für die Höhe kann in der Einstellungen-Registerkarte geändert werden.
- **Massstab**: Der aktuelle Massstab der Kartenansicht wird neben dem Koordinatenfeld angezeigt. Der Menü ermöglicht die Auswahl zwischen vordefinierten Skalen zwischen 1:500 und 1:100000000. Mit dem Schloss-Symbol kann der aktuelle Kartenmassstab fixiert werden, das Zoomen wirkt sich dann nur noch auf den Vergrößerungsfaktor aus.
- Koordinatenbezugssystem\_\*\*: In dieser Auswahlschaltfläche ausgewählt werden, welche Projektion für die Karte verwendet werden soll. Weicht die gewählte Projektion von der nativen Projektion eines Datensatzes ab, wird dieser neu projiziert, was je nach Datenmenge zu Leistungseinbussen führen kann.
