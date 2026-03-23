<!-- Recovered from: docs_old/html/fr/fr/working_with_raster/supported_data/index.html -->
<!-- Language: fr | Section: working_with_raster/supported_data -->

# Les données raster

Cette section explique comment visualiser et définir les propriétés d'une couche raster. KADAS utilise la bibliothèque GDAL pour lire et écrire des raster de multiples formats dont ArcInfo Binary Grid, ArcInfo ASCII Grid, GeoTIFF, ERDAS IMAGINE et bien d'autres. La gestion des raster GRASS se fait de manière native via une extension spécifique. Des raster peuvent également être lus par KADAS depuis des archives zip et gzip.

A ce jour, plus de 100 formats raster sont gérés par la bibliothèque GDAL. La liste complète est disponible sur cette page: <http://www.gdal.org/formats_list.html>.

Note

Certains des formats listés peuvent ne pas fonctionner dans KADAS pour diverses raisons. Par exemple, certains formats requièrent une bibliothèque commerciale externe ou la bibliothèque GDAL qui n'a peut-être pas été compilée sur votre système d'exploitation pour gérer le format souhaité. Seuls les formats ayant été testés correctement apparaissent dans la liste des types de fichiers proposés au moment de l'ajout de données raster dans KADAS. Les autres formats peuvent être chargés en sélectionnant `[GDAL] Tous les fichiers (*)`.

## Qu'est ce qu'un raster ?

Les données raster dans les SIG sont des matrices de cellules discrètes qui représentent des objets, au-dessus ou en dessous de la surface de la Terre. Les cellules de la grille raster sont de la même taille et généralement rectangulaires (dans KADAS, elles seront toujours rectangulaires). Les jeux de données raster les plus classiques sont des données de télédétection telles que des photographies aériennes ou des images satellitaires et des données issues de modèles telles que les matrices d'élévation.

Contrairement aux données vectorielles, les données raster n'ont pas de base de données associée. Elles sont géoréférencées grâce à la résolution des pixels et les coordonnées x/y du pixel d'un des coins de la couche raster. Cela permet à KADAS de positionner les données correctement dans la zone de la carte.

Pour afficher correctement les données, KADAS utilise les informations de géoréférencement intégrées aux couches raster (par exemple GeoTiff) ou présentes dans un fichier world.
