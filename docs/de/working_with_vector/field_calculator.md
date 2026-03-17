<!-- Recovered from: share/docs/html/de/de/working_with_vector/field_calculator/index.html -->
<!-- Language: de | Section: working_with_vector/field_calculator -->

# Feldrechner

Mit ![](../../../../images/mActionCalculateField.png) _Feldrechner_ Knopf in der Attributtabelle können Sie Berechnungen auf Basis von bestehenden Attributwerten oder definierten Funktionen durchführen, z.B. um die Länge oder die Fläche von Geometrieobjekten zu berechnen. Die Ergebnisse können in eine neue Attributspalte geschrieben werden, in ein virtuelles Feld oder Sie können verwendet werden um Werte in einer vorhandenen Spalte zu updaten.

**Virtuelle Felder**

- Virtuelle Felder sind nicht dauerhaft vorhanden und werden nicht gespeichert.
- Ein Feld kann nur beim Erstellen virtuell gemacht werden.

Der Feldrechner ist jetzt über jeden Layer der Bearbeitung unterstützt erreichbar. Wenn Sie auf das Feldrechner Icon klicken öffnet sich ein Dialog. Wenn der Layer nicht im Bearbeitungsmodus ist wird eine Warnung gezeigt und das Verwenden des Feldrechners bewirkt, dass der Layer in den Bearbeitungsmodus gesetzt wird bevor die Berechnung gemacht wird.

The quick field calculation bar on top of the attribute table is only visible if the layer is editable.

In der schnellen Feldberechnungsleiste wählen Sie erst einen bestehenden Feldnamen aus, öffnen dann den Ausdrucksdialog um Ihren Ausdruck zu erstellen oder schreiben ihn direkt in das Feld und klicken dann den **Alle aktualisieren** Knopf.

## Ausdrücke

Im Feldberechnungsdialog müssen Sie erst auswählen ob sie nur ausgewählte Objekte updaten wollen, eine neues Feld anlegen, in das die Ergebnisse der Berechnung eingefügt werden oder ob Sie ein vorhandenes Feld erneuern wollen.

![](../../../../images/fieldcalculator.png)

Wenn Sie sich entschließen ein neues Feld hinzuzufügen, müssen Sie einen Feldnamen, einen Feldtyp (Ganzzahl, Dezimalzahl, Text oder Datum), die Ausgabefeldbreite und die Genauigkeit eingeben. Zum Beispiel wenn Sie ein Ausgabefeldbreite von 10 und eine Genauigkeit von 3 wählen, heißt das, dass 6 Einträge vor dem Komma stehen, dann das Komma und dann weitere 3 Einträge für die Genauigkeit.

A short example illustrates how field calculator works when using the _Expression_ tab. We want to calculate the length in km of the `railroads` layer from the KADAS sample dataset:

1. Laden Sie das Shape `railroads.shp` in KADAS und öffnen Sie die den Dialog ![](../../../../images/mActionOpenTable.png) _Attributtabelle öffnen_.
2. Klicken Sie auf ![](../../../../images/mActionToggleEditing.png) _Bearbeitungsmodus umschalten_ und öffnen Sie den ![](../../../../images/mActionCalculateField.png) _Feldrechner_ Dialog.
3. Wählen Sie das ![](../../../../images/checkbox.png) _Neues Feld anlegen_ Kontrollkästchen um die Berechnungen in ein neues Feld zu speichern.
4. Setzen Sie `laenge` als Ausgabefeldname, `real` als Ausgabefeldtyp und definieren Sie die Ausgabefeldbreite mit 10 und die Ausgabefeldgenauigkeit mit 3.
5. Machen Sie jetzt einen Doppelklick auf die Funktion `$length` in der _Geometrie_ Gruppe und fügen Sie sie in die Ausdruck Box des Feldrechners ein.
6. Vervollständigen Sie den Ausdruck indem Sie “/1000” im Feldrechnerausdruckfenster und klicken Sie **[OK]**.
7. Sie können jetzt eine neue Spalte `laenge` in der Attributtabelle finden.

Die erhältlichen Funktionen sind im Kapitel [Ausdrücke](../expression/) aufgeführt.

## Funktionseditor

Mit dem Funktionseditor können Sie auf komfortable Weise Ihre eigenen Python-Funktionen definieren. Der Funktionseditor erstellt neue Python-Dateien in `qgis2pythonexpressions` und lädt automatisch alle beim Start von QGIS definierten Funktionen. Beachten Sie, dass neue Funktionen nur im Ordner `expressions` und nicht in der Projektdatei gespeichert werden. Wenn Sie ein Projekt haben, das eine Ihrer benutzerdefinierten Funktionen verwendet, müssen Sie die.py-Datei auch im Ordner expressions freigeben.

Hier ist ein kurzes Beispiel, wie Sie Ihre eigenen Funktionen erstellen können:

```
@qgsfunction(args="auto", group='Custom')
def myfunc(value1, value2 feature, parent):
    pass
```

Das kurze Beispiel erstellt eine Funktion `myfunc`, die Ihnen eine Funktion mit zwei Werten bietet. Bei Verwendung des Funktionsarguments args='auto' wird die Anzahl der benötigten Funktionsargumente aus der Anzahl der Argumente berechnet, mit denen die Funktion in Python definiert wurde (minus 2 - Merkmal und übergeordnet).

Diese Funktion kann dann mit dem folgenden Ausdruck verwendet werden:

```
myfunc('test1','test2')
```

Ihre Funktion wird in den benutzerdefinierten _Funktionen_ der Registerkarte _Ausdruck_ nach Verwendung der Schaltfläche _Skript ausführen_ implementiert.

Weitere Informationen zur Erstellung von Python-Code finden Sie unter <http://www.qgis.org/html/en/docs/pyqgis_developer_cookbook/index.html>.

Der Funktionseditor beschränkt sich nicht nur auf die Arbeit mit dem Feldrechner, er ist auch immer dann zu finden, wenn Sie mit Ausdrücken arbeiten.
