<!-- Recovered from: share/docs/html/it/it/working_with_vector/expression/index.html -->
<!-- Language: it | Section: working_with_vector/expression -->

# Espressioni

La funzione **Espressioni** è disponibile tramite il calcolatore di campo o il pulsante per aggiungere una nuova colonna nella tabella degli attributi o nella scheda Campo nelle proprietà del livello; tramite il rendering graduato, categorizzato e basato su regole nella scheda Stile delle proprietà del livello; tramite l'etichettatura basata sulle espressioni ![](../../images/browsebutton.png) nell'applicazione principale _Etichettatura_ ![](../../images/mActionLabeling.png); tramite la selezione delle funzioni e tramite la scheda diagramma delle proprietà del livello, nonché le proprietà principali dell'elemento etichetta e la scheda generazione di _Atlante_ nel Compositore di stampa.

Sono un modo potente per manipolare il valore degli attributi al fine di modificare dinamicamente il valore finale per cambiare lo stile della geometria, il contenuto dell'etichetta, il valore per il diagramma, selezionare qualche caratteristica o creare colonne virtuali.

## Lista delle funzioni

La **Lista funzioni** contiene funzioni, campi e valori. Visualizzare la funzione di aiuto nella **Aiuto funzioni selezionate**. In **Espressione** vengono visualizzate le espressioni di calcolo create con la **Lista funzioni**. Per gli operatori più comunemente utilizzati, vedere **Operatori**.

Nella **Lista funzioni**, fare clic su _Campi e valori_ per visualizzare tutti gli attributi della tabella degli attributi da ricercare. Per aggiungere un attributo al campo Campo calcolatrice **Espressione**, fare doppio clic sul suo nome nell'elenco _Campi e valori_. Generalmente, è possibile utilizzare i vari campi, valori e funzioni per costruire l'espressione di calcolo, oppure è sufficiente digitarla nella casella. Per visualizzare i valori di un campo, è sufficiente fare clic con il tasto destro del mouse sul campo appropriato. È possibile scegliere tra _Carica i 10 valori unici_ e _Carica tutti i valori unici_. Sul lato destro, si apre l'elenco **Valori di campo** con i valori univoci. Per aggiungere un valore alla casella Campo calcolatrice **Espressione**, fare doppio clic sul suo nome nell'elenco **Valori di campo**.

I gruppi _Operatori_, _Math_, _Conversioni_, _String_, _Geometria_ e _Registrazione_ offrono diverse funzioni. In _Operatori_, si trovano gli operatori matematici. Cerca in _Matematica_ per le funzioni matematiche. Il gruppo _Conversioni_ contiene funzioni che convertono un tipo di dati in un altro. Il gruppo _String_ fornisce funzioni per le stringhe di dati. Nel gruppo _Geometria_ si trovano le funzioni per gli oggetti geometrici. Con le funzioni del gruppo _Record_ è possibile aggiungere una numerazione al proprio set di dati. Per aggiungere una funzione alla casella Campo calcolatrice **Espressione**, fare clic su > quindi fare doppio clic sulla funzione.

### Operatori

Questo gruppo contiene operatori (ad es., +, -, \*).

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

**Alcuni esempi:**

- Unisce una stringa e un valore di una colonna:

  ```
  'ID oggetto: ' || "gid"
  ```
- Verifica se il valore della colonna "descrizione" inizia con 'Hello' (nota la posizione del carattere %):

  ```
  "descrizione" LIKE 'Hello%'
  ```

### Condizionali

Questo gruppo contiene funzioni per gestire i controlli condizionali nelle espressioni.

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

**Esempio:**

- Inviare un valore se la prima condizione è vera, altrimenti un altro valore:

  ```
  CASE WHEN "software" LIKE '%QGIS%' THEN 'QGIS' ELSE 'Other'
  ```

### Funzioni matematiche

Questo gruppo contiene funzioni matematiche (es. radice quadrata, peccato e cos).

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

### Conversioni

Questo gruppo contiene funzioni per convertire un tipo di dati in un altro (ad esempio, da stringa a intero, da intero a stringa).

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

### Funzioni data e ora

Questo gruppo contiene funzioni per la gestione dei dati di data e ora.

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

**Esempio:**

- Estrarre il mese e l'anno di oggi nel formato “10/2014”

  ```
  month($now) || '/' || year($now)
  ```

### Funzioni stringa

Questo gruppo contiene funzioni che operano su stringhe (ad esempio, che sostituiscono, convertono in lettere maiuscole).

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

### Funzioni colore

Questo gruppo contiene funzioni per la manipolazione dei colori.

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

### Funzioni geometria

Questo gruppo contiene funzioni che operano su oggetti geometrici (ad es. lunghezza, area).

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

### Funzioni record

Questo gruppo contiene funzioni che operano su identificatori di record.

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

### Campi e valori

Contiene un elenco di campi del livello. I valori dei campioni sono accessibili anche con il tasto destro del mouse.

Selezionare il nome del campo dall'elenco, quindi fare clic con il pulsante destro del mouse per accedere a un menu contestuale con le opzioni per caricare i valori del campione dal campo selezionato.

Il nome del campo deve essere citato due volte. I valori o le stringhe dovrebbero essere semplici da citare.
