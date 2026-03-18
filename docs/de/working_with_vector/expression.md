<!-- Recovered from: share/docs/html/de/de/working_with_vector/expression/index.html -->
<!-- Language: de | Section: working_with_vector/expression -->

# Ausdrücke

Die Funktion **Expressions** steht über den Feldrechner oder das Hinzufügen einer neuen Spaltenschaltfläche in der Attributtabelle oder der Registerkarte Feld in den Layereigenschaften zur Verfügung; über das abgestufte, kategorisierte und regelbasierte Rendering in der Registerkarte Style der Layereigenschaften; über die expressionsbasierte Beschriftung ![](../../images/browsebutton.png) in der ![](../../images/mActionLabeling.png) _Labeling_ Kernanwendung; durch die Featureauswahl und durch den Diagrammreiter der Layereigenschaften sowie die _Haupteigenschaften_ des Labelelements und die Registerkarte _Atlasgenerierung_ im Print Composer.

Sie sind eine leistungsfähige Möglichkeit, den Attributwert zu manipulieren, um den Endwert dynamisch zu ändern, um den Geometrie-Stil, den Inhalt des Labels, den Wert für das Diagramm, die Auswahl eines Features oder die Erstellung einer virtuellen Spalte zu ändern.

## Funktionsliste

Die **Funktionsliste** enthält Funktionen genauso wie Felder und Werte. Sehen Sie sich die Hilfefunktionen im **Hilfe zu gewählten Funktionen** Bereich an. In **Ausdruck** sehen Sie die Berechnungsausdrücke, die Sie mit der **Funktionsliste** erstellt haben. Für die gebräuchlichsten Operatoren siehe **Operatoren**.

Klicken Sie in der **Funktionsliste** auf _Felder und Werte_ um alle durchsuchbaren Attribute der Attributtabelle anschauen zu können. Um ein Attribut dem Feldrechner **Ausdruck** Feld hinzuzufügen doppelklicken Sie seinen Namen in der _Felder und Werte_ Liste. Im Allgmeinen können Sie die diversen Felder, Werte und Funktionen benutzen um den Berechnungsausdruck aufzubauen, oder Sie geben in einfach direkt ins Fenster ein. Um die Werte eines Feldes darzustellen müssen Sie einfach einen Rechtsklick auf das entsprechende Feld machen. Sie können zwischen _10 Stichproben_ und _Alle eindeutigen_ wählen. Auf der rechten Seite öffnet sich die **Feldwerte** Liste mit den eindeutigen Werten. Um dem Feldrechner **Ausdruck** Fenster einen Wert hinzuzufügen, machen Sie einen Doppelklick auf den Namen in der **Feldwerte** Liste.

Die Gruppen _Operatoren_, _Mathematik_, _Umwandlungen_, _Zeichenketten_, _Geometrie_ und _Datensatz_ stellen zahlreiche Funktionen zur Verfügung. In _Operatoren_ finden Sie mathematische Operatoren. Suchen Sie in _Mathematik_ nach mathematischen Funktionen. Die _Umwandlungen_ Gruppe enthält Funktionen die einen Datentyp in einen anderen konvertieren. Die _Zeichenkette_ Gruppe stellt Funktionen für Datenketten zur Verfügung. In der _Geometrie_ Gruppe finden Sie Funktionen für Geometrieobjekte. Mit den Funktionen der _Datensatz_ Gruppe können Sie Ihren Datensatz mit einer Nummerierung versehen. Um eine Funktion in die **Ausdruck** Box des Feldrechners hinzuzufügen klicken Sie auf > und doppelklicken Sie dann die Funktion.

### Operatoren

Diese Gruppe enthält Operatoren (z.B. +, -, -, \*).

```
a + b      a plus b
a - b      a minus b
a * b      a multiplied by b
a / b      a divided by b
a % b      a modulo b (for example, 7 % 2 = 1, or 2 fits into 7 three
           times with remainder 1)
a ^ b      a power b (for example, 2^2=4 or 2^3=8)
a = b      a and b are equal
a > b      a is larger than b
a < b      a is smaller than b
a <> b     a and b are not equal
a != b     a and b are not equal
a <= b     a is less than or equal to b
a >= b     a is larger than or equal to b
a ~ b      a matches the regular expression b
+ a        positive sign
- a        negative value of a
||         joins two values together into a string 'Hello' || ' world'
LIKE       returns 1 if the string matches the supplied pattern
ILIKE      returns 1 if the string matches case-insensitive the supplied
           pattern (ILIKE can be used instead of LIKE to make the match
           case-insensitive)
IS         returns 1 if a is the same as b
OR         returns 1 when condition a or b is true
AND        returns 1 when condition a and b are true
NOT        returns 1 if a is not the same as b
column name "column name"     value of the field column name, take
                              care to not be confused with simple
                              quote, see below
'string'                      a string value, take care to not be
                              confused with double quote, see above
NULL                          null value
a IS NULL                     a has no value
a IS NOT NULL                 a has a value
a IN (value[,value])          a is below the values listed
a NOT IN (value[,value])      a is not below the values listed
```

**Beispiele:**

- Verbindet eine Zeichenkette und einen Wert von einem Spaltennamen:

  ```
  'My feature's id is: ' || "gid"
  ```
- Testen Sie, ob das Attributfeld "description" mit der Zeichenkette "Hello" im Wert beginnt (beachten Sie die Position des %-Zeichens):

  ```
  "description" LIKE 'Hello%'
  ```

### Bedingungen

Diese Gruppe enthält Funktionen um bedingte Prüfungen in Ausdrücken zu handhaben.

```
CASE                          evaluates multiple expressions and returns a
                              result
CASE ELSE                     evaluates multiple expressions and returns a
                              result
coalesce                      returns the first non-NULL value from the
                              expression list
regexp_match                  returns true if any part of a string matches
                              the supplied regular expression
```

**Beispiele:**

- Sende einen Wert zurück wenn die erste Bedingung wahr ist, sonst einen anderen Wert:

  ```
  CASE WHEN "software" LIKE '%QGIS%' THEN 'QGIS' ELSE 'Other'
  ```

### Mathematische Funktionen

Diese Gruppe enthält mathematische Funktionen (z.B. sqrt, sin und cos).

```
sqrt(a)                       square root of a
abs                           returns the absolute value of a number
sin(a)                        sine of a
cos(a)                        cosine of a
tan(a)                        tangent of a
asin(a)                       arcsin of a
acos(a)                       arccos of a
atan(a)                       arctan of a
atan2(y,x)                    arctan of y/x using the signs of the two
                              arguments to determine the quadrant of the
                              result
exp                           exponential of a value
ln                            value of the natural logarithm of the passed
                              expression
log10                         value of the base 10 logarithm of the passed
                              expression
log                           value of the logarithm of the passed value
                              and base
round                         round to number of decimal places
rand                          random integer within the range specified by
                              the minimum
                              and maximum argument (inclusive)
randf                         random float within the range specified by
                              the minimum
                              and maximum argument (inclusive)
max                           largest value in a set of values
min                           smallest value in a set of values
clamp                         restricts an input value to a specified
                              range
scale_linear                  transforms a given value from an input
                              domain to an output
                              range using linear interpolation
scale_exp                     transforms a given value from an input
                              domain to an output
                              range using an exponential curve
floor                         rounds a number downwards
ceil                          rounds a number upwards
$pi                           pi as value for calculations
```

### Umwandlungen

Diese Gruppe enthält Funktionen, um einen Datentypen in einen anderen umzuwandeln (z.B. Zeichenketten zu Ganzzahlen oder umgekehrt).

```
toint                        converts a string to integer number
toreal                       converts a string to real number
tostring                     converts number to string
todatetime                   converts a string into Qt data time type
todate                       converts a string into Qt data type
totime                       converts a string into Qt time type
tointerval                   converts a string to an interval type (can be
                             used to take days, hours, months, etc. off a
                             date)
```

### Datum und Zeit Funktionen

Diese Gruppe enthält Funktionen die auf Datums- und Zeitdaten angewendet werden können.

```
$now       current date and time
age        difference between two dates
year       extract the year part from a date, or the number of years from
           an interval
month      extract the month part from a date, or the number of months
           from an interval
week       extract the week number from a date, or the number of weeks
           from an interval
day        extract the day from a date, or the number of days from an
           interval
hour       extract the hour from a datetime or time, or the number
           of hours from an interval
minute     extract the minute from a datetime or time, or the number
           of minutes from an interval
second     extract the second from a datetime or time, or the number
           of minutes from an interval
```

**Einige Beispiele:**

- Lassen Sie sich den Monat und das Jahr von heute im Format “10/2014” herausgeben:

  ```
  month($now) || '/' || year($now)
  ```

### Zeichenkettenfunktionen

Diese Gruppe enthält Funktionen für Zeichenketten (z.B. Ersetzen und in Großbuchstaben umwandeln).

```
lower                        convert string a to lower case
upper                        convert string a to upper case
title                        converts all words of a string to title
                             case (all words lower case with leading
                             capital letter)
trim                         removes all leading and trailing white
                             space (spaces, tabs, etc.) from a string
wordwrap                     returns a string wrapped to a maximum/
                             minimum number of characters
length                       length of string a
replace                      returns a string with the supplied string
                             replaced
regexp_replace(a,this,that)  returns a string with the supplied regular
                             expression replaced
regexp_substr                returns the portion of a string which matches
                             a supplied regular expression
substr(*a*,from,len)         returns a part of a string
concat                       concatenates several strings to one
strpos                       returns the index of a regular expression
                             in a string
left                         returns a substring that contains the n
                             leftmost characters of the string
right                        returns a substring that contains the n
                             rightmost characters of the string
rpad                         returns a string with supplied width padded
                             using the fill character
lpad                         returns a string with supplied width padded
                             using the fill character
format                       formats a string using supplied arguments
format_number                returns a number formatted with the locale
                             separator for thousands (also truncates the
                             number to the number of supplied places)
format_date                  formats a date type or string into a custom
                             string format
```

### Farbfunktionen

Diese Gruppe enthält Funktionen zur Farbmanipulation.

```
color_rgb       returns a string representation of a color based on its
                red, green, and blue components
color_rgba      returns a string representation of a color based on its
                red, green, blue, and alpha (transparency) components
ramp_color      returns a string representing a color from a color ramp
color_hsl       returns a string representation of a color based on its
                hue, saturation, and lightness attributes
color_hsla      returns a string representation of a color based on its
                hue, saturation, lightness and alpha (transparency)
                attributes
color_hsv       returns a string representation of a color based on its
                hue, saturation, and value attributes
color_hsva      returns a string representation of a color based on its
                hue, saturation, value and alpha (transparency) attributes
color_cmyk      returns a string representation of a color based on its
                cyan, magenta, yellow and black components
color_cmyka     returns a string representation of a color based on its
                cyan, magenta, yellow, black and alpha (transparency)
                components
```

### Geometriefunktionen

Dies Gruppe enthält Funktionen für das Arbeiten mit Geometrieobjekten (z.B. Länge und Flächeninhalt).

```
$geometry        returns the geometry of the current feature (can be used
                 for processing with other functions)
$area            returns the area size of the current feature
$length          returns the length size of the current feature
$perimeter       returns the perimeter length of the current feature
$x               returns the x coordinate of the current feature
$y               returns the y coordinate of the current feature
xat              retrieves the nth x coordinate of the current feature.
                 n given as a parameter of the function
yat              retrieves the nth y coordinate of the current feature.
                 n given as a parameter of the function
xmin             returns the minimum x coordinate of a geometry.
                 Calculations are in the Spatial Reference System of this
                 Geometry
xmax             returns the maximum x coordinate of a geometry.
                 Calculations are in the Spatial Reference System of this
                 Geometry
ymin             returns the minimum y coordinate of a geometry.
                 Calculations are in the Spatial Reference System of this
                 Geometry
ymax             returns the maximum y coordinate of a geometry.
                 Calculations are in the Spatial Reference System of this
                 Geometry
geomFromWKT      returns a geometry created from a well-known text (WKT)
                 representation
geomFromGML      returns a geometry from a GML representation of geometry
bbox
disjoint         returns 1 if the geometries do not share any space
                 together
intersects       returns 1 if the geometries spatially intersect
                 (share any portion of space) and 0 if they don't
touches          returns 1 if the geometries have at least one point in
                 common, but their interiors do not intersect
crosses          returns 1 if the supplied geometries have some, but not
                 all, interior points in common
contains         returns true if and only if no points of b lie in the
                 exterior of a, and at least one point of the interior
                 of b lies in the interior of a
overlaps         returns 1 if the geometries share space, are of the
                 same dimension, but are not completely contained by
                 each other
within           returns 1 if geometry a is completely inside geometry b
buffer           returns a geometry that represents all points whose
                 distance from this geometry is less than or equal to
                 distance
centroid         returns the geometric center of a geometry
bounds           returns a geometry which represents the bounding box of
                 an input geometry. Calculations are in the Spatial
                 Reference System of this Geometry.
bounds_width     returns the width of the bounding box of a geometry.
                 Calculations are in the Spatial Reference System of
                 this Geometry.
bounds_height    returns the height of the bounding box of a geometry.
                 Calculations are in the Spatial Reference System of
                 this Geometry.
convexHull       returns the convex hull of a geometry (this represents
                 the minimum convex geometry that encloses all geometries
                 within the set)
difference       returns a geometry that represents that part of geometry
                 a that does not intersect with geometry b
distance         returns the minimum distance (based on spatial ref)
                 between two geometries in projected units
intersection     returns a geometry that represents the shared portion
                 of geometry a and geometry b
symDifference    returns a geometry that represents the portions of a and
                 b that do not intersect
combine          returns the combination of geometry a and geometry b
union            returns a geometry that represents the point set union of
                 the geometries
geomToWKT        returns the well-known text (WKT) representation of the
                 geometry without SRID metadata
geometry         returns the feature's geometry
transform        returns the geometry transformed from the source CRS to
                 the dest CRS
```

### Datensatzfunktionen

Diese Gruppe enthält Funktionen die sich auf datensatzbezeichner beziehen.

```
$rownum                  returns the number of the current row
$id                      returns the feature id of the current row
$currentfeature          returns the current feature being evaluated.
                         This can be used with the 'attribute' function
                         to evaluate attribute values from the current
                         feature.
$scale                   returns the current scale of the map canvas
$uuid                    generates a Universally Unique Identifier (UUID)
                         for each row. Each UUID is 38 characters long.
getFeature               returns the first feature of a layer matching a
                         given attribute value.
attribute                returns the value of a specified attribute from
                         a feature.
$map                     returns the id of the current map item if the map
                         is being drawn in a composition, or "canvas" if
                         the map is being drawn within the main QGIS
                         window.
```

### Felder und Werte

Enthält eine Liste von Feldern vom Layer. Auf Beispielwerte kann über einen Rechtsklick zugegriffen werden.

Wählen Sie einen Feldnamen aus der Liste, machen Sie dann einen Rechtsklick, um auf ein Kontextmenü mit Optionen zum Laden von Beispielwerten aus dem gewählten Feld zuzugreifen.

Feldnamen sollten in doppelte Anführungsstriche gesetzt werden. Werte oder Zeichenketten sollten in einfache Anführungsstriche gesetzt werden.
