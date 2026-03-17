<!-- Recovered from: share/docs/html/de/de/view/index.html -->
<!-- Language: de | Section: view -->

# Ansicht

## Vorheriger / Nächster Ausschnitt

Beim Bewegen in der Karte kann über die Funktion **Vorheriger Ausschnitt** auf den vorher angezeigten Kartenausschnitt zurückgekehrt werden. Über **Nächster Ausschnitt** wird wieder auf den danach gewählten Kartenausschnitt gewechselt.

## Lesezeichen

Lesezeichen ermöglichen das Speichern und Wiederherstellen des aktuellen Kartenzustandes, insbesondere der Kartenausdehnung und der Ebenenkonfiguration. Zu beachten ist, dass das Wiederherstellen eines Lesezeichens keine Ebenen wiederherstellt, die inzwischen aus dem Projekt entfernt worden sind.

## Neues Kartenfenster

Über die Funktion **Neues Kartenfenster** können sekundäre Kartenansichten geöffnet werden. Diese können durch Ziehen an der Titelleiste beliebig innerhalb des Hauptfenster oder entkoppelt davon angeordnet werden.

In den Unterfensteransichten können unabhängig von der Hauptansicht Ebenen aktiviert und deaktiviert werden. Der Ausschnitt lässt sich unabhängig von der Hauptansicht steuern oder mit dieser koppeln indem das Schloss-Symbol in der Titelleiste des Unterfenster aktiviert wird.

In sekundären Kartenansichten ist nur das Navigationswerkzeug verfügbar, sämtliche weitere Werkzeuge können nur in der Hauptansicht verwendet werden.

Der Titel der Unterfenster lässt sich bei Bedarf ändern.

![](../media/image13.png)

## 3D

Über die Funktion **3D** wird ein zusätzliches Fenster mit einer Globus-Ansicht geöffnet. Dieses Fenster wird automatisch angeordnet, kann aber auch mit der Maus an eine andere Stelle innerhalb oder ausserhalb des Programmfensters bewegt werden.

![](../media/image14.png)

### Einstellungen

Folgende Funktionen befinden sich in der Titelleiste des 3D Fensters:

- **Ausschnitt synchronisieren**: Hiermit wird auf dem Globus zum Kartenausschnitt des Hauptfensters navigiert.
- **Szene neu laden**: Hiermit werden alle ebenen im Globus neu geladen.
- **Globus Einstellungen**: Hiermit wird ein Dialog mit weiteren Einstellungsmöglichkeiten aufgerufen. Dort werden unter anderem die Geländemodelle für die 3D Ansicht konfiguriert, und es können ebenfalls Bildebenen hinzugefügt werden. Die darzustellende Ebenen der 2D Ansicht werden im Menü links in der Titelleiste ausgewählt. Im Interesse der Performance werden standardmässig nur lokale Ebenen der 2D Ansicht aktiviert - Hintergrundbildebenen sollten nach Möglichkeit direkt als Bildebenen im Globus Einstellungsdialog hinzugefügt werden.

Ebenen der 2D Ansicht werden standardmässig als Textur über das Gelände des Globus gezeichnet. Vektorebenen (darunter Redlining) können alternativ entweder als extrudierte 2.5D Modelle oder als 3D Modelle gezeichnet werden, wobei der Stil der 2D Ansicht soweit wie möglich in der 3D Ansicht übernommen wird. Für die Darstellung als 3D Modelle müssen die Geometrien der Ebene mit Höheninformationen (Z-Koordinaten) versehen sein, und diese müssen entweder gegenüber dem Gelände oder dem Meeresspiegel ausgedrückt sein. Die Darstellungsoptionen für Vektorebenen kann man in den entsprechenden Layereigenschaften setzten.

Die Tooltips der Eingabefelder in den Globus-Einstellungen der Ebene beschreiben die verschiedenen Optionen im Detail.

Die Schattierung von 3D Modelle hängt vom Sonnenstand ab. Der Sonnenstand kann durch eine benutzerdefinierte Zeit/Datumsangabe in den Globus Einstellungen gesteuert werden.

![](../media/image15.png)

Stecknadeln, Kamerabilder und einpunkt MSS Symbole werden als Billboards angezeigt.

### Navigation in der 3D-Ansicht

- Blickwinkel\*\*: Mit der oberen Navigationssteuerung kann der Benutzer den horizontalen und vertikalen Blickwinkel der Kamera ändern.
- Kameraposition\*\*: Die untere Navigationssteuerung verändert die Kameraposition rund um den Globus. Das Gleiche kann durch Drücken der Tastaturpfeile erreicht werden.
- **+**: Reduziert die Kamerahöhe.
- **-**: Erhöht den Kamerahöhe.

## Gitter

In der Registerkarte Ansicht kann das Kartengitter aktiviert werden.

Im Eigenschaftenbereich des Gitters können Gittertyp, Intervalle und Darstellung angepasst werden.
