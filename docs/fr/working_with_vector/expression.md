<!-- Recovered from: share/docs/html/fr/fr/working_with_vector/expression/index.html -->
<!-- Language: fr | Section: working_with_vector/expression -->

# Expressions

Les **Expressions** sont disponibles dans la Calculatrice de champ ou via le bouton Ajout d'une nouvelle colonne dans la table attributaire ou via l'onglet Champ dans les Propriétés de la couche; via les rendus Gradué, Catégorisé et Basé sur un ensemble de règles dans l'onglet Style des Propriétés de la couche; via le bouton ![](../../../../images/browsebutton.png) d'étiquetage basé sur une formule dans l’_Étiquetage_ ![](../../../../images/mActionLabeling.png); via la sélection d'entité et via l'onglet diagramme des Propriétés de la couche ainsi que dans le Composeur d'impression pour les _Propriétés principales_ d'un objet étiquette et dans l'onglet _Génération d'Atlas_ .

Ils constituent un moyen efficace de manipuler la valeur d'attribut pour changer dynamiquement la valeur finale afin de changer le style de géométrie, le contenu d'une étiquette, la valeur d'un diagramme, sélectionner des entités ou créer une colonne virtuelle.

## Liste de fonctions

La **Liste de fonctions** contient aussi bien des fonctions que des champs et des valeurs. Référez-vous à la fonction d'aide dans l’**Aide pour la fonction sélectionnée**. Dans **Expression**, vous pouvez voir les expressions de calcul que vous créez avec la **Liste de fonctions**. Pour les opérateurs les plus couramment utilisés, voir sous **Opérateurs**.

Dans la **Liste de fonctions**, cliquez sur _Champs et valeurs_ pour visualiser tous les attributs de la table attributaire à chercher. Pour ajouter un attribut au champ ‘**Expression** de la Calculatrice de champ, double-cliquez sur son nom dans la liste _Champs et valeurs_. Vous pouvez en général utiliser les différents champs, valeurs et fonctions pour construire votre expression de calcul, ou vous pouvez simplement les taper dans la zone expression. Pour afficher les valeurs d'un champ, faites un clic-droit sur le champ voulu. Vous avez le choix entre _Charger les 10 premières valeurs uniques_ et _Charger toutes les valeurs uniques_. Une liste **Valeurs de champs** apparaît à droite avec les valeurs uniques. Pour ajouter une valeur à la zone **Expression** de la Calculatrice de champ, double-cliquez dessus dans la liste des **Valeurs de champs**.

Les groupes _Opérateurs_, _Math_, _Conversions_, _Chaîne_, _Géométrie_ et _Enregistrement_ offrent plusieurs fonctions. Dans _Opérateurs_, vous trouvez des opérateurs mathématiques. Regardez dans _Math_ pour les fonctions mathématiques. Le groupe _Conversions_ contient des fonctions qui convertissent un type de données en un autre. Le groupe _Chaîne_ contient des fonctions pour des chaînes de données. Dans le groupe _Géométrie_, vous trouvez des fonctions pour des objets géométriques. Avec le groupe de fonctions _Enregistrement_, vous pouvez ajouter une numérotation à votre jeu de données. Pour ajouter une fonction à la zone **Expression** de la Calculatrice de champ, cliquez sur > et ensuite double-cliquez sur la fonction.

### Opérateurs

This group contains operators (e.g., +, -, \*).

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

**Quelques exemples:**

- Joint une chaîne et une valeur depuis un nom de colonne:

  ```
  'My feature's id is: ' || "gid"
  ```
- Teste si la “description” du champ d'attribut commence avec la chaîne _Hello_ dans la valeur (notez la position du caractère %):

  ```
  "description" LIKE 'Hello%'
  ```

### Conditions

Ce groupe contient des fonctions permettant de gérer des conditions dans les expressions.

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

**Quelques exemples:**

- Envoie une valeur en retour si la première condition est vraie, sinon une autre valeur:

  ```
  CASE WHEN "software" LIKE '%QGIS%' THEN 'QGIS' ELSE 'Other'
  ```

### Fonctions mathématiques

Ce groupe contient des fonctions mathématiques (par ex. racine carré, sin et cos).

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

### Conversions

Ce groupe contient des fonctions pour convertir un type de données en un autre (par ex. chaîne à entier, entier à chaîne).

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

### Fonctions de Date et Heure

Ce groupe contient des fonctions permettant de gérer des données de date et d'heure.

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

**Quelques exemples:**

- Obtenir le mois et l'année d'aujourd'hui dans le format “10/2014”

  ```
  month($now) || '/' || year($now)
  ```

### Fonctions de Chaîne

Ce groupe contient des fonctions qui opèrent sur des chaînes (par ex. qui remplace, convertit en majuscule).

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

### Fonctions de Couleur

Ce groupe contient des fonctions pour manipuler les couleurs.

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

### Fonctions de Géométrie

Ce groupe contient des fonctions qui opèrent sur des objets géométriques (par ex. longueur, aire).

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

### Fonctions d'Enregistrement

Ce groupe contient des fonctions qui permettent d'accéder aux identifiants des enregistrements.

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

### Champs et Valeurs

Contient une liste de champs de la couche. Un échantillon de valeurs peut aussi être obtenu avec un clic-droit.

Sélectionnez le nom du champ de la liste, puis faites un clic-droit pour accéder à un menu contextuel avec des options pour charger des valeurs d'échantillon du champ sélectionné.

Le nom des champs devrait être entre guillemets double. Les valeurs ou chaînes devraient être entre guillemets simples.
